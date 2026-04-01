// Copyright 2026 SFU GDSC — PerfMap Workshop
//
// A flat slot representation for open-addressing hash tables.
// Each slot lives in a contiguous std::vector, giving us cache-friendly
// sequential access during probing — the key advantage over chained maps.

#ifndef PERFMAP_SLOT_H_
#define PERFMAP_SLOT_H_

#include <cstdint>

namespace perfmap {

// Slot lifecycle:
//   kEmpty     → kOccupied  (insert)
//   kOccupied  → kDeleted   (erase — tombstone)
//   kDeleted   → kOccupied  (reuse during insert)
//
// Why kDeleted exists:
//   In open addressing, a lookup probes forward until it hits kEmpty.
//   If we erased a slot by setting it to kEmpty, we'd break any probe
//   chain that passes through it — subsequent lookups would stop too
//   early. Tombstones (kDeleted) say "keep searching, something used
//   to live here."
enum class SlotState : uint8_t {
  kEmpty,     // Never been used — ends a probe chain
  kOccupied,  // Contains a live key-value pair
  kDeleted,   // Tombstone — keep probing past this
};

// A single slot in the hash table's flat array.
// Storing state + key + value together in one struct means that when the
// CPU loads a cache line for the probe, it gets everything it needs
// without an extra pointer chase.
template <typename K, typename V>
struct Slot {
  SlotState state = SlotState::kEmpty;
  K key{};
  V value{};
};

}  // namespace perfmap

#endif  // PERFMAP_SLOT_H_
