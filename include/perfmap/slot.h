#ifndef PERFMAP_STARTER_SLOT_H_
#define PERFMAP_STARTER_SLOT_H_

namespace perfmap {

enum class SlotState {
  kEmpty,
  kOccupied,
  kDeleted,
};

template <typename K, typename V>
struct Slot {
  SlotState state = SlotState::kEmpty;
  K key{};
  V value{};
};

}  // namespace perfmap

#endif  // PERFMAP_STARTER_SLOT_H_
