# Colossus-X

Colossus-X is a future-facing, integer-only, bit-exact, memory-fabric-hard proof-of-work (PoW) design.

Its goal is not absolute ASIC elimination. Its goal is **memory-cost enforcement**: miners should materially benefit from holding a large resident dataset on future coherent/unified-memory hardware, while verifiers must still be able to validate blocks with a much smaller epoch state.

## Status

This repository is currently in the **reference-spec and reference-implementation** phase.

- Branch in focus: `test`
- Primary audience: reference implementers, reviewers, and benchmark authors
- Current implementation target: **bit-exact reference behavior first**, optimization later

## Source of Truth

The source of truth for Colossus-X is:

- [`spec.mdb](./spec.md)

If any code, benchmark, comment, or helper document conflicts with `spec.md`, **`spec.md` wins**.

## Document Map

- [`spec.md`](./spec.md): final compressed normative specification for Codex handoff
- [`REVIEW_CHECKLIST.md`](./REVIEW_CHECKLIST.md): implementation review checklist and rejection gates
- [`reference/profiles.md`](./reference/profiles.md): profile table and fixed protocol timing
- [`reference/test_vectors.md`](./reference/test_vectors.md): canonical test-vector format and required vector set
- [`IMPLEMENTATION_PLAN.md`](./IMPLEMENTATION_PLAN.md): implementation guide, repository layout, staging order, and acceptance criteria

## Design Goals

Colossus-X is built around the following goals:

1. **Integer-only consensus semantics**
2. **Bit-exact reproducibility across backends**
3. **Memory-cost enforcement instead of hash-rate-by-compute design**
4. **Miner / verifier state separation**
5. **Item-level semantics instead of physical page semantics**
6. **Memory-dominant hot loop**
7. **Writable scratchpad as a first-class dependency**
8. **Reduced-memory proof-free re-execution verification**
9. **Acceptance by measured TMTO and partial-retention penalties**

## Explicit Non-Goals

The following are **not** goals of Colossus-X v1:

- absolute ASIC resistance
- floating-point or tensor-core consensus semantics
- page-size-dependent consensus behavior
- fine-grained CPU-GPU round-by-round synchronization
- succinct proofs as a requirement for v1 verification

## High-Level Model

Colossus-X consists of five main consensus objects:

- `E`: epoch seed
- `C_e`: epoch cache (verifier-resident state)
- `D_e`: large resident dataset (miner-advantage state)
- `S_n`: nonce scratchpad (writable)
- `X_l`, `A_l`: per-lane state and accumulator

Miners may materialize and hold `D_e` resident.
Verifiers must **not** need full `D_e`; they must be able to re-derive needed items from `C_e` and replay the winning nonce.

## Repository Expectations for Implementers

Implementers must preserve the following invariants:

- consensus arithmetic is integer-only and deterministic
- `GET_ITEM(i)` is local and bit-exact
- hot-loop behavior is dominated by memory traffic, not repeated heavy XOF calls
- the round function uses:
  - 3 dataset item reads per round
  - 2 scratch reads per round
  - 1 scratch write per round
  - at least one intra-round sequential dependency chain
- verification is proof-free reduced-memory re-execution
- physical page semantics must never affect consensus

## Implementation Scope

The expected implementation order is:

1. epoch seed and header digest
2. epoch cache generation
3. `GET_ITEM(i)`
4. scratch initialization
5. round function
6. full nonce replay
7. verifier mode
8. test vectors
9. benchmark harness

## Acceptance Criteria

An implementation is not acceptable unless it satisfies all of the following:

- bit-exact deterministic behavior
- full verifier replay without full dataset materialization
- verifier peak RAM under 1 GiB
- verification latency within the limits defined in `spec.md`
- benchmark harness for:
  - full retention
  - 50% retention
  - 25% retention
  - minimal retention
  - hotset retention
  - compressed retention
  - verifier latency
  - cache rebuild latency

## Rejection Conditions

Reject any implementation or redesign that:

- changes consensus semantics across backends
- makes hashing dominate the hot loop
- removes writable scratch as a first-class dependency
- makes full dataset retention only weakly beneficial
- requires full dataset for verification
- depends on physical page semantics
- exceeds verifier hard limits

## Notes for Codex

Codex should treat this repository as a **spec-first** implementation target.

Do not optimize away semantics.
Do not replace deterministic integer transitions with backend-specific shortcuts.
Do not redefine correctness around performance.

Correctness first. Reproducibility second. Optimization third.
