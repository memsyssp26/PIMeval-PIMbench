// File: pimResMgr.cpp
// PIMeval Simulator - PIM Resource Manager
// Copyright (c) 2024 University of Virginia
// This file is licensed under the MIT License.
// See the LICENSE file in the root of this repository for more details.

#include "pimResMgr.h"
#include "pimDevice.h"
#include "pimUtils.h"
#include <algorithm>
#include <cstdio>
#include <cinttypes>

// pimRegion implementation
std::pair<unsigned, unsigned> pimRegion::locateIthElemInRegion(unsigned i) const
{
  if (isBuffer()) {
    return std::make_pair(0, i * m_numColsPerElem);
  }
  if (m_numColsPerElem == 1) { // V layout
    return std::make_pair(0, i);
  } else { // H layout
    return std::make_pair(0, i * m_numColsPerElem);
  }
}

void pimRegion::print() const
{
  printf("  Region: core=%d, row=%u, col=%u, nRows=%u, nCols=%u, elemIdx=[%" PRIu64 ", %" PRIu64 ")\n",
         m_coreId, m_rowIdx, m_colIdx, m_numAllocRows, m_numAllocCols, m_elemIdxBegin, m_elemIdxEnd);
}

// pimObjInfo implementation
pimObjInfo::~pimObjInfo() {
}

void pimObjInfo::finalize()
{
  m_numCoresUsed = 0;
  m_maxNumRegionsPerCore = 0;
  m_maxElementsPerRegion = 0;
  std::map<PimCoreId, unsigned> regionsPerCore;
  for (const auto& region : m_regions) {
    regionsPerCore[region.getCoreId()]++;
    m_maxElementsPerRegion = std::max(m_maxElementsPerRegion, (unsigned)region.getNumElemInRegion());
  }
  m_numCoresUsed = regionsPerCore.size();
  for (auto const& [coreId, count] : regionsPerCore) {
    m_maxNumRegionsPerCore = std::max(m_maxNumRegionsPerCore, count);
  }
}

std::vector<pimRegion> pimObjInfo::getRegionsOfCore(PimCoreId coreId) const
{
  std::vector<pimRegion> regions;
  for (const auto& region : m_regions) {
    if (region.getCoreId() == coreId) {
      regions.push_back(region);
    }
  }
  return regions;
}

unsigned pimObjInfo::getBitsPerElement(PimBitWidth bitWidthType) const
{
  if (bitWidthType == PimBitWidth::ACTUAL || bitWidthType == PimBitWidth::HOST) {
    return pimUtils::getNumBitsOfDataType(m_dataType, bitWidthType);
  }
  return m_bitsPerElementPadded;
}

void pimObjInfo::print() const
{
  printf("PIM Object ID %d: dataType=%s, allocType=%s, nElements=%" PRIu64 ", bitsPadded=%u, nRegions=%zu\n",
         m_objId, pimUtils::pimDataTypeEnumToStr(m_dataType).c_str(), 
         pimUtils::pimAllocEnumToStr(m_allocType).c_str(), m_numElements, m_bitsPerElementPadded, m_regions.size());
  for (const auto& region : m_regions) {
    region.print();
  }
}

void pimObjInfo::copyToObj(pimObjInfo& destObj, uint64_t idxBegin, uint64_t idxEnd) const {
  destObj.m_functionalTier->copyFrom(m_functionalTier.get(), idxBegin, idxEnd);
}

void pimObjInfo::setElementBits(uint64_t index, uint64_t bits)
{
  uint64_t existingBits = 0;
  m_functionalTier->read(index, existingBits);
  if (existingBits != bits) {
    m_functionalTier->write(index, bits);
    markDirtyFunctional();
  }
}

uint64_t pimObjInfo::getElementBits(uint64_t index) const
{
  if (m_syncState == PimObjSyncState::DIRTY_BIT_MAPPED) {
    const_cast<pimObjInfo*>(this)->syncFromSimulatedMem();
  }
  uint64_t bits = 0;
  m_functionalTier->read(index, bits);
  return bits;
}

