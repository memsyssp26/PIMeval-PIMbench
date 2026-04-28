// File: pimScratchpad.h
// PIMeval Simulator - Scratchpad / Register-File ECC Model
// Copyright (c) 2024 University of Virginia
// This file is licensed under the MIT License.
// See the LICENSE file in the root of this repository for more details.

#ifndef LAVA_PIM_SCRATCHPAD_H
#define LAVA_PIM_SCRATCHPAD_H

#include <cstdint>
#include <string>

/**
 * @class pimScratchpad
 * @brief Analytical model for on-chip SRAM scratchpad or register-file ECC in PIM architectures.
 *
 * Many PIM architectures include on-chip SRAM storage that participates in computation:
 *   - AiM (Aquabolt-XL): a global row buffer (SRAM) that is read on every GDL iteration
 *     during MAC operations, with data fanned out to all banks in a chip.
 *   - Bit-serial (BitSIMD): row registers (sense-amp row, DCC registers) act as a
 *     per-core register file; every rreg.* operation reads or writes one full register.
 *
 * Unlike DRAM ECC (which fires on row activation) or controller ECC (which fires at
 * host-device copy boundaries), scratchpad ECC fires on every SRAM word access during
 * in-memory computation.  Real SRAM scratchpads in high-reliability designs use SECDED
 * at the word granularity (typically 32 or 64 bits), adding ~1 cycle of decode latency
 * and a modest energy penalty per word accessed.
 *
 * This class is purely analytical — it computes overhead (latency, energy) given a
 * byte count.  No functional encode/decode is needed because the simulator's functional
 * tier already tracks correct data values; scratchpad contents are transient intermediate
 * storage that does not need to be modeled bit-for-bit.
 *
 * Reference parameters (defaults):
 *   - Word width:   32 bits  (standard SRAM word; some designs use 64b)
 *   - ECC type:     SECDED   (Hamming + overall parity; 1-bit correction / 2-bit detect)
 *   - Latency:      0.5 ns   per word accessed (SRAM ECC decode in 28/22nm ~0.3–0.7 ns)
 *   - Energy:       0.3 pJ   per word accessed (SRAM ECC logic, much cheaper than DRAM ECC)
 *
 * See: Guo et al., "Bit-Serial ECC for Reliable In-Memory Computing", DATE 2021.
 *      UPMEM SDK Reference, WRAM ECC configuration, 2023.
 */
class pimScratchpad
{
public:
  pimScratchpad(unsigned sizeKb        = 64,
                unsigned wordBits      = 32,
                const std::string& eccType = "secded",
                double latencyNs       = 0.5,
                double energyPj        = 0.3)
    : m_sizeKb(sizeKb),
      m_wordBits(wordBits),
      m_eccType(eccType),
      m_latencyNs(latencyNs),
      m_energyPj(energyPj)
  {}

  // ---- Configuration getters ----
  unsigned    getSizeKb()     const { return m_sizeKb; }
  unsigned    getWordBits()   const { return m_wordBits; }
  std::string getEccType()    const { return m_eccType; }
  double      getLatencyNs()  const { return m_latencyNs; }
  double      getEnergyPj()   const { return m_energyPj; }

  /// @brief Number of ECC parity bits added per codeword for storage overhead calculation.
  ///        SECDED on a W-bit word requires ceil(log2(W)) + 1 parity bits.
  unsigned getParityBitsPerWord() const {
    if (m_eccType == "none" || m_wordBits == 0) return 0;
    // SECDED: find k such that 2^k >= W + k + 1
    unsigned k = 0;
    while ((1U << k) < m_wordBits + k + 1) { ++k; }
    return k + 1; // +1 for overall parity bit (double-error detect)
  }

  /// @brief Storage overhead: total parity bits for a given number of data bits.
  uint64_t getParityBitsFor(uint64_t dataBits) const {
    if (m_eccType == "none" || m_wordBits == 0) return 0;
    uint64_t numWords = (dataBits + m_wordBits - 1) / m_wordBits;
    return numWords * getParityBitsPerWord();
  }

  /**
   * @brief Compute ECC latency and energy overhead for accessing numBytes of scratchpad data.
   *
   * Returns the overhead added to the perf/energy model when numBytes of SRAM data
   * are read or written with ECC enabled.  Each word-sized access triggers one ECC
   * decode (read) or encode (write).
   *
   * @param numBytes  Number of bytes accessed in the scratchpad.
   * @param latencyNsOut  [out] Additional latency in nanoseconds.
   * @param energyPjOut   [out] Additional energy in picojoules.
   */
  void getEccOverheadForBytes(uint64_t numBytes,
                               double& latencyNsOut,
                               double& energyPjOut) const {
    if (m_eccType == "none" || m_wordBits == 0) {
      latencyNsOut = 0.0;
      energyPjOut  = 0.0;
      return;
    }
    uint64_t bytesPerWord = (m_wordBits + 7) / 8;
    uint64_t numWords = (numBytes + bytesPerWord - 1) / bytesPerWord;
    latencyNsOut = static_cast<double>(numWords) * m_latencyNs;
    energyPjOut  = static_cast<double>(numWords) * m_energyPj;
  }

  /// @brief Correction capability (SECDED: 1-bit per codeword).
  unsigned getCorrectionCapability() const {
    return (m_eccType == "none") ? 0 : 1;
  }
  /// @brief Detection capability (SECDED: 2-bit per codeword).
  unsigned getDetectionCapability() const {
    return (m_eccType == "none") ? 0 : 2;
  }

private:
  unsigned    m_sizeKb;      ///< Total scratchpad capacity in KB
  unsigned    m_wordBits;    ///< SRAM word width in bits (ECC granularity)
  std::string m_eccType;     ///< ECC scheme: "secded" or "none"
  double      m_latencyNs;   ///< ECC decode latency per word access (ns)
  double      m_energyPj;    ///< ECC logic energy per word access (pJ)
};

#endif
