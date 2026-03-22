# Colossus-X Implementation Review Checklist

Use this checklist to review any Codex-generated or hand-written implementation against the normative requirements in [`spec.md`](./spec.md).

> `spec.md` is the source of truth.
> If this checklist and `spec.md` ever disagree, follow `spec.md`.

---

## 1. Consensus Semantics

- [ ] All consensus-visible arithmetic is integer-only.
- [ ] All arithmetic overflow behavior is explicitly modulo `2^64` where required.
- [ ] Endianness is fixed and documented as little-endian.
- [ ] No floating-point, BF16, FP16, FP32, or tensor-core semantics affect consensus.
- [ ] No undefined behavior is relied upon.
- [ ] Same input always produces the same output across CPU, CUDA, HIP, and Metal backends.

**Reject if:** any backend can produce a different consensus result.

---

## 2. Source-of-Truth Conformance

- [ ] `spec.md` is treated as normative.
- [ ] The implementation does not silently redefine parameter values from `spec.md`.
- [ ] Active profile values match the profile table in `spec.md`.
- [ ] Block timing and epoch timing match `spec.md`.

**Reject if:** implementation behavior is "close enough" but not exact.

---

## 3. Miner / Verifier State Separation

- [ ] The verifier does not require full dataset `D_e` materialization.
- [ ] The verifier can re-derive needed items from `C_e` on demand.
- [ ] The miner path may use resident `D_e`, but the verifier path must remain correct without it.
- [ ] The implementation clearly separates miner mode from verifier mode.

**Reject if:** verification practically requires the full dataset.

---

## 4. Item-Level Semantics

- [ ] Consensus semantics are defined at the item level.
- [ ] Physical pages, hugepages, NUMA layout, and migration policy are not part of consensus semantics.
- [ ] `GET_ITEM(i)` is deterministic, local, and bit-exact.
- [ ] `GET_ITEM(i)` can derive one item without building neighboring items.

**Reject if:** page size or OS memory behavior changes consensus-visible results.

---

## 5. Epoch Cache `C_e`

- [ ] `C_e` is derived only from `E` and fixed spec-defined rules.
- [ ] `C_e` fits within the configured profile cache size.
- [ ] `C_e` is sufficient as the verifier’s persistent epoch state.
- [ ] `C_e` rebuild is deterministic and reproducible.

**Reject if:** verifier correctness depends on any additional hidden epoch state.

---

## 6. Dataset `D_e`

- [ ] `D_e` is conceptually a large array of fixed-size items.
- [ ] The miner can load items from resident `D_e`.
- [ ] The verifier can derive the same items via `GET_ITEM(i)`.
- [ ] Resident retention is materially beneficial to miners.

**Reject if:** full dataset retention is only weakly beneficial.

---

## 7. `GET_ITEM(i)`

- [ ] `GET_ITEM(i)` reads the required number of dispersed cache taps.
- [ ] `GET_ITEM(i)` includes the required data-dependent tap perturbations.
- [ ] `GET_ITEM(i)` uses only deterministic integer operations after initial seed expansion.
- [ ] `GET_ITEM(i)` output is exactly one fixed-size item.
- [ ] `GET_ITEM(i)` is expensive enough that on-demand mining is materially worse than resident retention.

**Reject if:** item derivation is so cheap that dataset residency barely matters.

---

## 8. Scratch `S_n`

- [ ] Scratch is nonce-local.
- [ ] Scratch initialization is deterministic from `(E, header_digest, nonce, lane_id)`.
- [ ] Scratch is actively read during the hot loop.
- [ ] Scratch is actively written during the hot loop.
- [ ] Future rounds depend on scratch contents.

**Reject if:** scratch is decorative, optional, or can be removed without changing PoW character.

---

## 9. Round Function Structure

Per round, confirm the minimum mandatory structure:

- [ ] 3 dataset item reads
- [ ] 2 scratch reads
- [ ] 1 scratch write
- [ ] at least one intra-round sequential dependency chain

