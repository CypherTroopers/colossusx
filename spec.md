Colossus-X Final Compressed Spec v0.1
Status: codex implementation handoff draft
Normative language: MUST / MUST NOT / SHOULD / MAY

1. Purpose
Colossus-X is an integer-only, bit-exact, memory-fabric-hard PoW designed so that
miners benefit from holding a large resident dataset on future coherent/unified-memory
hardware, while verifiers re-execute with a much smaller epoch state.

2. Non-negotiable invariants
2.1 Consensus semantics MUST be integer-only and deterministic.
2.2 Security target is memory-cost enforcement, not absolute ASIC elimination.
2.3 Miner state and verifier state MUST be separated.
2.4 The large dataset MUST be economically worth resident retention.
2.5 Consensus unit MUST be item-level, not physical page-level.
2.6 The hot loop MUST be dominated by memory movement, not hashing.
2.7 A writable scratchpad MUST exist in addition to the large read-mostly dataset.
2.8 Fine-grained CPU-GPU ping-pong MUST NOT be required.
2.9 Reduced-memory re-execution verification MUST work without proofs.
2.10 Designs are accepted/rejected by measured TMTO / partial-retention penalties.

3. Consensus primitives
3.1 Byte order: little-endian.
3.2 Integer domain: u64.
3.3 Allowed round arithmetic: xor, add mod 2^64, mul mod 2^64, rotate, fixed permutation.
3.4 Floating-point, BF16, FP16, FP32, tensor-core semantics MUST NOT affect consensus.
3.5 SHAKE256 is allowed for epoch/header/cache/scratch initialization only.
3.6 The hot loop MUST NOT depend on repeated heavy XOF calls.

4. Consensus objects
4.1 E  = epoch seed.
4.2 C_e = epoch cache, verifier-resident state.
4.3 D_e = large resident dataset, miner-advantage state.
4.4 S_n = nonce scratchpad, writable.
4.5 X_l, A_l = per-lane state and accumulator.

5. Active profiles
Exactly one profile is active per chain configuration.
Recommended initial testnet profile: CX-18.

CX-18:
  dataset_size   = 12 GiB
  cache_size     = 256 MiB
  scratch_total  = 128 MiB
  lanes          = 8
  item_size      = 256 B
  scratch_unit   = 128 B
  rounds         = 8192
  get_item_taps  = 24

CX-32:
  dataset_size   = 24 GiB
  cache_size     = 256 MiB
  scratch_total  = 192 MiB
  lanes          = 8
  item_size      = 256 B
  scratch_unit   = 128 B
  rounds         = 12288
  get_item_taps  = 24

CX-64:
  dataset_size   = 48 GiB
  cache_size     = 512 MiB
  scratch_total  = 256 MiB
  lanes          = 8
  item_size      = 256 B
  scratch_unit   = 128 B
  rounds         = 16384
  get_item_taps  = 24

CX-128:
  dataset_size   = 96 GiB
  cache_size     = 512 MiB
  scratch_total  = 384 MiB
  lanes          = 8
  item_size      = 256 B
  scratch_unit   = 128 B
  rounds         = 16384
  get_item_taps  = 24

6. Chain timing
6.1 target_block_time = 60 seconds.
6.2 target_epoch_wall_time = 72 hours.
6.3 epoch_length = round(72h / target_block_time) = 4320 blocks.

7. Epoch seed
7.1 E MUST be deterministically derived from:
    chain_id || epoch_index || prev_epoch_anchor_hash
7.2 E derivation MUST be domain-separated and bit-exact.

8. Header digest
8.1 header_digest MUST exclude nonce and PoW-result fields.
8.2 Serialization MUST be canonical and deterministic.

9. Epoch cache C_e
9.1 C_e MUST be derivable by all validators from E alone.
9.2 C_e generation MAY use SHAKE256 and fixed integer mixing.
9.3 C_e MUST fit within the profile cache_size.
9.4 C_e MUST be the only persistent verifier-side epoch state needed for PoW verification.

10. Dataset D_e
10.1 D_e is a conceptual array of 256-byte items.
10.2 Consensus semantics are item-level only.
10.3 Physical pages, hugepages, NUMA policy, managed-memory migration policy, and layout
     are implementation details and MUST NOT affect consensus.
10.4 D_e items MUST be derivable individually from (E, C_e, item_index).
10.5 D_e MUST NOT need to be fully materialized by verifiers.

11. GET_ITEM(i)
11.1 Input: epoch seed E, cache C_e, item index i.
11.2 Output: exactly one 256-byte item.
11.3 GET_ITEM(i) MUST:
     - read 24 dispersed cache taps from C_e
     - include at least 2 data-dependent tap perturbations
     - use only integer operations after initial seed expansion
     - be deterministic and local: item i can be derived without building neighboring items
11.4 GET_ITEM(i) MUST be expensive enough that miners are materially better off holding D_e resident.

12. Scratch S_n
12.1 S_n is nonce-local and writable.
12.2 S_n MUST be initialized deterministically from (E, header_digest, nonce, lane_id).
12.3 S_n MUST be actively read and written during the hot loop.
12.4 S_n MUST be large enough that fully absorbing it into tiny on-chip memory is not the intended path.

