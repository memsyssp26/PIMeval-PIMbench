// File: pimCore.h
// PIMeval Simulator - PIM Core
// Copyright (c) 2024 University of Virginia
// This file is licensed under the MIT License.
// See the LICENSE file in the root of this repository for more details.

#ifndef LAVA_PIM_CORE_H
#define LAVA_PIM_CORE_H

#include "libpimeval.h"
#include <vector>
#include <string>
#include <map>
#include <cassert>
#include <cstdint>


//! @class  pimCore
//! @brief  A PIM core which performs computation on a 2D memory subarray
class pimCore
{
public:
  pimCore(unsigned numRows, unsigned numCols);
  ~pimCore();

  // ID
  void setCoreId(int id) { m_coreId = id; }
  PimCoreId getCoreId() const { return m_coreId; }

  // Row-based operations
  bool readRow(unsigned rowIndex);
  bool writeRow(unsigned rowIndex);
  std::vector<bool>& getSenseAmpRow() { return m_rowRegs[PIM_RREG_SA]; }
  bool setSenseAmpRow(const std::vector<bool>& vals);
  bool readMultiRows(const std::vector<std::pair<unsigned, bool>>& rowIdxs);
  bool writeMultiRows(const std::vector<std::pair<unsigned, bool>>& rowIdxs);

  // Column-based operations
  bool readCol(unsigned colIndex);
  bool writeCol(unsigned colIndex);
  std::vector<bool>& getSenseAmpCol() { return m_senseAmpCol; }
  bool setSenseAmpCol(const std::vector<bool>& vals);

  // Reg access
  std::vector<bool>& getRowReg(PimRowReg reg) { return m_rowRegs[reg]; }

  // Utilities
  bool declareRowReg(PimRowReg reg);
  bool declareColReg(const std::string& name);
  void print() const;

