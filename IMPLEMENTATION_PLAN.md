# Colossus-X Implementation Plan

This document defines the implementation scope, repository layout, work order, acceptance gates, and review expectations for the Colossus-X reference implementation.

> `spec.md` is the source of truth.
> If any implementation detail, benchmark choice, helper script, or test harness conflicts with `spec.md`, follow `spec.md`.

---

## 1. Purpose

This repository is for the **spec-first reference implementation** of Colossus-X.

The primary goals are:

1. produce a **bit-exact reference implementation**
2. keep miner mode and verifier mode **semantically identical**
3. prove that **reduced-memory proof-free verification** works
4. generate **canonical test vectors**
5. measure **TMTO / partial-retention / compression penalties**

This phase is **not** about aggressive optimization first.
Correctness first. Determinism second. Benchmarking third. Optimization last.

---

## 2. Branch and Change Scope

### Target branch

- active implementation branch: `test`

### Source of truth

- [`spec.md`](./spec.md)
- [`REVIEW_CHECKLIST.md`](./REVIEW_CHECKLIST.md)
- [`reference/profiles.md`](./reference/profiles.md)
- [`reference/test_vectors.md`](./reference/test_vectors.md)

### Allowed work

Codex and human implementers may add or modify files in:

- `core/`
- `tests/`
- `bench/`
- `reference/`
- `README.md`
- `IMPLEMENTATION_PLAN.md`

### Forbidden assumptions

Implementers must **not**:

- redefine consensus semantics around backend behavior
- make page size or memory migration part of consensus
- introduce floating-point or tensor-core consensus math
- require full dataset materialization for verification
- replace memory-dominant round behavior with hash-dominant shortcuts
- make correctness depend on CPU↔GPU round-by-round ping-pong

---

## 3. Repository Layout

The intended repository structure is:

```text
colossusx/
├─ spec.md
├─ REVIEW_CHECKLIST.md
├─ README.md
├─ IMPLEMENTATION_PLAN.md
├─ reference/
│  ├─ test_vectors.md
│  └─ profiles.md
├─ core/
│  ├─ epoch_cache.*
│  ├─ get_item.*
│  ├─ scratch.*
│  ├─ round_function.*
│  ├─ pow.*
│  └─ verifier.*
├─ tests/
│  ├─ test_epoch_cache.*
│  ├─ test_get_item.*
│  ├─ test_round_function.*
│  ├─ test_pow_replay.*
│  └─ test_vectors.*
└─ bench/
   ├─ bench_full_retention.*
   ├─ bench_half_retention.*
   ├─ bench_quarter_retention.*
   ├─ bench_minimal_retention.*
   ├─ bench_hotset_retention.*
   ├─ bench_compressed_retention.*
   └─ bench_verifier.*
```

Language-specific file extensions may vary.
The logical module boundaries should remain the same.

---

## 4. Required Module Responsibilities

### `core/epoch_cache.*`

Must implement:

- epoch seed derivation inputs
- deterministic epoch cache generation
- cache rebuild path for verifier mode

Must not implement:

- miner-only retention logic
- backend-specific consensus shortcuts

### `core/get_item.*`

Must implement:

- `GET_ITEM(i)` from `(E, C_e, item_index)`
- local on-demand item derivation
- identical semantics for miner and verifier paths

Must not implement:

- dependency on neighboring item generation
- page-size-dependent semantics

### `core/scratch.*`

Must implement:

- nonce-local deterministic scratch initialization
- scratch read/write primitives used by replay

Must not implement:

- optional scratch semantics
- write-elision as a consensus-visible optimization

### `core/round_function.*`

Must implement the mandatory round structure from `spec.md`:

- 3 dataset item reads per round
- 2 scratch reads per round
- 1 scratch write per round
- at least one intra-round sequential dependency chain

### `core/pow.*`

Must implement:

- full lane replay
- final deterministic reduction
- final `pow_hash`

### `core/verifier.*`

Must implement:

- reduced-memory proof-free re-execution
- no full dataset requirement
- exact replay for winning nonce validation

---

## 5. Implementation Order

Work must proceed in this order.
Do not skip ahead before earlier stages are correct and testable.

### Stage 1 — Consensus Inputs

Implement:

- canonical header serialization
- header digest derivation
- epoch index derivation
- epoch seed derivation

Deliverables:

- tests for header digest
- tests for epoch seed

### Stage 2 — Epoch Cache

Implement:

- deterministic `C_e` generation
- cache rebuild path
- profile-aware cache sizing

Deliverables:

- cache unit tests
- sampled cache-output checks

### Stage 3 — Item Derivation

Implement:

- deterministic `GET_ITEM(i)`
- item derivation from `(E, C_e, i)` only
- exact item-size output

Deliverables:

- single-item tests
- low/mid/high index tests

### Stage 4 — Scratch Initialization

Implement:

- nonce-local scratch initialization
- deterministic lane-specific initialization

