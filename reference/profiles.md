# Colossus-X Profiles

This document provides a quick reference for the active Colossus-X profile table.

> `spec.md` is the source of truth.
> This file is a convenience reference for implementers and reviewers.

## Protocol Timing

- `target_block_time = 60 seconds`
- `target_epoch_wall_time = 72 hours`
- `epoch_length = 4320 blocks`

## Profile Rules

- Exactly one profile is active per chain configuration.
- A chain must not mix multiple profiles at the same time.
- Profile changes are protocol changes and must not be treated as runtime heuristics.

## Active Profiles

| Profile | Dataset Size | Cache Size | Scratch Total | Lanes | Item Size | Scratch Unit | Rounds | GET_ITEM Taps |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| CX-18  | 12 GiB | 256 MiB | 128 MiB | 8 | 256 B | 128 B | 8192  | 24 |
| CX-32  | 24 GiB | 256 MiB | 192 MiB | 8 | 256 B | 128 B | 12288 | 24 |
| CX-64  | 48 GiB | 512 MiB | 256 MiB | 8 | 256 B | 128 B | 16384 | 24 |
| CX-128 | 96 GiB | 512 MiB | 384 MiB | 8 | 256 B | 128 B | 16384 | 24 |

## Recommended Initial Testnet Profile

The recommended initial testnet profile is:

- **CX-18**

This is the preferred starting point for early reference implementations, correctness work, and benchmark development.

## Profile Interpretation Notes

### Dataset Size

`dataset_size` is the miner-advantage resident dataset size.

- miners may hold it resident
- verifiers must not require full materialization
- full dataset retention should be materially beneficial

### Cache Size

`cache_size` is the verifier-resident epoch state budget.

- `C_e` must fit within this size
- `C_e` is the persistent verifier-side epoch state for PoW verification
- verifiers derive required items from `C_e` on demand

### Scratch Total

`scratch_total` is the total writable nonce-local scratch budget.

- it must be actively read and written during the hot loop
- it must remain consensus-visible through deterministic replay
- it must be large enough that it is not intended to collapse entirely into tiny on-chip memory

### Lanes

`lanes` is the number of per-lane execution contexts used for one nonce replay.

- lane execution order must not affect consensus
- implementations may schedule lanes differently, but the result must remain bit-exact

### Item Size

`item_size` is the consensus unit for dataset access.

- consensus is item-level, not page-level
- page size, hugepage behavior, and migration policy are implementation details only

### Scratch Unit

`scratch_unit` is the basic scratch access granularity used by the round function.

### Rounds

`rounds` is the exact round count executed per lane.

- this is a consensus parameter
- implementations must not auto-tune or adapt it dynamically

### GET_ITEM Taps

`get_item_taps` is the number of dispersed cache taps required for one dataset item derivation.

- item derivation must remain local and deterministic
- item derivation must remain expensive enough that resident dataset retention is materially beneficial to miners

## Hard Constraints

All profiles are subject to the following hard verifier constraints:

- `verifier_peak_ram < 1 GiB`
- `verification_latency <= block_time × 0.10`
- `cache_rebuild_time <= epoch_wall_time × 0.01`

Preferred targets:

- `verification_latency <= block_time × 0.05`
- `cache_rebuild_time <= epoch_wall_time × 0.005`

## Implementation Reminder

Do not reinterpret these values as soft performance hints.
They are protocol parameters for the selected profile.