  // Directly manipulate bits for functional implementation
  //! @brief  Directly set a bit for functional simulation
  inline void setBit(unsigned rowIdx, unsigned colIdx, bool val) {
    assert(rowIdx < m_numRows && colIdx < m_numCols);
    if (m_rows[rowIdx].empty()) {
      if (!val) return;
      m_rows[rowIdx].resize(m_bytesPerRow, 0);
    }
    uint8_t mask = 1 << (colIdx & 7);
    if (val) m_rows[rowIdx][colIdx >> 3] |= mask;
    else m_rows[rowIdx][colIdx >> 3] &= ~mask;
  }
  //! @brief  Directly get a bit for functional simulation
  inline bool getBit(unsigned rowIdx, unsigned colIdx) const {
    assert(rowIdx < m_numRows && colIdx < m_numCols);
    if (m_rows[rowIdx].empty()) return false;
    uint8_t mask = 1 << (colIdx & 7);
    return (m_rows[rowIdx][colIdx >> 3] & mask) != 0;
  }
  //! @brief  Directly set #numBits bits for V-layout functional simulation
  inline void setBitsV(unsigned rowIdx, unsigned colIdx, uint64_t val, unsigned numBits) {
    assert(numBits > 0 && numBits <= 64);
    assert(rowIdx + (numBits - 1) < m_numRows && colIdx < m_numCols);
    for (unsigned i = 0; i < numBits; ++i) {
      bool bitVal = val & 1;
      setBit(rowIdx + i, colIdx, bitVal);
      val = val >> 1;
    }
  }
  //! @brief  Directly get #numBits bits for V-layout functional simulation
  inline uint64_t getBitsV(unsigned rowIdx, unsigned colIdx, unsigned numBits) const {
    assert(numBits > 0 && numBits <= 64);
    assert(rowIdx + (numBits - 1) < m_numRows && colIdx < m_numCols);
    uint64_t val = 0;
    for (int i = (numBits - 1); i >= 0; --i) {
      bool bitVal = getBit(rowIdx + i, colIdx);
      val = (val << 1) | bitVal;
    }
    return val;
  }
  //! @brief  Directly set bits for V-layout functional simulation using bit vector
  inline void setBitsVVec(unsigned rowIdx, unsigned colIdx, const std::vector<bool>& bits) {
    unsigned numBits = bits.size();
    assert(rowIdx + (numBits - 1) < m_numRows && colIdx < m_numCols);
    for (unsigned i = 0; i < numBits; ++i) {
      setBit(rowIdx + i, colIdx, bits[i]);
    }
  }
  //! @brief  Directly get bits for V-layout functional simulation using bit vector
  inline std::vector<bool> getBitsVVec(unsigned rowIdx, unsigned colIdx, unsigned numBits) const {
    assert(rowIdx + (numBits - 1) < m_numRows && colIdx < m_numCols);
    std::vector<bool> bits(numBits);
    for (unsigned i = 0; i < numBits; ++i) {
      bits[i] = getBit(rowIdx + i, colIdx);
    }
    return bits;
  }
  //! @brief  Directly set bits for H-layout functional simulation using bit vector
  inline void setBitsHVec(unsigned rowIdx, unsigned colIdx, const std::vector<bool>& bits) {
    unsigned numBits = bits.size();
    assert(rowIdx < m_numRows && colIdx + (numBits - 1) < m_numCols);
    for (unsigned i = 0; i < numBits; ++i) {
      setBit(rowIdx, colIdx + i, bits[i]);
    }
  }
  //! @brief  Directly get bits for H-layout functional simulation using bit vector
  inline std::vector<bool> getBitsHVec(unsigned rowIdx, unsigned colIdx, unsigned numBits) const {
    assert(rowIdx < m_numRows && colIdx + (numBits - 1) < m_numCols);
    std::vector<bool> bits(numBits);
    for (unsigned i = 0; i < numBits; ++i) {
      bits[i] = getBit(rowIdx, colIdx + i);
    }
    return bits;
  }
  //! @brief  Directly set #numBits bits for H-layout functional simulation
  inline void setBitsH(unsigned rowIdx, unsigned colIdx, uint64_t val, unsigned numBits) {
    assert(numBits > 0 && numBits <= 64);
    assert(rowIdx < m_numRows && colIdx + (numBits - 1) < m_numCols);
    for (unsigned i = 0; i < numBits; ++i) {
      bool bitVal = val & 1;
      setBit(rowIdx, colIdx + i, bitVal);
      val = val >> 1;
    }
  }
  //! @brief  Directly get #numBits bits for H-layout functional simulation
  inline uint64_t getBitsH(unsigned rowIdx, unsigned colIdx, unsigned numBits) const {
    assert(numBits > 0 && numBits <= 64);
    assert(rowIdx < m_numRows && colIdx + (numBits - 1) < m_numCols);
    uint64_t val = 0;
    for (int i = (numBits - 1); i >= 0; --i) {
      bool bitVal = getBit(rowIdx, colIdx + i);
      val = (val << 1) | bitVal;
    }
    return val;
  }

