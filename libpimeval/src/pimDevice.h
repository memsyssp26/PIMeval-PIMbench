// File: pimDevice.h
// PIMeval Simulator - PIM Device
// Copyright (c) 2024 University of Virginia
// This file is licensed under the MIT License.
// See the LICENSE file in the root of this repository for more details.

#ifndef LAVA_PIM_DEVICE_H
#define LAVA_PIM_DEVICE_H

#include "libpimeval.h"
#include "pimSimConfig.h"
#include "pimCore.h"
#include "pimCmd.h"
#include "pimPerfEnergyBase.h"
#ifdef DRAMSIM3_INTEG
#include "cpu.h"
#endif
#include <memory>

class pimResMgr;


//! @class  pimDevice
//! @brief  PIM device
class pimDevice
{
public:
  pimDevice(const pimSimConfig& config, const pimParamsDram& paramsDram)
    : m_config(config), m_paramsDram(paramsDram) {}
  virtual ~pimDevice();

  const pimSimConfig& getConfig() const { return m_config; }

  // Metadata
  virtual PimDeviceEnum getDeviceType() const { return m_config.getDeviceType(); }
  virtual PimDeviceEnum getSimTarget() const { return m_config.getSimTarget(); }
  
  unsigned getNumRanks() const { return m_config.getNumRanks(); }
  unsigned getNumBankPerRank() const { return m_config.getNumBankPerRank(); }
  unsigned getNumSubarrayPerBank() const { return m_config.getNumSubarrayPerBank(); }
  unsigned getNumRowPerSubarray() const { return m_config.getNumRowPerSubarray(); }
  unsigned getNumColPerSubarray() const { return m_config.getNumColPerSubarray(); }

  // Virtual Layout Properties
  virtual bool isVLayoutDevice() const = 0;
  virtual bool isHLayoutDevice() const = 0;
  virtual bool isHybridLayoutDevice() const { return false; }
  PimDataLayout getDeviceDataLayout() const {
    if (isVLayoutDevice()) return PimDataLayout::V;
    if (isHLayoutDevice()) return PimDataLayout::H;
    return PimDataLayout::UNKNOWN;
  }

  // Virtual initialization (Subclasses implement their own aggregation logic)
  virtual bool init() = 0;

  // Resource & Execution (Common to all devices)
  unsigned getNumCores() const { return m_numCores; }
  unsigned getNumRows() const { return m_numRows; }
  unsigned getNumCols() const { return m_numCols; }
  unsigned getBufferSize() const { return m_bufferSize; }
  bool isValid() const { return m_isValid; }

  PimObjId pimAlloc(PimAllocEnum allocType, uint64_t numElements, PimDataType dataType);
  PimObjId pimAllocAssociated(PimObjId assocId, PimDataType dataType);
  PimObjId pimAllocBuffer(uint32_t numElements, PimDataType dataType);
  PimStatus pimFree(PimObjId obj);
  PimStatus pimInjectError(PimObjId obj, uint64_t elemIdx, unsigned bitIdx);
  PimStatus pimInjectBurstError(PimObjId obj, uint64_t elemIdx, unsigned bitIdx, unsigned length);
  PimObjId pimCreateRangedRef(PimObjId refId, uint64_t idxBegin, uint64_t idxEnd);
  PimObjId pimCreateDualContactRef(PimObjId refId);

  PimStatus pimCopyMainToDevice(void* src, PimObjId dest, uint64_t idxBegin = 0, uint64_t idxEnd = 0);
  PimStatus pimCopyDeviceToMain(PimObjId src, void* dest, uint64_t idxBegin = 0, uint64_t idxEnd = 0);
  PimStatus pimCopyMainToDeviceWithType(PimCopyEnum copyType, void* src, PimObjId dest, uint64_t idxBegin = 0, uint64_t idxEnd = 0);
  PimStatus pimCopyDeviceToMainWithType(PimCopyEnum copyType, PimObjId src, void* dest, uint64_t idxBegin = 0, uint64_t idxEnd = 0);
  PimStatus pimCopyDeviceToDevice(PimObjId src, PimObjId dest, uint64_t idxBegin = 0, uint64_t idxEnd = 0);

  // Bit-level data access for synchronization
  PimStatus readBitMappedData(PimObjId objId, uint64_t elemIdx, uint64_t& bits);
  PimStatus writeBitMappedData(PimObjId objId, uint64_t elemIdx, uint64_t bits);

  pimResMgr* getResMgr() { return m_resMgr.get(); }
  pimPerfEnergyBase* getPerfEnergyModel() { return m_perfEnergyModel.get(); }
  pimCore& getCore(PimCoreId coreId) { return m_cores[coreId]; }
  PimStatus executeCmd(std::unique_ptr<pimCmd> cmd);

#ifdef DRAMSIM3_INTEG
  // Initialise DRAMSim3 instances — called from commonInit().
  void initDramSim3();

  // Simulate a bulk DRAM transfer of numBytes through one rank's DRAMSim3
  // model, then scale timing and energy to m_numRanks parallel ranks.
  // Returns a perfEnergy with cycle-accurate runtime (ms) and IDD/VDD-derived
  // energy (mJ).  Uses m_dramNextAddr internally to ensure consecutive calls
  // never alias the same DRAM rows (prevents row-buffer state bleed).
  pimeval::perfEnergy dramsim3SimulateTransfer(uint64_t numBytes,
                                               bool     isWrite);
#endif

protected:
  // Helpers for subclasses
  bool commonInit(unsigned numCores, unsigned numRows, unsigned numCols, unsigned bufferSize);

  const pimSimConfig& m_config;
  const pimParamsDram& m_paramsDram;
  unsigned m_numCores = 0;
  unsigned m_numRows = 0;
  unsigned m_numCols = 0;
  unsigned m_bufferSize = 0;
  bool m_isValid = false;
  std::unique_ptr<pimResMgr> m_resMgr;
  std::unique_ptr<pimPerfEnergyBase> m_perfEnergyModel;
  std::vector<pimCore> m_cores;

#ifdef DRAMSIM3_INTEG
  dramsim3::PIMCPU* m_deviceMemory = nullptr;
  // Monotonically advancing synthetic address used by dramsim3SimulateTransfer.
  // Each call advances past the range it used so consecutive transfers never
  // alias the same DRAMSim3 addresses and row-buffer state from one transfer
  // cannot bleed into the next.
  uint64_t m_dramNextAddr = 0;
#endif
};

/**
 * @class pimDeviceFactory
 * @brief Factory to create polymorphic PIM devices.
 */
class pimDeviceFactory {
public:
    static std::unique_ptr<pimDevice> create(const pimSimConfig& config, const pimParamsDram& paramsDram);
};

#endif

