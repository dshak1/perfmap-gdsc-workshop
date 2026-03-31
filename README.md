# PerfMap 10D Starter

This is the incomplete workshop starter version of PerfMap.

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

## First Run: What Should Happen

On the first clone, the project should:

- configure successfully
- build successfully
- pass the easy correctness tests
- fail exactly two tests

Those two failing tests are the workshop target:

- `HashMapStarterTest.DeleteShouldNotBreakProbeChain`
- `HashMapStarterTest.RehashPreservesEntries`

That is intentional. The repo is designed to give you a fast feedback loop:
build works, most tests pass, and two important invariants are still broken.

## Best Workshop Loop

Start with the full run once:

```bash
ctest --test-dir build --output-on-failure
```

Then switch to focused reruns for the quick dopamine hit:

```bash
ctest --test-dir build -R DeleteShouldNotBreakProbeChain --output-on-failure
ctest --test-dir build -R RehashPreservesEntries --output-on-failure
```

Suggested order:

1. Fix tombstone-aware deletion first.
2. Re-run only `DeleteShouldNotBreakProbeChain` until it passes.
3. Fix growth / rehash behavior.
4. Re-run only `RehashPreservesEntries` until it passes.
5. Run the full suite again.

Final check:

```bash
ctest --test-dir build --output-on-failure
```

## About Benchmarking

This starter repo is correctness-first. It is meant to teach the core data
structure ideas before throwing students into a larger benchmark harness.

So the fast feedback loop here is:

- red test
- code change
- green test

If you want the bigger performance dopamine hit after that, compare your
understanding against the completed `perfmap` repo, which includes the full
Google Benchmark setup and the workload-specific optimized variants.

## Important Notes

- The starter project is supposed to have failing tests at first.
- The full polished implementation lives in the main `perfmap` repo.
- The point of this repo is to teach the core invariants, not to hand over the
  final answer immediately.