void pimObjInfo::copyFromHost(void* src, uint64_t idxBegin, uint64_t idxEnd)
{
  uint64_t end = (idxEnd == 0) ? m_numElements : idxEnd;
  unsigned bitsPerElem = pimUtils::getNumBitsOfDataType(m_dataType, PimBitWidth::HOST);
  unsigned bytesPerElem = (bitsPerElem + 7) / 8;
  uint8_t* srcPtr = static_cast<uint8_t*>(src);
  bool changed = false;
  for (uint64_t i = idxBegin; i < end; ++i) {
    uint64_t newBits = 0;
    std::memcpy(&newBits, srcPtr + (i - idxBegin) * bytesPerElem, bytesPerElem);
    uint64_t existingBits = 0;
    m_functionalTier->read(i, existingBits);
    if (existingBits != newBits) {
      m_functionalTier->write(i, newBits);
      changed = true;
    }
  }
  if (changed) {
    markDirtyFunctional();
  }
}

void pimObjInfo::copyToHost(void* dest, uint64_t idxBegin, uint64_t idxEnd) const
{
  if (m_syncState == PimObjSyncState::DIRTY_BIT_MAPPED) {
    const_cast<pimObjInfo*>(this)->syncFromSimulatedMem();
  }
  uint64_t numBytes = m_functionalTier->getNumBytes(idxBegin, idxEnd);
  std::memcpy(dest, m_functionalTier->getRawPtr(idxBegin), numBytes);
}

void pimObjInfo::syncFromSimulatedMem()
{
  if (m_syncState != PimObjSyncState::DIRTY_BIT_MAPPED) return;
  for (const auto& region : m_regions) {
    // Only write if there's a difference to avoid "input modified" false positives
    for (uint64_t i = region.getElemIdxBegin(); i < region.getElemIdxEnd(); ++i) {
      uint64_t paddedBits = 0;
      if (m_bitMappedTier->read(i, paddedBits) == PIM_OK) {
        uint64_t decodedBits = paddedBits;
        if (m_device->getConfig().isEccEnabled()) {
          const pimEccStrategy* eccStrategy = m_device->getConfig().getEccStrategy();
          if (eccStrategy) {
            unsigned unpaddedWidth = pimUtils::getNumBitsOfDataType(m_dataType, PimBitWidth::SIM);
            int eccStatus = 0;
            decodedBits = eccStrategy->decode(paddedBits, unpaddedWidth, eccStatus);
          }
        }
        uint64_t functionalBits = 0;
        m_functionalTier->read(i, functionalBits);
        if (functionalBits != decodedBits) {
          m_functionalTier->write(i, decodedBits);
        }
      }
    }
  }
  markClean();
}

void pimObjInfo::syncFromSimulatedMem(const pimRegion& region)
{
  if (m_syncState != PimObjSyncState::DIRTY_BIT_MAPPED) return;
  // Do not markClean here, as this may be a partial sync
  unsigned unpaddedWidth = pimUtils::getNumBitsOfDataType(m_dataType, PimBitWidth::SIM);
  const pimEccStrategy* eccStrategy = m_device->getConfig().isEccEnabled() ? m_device->getConfig().getEccStrategy() : nullptr;

  for (uint64_t i = region.getElemIdxBegin(); i < region.getElemIdxEnd(); ++i) {
    uint64_t paddedBits = 0;
    if (m_bitMappedTier->read(i, paddedBits) == PIM_OK) {
      uint64_t decodedBits = paddedBits;
      if (eccStrategy) {
        int eccStatus = 0;
        decodedBits = eccStrategy->decode(paddedBits, unpaddedWidth, eccStatus);
      } else {
        decodedBits = pimUtils::signExt(paddedBits, m_dataType);
      }
      
      uint64_t functionalBits = 0;
      m_functionalTier->read(i, functionalBits);
      if (functionalBits != decodedBits) {
        m_functionalTier->write(i, decodedBits);
      }
    }
  }
}

