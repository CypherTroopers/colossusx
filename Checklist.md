# Colossus-X Codex Implementation Review Checklist

> Purpose: use this checklist to review any Codex-produced implementation of Colossus-X and detect design drift before benchmarking or integration.

## Review Outcome

- [ ] **Accepted for benchmark phase**
- [ ] **Rejected due to consensus risk**
- [ ] **Rejected due to economic/security drift**
- [ ] **Rejected due to verifier failure**
- [ ] **Rejected due to implementation non-determinism**

---

## 1. Scope and Repository Hygiene

- [ ] The repository clearly separates **reference implementation**, **miner path**, **verifier path**, and **benchmark harness**.
- [ ] The active profile table is present and matches the current Colossus-X compressed spec.
- [ ] Consensus-critical code is isolated from backend-specific optimization code.
- [ ] Backend-specific code is clearly marked as **non-consensus**.
- [ ] There is no hidden dependency on OS page size, NUMA policy, hugepages, or managed-memory migration behavior in consensus-critical logic.
- [ ] The implementation includes reproducible build instructions.
- [ ] The implementation includes deterministic test vectors for all consensus-critical stages.

**Fail immediately if:**
- [ ] consensus logic is mixed with CUDA/HIP/Metal-specific assumptions
- [ ] build flags or compiler behavior can change consensus results
- [ ] repository structure makes reference verification impossible

---

## 2. Consensus Determinism

- [ ] All consensus-critical arithmetic is integer-only.
- [ ] The implementation uses explicit little-endian encoding/decoding.
- [ ] Overflow behavior is explicit and matches modulo `2^64` semantics.
- [ ] Rotation behavior is explicit and identical across platforms.
- [ ] There is no floating-point usage in consensus code.
- [ ] There is no BF16/FP16/FP32 usage in consensus code.
- [ ] There is no tensor-core-dependent behavior in consensus code.
- [ ] There is no undefined behavior in C/C++/Rust/Go code that could vary by compiler or architecture.
- [ ] The same `(header, nonce, profile, epoch state)` always yields the same `pow_hash` on all supported backends.

**Required evidence:**
- [ ] cross-platform test vector comparison
- [ ] backend parity test report
- [ ] bit-exact replay test

**Fail immediately if:**
- [ ] CPU and GPU produce different outputs for the same test vector
- [ ] any consensus path depends on floating-point or approximate math
- [ ] endian behavior is implicit instead of specified

---

## 3. Item-Level Semantics

- [ ] Consensus semantics are defined at the **item** level, not page level.
- [ ] `item_size` is exactly the configured consensus value.
- [ ] Physical page size is treated as an implementation detail only.
- [ ] The code does not require 4 KiB, 16 KiB, 64 KiB, hugepages, or any specific VM/page configuration for correctness.
- [ ] Item addressing is deterministic and independent of backend memory layout.

**Fail immediately if:**
- [ ] page size affects consensus output
- [ ] correctness changes when layout or page size changes
- [ ] TLB/page-migration behavior is assumed by consensus logic

---

## 4. Epoch Seed and Header Digest

- [ ] Epoch seed derivation is domain-separated and deterministic.
- [ ] Header digest excludes nonce and PoW result fields exactly as specified.
- [ ] Header serialization is canonical.
- [ ] Test vectors exist for epoch seed and header digest.

**Fail immediately if:**
- [ ] alternate serializations can produce different valid results
- [ ] domain separation tags are inconsistent or omitted

---

## 5. Epoch Cache (`C_e`)

- [ ] `C_e` is derived only from consensus-defined inputs.
- [ ] `C_e` generation is verifier-accessible and backend-independent.
- [ ] `C_e` fits the profile’s configured `cache_size`.
- [ ] `C_e` is the only required persistent epoch-side verifier state.
- [ ] Test vectors exist for cache generation.

**Fail immediately if:**
- [ ] verifier needs full dataset materialization instead of `C_e`
- [ ] cache generation differs across backends
- [ ] cache generation depends on runtime memory layout

---

## 6. Dataset (`D_e`) and `GET_ITEM(i)`

- [ ] `D_e` is modeled as an array of items, not pages.
- [ ] `GET_ITEM(i)` is deterministic and local.
- [ ] `GET_ITEM(i)` can derive item `i` without precomputing neighboring items.
- [ ] `GET_ITEM(i)` uses the configured number of dispersed cache taps.
- [ ] `GET_ITEM(i)` includes the required data-dependent perturbations.
- [ ] `GET_ITEM(i)` uses only integer operations after seed expansion.
- [ ] Test vectors exist for single-item derivation.
- [ ] Miner mode can load from resident `D_e`.
- [ ] Verifier mode can derive the same item on demand from `C_e`.

