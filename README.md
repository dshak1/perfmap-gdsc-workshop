# PerfMap 10D Starter

This is the **intentionally incomplete** workshop starter version of PerfMap.

The goal is to give attendees something they can clone, build, and fix during
the session instead of just reading the finished implementation.

## What Is Included

- a minimal open-addressing hash map skeleton
- a focused test suite
- TODOs around the two most important ideas:
  - tombstone-aware deletion
  - rehashing when load factor grows too high

## Workshop Goal

By the end of the session, attendees should be able to:

1. explain why open addressing is cache-friendly
2. implement or repair tombstone semantics
3. make the map survive growth and rehashing
4. compare their starter build against the completed main repo

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