Also confirm:

- [ ] later item indices depend on earlier item contents and scratch contents
- [ ] the next round depends on updated state and updated scratch
- [ ] the round function is deterministic and bit-exact

**Reject if:** all indices are known up front and the round can be fully prefetched without real dependency.

---

## 10. Hot-Loop Dominance

- [ ] The hot loop is dominated by memory traffic, not repeated heavy XOF calls.
- [ ] SHAKE256 or other heavy XOF usage is limited to initialization stages allowed by `spec.md`.
- [ ] Integer mix operations remain lightweight enough that memory pressure stays primary.

**Reject if:** hashing becomes the main cost driver.

---

## 11. Final Reduction and PoW Output

- [ ] Lane states are folded deterministically.
- [ ] Final PoW output size matches the spec.
- [ ] PoW validity test matches `pow_hash < target` with the correct byte interpretation.

**Reject if:** any backend computes a different final digest for the same input.

---

## 12. Verifier Hard Limits

- [ ] `verifier_peak_ram < 1 GiB`
- [ ] verifier peak RAM is within the preferred profile-aware budget
- [ ] verification latency is within the hard limit from `spec.md`
- [ ] cache rebuild latency is within the hard limit from `spec.md`

**Reject if:** the verifier only works in theory, or only works with miner-class memory.

---

## 13. TMTO / Retention Benchmarks

- [ ] Full-retention benchmark exists.
- [ ] 50% retention benchmark exists.
- [ ] 25% retention benchmark exists.
- [ ] Minimal-retention benchmark exists.
- [ ] Hotset-retention benchmark exists.
- [ ] Compressed-retention benchmark exists.

Required checks:

- [ ] `S_50 >= 1.8`
- [ ] `S_25 >= 3.5`
- [ ] `S_min >= 8.0`
- [ ] top 10% hotset does not exceed 70% of full-retention throughput
- [ ] top 25% hotset does not exceed 85% of full-retention throughput
- [ ] compressed retention does not exceed 80% of full-retention throughput
- [ ] scratch elision / recomputation does not exceed 90% of full-retention throughput

**Reject if:** retention penalties are too small.

---

## 14. Backend Independence

- [ ] CPU reference path exists.
- [ ] Optional optimized backends preserve exact semantics.
- [ ] Backend-specific optimizations do not alter item derivation, round transitions, or final reduction.
- [ ] Lane execution order and scheduling differences do not alter results.

**Reject if:** correctness depends on warp width, SIMD width, thread interleaving, or page layout.

---

## 15. Unified / Coherent Memory Assumptions

- [ ] The implementation may benefit from unified/coherent-memory hardware, but correctness does not require it.
- [ ] Fine-grained CPU-GPU ping-pong is not required.
- [ ] Managed-memory or migration behavior is not part of consensus semantics.

**Reject if:** the design only works correctly under one vendor-specific coherence model.

---

## 16. Test Vectors

- [ ] Epoch seed test vectors exist.
- [ ] Header digest test vectors exist.
- [ ] Cache-generation test vectors exist.
- [ ] Single-item derivation test vectors exist.
- [ ] Scratch-initialization test vectors exist.
- [ ] One-lane one-round transition test vectors exist.
- [ ] Full nonce replay test vectors exist.
- [ ] Final `pow_hash` test vectors exist.

**Reject if:** a claimed implementation cannot reproduce the canonical vectors exactly.

---

## 17. Final Review Gate

An implementation is acceptable only if all of the following are true:

- [ ] Semantics are bit-exact and integer-only
- [ ] Verifier is reduced-memory and proof-free
- [ ] Full dataset retention is materially beneficial
- [ ] Writable scratch is first-class and unavoidable
- [ ] Hot loop is memory-dominant
- [ ] TMTO / partial-retention thresholds pass
- [ ] Verifier hard limits pass
- [ ] Test vectors pass

If any item above fails, the implementation should be considered **non-conforming**.
