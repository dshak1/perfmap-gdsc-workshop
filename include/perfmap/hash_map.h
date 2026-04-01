// Copyright 2026 SFU GDSC — PerfMap Workshop
//
// A cache-aware, open-addressing hash map built from scratch.
//
// Design choices (Google-style rationale):
//   - Flat storage via std::vector<Slot>  → cache-friendly linear probing
//   - absl::StatusOr / absl::Status       → explicit error handling (no exceptions)
//   - Tombstone-aware deletion             → correct probe-chain semantics
//   - Load-factor-triggered rehash at 0.7  → amortized O(1) ops
//
// Performance optimizations over the naive version:
//   1. Fibonacci hash mixing  → eliminates clustering from identity hashes
//   2. Power-of-two capacity  → bitmask instead of expensive modulo
//   3. Tombstone counting     → triggers compacting rehash, faster miss path
//   4. __builtin_prefetch     → hides memory latency on probe chains
//
// This is NOT a production hash map. It is an educational implementation
// designed to teach performance engineering concepts and Google-style C++.

#ifndef PERFMAP_HASH_MAP_H_
#define PERFMAP_HASH_MAP_H_

#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <string_view>
#include <utility>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "perfmap/slot.h"

namespace perfmap {

// Fibonacci hash mixing — scrambles the bits of a hash value so that
// even a terrible hash function (like std::hash<int> which is often just
// the identity) produces a uniform distribution across buckets.
//
// The magic constant is 2^64 / φ (golden ratio).  Multiplying by it and
// then shifting right produces excellent avalanche properties: changing
// one input bit flips ~50% of output bits.
//
// This is the same principle behind the "Knuth multiplicative hash."
inline size_t MixHash(size_t h) {
  // 64-bit: 2^64 / φ = 11400714819323198485
  // 32-bit: 2^32 / φ = 2654435769
  if constexpr (sizeof(size_t) == 8) {
    h *= UINT64_C(11400714819323198485);
    h ^= h >> 32;  // Extra xorshift for more avalanche
  } else {
    h *= UINT32_C(2654435769);
    h ^= h >> 16;
  }
  return h;
}

// Round up to the next power of two.  Used to ensure table capacity is
// always a power of two so we can use bitmask instead of modulo.
inline size_t RoundUpPow2(size_t v) {
  if (v == 0) return 1;
  --v;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  if constexpr (sizeof(size_t) == 8) {
    v |= v >> 32;
  }
  return v + 1;
}

// Workload-tuned policies for the same open-addressing core.
// The point is not "one winner", but demonstrating how different
// workload assumptions change the right trade-off.
struct BalancedWorkloadPolicy {
  static constexpr std::string_view kBenchmarkName = "perfmap_balanced";
  static constexpr std::string_view kTestName = "PerfMapBalanced";
  static constexpr std::string_view kDisplayName =
      "perfmap::HashMap(balanced)";
  static constexpr size_t kMaxLoadNumerator = 7;
  static constexpr size_t kMaxLoadDenominator = 10;
  static constexpr size_t kMaxTombstoneNumerator = 2;
  static constexpr size_t kMaxTombstoneDenominator = 10;
  static constexpr bool kEnablePrefetch = true;
};

struct ReadHeavyPolicy {
  static constexpr std::string_view kBenchmarkName = "perfmap_read_heavy";
  static constexpr std::string_view kTestName = "PerfMapReadHeavy";
  static constexpr std::string_view kDisplayName =
      "perfmap::HashMap(read-heavy)";
  static constexpr size_t kMaxLoadNumerator = 5;
  static constexpr size_t kMaxLoadDenominator = 10;
  static constexpr size_t kMaxTombstoneNumerator = 2;
  static constexpr size_t kMaxTombstoneDenominator = 10;
  static constexpr bool kEnablePrefetch = true;
};

struct ChurnHeavyPolicy {
  static constexpr std::string_view kBenchmarkName = "perfmap_churn_heavy";
  static constexpr std::string_view kTestName = "PerfMapChurnHeavy";
  static constexpr std::string_view kDisplayName =
      "perfmap::HashMap(churn-heavy)";
  static constexpr size_t kMaxLoadNumerator = 6;
  static constexpr size_t kMaxLoadDenominator = 10;
  static constexpr size_t kMaxTombstoneNumerator = 1;
  static constexpr size_t kMaxTombstoneDenominator = 20;
  static constexpr bool kEnablePrefetch = true;
};

struct SpaceEfficientPolicy {
  static constexpr std::string_view kBenchmarkName = "perfmap_space_efficient";
  static constexpr std::string_view kTestName = "PerfMapSpaceEfficient";
  static constexpr std::string_view kDisplayName =
      "perfmap::HashMap(space-efficient)";
  static constexpr size_t kMaxLoadNumerator = 17;
  static constexpr size_t kMaxLoadDenominator = 20;
  static constexpr size_t kMaxTombstoneNumerator = 1;
  static constexpr size_t kMaxTombstoneDenominator = 4;
  static constexpr bool kEnablePrefetch = false;
};

// A simple open-addressing hash map with linear probing.
//
// Template parameters:
//   K    — key type   (must be equality-comparable and hashable)
//   V    — value type
//   Hash   — hash functor (defaults to std::hash<K>)
//   Policy — workload-tuning policy
//
// Example usage:
//   HashMap<int, std::string> map;
//   auto status = map.Insert(42, "answer");
//   auto result = map.Find(42);
//   if (result.ok()) {
//     std::cout << *result << std::endl;  // "answer"
//   }
template <typename K, typename V, typename Hash = std::hash<K>,
          typename Policy = BalancedWorkloadPolicy>
class HashMap {
 public:
  // Construct a map with the given initial bucket count.
  // Capacity is rounded up to a power of two for bitmask probing.
  explicit HashMap(size_t initial_capacity = 16)
      : table_(RoundUpPow2(initial_capacity)),
        mask_(table_.size() - 1),
        size_(0),
        tombstone_count_(0) {}

