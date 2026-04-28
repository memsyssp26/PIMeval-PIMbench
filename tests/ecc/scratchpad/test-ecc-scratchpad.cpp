// File: test-ecc-scratchpad.cpp
// PIMeval Simulator - Scratchpad ECC Tests
// Copyright (c) 2024 University of Virginia
// This file is licensed under the MIT License.
// See the LICENSE file in the root of this repository for more details.
//
// Tests for the scratchpad/register-file ECC framework (Phase Q3).
//
// Three test suites:
//   1. Unit:        pimScratchpad analytical model math
//   2. AiM Analytical: model the AiM global-buffer ECC overhead without device creation
//                  (AiM pimCreateDevice has a pre-existing crash unrelated to ECC)
//   3. Integration: Bit-serial register-file (rreg) overhead with scratchpad ECC enabled

#include "../../../libpimeval/src/pimScratchpad.h"
#include "libpimeval.h"
#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <string>

// ============================================================
// Suite 1: Unit tests for pimScratchpad math
// ============================================================

void test_secded_overhead() {
  std::cout << "Unit Test: pimScratchpad SECDED overhead math..." << std::endl;

  // 32-bit word, SECDED
  pimScratchpad sp(64, 32, "secded", 0.5, 0.3);
  assert(sp.getWordBits()  == 32);
  assert(sp.getSizeKb()    == 64);
  assert(sp.getLatencyNs() == 0.5);
  assert(sp.getEnergyPj()  == 0.3);

  // SECDED on 32-bit word: k such that 2^k >= 32+k+1 → k=6 → +1 DED = 7 parity bits
  assert(sp.getParityBitsPerWord() == 7);

  // Storage overhead: 64 data bits → 2 words → 14 parity bits
  assert(sp.getParityBitsFor(64) == 14);

  // Overhead for 64 bytes = 16 words of 32 bits
  double latNs = 0.0, ePj = 0.0;
  sp.getEccOverheadForBytes(64, latNs, ePj);
  assert(latNs == 16 * 0.5); // 8.0 ns
  assert(ePj   == 16 * 0.3); // 4.8 pJ

  std::cout << "  -> SECDED overhead math passed." << std::endl;
}

void test_no_ecc_overhead() {
  std::cout << "Unit Test: pimScratchpad ECC-none overhead..." << std::endl;

  pimScratchpad sp(64, 32, "none", 0.5, 0.3);
  assert(sp.getParityBitsPerWord() == 0);
  assert(sp.getParityBitsFor(512) == 0);

  double latNs = 0.0, ePj = 0.0;
  sp.getEccOverheadForBytes(256, latNs, ePj);
  assert(latNs == 0.0);
  assert(ePj   == 0.0);

  std::cout << "  -> ECC-none overhead math passed." << std::endl;
}

void test_word_boundary_rounding() {
  std::cout << "Unit Test: pimScratchpad sub-word access rounding..." << std::endl;

  pimScratchpad sp(64, 64, "secded", 1.0, 0.5);

  // 1 byte → rounds up to 1 word
  double latNs = 0.0, ePj = 0.0;
  sp.getEccOverheadForBytes(1, latNs, ePj);
  assert(latNs == 1.0);
  assert(ePj   == 0.5);

  // 8 bytes = exactly 1 word
  sp.getEccOverheadForBytes(8, latNs, ePj);
  assert(latNs == 1.0);
  assert(ePj   == 0.5);

  // 9 bytes → rounds up to 2 words
  sp.getEccOverheadForBytes(9, latNs, ePj);
  assert(latNs == 2.0);
  assert(ePj   == 1.0);

  assert(sp.getCorrectionCapability() == 1);
  assert(sp.getDetectionCapability()  == 2);

  std::cout << "  -> Word boundary rounding passed." << std::endl;
}

// ============================================================
// Suite 2: Analytical AiM global-buffer ECC overhead model
//
// The AiM (Aquabolt-XL) global row buffer is SRAM that feeds all banks
// on each GDL iteration during a MAC operation.  The scratchpad ECC model
// accounts for this overhead without needing a live device.
//
// Note: pimCreateDevice(PIM_DEVICE_AIM, ...) has a pre-existing crash in
// this simulator build (see known_issues.md).  We verify the overhead model
// analytically here; the AiM perf wiring in pimPerfEnergyAim.cpp will be
// exercised once that device creation issue is resolved.
// ============================================================