void pimObjInfo::syncToSimulatedMem() const
{
  if (m_syncState != PimObjSyncState::DIRTY_FUNCTIONAL) return;
  for (const auto& region : m_regions) {
    for (uint64_t i = region.getElemIdxBegin(); i < region.getElemIdxEnd(); ++i) {
      uint64_t bits = 0;
      m_functionalTier->read(i, bits);
      
      uint64_t paddedBits = bits;
      if (m_device->getConfig().isEccEnabled()) {
        const pimEccStrategy* eccStrategy = m_device->getConfig().getEccStrategy();
        if (eccStrategy) {
          unsigned unpaddedWidth = pimUtils::getNumBitsOfDataType(m_dataType, PimBitWidth::SIM);
          paddedBits = eccStrategy->encode(bits, unpaddedWidth);
        }
      }
      
      uint64_t existingPaddedBits = 0;
      if (m_bitMappedTier->read(i, existingPaddedBits) != PIM_OK || existingPaddedBits != paddedBits) {
        m_bitMappedTier->write(i, paddedBits);
      }
    }
  }
  const_cast<pimObjInfo*>(this)->markClean();
}

void pimObjInfo::syncToSimulatedMem(const pimRegion& region) const
{
  if (m_syncState != PimObjSyncState::DIRTY_FUNCTIONAL) return;
  // Do not markClean here, as this may be a partial sync
  for (uint64_t i = region.getElemIdxBegin(); i < region.getElemIdxEnd(); ++i) {
    uint64_t bits = 0;
    m_functionalTier->read(i, bits);
    
    uint64_t paddedBits = bits;
    if (m_device->getConfig().isEccEnabled()) {
      const pimEccStrategy* eccStrategy = m_device->getConfig().getEccStrategy();
      if (eccStrategy) {
        unsigned unpaddedWidth = pimUtils::getNumBitsOfDataType(m_dataType, PimBitWidth::SIM);
        paddedBits = eccStrategy->encode(bits, unpaddedWidth);
      }
    }
    m_bitMappedTier->write(i, paddedBits);
  }
}

// pimResMgr implementation
pimResMgr::pimResMgr(pimDevice* device)
  : m_device(device),
    m_availObjId(0),
    m_debugAlloc(false)
{
  unsigned numCores = m_device->getNumCores();
  unsigned numRowsPerCore = m_device->getNumRows() / numCores;
  for (unsigned i = 0; i < numCores; ++i) {
    m_coreUsage[i] = std::make_unique<coreUsage>(numRowsPerCore);
  }
}

pimResMgr::~pimResMgr() {}

