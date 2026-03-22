# Project Plan: PIM-Compatible ECC Implementation

## Overview
This project aims to implement and evaluate PIM-compatible Error Correction Code (ECC) mechanisms within the PIMeval simulator. The goal is to provide a framework for modeling various ECC schemes, their overheads (capacity, performance, energy), and their effectiveness in protecting PIM data.

## Phase 1: Foundation and Baseline Analysis (Completed)
- [x] Review existing SECDED implementation in `pimEcc.h`.
- [x] Analyze ECC integration in `pimResMgr.cpp` (resource management and sync).
- [x] Understand existing ECC configuration in `pimSimConfig`.
- [x] Evaluate the baseline test `tests/test-ecc-flexibility/`.

## Phase 2: Enhanced ECC Modeling and Framework (Completed)
- [x] **Refine Configuration:** Ensure ECC can be toggled via config files and environment variables consistently across all device types.
- [x] **Granularity Support:** Validate and enhance support for different ECC granularities (32B, 64B, 128B) and their impact on allocation.
- [x] **Bank-level ECC Modeling:** Investigate and implement logic to distinguish between bank-level and rank-level ECC if applicable to the simulator's hierarchy.

## Phase 3: Advanced ECC Schemes (Completed)
- [x] **Reed-Solomon (RS):** Implement RS encoding/decoding for better multi-bit error correction.
- [x] **CRC Support:** Implement Cyclic Redundancy Check (CRC-32) for error detection.
- [x] **Configurable Schemes:** Allow users to specify ECC schemes and their parameters.

## Phase 4: Performance and Energy Impact (Completed)
- [x] **Latency Modeling:** Add latency overhead for ECC encoding/decoding during PIM operations and data movement.
- [x] **Energy Modeling:** Add energy consumption for ECC logic activations.
- [x] **Trade-off Analysis:** Enable systematic evaluation of capacity vs. reliability vs. performance/energy.

## Phase 5: Comprehensive Testing and Validation (Completed)
- [x] **Error Injection Suite:** Expand `pimInjectError` to support more complex error patterns.
- [x] **Automated Validation:** Create a suite of tests that automatically verify detection/correction for each implemented ECC scheme.
- [x] **Benchmark Evaluation:** Run standard PIMbench workloads with ECC enabled and measure the impact.

## Phase 7: Production Hardening (Completed)
- [x] **Burst Error Support:** Implemented `pimInjectBurstError` to model multi-bit failure patterns.
- [x] **Detailed Reporting:** Added ECC reliability metrics (Corrected vs. Uncorrectable) to `pimShowStats`.
- [x] **Stability Proof:** Verified that RS and CRC provide defense against burst failures that break SECDED.

## Phase 8: Portability & Onboarding (Completed)
- [x] **Automated Setup:** Created `setup.sh` to handle dependencies and submodules.
- [x] **Containerization:** Provided a `Dockerfile` for reproducible, isolated environments.
- [x] **One-Command Verification:** Added `make check` to verify the entire stack instantly.
- [x] **Documentation:** Simplified `README.md` for a "3-minute onboarding" experience.

## Phase 9: Documentation & Developer Experience (Completed)
- [x] **Doxygen Integration:** Created `Doxyfile` and added API comments for automated documentation.
- [x] **Usage Examples:** Added "Basic Usage" snippets to `README.md`.
- [x] **Example Sandbox:** Created `examples/hello_world_pim.cpp` for easy onboarding.
- [x] **Architectural Deep Dive:** Documented ECC internals in `docs/ECC.md`.

## Phase 10: Architectural Refactoring (Completed)
- [x] **ECC Strategy Pattern:** Replaced `if-else` string checks with a polymorphic `pimEccStrategy` and factory.
- [x] **Polymorphic Devices:** Transitioned `pimDevice` into an abstract base, eliminating massive switch statements.
- [x] **Type-Safe Configuration:** Introduced `pimConfigParam<T>` template for centralized parsing, validation, and defaults.
- [x] **Smart Memory Management:** Switched to `std::shared_ptr` and RAII for PIM objects, ensuring safe references and automatic row reclamation.

## Phase 6: DRAMSim3 Integration (Future Work)
- **Goal:** Provide high-fidelity, cycle-accurate timing and power analysis.
- **Steps:**
    1.  Investigate current hooks in `third-party/DRAMsim3`.
    2.  Map PIM functional operations to specific DRAM command sequences.
    3.  Hook ECC events to DRAMSim3 for detailed reliability-performance trade-off analysis.

## Potential Future Paths
1.  **Memory Optimization:** Refactor `pimCore` bit-storage to use bit-packing or sparse row models to support massive system scaling (e.g., 128+ banks/rank) without hitting OS memory limits.
2.  **Advanced PIM Target Specialization:** Implement high-fidelity functional and performance models for `AiM` and `Aquabolt`, leveraging their specialized MAC units and on-chip buffer management.
3.  **Compiler Integration:** Bridge the gap between high-level PIM kernels and hardware-specific instructions through a unified PIM intermediate representation (IR).

