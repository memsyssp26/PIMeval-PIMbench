// File: pimResMgr.h
// PIMeval Simulator - PIM Resource Manager
// Copyright (c) 2024 University of Virginia
// This file is licensed under the MIT License.
// See the LICENSE file in the root of this repository for more details.

#ifndef LAVA_PIM_RES_MGR_H
#define LAVA_PIM_RES_MGR_H

#include "libpimeval.h"      // for PimObjId, PimDataType
#include "pimUtils.h"        // for getNumBitsOfDataType, signExt, pimDataTypeEnumToStr, castTypeToBits
#include "pimMemoryTier.h"
#include <cinttypes>
#include <vector>            // for vector
#include <unordered_map>     // for unordered_map
#include <set>               // for set
#include <map>               // for map
#include <string>            // for string
#include <memory>            // for unique_ptr
#include <cassert>           // for assert

class pimDevice;


//! @class  pimRegion
//! @brief  Represent a rectangle region in a PIM core
class pimRegion
{
public:
  pimRegion() {}
  ~pimRegion() {}

  void setCoreId(PimCoreId coreId) { m_coreId = coreId; }
  void setRowIdx(unsigned rowIdx) { m_rowIdx = rowIdx; }
  void setColIdx(unsigned colIdx) { m_colIdx = colIdx; }
  void setNumAllocRows(unsigned numAllocRows) { m_numAllocRows = numAllocRows; }
  void setNumAllocCols(unsigned numAllocCols) { m_numAllocCols = numAllocCols; }
  void setElemIdxBegin(uint64_t idx) { m_elemIdxBegin = idx; }
  void setElemIdxEnd(uint64_t idx) { m_elemIdxEnd = idx; }
  void setIsValid(bool val) { m_isValid = val; }
  void setNumColsPerElem(unsigned val) { m_numColsPerElem = val; }
  void setIsBuffer(bool val) { m_isBuffer = val; }

  PimCoreId getCoreId() const { return m_coreId; }
  unsigned getRowIdx() const { return m_rowIdx; }
  unsigned getColIdx() const { return m_colIdx; }
  unsigned getNumAllocRows() const { return m_numAllocRows; }
  unsigned getNumAllocCols() const { return m_numAllocCols; }
  uint64_t getElemIdxBegin() const { return m_elemIdxBegin; }
  uint64_t getElemIdxEnd() const { return m_elemIdxEnd; }
  uint64_t getNumElemInRegion() const { return m_elemIdxEnd - m_elemIdxBegin; }
  unsigned getNumColsPerElem() const { return m_numColsPerElem; }
  bool isBuffer() const { return m_isBuffer; }

  std::pair<unsigned, unsigned> locateIthElemInRegion(unsigned i) const;

  bool isValid() const { return m_isValid && m_coreId >= 0 && m_numAllocRows > 0 && m_numAllocCols > 0; }

  void print() const;

private:
  PimCoreId m_coreId = -1;
  unsigned m_rowIdx = 0;        // starting row index
  unsigned m_colIdx = 0;        // starting col index
  unsigned m_numAllocRows = 0;  // number of rows of this region
  unsigned m_numAllocCols = 0;  // number of cols of this region
  uint64_t m_elemIdxBegin = 0;  // begin element index in this region
  uint64_t m_elemIdxEnd = 0;    // end element index in this region
  unsigned m_numColsPerElem = 0;  // number of cols per element
  bool m_isValid = false;
  bool m_isBuffer = false;  // true if this region is a buffer region
};

//! @enum   PimObjSyncState
//! @brief  Sync state between functional data holder and bit-mapped simulated memory
enum class PimObjSyncState {
  CLEAN = 0,
  DIRTY_FUNCTIONAL,  // Most recent data is in functional data holder
  DIRTY_BIT_MAPPED   // Most recent data is in bit-mapped simulated memory
};

//! @class  pimObjInfo
//! @brief  Meta data of a PIM object which includes
//!         - PIM object ID
//!         - One or more rectangle regions allocated in one or more PIM cores
//!         - Allocation type which specifies how data is stored in a region
class pimObjInfo
{
public:
  pimObjInfo(PimObjId objId, PimDataType dataType, PimAllocEnum allocType, uint64_t numElements, unsigned bitsPerElementPadded, pimDevice* device)
    : m_objId(objId),
      m_assocObjId(objId),
      m_dataType(dataType),
      m_allocType(allocType),
      m_numElements(numElements),
      m_bitsPerElementPadded(bitsPerElementPadded),
      m_device(device)
  {
    m_functionalTier = std::make_unique<pimFunctionalTier>(dataType, numElements);
    m_bitMappedTier = std::make_unique<pimBitMappedTier>(device, objId, numElements);
  }
  pimObjInfo(PimObjId objId, PimDataType dataType, PimAllocEnum allocType, uint64_t numElements, unsigned bitsPerElementPadded, pimDevice* device, bool isBuffer)
    : m_objId(objId),
      m_assocObjId(objId),
      m_dataType(dataType),
      m_allocType(allocType),
      m_numElements(numElements),
      m_bitsPerElementPadded(bitsPerElementPadded),
      m_device(device),
      m_isBuffer(isBuffer)
  {
    m_functionalTier = std::make_unique<pimFunctionalTier>(dataType, numElements);
    m_bitMappedTier = std::make_unique<pimBitMappedTier>(device, objId, numElements);
  }
  virtual ~pimObjInfo();