Deliverables:

- scratch prefix tests
- scratch hash tests

### Stage 5 — Round Function

Implement:

- 3 item reads
- 2 scratch reads
- 1 scratch write
- sequential dependency chain
- state and accumulator updates

Deliverables:

- one-lane one-round tests
- explicit intermediate-index checks (`i0`, `i1`, `i2`)

### Stage 6 — Full Nonce Replay

Implement:

- all-lane replay
- final reduction
- final `pow_hash`

Deliverables:

- full replay tests
- final digest tests

### Stage 7 — Verifier Mode

Implement:

- reduced-memory replay path
- on-demand item derivation
- target comparison

Deliverables:

- verifier correctness tests
- verifier resource tracking

### Stage 8 — Test Vector Freeze

Implement:

- canonical vector generation tools
- vector export format matching `reference/test_vectors.md`

Deliverables:

- frozen vectors for TV-001 through TV-008

### Stage 9 — Benchmark Harness

Implement:

- full-retention benchmark
- 50% retention benchmark
- 25% retention benchmark
- minimal-retention benchmark
- hotset-retention benchmark
- compressed-retention benchmark
- verifier-latency benchmark
- cache-rebuild benchmark

Deliverables:

- reproducible benchmark scripts or commands
- machine-readable output format preferred

---

## 6. Acceptance Criteria

An implementation is not acceptable unless all of the following are true.

### Consensus correctness

- semantics are integer-only and bit-exact
- all required outputs are deterministic
- miner and verifier produce identical consensus results

### Verifier viability

- verifier does not require full dataset materialization
- verifier peak RAM stays below the limit in `spec.md`
- verification latency stays within the limit in `spec.md`
- cache rebuild latency stays within the limit in `spec.md`

### Structural conformance

- hot loop remains memory-dominant
- writable scratch remains first-class and unavoidable
- round structure matches the mandatory pattern
- item-level semantics are preserved

### Test coverage

- all canonical test vectors pass exactly
- tests exist for each implementation stage

### Benchmark coverage

- all required retention and verifier benchmarks exist
- TMTO / partial-retention / compression thresholds are measured

---

## 7. Benchmark Requirements

The following benchmark modes are mandatory.

### Retention benchmarks

- full retention
- 50% retention
- 25% retention
- minimal retention
- hotset retention
- compressed retention

### Verifier benchmarks

- verifier replay latency
- verifier peak RAM
- cache rebuild time

### Required metrics

At minimum, benchmarks must report:

- throughput or replay rate
- resident memory footprint
- verifier peak RAM where applicable
- latency per replay or per winning nonce verification

### Required acceptance checks

- `S_50 >= 1.8`
- `S_25 >= 3.5`
- `S_min >= 8.0`
- top 10% hotset must not exceed 70% of full-retention throughput
- top 25% hotset must not exceed 85% of full-retention throughput
- compressed retention must not exceed 80% of full-retention throughput
- scratch elision / recomputation must not exceed 90% of full-retention throughput

---

## 8. Test Vector Policy

`reference/test_vectors.md` defines the required vector set and format.

Implementation work must support freezing vectors for:

- epoch seed derivation
- header digest derivation
- epoch cache generation
- single-item derivation
- scratch initialization
- one-lane one-round transition
- full nonce replay
- final PoW validity check

Do not hand-invent vector outputs.
Vectors must be generated from the accepted bit-exact reference implementation and then frozen.

---

## 9. Review Workflow

Every substantial change should be reviewed against:

1. [`spec.md`](./spec.md)
2. [`REVIEW_CHECKLIST.md`](./REVIEW_CHECKLIST.md)
3. benchmark expectations in this file

Review order should be:

1. semantics review
2. replay correctness review
3. verifier review
4. benchmark review
5. optional optimization review

Optimization review must never come before correctness review.

---

## 10. Rejection Conditions

Reject any implementation or redesign that:

- changes consensus semantics across backends
- makes hashing dominate the hot loop
- removes writable scratch as a first-class dependency
- makes resident dataset retention only weakly beneficial
- requires full dataset for verification
- depends on physical page semantics
- exceeds verifier hard limits
- cannot reproduce canonical test vectors exactly
- skips required benchmark modes

---

## 11. Codex Guidance

Codex should treat this repository as a **reference-implementation project**, not an optimization contest.

Priorities are:

1. exact conformance to `spec.md`
2. deterministic replay and verification
3. benchmarkability
4. performance optimization only after the above are satisfied

Codex must prefer explicit, auditable logic over clever shortcuts.
Any shortcut that risks semantic drift should be rejected.

---

## 12. Exit Condition for This Phase

This implementation phase is complete only when all of the following are true:

- reference implementation exists
- verifier mode exists
- canonical test vectors are frozen
- benchmark harness exists
- review checklist passes
- TMTO and retention measurements are available

Until then, the repository should be treated as **under active specification-constrained implementation**.
