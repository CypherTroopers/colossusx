# Colossus-X Test Vectors

This document defines the **required canonical test-vector set** for any Colossus-X reference implementation.

> `spec.md` is the source of truth.
> This file defines **what vectors must exist** and **how they must be recorded**.

## Purpose

The goal of this file is to make it impossible for a claimed implementation to be "almost correct".

A conforming implementation must reproduce the canonical vectors for:

1. epoch seed derivation
2. header digest derivation
3. epoch cache generation
4. single-item derivation
5. scratch initialization
6. one-lane one-round transition
7. full nonce replay
8. final PoW output

## Important Note

This file intentionally defines the **vector format and required cases**.
It does **not** invent final numeric outputs by hand.

The canonical hex outputs must be generated from the accepted bit-exact reference implementation, then frozen here.
Do not guess them. Do not hand-edit them after generation except to correct formatting mistakes.

## Formatting Rules

All test vectors must follow these rules:

- all byte strings are lowercase hex
- no `0x` prefixes
- little-endian interpretation must match `spec.md`
- all field names are stable and exact
- vector IDs are immutable once published
- any change to canonical outputs requires a spec-level change review

## Required Vector Set

---

## TV-001: Epoch Seed Derivation

### Purpose
Validate deterministic derivation of `E` from:

- `chain_id`
- `epoch_index`
- `prev_epoch_anchor_hash`

### Required Fields

```text
vector_id: TV-001
chain_id:
epoch_index:
prev_epoch_anchor_hash:
expected_epoch_seed:
```

### Required Cases

- minimum nonzero epoch index
- typical mid-range epoch index
- large epoch index
- different `chain_id` values producing distinct outputs

---

## TV-002: Header Digest

### Purpose
Validate canonical header serialization excluding nonce and PoW result fields.

### Required Fields

```text
vector_id: TV-002
header_description:
serialized_header_without_nonce:
expected_header_digest:
```

### Required Cases

- minimal valid header
- header with nontrivial field values
- headers differing in one field only
- explicit case proving nonce exclusion

---

## TV-003: Epoch Cache Generation

### Purpose
Validate deterministic generation of `C_e` from `E`.

### Required Fields

```text
vector_id: TV-003
profile:
epoch_seed:
cache_size:
cache_sample_offsets:
expected_cache_sample_hex:
```

### Required Cases

- CX-18 cache sample
- CX-32 cache sample
- at least one case with multiple sample offsets

### Recording Rule

Do not dump the full cache in this file.
Instead, record:

- cache size
- exact sampled offsets
- exact bytes expected at those offsets
- optional whole-cache hash for integrity

Optional extra field:

```text
expected_cache_hash:
```

---

## TV-004: Single Item Derivation

### Purpose
Validate `GET_ITEM(i)`.

### Required Fields

```text
vector_id: TV-004
profile:
epoch_seed:
item_index:
expected_item_hex:
```

### Required Cases

- first item (`i = 0`)
- low index item
- mid-range item
- high index item near dataset end
- same index across different profiles where allowed

### Required Property

The item must be derivable from `E`, `C_e`, and `item_index` only.

---

## TV-005: Scratch Initialization

### Purpose
Validate deterministic scratch initialization from:

- `E`
- `header_digest`
- `nonce`
- `lane_id`

### Required Fields

```text
vector_id: TV-005
profile:
epoch_seed:
header_digest:
nonce:
lane_id:
expected_scratch_prefix_hex:
expected_scratch_hash:
```

### Recording Rule

Because scratch may be large, record:

- a fixed prefix
- a whole-scratch hash

Do not record only a hash. Prefix bytes are required to catch partial mistakes.

---

## TV-006: One-Lane One-Round Transition

### Purpose
Validate one complete round transition for exactly one lane.

### Required Fields

```text
vector_id: TV-006
profile:
epoch_seed:
header_digest:
nonce:
lane_id:
round_index:
initial_state_x:
initial_state_a:
initial_scratch_reads:
expected_i0:
expected_i1:
expected_i2:
expected_d0_hash:
expected_d1_hash:
expected_d2_hash:
expected_write_index:
expected_written_value:
expected_next_state_x:
expected_next_state_a:
```

### Required Cases

- first round of first lane
- nonzero round index case
- at least one case per active profile

### Recording Rule

Use hashes for large item payloads where needed, but record exact indices and exact final state words.

---

## TV-007: Full Nonce Replay

### Purpose
Validate end-to-end deterministic replay for one full nonce.

### Required Fields

```text
vector_id: TV-007
profile:
epoch_seed:
header_digest:
nonce:
expected_final_lane_state_hash:
expected_final_lane_acc_hash:
expected_pow_hash:
```

### Required Cases

- at least one per active profile
- one case using the recommended initial profile CX-18
- one case with a deliberately different nonce on the same header

---

## TV-008: PoW Validity Check

### Purpose
Validate final comparison semantics.

### Required Fields

```text
vector_id: TV-008
pow_hash:
target:
expected_valid:
```

### Required Cases

- `pow_hash < target`
- `pow_hash == target`
- `pow_hash > target`
- edge case near zero
- edge case near max 256-bit value

---

## Recommended Serialization Template

Use the following human-readable structure for frozen vectors:

```text
## TV-004-A
vector_id: TV-004-A
profile: CX-18
epoch_seed: <hex>
item_index: 0
expected_item_hex: <hex>
notes: first dataset item for CX-18
```

## Naming Convention

Use stable IDs like:

- `TV-001-A`
- `TV-001-B`
- `TV-004-A`
- `TV-006-C`

Do not renumber existing vectors after publication.

## Minimum Acceptance Rule

An implementation is non-conforming unless it reproduces all frozen vectors exactly.

That includes:

- exact bytes
- exact indices
- exact state transitions
- exact final `pow_hash`

## Generation Workflow

1. finish the accepted bit-exact reference implementation
2. generate vectors directly from the reference implementation
3. independently re-run generation at least once
4. freeze outputs in this file
5. use those vectors in CI

## Change Control

If a vector changes, one of only two things is true:

1. the implementation was wrong before and is being corrected
2. the spec changed

In either case, the change must be reviewed explicitly.

Silent vector drift is not allowed.

## TODO Before First Freeze

The following canonical values still need to be generated from the accepted reference implementation:

- [ ] TV-001 concrete outputs
- [ ] TV-002 concrete outputs
- [ ] TV-003 sampled cache outputs
- [ ] TV-004 item outputs
- [ ] TV-005 scratch outputs
- [ ] TV-006 one-round transition outputs
- [ ] TV-007 full replay outputs
- [ ] TV-008 comparison outputs
