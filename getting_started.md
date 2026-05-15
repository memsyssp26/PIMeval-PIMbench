# PIMeval ECC Framework — Getting Started

This document describes the ECC (Error-Correcting Code) framework added to PIMeval on the `feature/ecc-complete-framework` branch. It covers what changed from `main`, how to use the new features, and how to configure them.

## What Changed from Main

This branch adds ECC simulation support to PIMeval, allowing researchers to model the performance, energy, and reliability trade-offs of different error correction schemes in Processing-In-Memory architectures.

### New Capabilities

- **Three ECC schemes**: SECDED (single-error-correct, double-error-detect), Reed-Solomon (symbol-level detection), and CRC-32 (detection only)
- **Two-tier DRAM ECC model**: On-die ECC (ODECC, DDR5/HBM3-informed) fires on every row activation; controller-level ECC fires at transfer boundaries
- **Scratchpad/register-file ECC**: Analytical overhead model for on-chip SRAM in PIM architectures (bit-serial register files, AiM global buffers)
- **Bit Error Rate (BER) model**: Poisson-based analytical model for estimating corrected/uncorrectable error counts given raw BER
- **Configurable parameters**: ECC type, granularity (block size), recursive layers, latency, and energy per operation
- **Three configuration methods**: CLI arguments (`--pim-*`), config files (`.cfg` profiles), and environment variables (`PIMEVAL_*`)
- **Performance/energy modeling**: ECC overhead is automatically included in runtime and energy estimates
- **Error injection APIs**: `pimInjectError()` and `pimInjectBurstError()` for reliability testing
- **Pre-built config profiles**: Ready-to-use configurations in `configs/ecc_profiles/`

### Files Added

| File | Purpose |
|------|---------|
| `libpimeval/src/pimEcc.h` | Core ECC encode/decode implementations (SECDED, RS, CRC-32) |
| `libpimeval/src/pimEccStrategy.h/.cpp` | Strategy pattern for polymorphic ECC dispatch + factory |
| `libpimeval/src/pimMemoryTier.h/.cpp` | Memory tier abstraction (functional vs bit-mapped) |
| `libpimeval/src/pimBerModel.h` | Bit error rate analytical model (Poisson approximation) |
| `configs/ecc_profiles/*.cfg` | Pre-built ECC configuration profiles |
| `tests/ecc/` | ECC unit, integration, and hardening test suites |
| `demo_ecc_features.sh` | Interactive demo script |
| `compare_ecc.sh` | ECC vs no-ECC performance comparison script |
| `util/ecc_sweeper.py` | Parameter sweep utility |

### Files Modified

Key changes to existing files:

- **`pimSimConfig.h/.cpp`** — Registry-based config system with ECC parameters, `--pim-*` CLI arg parsing
- **`pimResMgr.h/.cpp`** — ECC encode/decode during memory sync operations
- **`pimPerfEnergyBase.cpp`** — ECC latency/energy included in performance model
- **`pimDevice.h/.cpp`** — Error injection API support
- **`pimSim.h/.cpp`** — `pimInit(argc, argv)` for CLI arg pre-parsing
- **`libpimeval.h/.cpp`** — Public API additions (`pimInit`, `pimInjectError`, `pimInjectBurstError`)
- **`util/util.h`** — `createDevice()` accepts optional `argc/argv` for CLI forwarding

## Building

```bash
# Standard build (no DRAMsim3)
make clean && make perf

# With DRAMsim3 integration
export DRAMSIM3_PATH=$(pwd)/third-party/DRAMsim3
make clean && make dramsim3_integ
```

## Configuring ECC

ECC can be configured three ways. All methods use the same parameter names:

| Parameter | Values | Default | Description |
|-----------|--------|---------|-------------|
| `ecc` | `0`, `1`, `true`, `false` | `0` | Enable/disable ECC |
| `ecc_type` | `secded`, `rs`, `crc32` | `secded` | ECC scheme |
| `ecc_granularity` | Power of 2, 8-512 | `64` | Protection block size in bits |
| `ecc_layers` | `1`-`4` | `1` | Recursive ECC layers |
| `ecc_latency_ns` | Float | Scheme default | Override ECC latency (ns per block) |
| `ecc_energy_pj` | Float | Scheme default | Override ECC energy (pJ per block) |

