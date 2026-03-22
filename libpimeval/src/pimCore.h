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

