# PerfMap 10D Starter

This is the ** incomplete workshop starter version of PerfMap.

The goal is to give attendees something they can clone, build, and fix during
the session instead of just reading the finished implementation.

## What Is Included

- a minimal open-addressing hash map skeleton
- a focused test suite
- TODOs around the two most important ideas:
  - tombstone-aware deletion
  - rehashing when load factor grows too high


## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
ctest --test-dir build --output-on-failure
```

## Important Notes

- The starter project is supposed to have failing tests at first.
- The full polished implementation lives in the main `perfmap` repo.
- The point of this repo is to teach the core invariants, not to hand over the
  final answer immediately.
