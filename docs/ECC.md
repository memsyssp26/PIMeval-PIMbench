# PIM-Compatible ECC Architecture

This document describes the design and implementation of Error Correction Codes (ECC) within the PIMeval simulator.

## Two-Tier ECC Model

PIMeval models ECC as a two-tier system, matching real DDR5/HBM3 DRAM:

### Tier 1: On-Die ECC (ODECC)
- **What it models**: Mandatory on-die ECC found in DDR5 and HBM3 JEDEC specifications.
- **Algorithm**: 128+8 SECDED (configurable) — operates transparently inside the DRAM die.
- **When it fires**: On **every DRAM row activation**, including during PIM in-memory compute operations. This is the key distinction from controller-level ECC.
- **Overhead**: Adds ~6.25% storage overhead (die-area cost), plus per-access latency (~1ns) and energy (~0.5pJ).
- **Enabled by default** when controller-level ECC is enabled (DDR5/HBM3 mandate it). Can be controlled independently.

### Tier 2: Controller-Level ECC
- **What it models**: The memory controller's ECC engine (optional, configurable).
- **When it fires**: Only at host-device transfer boundaries (encode on write, decode on read). Does **not** fire during PIM compute operations.
- **Schemes**: SECDED, CRC-32, or Reed-Solomon (see below).

### Why Two Tiers Matter for PIM
In PIM architectures, data is computed in-place inside DRAM without host intervention. Controller-level ECC never fires during these compute phases — only on-die ECC protects the data. This means:
- PIM compute cost includes ODECC overhead (per row activation)
- Controller ECC cost only appears during data transfers
- Storage overhead is the sum of both tiers

## Supported Controller-Level Schemes

### 1. SECDED (Single Error Correction, Double Error Detection)
- **Algorithm**: Standard Hamming code with an additional parity bit for double-bit detection.
- **Overhead**: Calculates parity bits based on the specified granularity (e.g., 8 bits of overhead for 64 bits of data).
- **Behavior**: Automatically corrects single-bit flips during `syncFromSimulatedMem` and logs uncorrectable double-bit errors.

### 2. CRC-32 (Cyclic Redundancy Check)
- **Algorithm**: Uses the standard CRC-32 polynomial (0x04C11DB7).
- **Overhead**: Fixed 32-bit overhead per block.
- **Behavior**: Purely detection-based. Any corruption in the data or the checksum will trigger an "Uncorrectable Error" status.

### 3. Reed-Solomon (Abstract Model)
- **Design**: Modeled as a symbol-based correction scheme.
- **Overhead**: Calculated as 8 bits of overhead for every 64 bits of data (matching typical RS symbol ratios).
- **Behavior**: Modeled to handle burst errors (symbol corruption).

## Configuration

### Controller-Level ECC

| Parameter | Environment Variable | CLI Argument | Values |
| :--- | :--- | :--- | :--- |
| `ecc` | `PIMEVAL_ECC` | `--pim-ecc` | `0` or `1` |
| `ecc_type` | `PIMEVAL_ECC_TYPE` | `--pim-ecc_type` | `secded`, `crc32`, `rs` |
| `ecc_granularity` | `PIMEVAL_ECC_GRANULARITY` | `--pim-ecc_granularity` | `32`, `64`, `128` (bits) |
| `ecc_layers` | `PIMEVAL_ECC_LAYERS` | `--pim-ecc_layers` | `1`, `2`, ... |

### On-Die ECC (ODECC)

| Parameter | Environment Variable | CLI Argument | Default | Description |
| :--- | :--- | :--- | :--- | :--- |
| `odecc` | `PIMEVAL_ODECC` | `--pim-odecc` | `true` when ECC enabled | Enable/disable ODECC |
| `odecc_data_width` | `PIMEVAL_ODECC_DATA_WIDTH` | `--pim-odecc_data_width` | `128` | Bits per ODECC codeword |
| `odecc_parity_width` | `PIMEVAL_ODECC_PARITY_WIDTH` | `--pim-odecc_parity_width` | `8` | Parity bits per codeword |
| `odecc_latency_ns` | `PIMEVAL_ODECC_LATENCY_NS` | `--pim-odecc_latency_ns` | `1.0` | Decode latency per access |
| `odecc_energy_pj` | `PIMEVAL_ODECC_ENERGY_PJ` | `--pim-odecc_energy_pj` | `0.5` | Decode energy per access |

## Implementation Details

### Padded Bit Modeling
PIMeval models ECC by increasing the "Simulated Bit Width" of PIM objects.
- ODECC overhead is applied first (die-area cost)
- Controller-level ECC overhead is applied second
- If you allocate an `INT32` object with both tiers enabled, each element will occupy more bits than the raw 32, reflecting realistic capacity loss.

### Stats Output
When ECC is enabled, the stats display shows both tiers:
```
ECC Reliability Stats:
                                    On-Die ECC : 0 corrected, 0 uncorrectable
                                Controller ECC : 0 corrected, 0 uncorrectable
```

### Fault Injection
Use `pimInjectBurstError` to verify reliability:
```cpp
// Flip 4 consecutive bits starting at bit 0 of element 10
pimInjectBurstError(objId, 10, 0, 4);
```
