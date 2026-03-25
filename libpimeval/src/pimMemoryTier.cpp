// File: pimMemoryTier.cpp
// PIMeval Simulator - Memory Tier Abstraction
// Copyright (c) 2024 University of Virginia
// This file is licensed under the MIT License.
// See the LICENSE file in the root of this repository for more details.

#include "pimMemoryTier.h"
#include "pimDevice.h"

PimStatus
pimBitMappedTier::read(uint64_t index, uint64_t& bits) const {
  if (index >= m_numElements) return PIM_ERR_INVALID_ARG;
  return m_device->readBitMappedData(m_objId, index, bits);
}

PimStatus
pimBitMappedTier::write(uint64_t index, uint64_t bits) {
  if (index >= m_numElements) return PIM_ERR_INVALID_ARG;
  return m_device->writeBitMappedData(m_objId, index, bits);
}

PimStatus
pimBitMappedTier::copyFrom(const pimMemoryTier* src, uint64_t idxBegin, uint64_t idxEnd) {
  uint64_t end = (idxEnd == 0) ? m_numElements : idxEnd;
  for (uint64_t i = idxBegin; i < end; ++i) {
    uint64_t bits;
    PimStatus status = src->read(i, bits);
    if (status != PIM_OK) return status;
    status = write(i, bits);
    if (status != PIM_OK) return status;
  }
  return PIM_OK;
}