13. Per-lane state
13.1 Each lane l has state X_l and accumulator A_l.
13.2 X_l and A_l MUST be initialized deterministically from (E, header_digest, nonce, lane_id).
13.3 Lane execution order MUST NOT affect consensus result.

14. Round function
Each lane executes exactly profile.rounds rounds.

14.1 Per round, the minimum mandatory structure is:
     - 3 dataset item reads
     - 2 scratch reads
     - 1 scratch write
     - at least one intra-round sequential dependency chain

14.2 Abstract round sequence:
     a) derive provisional indices from (X_l, A_l, round)
     b) read scratch slots s0, s1
     c) derive i0 from (X_l, A_l, round)
     d) d0 = LOAD_ITEM(i0)
     e) derive i1 from (X_l, A_l, d0, s0)
     f) d1 = LOAD_ITEM(i1)
     g) derive i2 from (X_l, A_l, d0, d1, s1)
     h) d2 = LOAD_ITEM(i2)
     i) compute T0, T1, T2 using integer-only Mix()
     j) update X_l and A_l
     k) write one scratch slot with derived value W
     l) next round depends on updated X_l, A_l, and scratch contents

14.3 LOAD_ITEM(i):
     - miners MAY serve from resident D_e
     - verifiers MUST be able to call GET_ITEM(i) on demand
     - consensus result MUST be identical

14.4 Mix() requirements:
     - integer-only
     - bit-exact
     - bounded arithmetic mod 2^64
     - no undefined behavior
     - strong enough diffusion for state evolution
     - low enough compute cost that memory traffic remains dominant

15. Final reduction
15.1 After all lanes finish, lane states are folded deterministically.
15.2 Final PoW output MUST be a 32-byte digest.
15.3 Valid block condition: pow_hash interpreted as LE 256-bit integer < target.

16. Verification model
16.1 V1 verification MUST be proof-free reduced-memory re-execution.
16.2 Verifier inputs:
     - chain config
     - active profile
     - block header
     - nonce
     - target
     - E
     - C_e
16.3 Verifier procedure:
     - recompute header_digest
     - recompute or load E
     - recompute or load C_e
     - initialize scratch and lane states
     - re-derive only needed items via GET_ITEM(i)
     - replay all rounds
     - recompute pow_hash
     - compare against target
16.4 Verifier MUST NOT require full D_e materialization.
16.5 Verifier MUST NOT require sampled proofs, Merkle proofs, STARKs, or SNARKs.

17. Verifier hard limits
17.1 verifier_peak_ram MUST be < 1 GiB.
17.2 verifier_peak_ram SHOULD satisfy:
     peak_ram <= cache_size + scratch_total + 64 MiB
17.3 verification_latency MUST be <= block_time * 0.10
17.4 preferred target: verification_latency <= block_time * 0.05
17.5 cache_rebuild_time MUST be <= epoch_wall_time * 0.01
17.6 preferred target: cache_rebuild_time <= epoch_wall_time * 0.005

18. Attack acceptance thresholds
18.1 TMTO metrics:
     S_50  = throughput(full retention) / throughput(50% retention)
     S_25  = throughput(full retention) / throughput(25% retention)
     S_min = throughput(full retention) / throughput(minimal retention)

18.2 Minimum acceptance targets:
     S_50  >= 1.8
     S_25  >= 3.5
     S_min >= 8.0

18.3 Partial-retention rejection rules:
     - if top 10% hotset retention yields >70% of full-retention throughput: reject
     - if top 25% hotset retention yields >85% of full-retention throughput: reject

18.4 Compression rejection rules:
     - if compressed-item retention yields >80% of full-retention throughput: reject
     - if scratch elision / recomputation yields >90% of full-retention throughput: reject

19. Explicit non-goals
19.1 Absolute ASIC resistance is NOT a goal.
19.2 Floating-point/tensor-core consensus semantics are NOT allowed.
19.3 Page-size-dependent consensus behavior is NOT allowed.
19.4 Fine-grained CPU-GPU round-by-round synchronization is NOT required.
19.5 V1 does NOT require succinct proofs.

20. Implementation deliverables expected from codex
20.1 Reference implementation with bit-exact deterministic behavior.
20.2 Separate miner mode and verifier mode.
20.3 GET_ITEM on-demand verifier path.
20.4 Profile-configurable parameters using the fixed profile table above.
20.5 Test vectors for:
     - epoch seed
     - header digest
     - cache generation
     - single item derivation
     - scratch initialization
     - one-lane one-round transition
     - full nonce replay
     - final pow_hash
20.6 Benchmark harness for:
     - full retention
     - 50% retention
     - 25% retention
     - minimal retention
     - hotset retention
     - compressed retention
     - verifier latency
     - cache rebuild latency

21. Rejection conditions
Reject any implementation or redesign that:
- changes consensus semantics across backends
- makes hashing, not memory traffic, dominate the hot loop
- removes writable scratch as a first-class dependency
- makes full dataset retention only weakly beneficial
- requires full dataset for verification
- depends on physical page semantics
- exceeds verifier hard limits