  //! @brief  Read row into sense amplifier
  void readRowToSa(unsigned rowIdx) {
    if (m_rows[rowIdx].empty()) {
      m_senseAmpCol.assign(m_numCols, false);
    } else {
      for (unsigned i = 0; i < m_numCols; ++i) {
        m_senseAmpCol[i] = getBit(rowIdx, i);
      }
    }
  }
  //! @brief  Write sense amplifier to row
  void writeSaToRow(unsigned rowIdx) {
    if (m_rows[rowIdx].empty()) {
      m_rows[rowIdx].resize(m_bytesPerRow, 0);
    }
    for (unsigned i = 0; i < m_numCols; ++i) {
      setBit(rowIdx, i, m_senseAmpCol[i]);
    }
  }
  //! @brief  Triple-row activation (majority function)
  void tra(unsigned r1, unsigned r2, unsigned r3) {
    for (unsigned i = 0; i < m_numCols; ++i) {
      bool b1 = getBit(r1, i);
      bool b2 = getBit(r2, i);
      bool b3 = getBit(r3, i);
      m_senseAmpCol[i] = (b1 && b2) || (b1 && b3) || (b2 && b3);
    }
  }
  //! @brief  Analog activation-to-activation propagation
  void aap(const std::vector<unsigned>& srcRows, const std::vector<unsigned>& destRows) {
    // Basic functional model: destRows[i] = majority(srcRows)
    for (unsigned i = 0; i < m_numCols; ++i) {
      unsigned count = 0;
      for (unsigned r : srcRows) if (getBit(r, i)) count++;
      bool result = (count > srcRows.size() / 2);
      for (unsigned r : destRows) setBit(r, i, result);
    }
  }
  //! @brief  Row register operations
  void rreg_mov(PimRowReg src, PimRowReg dest) {
    if (src == PIM_RREG_SA) m_rowRegs[dest] = m_senseAmpCol;
    else if (dest == PIM_RREG_SA) m_senseAmpCol = m_rowRegs[src];
    else m_rowRegs[dest] = m_rowRegs[src];
  }
  void rreg_set(PimRowReg dest, bool val) {
    if (dest == PIM_RREG_SA) m_senseAmpCol.assign(m_numCols, val);
    else m_rowRegs[dest].assign(m_numCols, val);
  }
  void rreg_not(PimRowReg src, PimRowReg dest) {
    const std::vector<bool>& s = (src == PIM_RREG_SA) ? m_senseAmpCol : m_rowRegs[src];
    std::vector<bool> d(m_numCols);
    for (unsigned i = 0; i < m_numCols; ++i) d[i] = !s[i];
    if (dest == PIM_RREG_SA) m_senseAmpCol = d;
    else m_rowRegs[dest] = d;
  }
  void rreg_and(PimRowReg s1, PimRowReg s2, PimRowReg dest) {
    const std::vector<bool>& v1 = (s1 == PIM_RREG_SA) ? m_senseAmpCol : m_rowRegs[s1];
    const std::vector<bool>& v2 = (s2 == PIM_RREG_SA) ? m_senseAmpCol : m_rowRegs[s2];
    std::vector<bool> d(m_numCols);
    for (unsigned i = 0; i < m_numCols; ++i) d[i] = v1[i] && v2[i];
    if (dest == PIM_RREG_SA) m_senseAmpCol = d;
    else m_rowRegs[dest] = d;
  }
  void rreg_or(PimRowReg s1, PimRowReg s2, PimRowReg dest) {
    const std::vector<bool>& v1 = (s1 == PIM_RREG_SA) ? m_senseAmpCol : m_rowRegs[s1];
    const std::vector<bool>& v2 = (s2 == PIM_RREG_SA) ? m_senseAmpCol : m_rowRegs[s2];
    std::vector<bool> d(m_numCols);
    for (unsigned i = 0; i < m_numCols; ++i) d[i] = v1[i] || v2[i];
    if (dest == PIM_RREG_SA) m_senseAmpCol = d;
    else m_rowRegs[dest] = d;
  }
  void rreg_nand(PimRowReg s1, PimRowReg s2, PimRowReg dest) {
    const std::vector<bool>& v1 = (s1 == PIM_RREG_SA) ? m_senseAmpCol : m_rowRegs[s1];
    const std::vector<bool>& v2 = (s2 == PIM_RREG_SA) ? m_senseAmpCol : m_rowRegs[s2];
    std::vector<bool> d(m_numCols);
    for (unsigned i = 0; i < m_numCols; ++i) d[i] = !(v1[i] && v2[i]);
    if (dest == PIM_RREG_SA) m_senseAmpCol = d;
    else m_rowRegs[dest] = d;
  }
  void rreg_nor(PimRowReg s1, PimRowReg s2, PimRowReg dest) {
    const std::vector<bool>& v1 = (s1 == PIM_RREG_SA) ? m_senseAmpCol : m_rowRegs[s1];
    const std::vector<bool>& v2 = (s2 == PIM_RREG_SA) ? m_senseAmpCol : m_rowRegs[s2];
    std::vector<bool> d(m_numCols);
    for (unsigned i = 0; i < m_numCols; ++i) d[i] = !(v1[i] || v2[i]);
    if (dest == PIM_RREG_SA) m_senseAmpCol = d;
    else m_rowRegs[dest] = d;
  }
  void rreg_xor(PimRowReg s1, PimRowReg s2, PimRowReg dest) {
    const std::vector<bool>& v1 = (s1 == PIM_RREG_SA) ? m_senseAmpCol : m_rowRegs[s1];
    const std::vector<bool>& v2 = (s2 == PIM_RREG_SA) ? m_senseAmpCol : m_rowRegs[s2];
    std::vector<bool> d(m_numCols);
    for (unsigned i = 0; i < m_numCols; ++i) d[i] = v1[i] ^ v2[i];
    if (dest == PIM_RREG_SA) m_senseAmpCol = d;
    else m_rowRegs[dest] = d;
  }
  void rreg_xnor(PimRowReg s1, PimRowReg s2, PimRowReg dest) {
    const std::vector<bool>& v1 = (s1 == PIM_RREG_SA) ? m_senseAmpCol : m_rowRegs[s1];
    const std::vector<bool>& v2 = (s2 == PIM_RREG_SA) ? m_senseAmpCol : m_rowRegs[s2];
    std::vector<bool> d(m_numCols);
    for (unsigned i = 0; i < m_numCols; ++i) d[i] = !(v1[i] ^ v2[i]);
    if (dest == PIM_RREG_SA) m_senseAmpCol = d;
    else m_rowRegs[dest] = d;
  }
  void rreg_maj(PimRowReg s1, PimRowReg s2, PimRowReg s3, PimRowReg dest) {
    const std::vector<bool>& v1 = (s1 == PIM_RREG_SA) ? m_senseAmpCol : m_rowRegs[s1];
    const std::vector<bool>& v2 = (s2 == PIM_RREG_SA) ? m_senseAmpCol : m_rowRegs[s2];
    const std::vector<bool>& v3 = (s3 == PIM_RREG_SA) ? m_senseAmpCol : m_rowRegs[s3];
    std::vector<bool> d(m_numCols);
    for (unsigned i = 0; i < m_numCols; ++i) d[i] = (v1[i] && v2[i]) || (v1[i] && v3[i]) || (v2[i] && v3[i]);
    if (dest == PIM_RREG_SA) m_senseAmpCol = d;
    else m_rowRegs[dest] = d;
  }
  void rreg_sel(PimRowReg sc, PimRowReg s1, PimRowReg s2, PimRowReg dest) {
    const std::vector<bool>& vc = (sc == PIM_RREG_SA) ? m_senseAmpCol : m_rowRegs[sc];
    const std::vector<bool>& v1 = (s1 == PIM_RREG_SA) ? m_senseAmpCol : m_rowRegs[s1];
    const std::vector<bool>& v2 = (s2 == PIM_RREG_SA) ? m_senseAmpCol : m_rowRegs[s2];
    std::vector<bool> d(m_numCols);
    for (unsigned i = 0; i < m_numCols; ++i) d[i] = vc[i] ? v1[i] : v2[i];
    if (dest == PIM_RREG_SA) m_senseAmpCol = d;
    else m_rowRegs[dest] = d;
  }
  void rreg_rotate_r(PimRowReg r) {
    std::vector<bool>& v = (r == PIM_RREG_SA) ? m_senseAmpCol : m_rowRegs[r];
    if (v.empty()) return;
    bool last = v.back();
    for (int i = m_numCols - 1; i > 0; --i) v[i] = v[i-1];
    v[0] = last;
  }
  void rreg_rotate_l(PimRowReg r) {
    std::vector<bool>& v = (r == PIM_RREG_SA) ? m_senseAmpCol : m_rowRegs[r];
    if (v.empty()) return;
    bool first = v[0];
    for (unsigned i = 0; i < m_numCols - 1; ++i) v[i] = v[i+1];
    v.back() = first;
  }

private:
  PimCoreId m_coreId;
  unsigned m_numRows;
  unsigned m_numCols;
  unsigned m_bytesPerRow;

  std::vector<std::vector<uint8_t>> m_rows;
  std::vector<bool> m_senseAmpCol;

  std::map<PimRowReg, std::vector<bool>> m_rowRegs;
  std::map<std::string, std::vector<bool>> m_colRegs;
};

#endif

