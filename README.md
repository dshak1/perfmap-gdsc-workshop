# PerfMap Workshop Demo Repo

This repo contains the **minimal working version** of the PerfMap workshop
hash map.

It keeps only the files needed for the workshop demo:

- the core `HashMap` implementation
- the `Slot` representation
- the focused workshop test suite
- the small CMake build setup

The point of this repo is to give you the clean working version that turns the
workshop tests green.

## Build And Run

Run these commands from the repo root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
ctest --test-dir build --output-on-failure
```

## Expected Result

This repo should:

- configure successfully
- build successfully
- pass the workshop tests

The key tests are:

- `HashMapStarterTest.DeleteShouldNotBreakProbeChain`
- `HashMapStarterTest.RehashPreservesEntries`

Those are the two invariants the workshop starter was designed to teach:

- tombstone-aware deletion
- preserving entries across growth and rehashing

## What This Repo Is For

Use this repo when you want:

- the minimal passing implementation
- a clean demo for the workshop
- a compact reference for the final solution

If you want the full project with benchmarks, workload-specific variants, and
the larger performance story, use the main `perfmap` repo instead.