PimObjId
pimResMgr::pimAlloc(PimAllocEnum allocType, uint64_t numElements, PimDataType dataType)
{
  if (!m_device) return -1;


  unsigned bitsPerElement = pimUtils::getNumBitsOfDataType(dataType, PimBitWidth::SIM);
  if (m_device->getConfig().isEccEnabled()) {
    const pimEccStrategy* eccStrategy = m_device->getConfig().getEccStrategy();
    if (eccStrategy) {
      unsigned granularity = m_device->getConfig().getEccGranularity();
      if (granularity == 0) bitsPerElement = eccStrategy->getTotalBits(bitsPerElement);
      else {
        unsigned totalBits = eccStrategy->getTotalBits(granularity);
        bitsPerElement += (totalBits - granularity) * ((bitsPerElement + granularity - 1) / granularity);
      }
    }
  }

  if (allocType == PIM_ALLOC_AUTO) {
    if (m_device->getDeviceDataLayout() == PimDataLayout::V) allocType = PIM_ALLOC_V;
    else allocType = PIM_ALLOC_H;
  }

  std::vector<PimCoreId> sortedCoreId = getCoreIdsSortedByLeastUsage();
  auto newObj = std::make_shared<pimObjInfo>(m_availObjId++, dataType, (allocType == PIM_ALLOC_V || allocType == PIM_ALLOC_V1 ? PIM_ALLOC_V : PIM_ALLOC_H), numElements, bitsPerElement, m_device);

  unsigned numCores = m_device->getNumCores();
  unsigned numCols = m_device->getNumCols();
  unsigned numRowsToAlloc = 0;
  uint64_t numRegions = 0;
  unsigned numColsToAllocLast = 0;
  uint64_t numElemPerRegion = 0;
  uint64_t numElemPerRegionLast = 0;
  unsigned numColsPerElem = 0;

  if (numElements == 0) return -1;

  if (allocType == PIM_ALLOC_V || allocType == PIM_ALLOC_V1) {
    numRowsToAlloc = bitsPerElement;
    numRegions = (numElements - 1) / numCols + 1;
    numColsToAllocLast = numElements % numCols;
    if (numColsToAllocLast == 0) numColsToAllocLast = numCols;
    numElemPerRegion = numCols;
    numElemPerRegionLast = numColsToAllocLast;
    numColsPerElem = 1;
  } else if (allocType == PIM_ALLOC_H || allocType == PIM_ALLOC_H1) {
    numRowsToAlloc = 1;
    numRegions = (numElements * bitsPerElement - 1) / numCols + 1;
    numColsToAllocLast = (numElements * bitsPerElement) % numCols;
    if (numColsToAllocLast == 0) numColsToAllocLast = numCols;
    numElemPerRegion = numCols / bitsPerElement;
    numElemPerRegionLast = numColsToAllocLast / bitsPerElement;
    numColsPerElem = bitsPerElement;
  } else return -1;

  if (numRegions > numCores && (allocType == PIM_ALLOC_V1 || allocType == PIM_ALLOC_H1)) {
    return -1;
  }

  bool success = true;
  for (unsigned i = 0; i < numCores; ++i) m_coreUsage.at(i)->newAllocStart();

  uint64_t elemIdx = 0;
  for (uint64_t i = 0; i < numRegions; ++i) {
    PimCoreId coreId = sortedCoreId[i % numCores];
    unsigned nElem = (i == numRegions - 1 ? numElemPerRegionLast : numElemPerRegion);
    unsigned r = m_coreUsage.at(coreId)->findAvailRange(numRowsToAlloc);
    if (r == (unsigned)-1) {
      success = false;
      break;
    }
    pimRegion newRegion;
    newRegion.setCoreId(coreId);
    newRegion.setRowIdx(r);
    newRegion.setNumAllocRows(numRowsToAlloc);
    newRegion.setNumAllocCols(nElem * numColsPerElem);
    newRegion.setIsValid(true);
    newRegion.setElemIdxBegin(elemIdx);
    elemIdx += nElem;
    newRegion.setElemIdxEnd(elemIdx);
    newRegion.setNumColsPerElem(numColsPerElem);
    newObj->addRegion(newRegion);

    m_coreUsage.at(coreId)->addRange(std::make_pair(newRegion.getRowIdx(), numRowsToAlloc), newObj->getObjId());
  }
  for (unsigned i = 0; i < numCores; ++i) m_coreUsage.at(i)->newAllocEnd(success);

  if (success && newObj->isValid()) {
    newObj->finalize();
    m_objMap[newObj->getObjId()] = newObj;
    m_refMap[newObj->getObjId()].insert(newObj->getObjId());
    return newObj->getObjId();
  }
  return -1;
}

