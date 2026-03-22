# PIM-Compatible ECC Architecture

This document describes the design and implementation of Error Correction Codes (ECC) within the PIMeval simulator.

## Supported Schemes

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

ECC is configured either via the Simulator Configuration File (`.cfg`) or environment variables:

| Parameter | Environment Variable | Values |
| :--- | :--- | :--- |
| `ecc_enabled` | `PIMEVAL_ECC` | `0` or `1` |
| `ecc_type` | `PIMEVAL_ECC_TYPE` | `secded`, `crc32`, `rs` |
| `ecc_granularity` | `PIMEVAL_ECC_GRANULARITY` | `32`, `64`, `128` (bits) |
| `ecc_layers` | `PIMEVAL_ECC_LAYERS` | `1`, `2`, ... |

## Implementation Details

### Padded Bit Modeling
PIMeval models ECC by increasing the "Simulated Bit Width" of PIM objects. 
- If you allocate an `INT32` object with SECDED at 32-bit granularity, each element will physically occupy ~39 bits in the simulated DRAM array.
- This allows the simulator to accurately model the **capacity loss**, **performance latency**, and **energy consumption** of real ECC hardware.

### Fault Injection
Use `pimInjectBurstError` to verify reliability:
```cpp
// Flip 4 consecutive bits starting at bit 0 of element 10
pimInjectBurstError(objId, 10, 0, 4);
```
