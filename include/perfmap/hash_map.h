#ifndef PERFMAP_STARTER_HASH_MAP_H_
#define PERFMAP_STARTER_HASH_MAP_H_

#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "perfmap/slot.h"

namespace perfmap {

inline size_t MixHash(size_t h) {
  if constexpr (sizeof(size_t) == 8) {
    h *= UINT64_C(11400714819323198485);
    h ^= h >> 32;
  } else {
    h *= UINT32_C(2654435769);
    h ^= h >> 16;
  }
  return h;
}

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

template <typename K, typename V, typename Hash = std::hash<K>>
class HashMap {
 public:
  explicit HashMap(size_t initial_capacity = 16)
      : table_(RoundUpPow2(initial_capacity)),
        mask_(table_.size() - 1),
        size_(0) {}

  absl::Status Insert(const K& key, const V& value) {
    // TODO(workshop): add real load-factor-triggered rehashing.
    const size_t idx = FindSlotIndex(key);
    if (table_[idx].state != SlotState::kOccupied) {
      ++size_;
    }
    table_[idx] = {SlotState::kOccupied, key, value};
    return absl::OkStatus();
  }

  absl::StatusOr<V> Find(const K& key) const {
    const size_t idx = FindSlotIndexForLookup(key);
    if (idx == kNotFound) {
      return absl::NotFoundError("key not found");
    }
    return table_[idx].value;
  }

  absl::Status Erase(const K& key) {
    const size_t idx = FindSlotIndexForLookup(key);
    if (idx == kNotFound) {
      return absl::NotFoundError("key not found");
    }

    // TODO(workshop): this is intentionally wrong.
    // A correct open-addressing map should not mark erased slots as empty if
    // later elements in the probe chain still depend on them.
    table_[idx].state = SlotState::kEmpty;
    --size_;
    return absl::OkStatus();
  }

  void Reserve(size_t expected_size) {
    if (expected_size > table_.size()) {
      Rehash(expected_size * 2);
    }
  }

  size_t size() const { return size_; }
  bool empty() const { return size_ == 0; }
  size_t capacity() const { return table_.size(); }

  void Rehash(size_t new_capacity) {
    new_capacity = RoundUpPow2(new_capacity);
    std::vector<Slot<K, V>> old_table = std::move(table_);
    table_.assign(new_capacity, Slot<K, V>{});
    mask_ = new_capacity - 1;
    size_ = 0;

    for (const auto& slot : old_table) {
      if (slot.state == SlotState::kOccupied) {
        (void)Insert(slot.key, slot.value);
      }
    }
  }

 private:
  static constexpr size_t kNotFound = std::numeric_limits<size_t>::max();

  size_t HashKey(const K& key) const { return MixHash(hasher_(key)); }

  size_t FindSlotIndex(const K& key) const {
    size_t index = HashKey(key) & mask_;

    for (size_t i = 0; i < table_.size(); ++i) {
      const auto& slot = table_[index];
      if (slot.state == SlotState::kEmpty) {
        return index;
      }
      if (slot.state == SlotState::kOccupied && slot.key == key) {
        return index;
      }
      index = (index + 1) & mask_;
    }

    return 0;
  }

  size_t FindSlotIndexForLookup(const K& key) const {
    size_t index = HashKey(key) & mask_;

    for (size_t i = 0; i < table_.size(); ++i) {
      const auto& slot = table_[index];
      if (slot.state == SlotState::kEmpty) {
        return kNotFound;
      }
      if (slot.state == SlotState::kOccupied && slot.key == key) {
        return index;
      }
      index = (index + 1) & mask_;
    }

    return kNotFound;
  }

  std::vector<Slot<K, V>> table_;
  size_t mask_;
  size_t size_;
  Hash hasher_;
};

}  // namespace perfmap

#endif  // PERFMAP_STARTER_HASH_MAP_H_