  void addRegion(pimRegion region) { m_regions.push_back(region); }
  void setObjId(PimObjId objId) { m_objId = objId; }
  void setAssocObjId(PimObjId assocObjId) { m_assocObjId = assocObjId; }
  void setRefObj(std::shared_ptr<pimObjInfo> refObj) { m_refObj = refObj; }
  void setIsDualContactRef(bool val) { m_isDualContactRef = val; }
  void setNumColsPerElem(unsigned val) { m_numColsPerElem = val; }
  void finalize();

  PimObjId getObjId() const { return m_objId; }
  PimObjId getAssocObjId() const { return m_assocObjId; }
  std::shared_ptr<pimObjInfo> getRefObj() const { return m_refObj; }
  bool isDualContactRef() const { return m_isDualContactRef; }
  PimAllocEnum getAllocType() const { return m_allocType; }
  PimDataType getDataType() const { return m_dataType; }
  uint64_t getNumElements() const { return m_numElements; }
  unsigned getBitsPerElement(PimBitWidth bitWidthType) const;
  pimDevice* getDevice() { return m_device; }
  bool isValid() const { return m_numElements > 0 && m_bitsPerElementPadded > 0 && !m_regions.empty(); }
  bool isVLayout() const { return m_allocType == PIM_ALLOC_V || m_allocType == PIM_ALLOC_V1; }
  bool isHLayout() const { return m_allocType == PIM_ALLOC_H || m_allocType == PIM_ALLOC_H1; }
  bool isLoadBalanced() const { return m_isLoadBalanced; }
  bool isBuffer() const { return m_isBuffer; }

  const std::vector<pimRegion>& getRegions() const { return m_regions; }
  std::vector<pimRegion> getRegionsOfCore(PimCoreId coreId) const;
  unsigned getMaxNumRegionsPerCore() const { return m_maxNumRegionsPerCore; }
  unsigned getNumCoresUsed() const { return m_numCoresUsed; }
  unsigned getNumCoreAvailable() const { return m_numCoreAvailable; }
  unsigned getMaxElementsPerRegion() const { return m_maxElementsPerRegion; }
  unsigned getNumColsPerElem() const { return m_numColsPerElem; }

  void print() const;

  // Note: Below functions are wraper APIs to access PIM object memory tiers
  // For regular PIM objects:
  // - Support host-to-device, device-to-host, and device-to-device copying
  // - Use bit representation to set or get an element at specific element index
  // - Support ranges in [idxBegin, idxEnd). Use full range if idxEnd is 0
  // For reference PIM objects:
  // - A ref object directly access the memory tiers of the ref-to object
  // - Dual-contact ref negates all bits during operations
  void copyFromHost(void* src, uint64_t idxBegin = 0, uint64_t idxEnd = 0);
  void copyToHost(void* dest, uint64_t idxBegin = 0, uint64_t idxEnd = 0) const;
  void copyToObj(pimObjInfo& destObj, uint64_t idxBegin = 0, uint64_t idxEnd = 0) const;
  void setElementBits(uint64_t index, uint64_t bits);
  uint64_t getElementBits(uint64_t index) const;
  template <typename T> void setElement(uint64_t index, T val) {
    setElementBits(index, pimUtils::castTypeToBits(val));
  }

  // Note: Below two functions are for supporting mixed functional and micro-ops level simulation.
  // Functional simulation purely uses the functional memory tier for simulation speed,
  // while micro-ops level simulation uses the bit-mapped simulated memory tier.
  // When a functional API is called during micro-ops level simulation, call below two functions
  // to sync the data between the functional and bit-mapped tiers.
  void syncFromSimulatedMem();
  void syncFromSimulatedMem(const pimRegion& region);
  void syncToSimulatedMem() const;
  void syncToSimulatedMem(const pimRegion& region) const;

  // Sync state management
  void markDirtyFunctional() { m_syncState = PimObjSyncState::DIRTY_FUNCTIONAL; }
  void markDirtyBitMapped() { m_syncState = PimObjSyncState::DIRTY_BIT_MAPPED; }
  void markClean() { m_syncState = PimObjSyncState::CLEAN; }
  PimObjSyncState getSyncState() const { return m_syncState; }

