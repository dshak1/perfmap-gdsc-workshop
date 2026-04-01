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

## Setup

To run this repo on a fresh machine, you need:

- a C++17 compiler
- CMake 3.14 or newer
- Git
- internet access on the first build so CMake can fetch dependencies

This repo uses CMake `FetchContent`, so Google Test and Abseil are downloaded
automatically during the first configure step.

### Quick Install Hints

**macOS**

- install Xcode Command Line Tools:

```bash
xcode-select --install
```

- install CMake if you do not already have it

**Ubuntu / Debian**

```bash
sudo apt update
sudo apt install -y build-essential cmake git
```

**Windows**

- install Visual Studio with the C++ build tools
- install CMake
- use either PowerShell, Developer PowerShell, or a terminal with CMake in `PATH`

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
