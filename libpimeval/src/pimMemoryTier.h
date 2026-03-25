// File: pimMemoryTier.h
// PIMeval Simulator - Memory Tier Abstraction
// Copyright (c) 2024 University of Virginia
// This file is licensed under the MIT License.
// See the LICENSE file in the root of this repository for more details.

#ifndef LAVA_PIM_MEMORY_TIER_H
#define LAVA_PIM_MEMORY_TIER_H

#include "libpimeval.h"
#include "pimUtils.h"
#include <cstdint>
#include <vector>
#include <cstring>

class pimDevice;

//! @class  pimMemoryTier
//! @brief  Abstract base class for different memory domains
class pimMemoryTier {
public:
  virtual ~pimMemoryTier() {}

  //! @brief  Read bit representation of an element
  virtual PimStatus read(uint64_t index, uint64_t& bits) const = 0;

  //! @brief  Write bit representation of an element
  virtual PimStatus write(uint64_t index, uint64_t bits) = 0;

  //! @brief  Bulk copy from another tier
  virtual PimStatus copyFrom(const pimMemoryTier* src, uint64_t idxBegin, uint64_t idxEnd) = 0;

  //! @brief  Check if this tier is valid
  virtual bool isValid() const = 0;
};

//! @class  pimFunctionalTier
//! @brief  Functional host memory domain
class pimFunctionalTier : public pimMemoryTier {
public:
  pimFunctionalTier(PimDataType dataType, uint64_t numElements)
    : m_dataType(dataType), m_numElements(numElements) {
    unsigned numBitsOfDataType = pimUtils::getNumBitsOfDataType(m_dataType, PimBitWidth::HOST);
    m_bytesPerElement = (numBitsOfDataType + 7) / 8;
    m_data.resize(m_numElements * m_bytesPerElement, 0);
  }

  virtual PimStatus read(uint64_t index, uint64_t& bits) const override {
    if (index >= m_numElements) return PIM_ERR_INVALID_ARG;
    bits = 0;
    uint64_t byteIndex = index * m_bytesPerElement;
    std::memcpy(&bits, m_data.data() + byteIndex, m_bytesPerElement);
    bits = pimUtils::signExt(bits, m_dataType);
    return PIM_OK;
  }

  virtual PimStatus write(uint64_t index, uint64_t bits) override {
    if (index >= m_numElements) return PIM_ERR_INVALID_ARG;
    uint64_t byteIndex = index * m_bytesPerElement;
    std::memset(m_data.data() + byteIndex, 0, m_bytesPerElement);
    std::memcpy(m_data.data() + byteIndex, &bits, m_bytesPerElement);
    return PIM_OK;
  }

  virtual PimStatus copyFrom(const pimMemoryTier* src, uint64_t idxBegin, uint64_t idxEnd) override {
    uint64_t end = (idxEnd == 0) ? m_numElements : idxEnd;
    for (uint64_t i = idxBegin; i < end; ++i) {
      uint64_t bits;
      PimStatus status = src->read(i, bits);
      if (status != PIM_OK) return status;
      write(i, bits);
    }
    return PIM_OK;
  }

  virtual bool isValid() const override { return !m_data.empty(); }

  // Raw access for host memcpy
  void* getRawPtr(uint64_t index) { return m_data.data() + (index * m_bytesPerElement); }
  uint64_t getNumBytes(uint64_t idxBegin, uint64_t idxEnd) const {
    uint64_t n = (idxEnd == 0 ? m_numElements : idxEnd - idxBegin);
    return n * m_bytesPerElement;
  }

private:
  std::vector<uint8_t> m_data;
  PimDataType m_dataType;
  uint64_t m_numElements;
  unsigned m_bytesPerElement;
};

//! @class  pimBitMappedTier
//! @brief  Simulated bit-mapped memory domain
class pimBitMappedTier : public pimMemoryTier {
public:
  pimBitMappedTier(pimDevice* device, PimObjId objId, uint64_t numElements)
    : m_device(device), m_objId(objId), m_numElements(numElements) {}

  virtual PimStatus read(uint64_t index, uint64_t& bits) const override;
  virtual PimStatus write(uint64_t index, uint64_t bits) override;
  virtual PimStatus copyFrom(const pimMemoryTier* src, uint64_t idxBegin, uint64_t idxEnd) override;
  virtual bool isValid() const override { return m_device != nullptr; }

private:
  pimDevice* m_device;
  PimObjId m_objId;
  uint64_t m_numElements;
};

#endif