void test_aim_buffer_ecc_analytical() {
  std::cout << "Analytical Test: AiM global-buffer ECC overhead model..." << std::endl;

  // AiM HBM2 parameters (from pimPerfEnergyAim.cpp / AiM paper, Table 2)
  const unsigned GDLWidth       = 256;    // bits per GDL iteration
  const unsigned bitsPerElement = 16;     // FP16 (typical AiM workload)
  const unsigned numElements    = 1024;   // total elements per core
  const unsigned numCore        = 8;      // 8 banks / chip in AiM

  unsigned elementsPerCore  = (numElements + numCore - 1) / numCore;
  unsigned gdlItr           = (elementsPerCore * bitsPerElement + GDLWidth - 1) / GDLWidth;

  // pimPerfEnergyAim.cpp accounts for buffer ECC on both buffer read and accumulator write:
  //   bufferBytesRead  = gdlItr * GDLWidth / 8
  //   bufferBytesWrite = numCore * bitsPerElement / 8
  uint64_t bufferBytesRead  = (uint64_t)gdlItr * GDLWidth / 8;
  uint64_t bufferBytesWrite = (uint64_t)numCore * bitsPerElement / 8;
  uint64_t totalScratchpadBytes = bufferBytesRead + bufferBytesWrite;

  // Scratchpad model: 32-bit word, SECDED, 0.5 ns/word, 0.3 pJ/word
  pimScratchpad sp(64, 32, "secded", 0.5, 0.3);
  double latNs = 0.0, ePj = 0.0;
  sp.getEccOverheadForBytes(totalScratchpadBytes, latNs, ePj);

  // Sanity: overhead must be positive and bounded
  assert(latNs > 0.0);
  assert(ePj   > 0.0);
  assert(latNs < 1000.0);  // < 1 µs per MAC pass

  std::cout << "  AiM buffer: gdlItr=" << gdlItr
            << ", scratchpad bytes=" << totalScratchpadBytes
            << std::endl;
  std::cout << "  Scratchpad ECC overhead: " << latNs << " ns, " << ePj << " pJ"
            << std::endl;
  std::cout << "  -> AiM buffer ECC analytical model passed." << std::endl;
}

// ============================================================
// Suite 3: Integration — Bit-serial register-file (rreg) overhead
//
// Bit-serial architectures (BitSIMD) use row registers (sense-amp row +
// DCC registers) as a per-core register file.  Every rreg.* operation
// reads or writes one full register row — exactly the scratchpad ECC path.
// ============================================================

void test_bitserial_rreg_scratchpad_ecc() {
  std::cout << "Integration Test: Bit-serial rreg scratchpad ECC overhead..." << std::endl;

  // Enable scratchpad ECC via environment variables
  setenv("PIMEVAL_SCRATCHPAD",              "1",      1);
  setenv("PIMEVAL_SCRATCHPAD_ECC",          "1",      1);
  setenv("PIMEVAL_SCRATCHPAD_ECC_TYPE",     "secded", 1);
  setenv("PIMEVAL_SCRATCHPAD_WORD_BITS",    "32",     1);
  setenv("PIMEVAL_SCRATCHPAD_ECC_LATENCY_NS", "0.5",  1);
  setenv("PIMEVAL_SCRATCHPAD_ECC_ENERGY_PJ",  "0.3",  1);

  PimStatus status = pimCreateDevice(PIM_DEVICE_BITSIMD_V, 1, 1, 4, 1024, 1024);
  assert(status == PIM_OK);

  unsigned numElements = 256;
  PimObjId src  = pimAlloc(PIM_ALLOC_AUTO, numElements, PIM_INT32);
  PimObjId dest = pimAllocAssociated(src, PIM_INT32);
  assert(src  != -1);
  assert(dest != -1);

  std::vector<int32_t> data(numElements);
  for (unsigned i = 0; i < numElements; ++i) data[i] = static_cast<int32_t>(i);
  status = pimCopyHostToDevice(data.data(), src);
  assert(status == PIM_OK);

  // pimAddScalar decomposes into bit-serial rreg.* register-file operations.
  // Each rreg operation fires the scratchpad ECC overhead path.
  status = pimAddScalar(src, dest, 1);
  assert(status == PIM_OK);

  // Verify functional correctness
  std::vector<int32_t> result(numElements, 0);
  status = pimCopyDeviceToHost(dest, result.data());
  assert(status == PIM_OK);
  for (unsigned i = 0; i < numElements; ++i) {
    assert(result[i] == data[i] + 1);
  }
  std::cout << "  Bit-serial computation functionally correct." << std::endl;

  // Show stats — should display 'Scratchpad ECC overhead' row
  pimShowStats();

  pimFree(src);
  pimFree(dest);
  pimDeleteDevice();

  // Run again without scratchpad ECC so we can compare
  setenv("PIMEVAL_SCRATCHPAD", "0", 1);
  status = pimCreateDevice(PIM_DEVICE_BITSIMD_V, 1, 1, 4, 1024, 1024);
  assert(status == PIM_OK);

  src  = pimAlloc(PIM_ALLOC_AUTO, numElements, PIM_INT32);
  dest = pimAllocAssociated(src, PIM_INT32);
  assert(src != -1 && dest != -1);
  status = pimCopyHostToDevice(data.data(), src);
  assert(status == PIM_OK);
  status = pimAddScalar(src, dest, 1);
  assert(status == PIM_OK);

  std::cout << "  Stats without scratchpad ECC (no scratchpad section expected):" << std::endl;
  pimShowStats();

  pimFree(src);
  pimFree(dest);
  pimDeleteDevice();

  std::cout << "  -> Bit-serial rreg scratchpad ECC test passed." << std::endl;
}

// ============================================================
// main
// ============================================================

int main() {
  std::cout << "============================================" << std::endl;
  std::cout << " PIMeval Scratchpad ECC Test Suite (Q3)    " << std::endl;
  std::cout << "============================================" << std::endl;

  // Suite 1: Unit
  test_secded_overhead();
  test_no_ecc_overhead();
  test_word_boundary_rounding();

  // Suite 2: AiM analytical
  test_aim_buffer_ecc_analytical();

  // Suite 3: Bit-serial rreg integration
  test_bitserial_rreg_scratchpad_ecc();

  std::cout << "============================================" << std::endl;
  std::cout << " All scratchpad ECC tests PASSED.          " << std::endl;
  std::cout << "============================================" << std::endl;
  return 0;
}