  // Tier access
  pimFunctionalTier* getFunctionalTier() { return m_functionalTier.get(); }
  pimBitMappedTier* getBitMappedTier() { return m_bitMappedTier.get(); }

private:
  PimObjId m_objId = -1;
  PimObjId m_assocObjId = -1;
  std::shared_ptr<pimObjInfo> m_refObj = nullptr;
  PimDataType m_dataType;
  PimAllocEnum m_allocType;
  std::unique_ptr<pimFunctionalTier> m_functionalTier;
  std::unique_ptr<pimBitMappedTier> m_bitMappedTier;
  uint64_t m_numElements = 0;
  unsigned m_bitsPerElementPadded = 0;
  unsigned m_numCoreAvailable = 0;
  std::vector<pimRegion> m_regions;  // a list of core ID and regions
  unsigned m_maxNumRegionsPerCore = 0;
  unsigned m_numCoresUsed = 0;
  unsigned m_maxElementsPerRegion = 0;
  unsigned m_numColsPerElem = 0; // number of cols per element
  bool m_isDualContactRef = false;
  pimDevice* m_device = nullptr; // for accessing simulated memory
  bool m_isLoadBalanced = true;
  bool m_isBuffer = false; // true if this is a global buffer
  PimObjSyncState m_syncState = PimObjSyncState::CLEAN;
};


//! @class  pimResMgr
//! @brief  PIM resource manager
class pimResMgr
{
public:
  pimResMgr(pimDevice* device);
  ~pimResMgr();

  PimObjId pimAlloc(PimAllocEnum allocType, uint64_t numElements, PimDataType dataType);
  PimObjId pimAllocAssociated(PimObjId assocId, PimDataType dataType);
  PimObjId pimAllocBuffer(uint32_t numElements, PimDataType dataType);
  PimStatus pimFree(PimObjId obj);
  PimObjId pimCreateRangedRef(PimObjId refId, uint64_t idxBegin, uint64_t idxEnd);
  PimObjId pimCreateDualContactRef(PimObjId refId);

  bool isValidObjId(PimObjId objId) const { return m_objMap.find(objId) != m_objMap.end(); }
  const pimObjInfo& getObjInfo(PimObjId objId) const { assert(objId != -1); return *(m_objMap.at(objId)); }
  pimObjInfo& getObjInfo(PimObjId objId) { assert(objId != -1); return *(m_objMap.at(objId)); }

  bool isVLayoutObj(PimObjId objId) const;
  bool isHLayoutObj(PimObjId objId) const;
  bool isHybridLayoutObj(PimObjId objId) const;

  void reset();

private:
  pimRegion findAvailRegionOnCore(PimCoreId coreId, unsigned numAllocRows, unsigned numAllocCols) const;
  std::vector<PimCoreId> getCoreIdsSortedByLeastUsage() const;
  
  //! @class  coreUsage
  //! @brief  Track row usage for allocation using interval-based tracking
  class coreUsage {
  public:
    coreUsage(unsigned numRowsPerCore) : m_numRowsPerCore(numRowsPerCore) {
      m_freeIntervals[0] = numRowsPerCore;
    }
    ~coreUsage() {}
    unsigned getNumRowsPerCore() const { return m_numRowsPerCore; }
    unsigned getTotRowsInUse() const { return m_totRowsInUse; }
    unsigned findAvailRange(unsigned numRowsToAlloc);
    bool isAvailable(unsigned rowIdx, unsigned numRows) const;
    void addRange(std::pair<unsigned, unsigned> range, PimObjId objId);
    void deleteObj(PimObjId objId);
    void newAllocStart();
    void newAllocEnd(bool success);
    void reset();
    
  private:
    void mergeFreeIntervals();
    unsigned m_numRowsPerCore = 0;
    unsigned m_totRowsInUse = 0;
    std::map<unsigned, unsigned> m_freeIntervals; // startIdx -> length
    std::map<unsigned, unsigned> m_usedIntervals; // startIdx -> length (persisted)
    std::map<unsigned, unsigned> m_tempIntervals; // startIdx -> length (pending)
    std::map<PimObjId, std::vector<std::pair<unsigned, unsigned>>> m_objToIntervals;
  };

  pimDevice* m_device;
  PimObjId m_availObjId;
  std::unordered_map<PimObjId, std::shared_ptr<pimObjInfo>> m_objMap;
  std::unordered_map<PimCoreId, std::unique_ptr<pimResMgr::coreUsage>> m_coreUsage;
  std::unordered_map<PimObjId, std::set<PimObjId>> m_refMap;
  bool m_debugAlloc = 0;
};

#endif