Default latency/energy per scheme (when not overridden):

| Scheme | Latency (ns) | Energy (pJ) | Capability |
|--------|-------------|-------------|------------|
| SECDED | 2.0 | 2.0 | Corrects 1-bit, detects 2-bit errors |
| CRC-32 | 5.0 | 5.0 | Detects errors (no correction) |
| RS | 20.0 | 20.0 | Detects symbol-level errors |

### Method 1: CLI Arguments (Recommended)

Pass `--pim-<param>=<value>` flags directly to any benchmark that calls `pimInit()`:

```bash
./vec-add.out --pim-ecc=1 --pim-ecc_type=secded --pim-ecc_granularity=64 -l 1024

# With latency/energy overrides
./vec-add.out --pim-ecc=1 --pim-ecc_type=rs --pim-ecc_latency_ns=50.0 --pim-ecc_energy_pj=25.0 -l 1024
```

The `--pim-*` args are automatically stripped before the benchmark's own argument parser sees them. Benchmark-specific flags (like `-l`) work normally alongside them.

> **Note**: For CLI args to work, the benchmark must call `pimInit(&argc, &argv)` before parsing its own args. Currently, `vec-add` has this integrated. Other benchmarks can adopt it by adding one line to `main()` (see [Adopting in Other Benchmarks](#adopting-in-other-benchmarks)).

### Method 2: Config File Profiles

Use a `.cfg` file with `--pim-sim_config=<path>`:

```bash
# Use a pre-built profile
./vec-add.out --pim-sim_config=configs/ecc_profiles/high_perf.cfg -l 1024

# Override specific values from a profile
./vec-add.out --pim-sim_config=configs/ecc_profiles/max_reliability.cfg --pim-ecc_latency_ns=50.0 -l 1024
```

Pre-built profiles in `configs/ecc_profiles/`:

| Profile | Scheme | Granularity | Latency | Energy | Use Case |
|---------|--------|-------------|---------|--------|----------|
| `high_perf.cfg` | SECDED | 64-bit | 2 ns | 2 pJ | Low-overhead protection |
| `max_reliability.cfg` | RS | 32-bit | 20 ns | 15 pJ | Maximum error coverage |
| `detect_only.cfg` | CRC-32 | 512-bit | 5 ns | 0.5 pJ | Lightweight detection |

Config file format (key=value, one per line):
```
ecc=1
ecc_type=secded
ecc_granularity=64
ecc_latency_ns=2.0
ecc_energy_pj=2.0
```

### Method 3: Environment Variables

Set `PIMEVAL_<PARAM>` environment variables (uppercased):

```bash
export PIMEVAL_ECC=1
export PIMEVAL_ECC_TYPE=secded
export PIMEVAL_ECC_GRANULARITY=64
./vec-add.out -l 1024
```

### Precedence

When the same parameter is set in multiple places, the highest-priority source wins:

**CLI args > Config file > Environment variables > Defaults**

## Running Tests

```bash
# ECC unit tests (SECDED math, CRC-32 math)
make -C tests/ecc perf
tests/ecc/unit/test-ecc-math.out

# ECC integration tests (round-trip encode/decode for all schemes)
tests/ecc/integration/test-ecc-system.out

# ECC hardening tests (error injection + detection)
tests/ecc/hardening/test-ecc-hardening.out

# All ECC tests
make -C tests/ecc run-unit run-integ run-harden

# Full project functional tests
tests/test-functional/run-pre-commit-tests.sh
```

## Running the Demo

```bash
chmod +x demo_ecc_features.sh
./demo_ecc_features.sh
```

This builds PIMeval and runs vec-add with five different ECC configurations, showing the performance impact of each.

## Adopting in Other Benchmarks

To add `--pim-*` CLI arg support to any benchmark, add one line at the top of `main()`:

```cpp
int main(int argc, char** argv)
{
    pimInit(&argc, &argv);  // Add this line — strips --pim-* args
    // ... rest of existing code unchanged ...
}
```

This is optional. Benchmarks that don't call `pimInit()` can still use ECC via environment variables or by passing a config file through the benchmark's existing `-c` flag (if it has one).

## Using the Error Injection API

For reliability studies, you can inject errors into PIM objects:

```cpp
// Inject a single-bit error at element 0, bit position 0
pimInjectError(objId, /*elemIdx=*/0, /*bitIdx=*/0);

// Inject a 4-bit burst error at element 10, starting at bit 0
pimInjectBurstError(objId, /*elemIdx=*/10, /*bitIdx=*/0, /*length=*/4);
```

When ECC is enabled, errors are detected/corrected during `pimCopyDeviceToHost()`. The ECC scheme determines whether errors are corrected (SECDED for 1-bit) or only detected (CRC-32, RS).

## Architecture Overview

```
  Benchmark (vec-add, etc.)
       |
       | pimInit(&argc, &argv)     -- parses --pim-* CLI args
       | pimCreateDevice(...)       -- creates device with ECC config
       |
  libpimeval
       |
       +-- pimSimConfig             -- registry-based config (CLI > cfg > env)
       |     +-- pimEccFactory      -- creates strategy from config
       |
       +-- pimEccStrategy           -- polymorphic encode/decode dispatch
       |     +-- SECDED strategy    -- Hamming(72,64) + DED
       |     +-- CRC-32 strategy    -- CRC-32 detection
       |     +-- RS strategy        -- Reed-Solomon symbol-level detection
       |
       +-- pimResMgr                -- ECC encode on write, decode on read
       |     +-- syncToSimulatedMem()   -- encode data before storing
       |     +-- syncFromSimulatedMem() -- decode data after loading
       |
       +-- pimPerfEnergyBase        -- adds ECC latency/energy to stats
```

## Scratchpad/Register-File ECC

PIM architectures with on-chip SRAM (scratchpad memories, register files, global row buffers) have a separate ECC tier. Enable it with:

```bash
./vec-add.out --pim-scratchpad=1 --pim-scratchpad_ecc=1 --pim-scratchpad_ecc_type=secded -l 1024
```

| Parameter | Default | Description |
|-----------|---------|-------------|
| `scratchpad` | `false` | Enable scratchpad modeling |
| `scratchpad_size_kb` | `64` | Capacity in KB |
| `scratchpad_word_bits` | `32` | SRAM word width (ECC granularity) |
| `scratchpad_ecc` | `true` | Enable ECC on scratchpad (requires `scratchpad=true`) |
| `scratchpad_ecc_type` | `secded` | Scheme: `secded` or `none` |
| `scratchpad_ecc_latency_ns` | `0.5` | Decode latency per word access (ns) |
| `scratchpad_ecc_energy_pj` | `0.3` | Logic energy per word access (pJ) |

## BER (Bit Error Rate) Model

The BER model estimates expected corrected and uncorrectable error counts using a Poisson approximation. Configure with:

| Parameter | Default | Description |
|-----------|---------|-------------|
| `dram_ber` | `1e-10` | Raw DRAM BER per bit (before on-die ECC) |
| `sram_ber` | `1e-15` | SRAM BER per bit (scratchpad/register file) |

BER results appear in the ODECC stats line when on-die ECC is enabled.

## Known Limitations

- **RS correction**: The Reed-Solomon implementation uses 8 check bits per 64 data bits (1 check symbol per block), which provides symbol-level error detection but not correction. Full RS correction would require additional check symbols.
- **Multi-layer ECC via uint64_t path**: The uint64_t convenience encode/decode in the SECDED strategy applies single-layer encoding. The vector<bool> path (used internally) correctly handles multiple layers.
- **Benchmark adoption**: Currently only `vec-add` has `pimInit()` integrated for CLI arg parsing. Other benchmarks can use environment variables or add the one-line `pimInit()` call.