  // -----------------------------------------------------------------------
  // Core API
  // -----------------------------------------------------------------------

  // Insert a key-value pair.  If the key already exists, its value is
  // updated (upsert semantics).  May trigger a rehash.
  absl::Status Insert(const K& key, const V& value) {
    if (table_.empty()) {
      return absl::InternalError("table has zero capacity");
    }
    // Rehash if live entries + tombstones together exceed threshold.
    // This ensures tombstones don't silently degrade performance.
    if (ShouldRehash()) {
      GrowOrCompact();
    }
    size_t idx = FindSlotIndex(key);
    if (table_[idx].state == SlotState::kOccupied) {
      // Key already present — update value
      table_[idx].value = value;
      return absl::OkStatus();
    }
    // Reclaim tombstone if we're inserting into one
    if (table_[idx].state == SlotState::kDeleted) {
      --tombstone_count_;
    }
    // Insert into empty or tombstone slot
    table_[idx] = {SlotState::kOccupied, key, value};
    ++size_;
    return absl::OkStatus();
  }

  // Look up a key.  Returns the associated value or NOT_FOUND.
  //
  // NOTE: This allocates on the miss path (absl::NotFoundError creates
  // a heap-allocated Status payload).  For performance-critical hot
  // loops, prefer FindPtr() which returns a raw pointer with zero
  // allocation overhead.
  absl::StatusOr<V> Find(const K& key) const {
    if (table_.empty()) {
      return absl::NotFoundError("empty table");
    }
    size_t idx = FindSlotIndexForLookup(key);
    if (idx == kNotFound) {
      return absl::NotFoundError("key not found");
    }
    return table_[idx].value;
  }

  // Zero-allocation lookup.  Returns a pointer to the value if found,
  // or nullptr if absent.  This is the preferred API for hot paths
  // (benchmarks, inner loops) because it avoids the heap allocation
  // that absl::NotFoundError incurs.
  //
  // The returned pointer is invalidated by any mutating operation
  // (Insert, Erase, Rehash).
  const V* FindPtr(const K& key) const {
    if (table_.empty()) return nullptr;
    size_t idx = FindSlotIndexForLookup(key);
    if (idx == kNotFound) return nullptr;
    return &table_[idx].value;
  }

  // Returns true if the key exists in the map.
  bool Contains(const K& key) const {
    if (table_.empty()) return false;
    return FindSlotIndexForLookup(key) != kNotFound;
  }

  // Erase a key by placing a tombstone.  Returns NOT_FOUND if absent.
  absl::Status Erase(const K& key) {
    if (table_.empty()) {
      return absl::NotFoundError("empty table");
    }
    size_t idx = FindSlotIndexForLookup(key);
    if (idx == kNotFound) {
      return absl::NotFoundError("key not found");
    }
    table_[idx].state = SlotState::kDeleted;
    --size_;
    ++tombstone_count_;
    return absl::OkStatus();
  }

  // -----------------------------------------------------------------------
  // Observers
  // -----------------------------------------------------------------------

  size_t size() const { return size_; }
  bool empty() const { return size_ == 0; }
  size_t capacity() const { return table_.size(); }

  // Reserve enough buckets to hold at least `expected_size` live entries
  // without crossing the target load factor.
  void Reserve(size_t expected_size) {
    if (expected_size == 0) return;
    const size_t min_capacity = RoundUpPow2(
        DivideRoundUp(expected_size * Policy::kMaxLoadDenominator,
                      Policy::kMaxLoadNumerator));
    if (min_capacity > table_.size()) {
      Rehash(min_capacity);
    }
  }

  double load_factor() const {
    if (table_.empty()) return 1.0;
    return static_cast<double>(size_) / static_cast<double>(table_.size());
  }

  // Effective load = (live + tombstones) / capacity.
  // This is what actually determines probe chain length.
  double effective_load_factor() const {
    if (table_.empty()) return 1.0;
    return static_cast<double>(size_ + tombstone_count_) /
           static_cast<double>(table_.size());
  }