PimObjId
pimResMgr::pimAllocBuffer(uint32_t numElements, PimDataType dataType)
{
  if (numElements == 0) return -1;
  unsigned bitsPerElement = pimUtils::getNumBitsOfDataType(dataType, PimBitWidth::SIM);
  auto newObj = std::make_shared<pimObjInfo>(m_availObjId++, dataType, PIM_ALLOC_V, numElements, bitsPerElement, m_device, true);

  pimRegion newRegion;
  newRegion.setCoreId(0);
  newRegion.setRowIdx(0);
  newRegion.setColIdx(0);
  newRegion.setNumAllocRows(1);
  newRegion.setNumAllocCols(numElements * bitsPerElement);
  newRegion.setIsBuffer(true);
  newRegion.setElemIdxBegin(0);
  newRegion.setElemIdxEnd(numElements);
  newRegion.setNumColsPerElem(bitsPerElement);
  newRegion.setIsValid(true);
  newObj->addRegion(newRegion);

  if (newObj->isValid()) {
    newObj->finalize();
    m_objMap[newObj->getObjId()] = newObj;
    return newObj->getObjId();
  }
  return -1;
}

PimObjId
pimResMgr::pimAllocAssociated(PimObjId assocId, PimDataType dataType)
{
  if (m_objMap.find(assocId) == m_objMap.end()) return -1;
  auto assocObj = m_objMap.at(assocId);
  if (assocObj->isBuffer()) return -1;

  unsigned bitsPerElement = pimUtils::getNumBitsOfDataType(dataType, PimBitWidth::SIM);
  PimAllocEnum allocType = (assocObj->isVLayout() ? PIM_ALLOC_V : PIM_ALLOC_H);
  auto newObj = std::make_shared<pimObjInfo>(m_availObjId++, dataType, allocType, assocObj->getNumElements(), bitsPerElement, m_device);
  newObj->setAssocObjId(assocObj->getAssocObjId());

  for (const auto& assocRegion : assocObj->getRegions()) {
    pimRegion newRegion = assocRegion;
    // For associated objects, we share the same rows but maybe use different bits per element.
    // However, the physical row allocation is determined by the first object in the association group.
    // We don't call addRange here because these rows are already managed by the base object.
    newObj->addRegion(newRegion);
  }

  if (newObj->isValid()) {
    newObj->finalize();
    m_objMap[newObj->getObjId()] = newObj;
    m_refMap[assocObj->getAssocObjId()].insert(newObj->getObjId());
    return newObj->getObjId();
  }
  return -1;
}

PimStatus
pimResMgr::pimFree(PimObjId objId)
{
  auto it = m_objMap.find(objId);
  if (it == m_objMap.end()) return PIM_ERR_OBJ_NOT_FOUND;
  
  PimObjId assocId = it->second->getAssocObjId();
  auto refIt = m_refMap.find(assocId);
  if (refIt != m_refMap.end()) {
    refIt->second.erase(objId);
    if (refIt->second.empty()) {
      // Entire association group is gone, now we can free the physical rows.
      // The physical rows are owned by assocId in m_objToIntervals.
      for (unsigned i = 0; i < m_device->getNumCores(); ++i) {
        m_coreUsage.at(i)->deleteObj(assocId);
      }
      m_refMap.erase(refIt);
    }
  }

  m_objMap.erase(it);
  return PIM_OK;
}

void
pimResMgr::reset()
{
  m_objMap.clear();
  for (auto const& [coreId, usage] : m_coreUsage) {
    usage->reset();
  }
}

PimObjId
pimResMgr::pimCreateRangedRef(PimObjId refId, uint64_t idxBegin, uint64_t idxEnd) { return -1; }
PimObjId
pimResMgr::pimCreateDualContactRef(PimObjId refId) { return -1; }

bool
pimResMgr::isVLayoutObj(PimObjId objId) const
{
  auto it = m_objMap.find(objId);
  return (it != m_objMap.end()) ? it->second->isVLayout() : false;
}

bool
pimResMgr::isHLayoutObj(PimObjId objId) const
{
  auto it = m_objMap.find(objId);
  return (it != m_objMap.end()) ? it->second->isHLayout() : false;
}

bool
pimResMgr::isHybridLayoutObj(PimObjId objId) const { return false; }

