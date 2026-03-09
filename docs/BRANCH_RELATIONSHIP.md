# Branch Relationship: master vs develop

This is a memory aid for current repository state.

## Shared Base

- Common ancestor: `b247807` (2022-05-11)

## Divergence Snapshot (when last checked)

- `master...develop` left/right count: `8 2`
  - `master` has 8 unique commits
  - `develop` has 2 unique commits

## Notable Develop-Only Commits

1. `ca0c6bb` Updated CMakeLists to make GUI build optional
2. `d87787d` Removed remotery profiler integration

## Practical Meaning

- `develop` contains newer rendering/build cleanup work.
- `master` contains additional repo/housekeeping commits (README/workflow/funding changes).