**Design intent check:**
- [ ] `GET_ITEM(i)` is expensive enough that full resident dataset retention is materially beneficial.
- [ ] `GET_ITEM(i)` is not so expensive that the PoW becomes compute-dominated instead of memory-dominated.

**Fail immediately if:**
- [ ] verifier must materialize full dataset to check a block
- [ ] `GET_ITEM(i)` depends on global mutable state
- [ ] on-demand derivation yields different bytes than resident dataset loading

---

## 7. Scratchpad (`S_n`)

- [ ] Scratch is nonce-local.
- [ ] Scratch is writable.
- [ ] Scratch initialization is deterministic from consensus inputs.
- [ ] Scratch read/write order is deterministic.
- [ ] Scratch participates in round index derivation.
- [ ] Scratch writes affect future rounds.
- [ ] Scratch cannot be removed without materially changing the PoW.
- [ ] Test vectors exist for scratch initialization.

**Fail immediately if:**
- [ ] scratch is effectively optional
- [ ] scratch writes do not influence future execution
- [ ] scratch behavior differs across backends

---

## 8. Round Function Structure

- [ ] Each round performs at least **3 dataset item reads**.
- [ ] Each round performs at least **2 scratch reads**.
- [ ] Each round performs at least **1 scratch write**.
- [ ] At least one intra-round sequential dependency chain exists.
- [ ] Later dataset indices depend on earlier dataset contents and scratch contents.
- [ ] The round update path uses only consensus-approved integer operations.
- [ ] Round execution order is fully deterministic.
- [ ] A one-lane, one-round test vector exists.

**Fail immediately if:**
- [ ] all dataset indices are known at round start
- [ ] scratch is read but not written
- [ ] round structure collapses to simple table lookup plus hashing
- [ ] heavy XOF/hash calls dominate the hot loop

---

## 9. Hot Loop Dominance

- [ ] Profiling shows the hot loop is dominated by memory traffic, not hashing.
- [ ] XOF/hash calls are confined to initialization and non-hot-path setup.
- [ ] Memory reads/writes scale with the intended PoW cost.
- [ ] Replacing memory traffic with extra compute is not cheap.

**Required evidence:**
- [ ] profiler output or instrumentation report
- [ ] bytes moved per hash/nonces-per-second report

**Fail immediately if:**
- [ ] SHAKE/XOF time dominates round execution
- [ ] the implementation is effectively compute-heavy

---

## 10. Miner / Verifier Separation

- [ ] Miner mode and verifier mode are explicitly separated.
- [ ] Miner mode may use resident `D_e`.
- [ ] Verifier mode must not require resident `D_e`.
- [ ] Verifier mode re-derives only needed items from `C_e`.
- [ ] Verifier and miner produce identical `pow_hash` for the same nonce.

**Fail immediately if:**
- [ ] verifier requires full dataset
- [ ] verifier depends on sampled proofs or auxiliary transcripts in v1
- [ ] verifier path is missing or incomplete

---

## 11. Verifier Hard Limits

- [ ] `verifier_peak_ram < 1 GiB`
- [ ] `verifier_peak_ram <= cache_size + scratch_total + 64 MiB` (preferred rule)
- [ ] `verification_latency <= block_time × 0.10`
- [ ] preferred target: `verification_latency <= block_time × 0.05`
- [ ] `cache_rebuild_time <= epoch_wall_time × 0.01`
- [ ] preferred target: `cache_rebuild_time <= epoch_wall_time × 0.005`
- [ ] item derivation is local enough that verifier does not drift toward full-dataset reconstruction.

**Required evidence:**
- [ ] peak RAM measurement
- [ ] verification latency measurement
- [ ] cache rebuild measurement

**Fail immediately if:**
- [ ] any verifier hard limit is exceeded
- [ ] verifier performance depends on full dataset materialization

---

## 12. CPU/GPU Boundary Discipline

- [ ] Consensus correctness does not depend on fine-grained CPU↔GPU ping-pong.
- [ ] Nonce or nonce-batch execution can run in one domain for a substantial interval.
- [ ] CUDA/HIP/Metal paths are optimization paths, not alternate consensus definitions.
- [ ] Hardware-coherent and software-coherent memory modes do not alter results.

**Fail immediately if:**
- [ ] per-round CPU↔GPU synchronization is required for correctness
- [ ] backend memory model changes consensus output

---

## 13. Profile Conformance

