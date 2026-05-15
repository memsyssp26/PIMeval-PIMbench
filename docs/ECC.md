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

## Tier 3: Scratchpad/Register-File ECC

PIM architectures that use on-chip SRAM (scratchpad memories, register files, or global row buffers) face a separate reliability concern from DRAM. Tier 3 models ECC protection for these on-chip structures.

### When It Fires
- On every **read or write to the scratchpad** during PIM compute operations
- For bit-serial architectures: on every register-file access during bitwise computation
- For AiM-style architectures: on global buffer accesses during GDL iterations

### Configuration

| Parameter | Environment Variable | CLI Argument | Default | Description |
| :--- | :--- | :--- | :--- | :--- |
| `scratchpad` | `PIMEVAL_SCRATCHPAD` | `--pim-scratchpad` | `false` | Enable scratchpad modeling |
| `scratchpad_size_kb` | `PIMEVAL_SCRATCHPAD_SIZE_KB` | `--pim-scratchpad_size_kb` | `64` | Capacity in KB |
| `scratchpad_word_bits` | `PIMEVAL_SCRATCHPAD_WORD_BITS` | `--pim-scratchpad_word_bits` | `32` | SRAM word width (ECC granularity) |
| `scratchpad_ecc` | `PIMEVAL_SCRATCHPAD_ECC` | `--pim-scratchpad_ecc` | `true` | Enable ECC on scratchpad |
| `scratchpad_ecc_type` | `PIMEVAL_SCRATCHPAD_ECC_TYPE` | `--pim-scratchpad_ecc_type` | `secded` | Scheme: `secded` or `none` |
| `scratchpad_ecc_latency_ns` | `PIMEVAL_SCRATCHPAD_ECC_LATENCY_NS` | `--pim-scratchpad_ecc_latency_ns` | `0.5` | Decode latency per word (ns) |
| `scratchpad_ecc_energy_pj` | `PIMEVAL_SCRATCHPAD_ECC_ENERGY_PJ` | `--pim-scratchpad_ecc_energy_pj` | `0.3` | Logic energy per word (pJ) |

### Stats Output
When scratchpad ECC is enabled, a third tier appears in the stats:
```
ECC Reliability Stats:
                      Scratchpad ECC (compute) :  0.002064    0.000001238  [64KB, 32-bit words, secded, 0.50ns/word, 0.30pJ/word, 0 corrected]
```

## Bit Error Rate (BER) Model

PIMeval includes an analytical BER model (`pimBerModel`) that estimates expected corrected and uncorrectable error counts based on the raw bit error rate and codeword parameters.

### Model
Uses a Poisson approximation to the binomial distribution. For a codeword of `w` bits with BER `p`:
- Expected bit errors per codeword: `lambda = w * p`
- P(correctable) = P(exactly 1 error) = `lambda * exp(-lambda)` (SECDED corrects single-bit)
- P(uncorrectable) = P(2+ errors) = `1 - exp(-lambda) * (1 + lambda)`

### Configuration

| Parameter | Environment Variable | Default | Description |
| :--- | :--- | :--- | :--- |
| `dram_ber` | `PIMEVAL_DRAM_BER` | `1e-10` | Raw DRAM BER per bit (before on-die ECC) |
| `sram_ber` | `PIMEVAL_SRAM_BER` | `1e-15` | SRAM BER per bit (scratchpad/register file) |

### Typical BER Values
- **DRAM (DDR5/HBM3 raw)**: ~1e-10 per bit
- **SRAM (scratchpad/register file)**: ~1e-15 per bit

The BER model results appear in the ODECC stats line:
```
On-Die ECC (ODECC) : 0.000032  0.000000016  [128+8 SECDED, 1.0ns, 0.5pJ, ~4.35e-07 corrected, ~2.37e-15 uncorrectable]
```

### Fault Injection
Use `pimInjectBurstError` to verify reliability:
```cpp
// Flip 4 consecutive bits starting at bit 0 of element 10
pimInjectBurstError(objId, 10, 0, 4);
```