std::vector<PimCoreId>
pimResMgr::getCoreIdsSortedByLeastUsage() const
{
  std::vector<PimCoreId> sortedCoreId;
  for (const auto& it : m_coreUsage) sortedCoreId.push_back(it.first);
  std::sort(sortedCoreId.begin(), sortedCoreId.end(), [this](PimCoreId a, PimCoreId b) {
    return m_coreUsage.at(a)->getTotRowsInUse() < m_coreUsage.at(b)->getTotRowsInUse();
  });
  return sortedCoreId;
}

// coreUsage interval tree implementation
unsigned pimResMgr::coreUsage::findAvailRange(unsigned numRowsToAlloc)
{
  for (auto const& [start, len] : m_freeIntervals) {
    if (len >= numRowsToAlloc) {
      // Ensure this sub-range isn't already taken by m_tempIntervals
      unsigned currentStart = start;
      unsigned remainingLen = len;
      while (remainingLen >= numRowsToAlloc) {
        bool overlap = false;
        for (auto const& [tStart, tLen] : m_tempIntervals) {
          if (!(currentStart + numRowsToAlloc <= tStart || currentStart >= tStart + tLen)) {
            overlap = true;
            currentStart = tStart + tLen;
            if (currentStart >= start + len) {
              remainingLen = 0;
            } else {
              remainingLen = (start + len) - currentStart;
            }
            break;
          }
        }
        if (!overlap) return currentStart;
      }
    }
  }
  return (unsigned)-1;
}

bool pimResMgr::coreUsage::isAvailable(unsigned rowIdx, unsigned numRows) const
{
  auto it = m_freeIntervals.upper_bound(rowIdx);
  if (it != m_freeIntervals.begin()) {
    --it;
    return rowIdx >= it->first && (rowIdx + numRows) <= (it->first + it->second);
  }
  return false;
}

void pimResMgr::coreUsage::addRange(std::pair<unsigned, unsigned> range, PimObjId objId)
{
  m_tempIntervals[range.first] = range.second;
  m_objToIntervals[objId].push_back(range);
}

void pimResMgr::coreUsage::deleteObj(PimObjId objId)
{
  auto it = m_objToIntervals.find(objId);
  if (it != m_objToIntervals.end()) {
    for (auto const& range : it->second) {
      m_usedIntervals.erase(range.first);
      m_freeIntervals[range.first] = range.second;
      m_totRowsInUse -= range.second;
    }
    m_objToIntervals.erase(it);
    mergeFreeIntervals();
  }
}

void pimResMgr::coreUsage::newAllocStart()
{
  m_tempIntervals.clear();
}

void pimResMgr::coreUsage::newAllocEnd(bool success)
{
  if (success) {
    for (auto const& [start, len] : m_tempIntervals) {
      m_usedIntervals[start] = len;
      m_totRowsInUse += len;
      
      // Update free intervals - find the one containing [start, start+len)
      auto it = m_freeIntervals.upper_bound(start);
      if (it != m_freeIntervals.begin()) {
        --it;
        unsigned freeStart = it->first;
        unsigned freeLen = it->second;
        if (start >= freeStart && (start + len) <= (freeStart + freeLen)) {
          m_freeIntervals.erase(it);
          if (start > freeStart) {
            m_freeIntervals[freeStart] = start - freeStart;
          }
          if (freeStart + freeLen > start + len) {
            m_freeIntervals[start + len] = (freeStart + freeLen) - (start + len);
          }
        }
      }
    }
  }
  m_tempIntervals.clear();
}

void pimResMgr::coreUsage::reset()
{
  m_freeIntervals.clear();
  m_usedIntervals.clear();
  m_tempIntervals.clear();
  m_objToIntervals.clear();
  m_totRowsInUse = 0;
  m_freeIntervals[0] = m_numRowsPerCore;
}

void pimResMgr::coreUsage::mergeFreeIntervals()
{
  if (m_freeIntervals.empty()) return;
  auto it = m_freeIntervals.begin();
  while (std::next(it) != m_freeIntervals.end()) {
    auto next = std::next(it);
    if (it->first + it->second == next->first) {
      it->second += next->second;
      m_freeIntervals.erase(next);
    } else {
      ++it;
    }
  }
}