  // -----------------------------------------------------------------------
  // Internals (public for educational / benchmarking access)
  // -----------------------------------------------------------------------

  // Force a rehash to a new capacity.  All occupied entries are
  // re-inserted; tombstones are discarded (table compaction).
  void Rehash(size_t new_capacity) {
    new_capacity = RoundUpPow2(new_capacity);
    std::vector<Slot<K, V>> old_table = std::move(table_);
    table_.assign(new_capacity, Slot<K, V>{});
    mask_ = new_capacity - 1;
    size_ = 0;
    tombstone_count_ = 0;
    for (auto& slot : old_table) {
      if (slot.state == SlotState::kOccupied) {
        // Ignore status — we just resized, so inserts cannot fail.
        (void)Insert(std::move(slot.key), std::move(slot.value));
      }
    }
  }

 private:
  // Sentinel value meaning "not found" in lookup paths.
  static constexpr size_t kNotFound = std::numeric_limits<size_t>::max();

  static size_t DivideRoundUp(size_t dividend, size_t divisor) {
    return (dividend + divisor - 1) / divisor;
  }

  // Hash a key: apply the user's hash functor, then mix the bits.
  size_t HashKey(const K& key) const {
    return MixHash(hasher_(key));
  }

  // Should we rehash?  Triggers when live + tombstones push effective
  // load above threshold, keeping probe chains short.
  bool ShouldRehash() const {
    return (size_ + tombstone_count_) * Policy::kMaxLoadDenominator >
           table_.size() * Policy::kMaxLoadNumerator;
  }

  // Decide whether to grow or just compact (discard tombstones).
  // If the table is mostly tombstones, compacting in place is cheaper.
  void GrowOrCompact() {
    if (tombstone_count_ * Policy::kMaxTombstoneDenominator >
        table_.size() * Policy::kMaxTombstoneNumerator) {
      // Many tombstones — rehash at same size to compact them away.
      Rehash(table_.size());
    } else {
      // Genuinely full — double the capacity.
      Rehash(table_.size() * 2);
    }
  }

  // Find the appropriate slot index for a key (used by Insert).
  //
  // Returns:
  //   - The index of the existing occupied slot if the key is present.
  //   - Otherwise, the best index to insert into: the first tombstone
  //     encountered along the probe chain, or the first empty slot.
  //
  // Probing strategy: LINEAR with prefetch.
  //   Sequential access is cache-friendly.  We also prefetch one step
  //   ahead to hide memory latency on longer probe chains.
  size_t FindSlotIndex(const K& key) const {
    size_t index = HashKey(key) & mask_;
    size_t first_deleted = kNotFound;

    for (size_t i = 0; i < table_.size(); ++i) {
      // Prefetch the next probe location while we process this one.
      size_t next = (index + 1) & mask_;
      if constexpr (Policy::kEnablePrefetch) {
        __builtin_prefetch(&table_[next], 0 /* read */, 1 /* low locality */);
      }

      const auto& slot = table_[index];

      if (slot.state == SlotState::kEmpty) {
        return (first_deleted != kNotFound) ? first_deleted : index;
      }

      if (slot.state == SlotState::kOccupied && slot.key == key) {
        return index;
      }

      if (slot.state == SlotState::kDeleted && first_deleted == kNotFound) {
        first_deleted = index;
      }

      index = next;
    }

    return (first_deleted != kNotFound) ? first_deleted : 0;
  }

  // Find the slot containing an existing key (used by Find / Erase).
  // Returns kNotFound if the key is not present.
  size_t FindSlotIndexForLookup(const K& key) const {
    size_t index = HashKey(key) & mask_;

    for (size_t i = 0; i < table_.size(); ++i) {
      // Prefetch next probe slot to overlap memory access with compare.
      size_t next = (index + 1) & mask_;
      if constexpr (Policy::kEnablePrefetch) {
        __builtin_prefetch(&table_[next], 0 /* read */, 1 /* low locality */);
      }

      const auto& slot = table_[index];

      if (slot.state == SlotState::kEmpty) {
        return kNotFound;
      }
      if (slot.state == SlotState::kOccupied && slot.key == key) {
        return index;
      }
      index = next;
    }
    return kNotFound;
  }

  std::vector<Slot<K, V>> table_;
  size_t mask_;              // table_.size() - 1, for fast bitmask probing
  size_t size_;
  size_t tombstone_count_;   // Track tombstones for compaction decisions
  Hash hasher_;
};

template <typename K, typename V, typename Hash = std::hash<K>>
using ReadHeavyHashMap = HashMap<K, V, Hash, ReadHeavyPolicy>;

template <typename K, typename V, typename Hash = std::hash<K>>
using ChurnHeavyHashMap = HashMap<K, V, Hash, ChurnHeavyPolicy>;

template <typename K, typename V, typename Hash = std::hash<K>>
using SpaceEfficientHashMap = HashMap<K, V, Hash, SpaceEfficientPolicy>;

}  // namespace perfmap

#endif  // PERFMAP_HASH_MAP_H_