- [ ] Active profile matches the chain configuration exactly.
- [ ] Dataset size, cache size, scratch size, lane count, item size, scratch unit, round count, and tap count match the profile.
- [ ] No implicit auto-tuning changes consensus-critical parameters.
- [ ] Any runtime tuning is explicitly limited to non-consensus optimization.

**Fail immediately if:**
- [ ] implementation silently changes a consensus parameter
- [ ] different backends pick different effective round structure or sizes

---

## 14. TMTO Evaluation

### Required benchmark modes
- [ ] Full retention
- [ ] 50% retention
- [ ] 25% retention
- [ ] Minimal retention
- [ ] Top-10% hotset retention
- [ ] Top-25% hotset retention
- [ ] Compressed-item retention
- [ ] Scratch elision / recomputation variant

### Required metrics
- [ ] Throughput / hashes per second
- [ ] Bytes moved per hash
- [ ] Resident memory footprint
- [ ] On-demand item derivations per hash
- [ ] Energy per hash, if available

### Acceptance thresholds
- [ ] `S_50 >= 1.8`
- [ ] `S_25 >= 3.5`
- [ ] `S_min >= 8.0`
- [ ] top-10% hotset retention does **not** exceed 70% of full-retention throughput
- [ ] top-25% hotset retention does **not** exceed 85% of full-retention throughput
- [ ] compressed-item retention does **not** exceed 80% of full-retention throughput
- [ ] scratch elision / recomputation does **not** exceed 90% of full-retention throughput

**Fail immediately if:**
- [ ] any mandatory TMTO threshold is missed without explicit design review sign-off
- [ ] full retention is only weakly better than partial retention

---

## 15. Attack-Model Coverage

Confirm that the implementation has been reviewed against all mandatory Colossus-X attack classes:

- [ ] Half-memory miner
- [ ] Quarter-memory miner
- [ ] Minimal-retention miner
- [ ] Hot-item / hotset miner
- [ ] Region-biased miner
- [ ] Adaptive cache miner
- [ ] Item compression miner
- [ ] Scratch elision miner
- [ ] Hybrid compress-recompute miner
- [ ] Early-index prefetch miner
- [ ] Batch-nonce miner
- [ ] Lane-fusion miner
- [ ] Pipeline miner
- [ ] SRAM-heavy ASIC model
- [ ] HBM-rich ASIC model
- [ ] Split-memory ASIC model
- [ ] Hardware-coherent advantage miner
- [ ] Software-coherent fallback miner
- [ ] Discrete-GPU emulation miner
- [ ] Verifier re-execution blowup
- [ ] Epoch rebuild blowup

**Fail immediately if:**
- [ ] the benchmark/report ignores one or more mandatory attack classes

---

## 16. Test Vectors and Replayability

- [ ] Test vectors exist for epoch seed derivation.
- [ ] Test vectors exist for header digest derivation.
- [ ] Test vectors exist for cache generation.
- [ ] Test vectors exist for single item derivation.
- [ ] Test vectors exist for scratch initialization.
- [ ] Test vectors exist for one-lane one-round state transition.
- [ ] Test vectors exist for full nonce replay.
- [ ] Test vectors exist for final `pow_hash`.
- [ ] Tests are reproducible across CPU and GPU backends.

**Fail immediately if:**
- [ ] full replay cannot be reproduced from published test vectors
- [ ] backend-specific test vectors differ in consensus output

---

## 17. Redesign / Drift Detection

Mark any of the following as a **design drift incident**:

- [ ] round item count reduced below mandatory minimum
- [ ] scratch write removed or made non-essential
- [ ] heavy hashing added into the hot loop
- [ ] dataset retention no longer materially beneficial
- [ ] verifier now requires full dataset or proof dependency
- [ ] page semantics leak into consensus
- [ ] consensus result depends on backend behavior
- [ ] new optimization changes consensus-critical structure

If any box above is checked:
- [ ] **Reject implementation until spec review is completed**

---

## 18. Final Sign-Off

### Consensus Safety
- [ ] Pass
- [ ] Fail

### Miner / Verifier Architecture
- [ ] Pass
- [ ] Fail

### Memory-Dominant Behavior
- [ ] Pass
- [ ] Fail

### TMTO / Partial-Retention Resistance
- [ ] Pass
- [ ] Fail

### Backend Neutrality
- [ ] Pass
- [ ] Fail

### Verifier Feasibility
- [ ] Pass
- [ ] Fail

### Overall Decision
- [ ] **Approve for implementation continuation**
- [ ] **Approve for benchmark phase only**
- [ ] **Reject and return to spec review**

---

## Reviewer Notes

### Findings


### Required Changes


### Open Questions


### Benchmark Artifacts Reviewed


