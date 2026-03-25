// File: pimSim.h
// PIMeval Simulator - PIM Simulator Main Entry
// Copyright (c) 2024 University of Virginia
// This file is licensed under the MIT License.
// See the LICENSE file in the root of this repository for more details.

#ifndef LAVA_PIM_SIM_H
#define LAVA_PIM_SIM_H

#include "libpimeval.h"
#include "pimSimConfig.h"
#include "pimDevice.h"
#include "pimStats.h"
#include "pimUtils.h"
#include <memory>
#include <vector>
#include <string>
#include <stdarg.h>


//! @class  pimSim
//! @brief  PIM simulator main class
class pimSim
{
public:
  static pimSim* get();
  static void destroy();

  pimSim();
  ~pimSim();

  void uninit();
  bool isValidDevice(bool showMsg = true) const;

  PimStatus createDevice(PimDeviceEnum deviceType, unsigned numRanks, unsigned numBankPerRank, unsigned numSubarrayPerBank, unsigned numRows, unsigned numCols, unsigned bufferSize);
  PimStatus createDeviceFromConfig(PimDeviceEnum deviceType, const char* configFilePath);
  PimStatus deleteDevice();
  PimStatus getDeviceProperties(PimDeviceProperties* deviceProperties);

  const pimSimConfig& getConfig() const { return *m_config; }
  PimDeviceEnum getDeviceType() const { return m_config->getDeviceType(); }
  PimDeviceEnum getSimTarget() const { return m_config->getSimTarget(); }
  bool isAnalysisMode() const { return m_config->isAnalysisMode(); }
  unsigned getNumThreads() const { return m_config->getNumThreads(); }
  bool isDebug(pimSimConfig::pimDebugFlags flag) const { return m_config->getDebug() & flag; }

  bool isInit() const { return m_device != nullptr; }
  PimStatus pimInit(int* argc, char*** argv);

  unsigned getNumCores() const;
  unsigned getNumRows() const;
  unsigned getNumCols() const;

  unsigned getNumRanks() const { return m_config->getNumRanks(); }
  unsigned getNumBankPerRank() const { return m_config->getNumBankPerRank(); }
  unsigned getNumSubarrayPerBank() const { return m_config->getNumSubarrayPerBank(); }
  unsigned getNumRowPerSubarray() const { return m_config->getNumRowPerSubarray(); }
  unsigned getNumColPerSubarray() const { return m_config->getNumColPerSubarray(); }

  void startKernelTimer() const;
  void endKernelTimer() const;
  void showStats() const;
  void resetStats() const;

  pimStatsMgr* getStatsMgr() { return m_statsMgr.get(); }
  pimUtils::threadPool* getThreadPool() { return m_threadPool.get(); }
  pimPerfEnergyBase* getPerfEnergyModel();
  const pimParamsDram& getParamsDram() const { return *m_paramsDram; }

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

  PimStatus pimCopyObjectToObject(PimObjId src, PimObjId dest);
  PimStatus pimConvertType(PimObjId src, PimObjId dest);
  PimStatus pimAdd(PimObjId src1, PimObjId src2, PimObjId dest);
  PimStatus pimSub(PimObjId src1, PimObjId src2, PimObjId dest);
  PimStatus pimDiv(PimObjId src1, PimObjId src2, PimObjId dest);
  PimStatus pimAbs(PimObjId src, PimObjId dest);
  PimStatus pimMul(PimObjId src1, PimObjId src2, PimObjId dest);
  PimStatus pimNot(PimObjId src, PimObjId dest);
  PimStatus pimOr(PimObjId src1, PimObjId src2, PimObjId dest);
  PimStatus pimAnd(PimObjId src1, PimObjId src2, PimObjId dest);
  PimStatus pimXor(PimObjId src1, PimObjId src2, PimObjId dest);
  PimStatus pimXnor(PimObjId src1, PimObjId src2, PimObjId dest);
  PimStatus pimGT(PimObjId src1, PimObjId src2, PimObjId dest);
  PimStatus pimLT(PimObjId src1, PimObjId src2, PimObjId dest);
  PimStatus pimEQ(PimObjId src1, PimObjId src2, PimObjId dest);
  PimStatus pimNE(PimObjId src1, PimObjId src2, PimObjId dest);
  PimStatus pimMin(PimObjId src1, PimObjId src2, PimObjId dest);
  PimStatus pimMax(PimObjId src1, PimObjId src2, PimObjId dest);
  PimStatus pimAdd(PimObjId src, PimObjId dest, uint64_t scalarValue);
  PimStatus pimSub(PimObjId src, PimObjId dest, uint64_t scalarValue);
  PimStatus pimMul(PimObjId src, PimObjId dest, uint64_t scalarValue);
  PimStatus pimDiv(PimObjId src, PimObjId dest, uint64_t scalarValue);
  PimStatus pimAnd(PimObjId src, PimObjId dest, uint64_t scalarValue);
  PimStatus pimOr(PimObjId src, PimObjId dest, uint64_t scalarValue);
  PimStatus pimXor(PimObjId src, PimObjId dest, uint64_t scalarValue);
  PimStatus pimXnor(PimObjId src, PimObjId dest, uint64_t scalarValue);
  PimStatus pimGT(PimObjId src, PimObjId dest, uint64_t scalarValue);
  PimStatus pimLT(PimObjId src, PimObjId dest, uint64_t scalarValue);
  PimStatus pimEQ(PimObjId src, PimObjId dest, uint64_t scalarValue);
  PimStatus pimNE(PimObjId src, PimObjId dest, uint64_t scalarValue);
  PimStatus pimMin(PimObjId src, PimObjId dest, uint64_t scalarValue);
  PimStatus pimMax(PimObjId src, PimObjId dest, uint64_t scalarValue);
  PimStatus pimScaledAdd(PimObjId src1, PimObjId src2, PimObjId dest, uint64_t scalarValue);
  PimStatus pimPopCount(PimObjId src, PimObjId dest);
  PimStatus pimRedSum(PimObjId src, void* sum, uint64_t idxBegin = 0, uint64_t idxEnd = 0);
  PimStatus pimRedMin(PimObjId src, void* min, uint64_t idxBegin = 0, uint64_t idxEnd = 0);
  PimStatus pimRedMax(PimObjId src, void* max, uint64_t idxBegin = 0, uint64_t idxEnd = 0);
  PimStatus pimBitSliceExtract(PimObjId src, PimObjId destBool, unsigned bitIdx);
  PimStatus pimBitSliceInsert(PimObjId srcBool, PimObjId dest, unsigned bitIdx);
  PimStatus pimCondCopy(PimObjId condBool, PimObjId src, PimObjId dest);
  PimStatus pimCondBroadcast(PimObjId condBool, uint64_t scalarBits, PimObjId dest);
  PimStatus pimCondSelect(PimObjId condBool, PimObjId src1, PimObjId src2, PimObjId dest);
  PimStatus pimCondSelectScalar(PimObjId condBool, PimObjId src1, uint64_t scalarBits, PimObjId dest);
  template <typename T> PimStatus pimBroadcast(PimObjId dest, T value);
  PimStatus pimRotateElementsRight(PimObjId src);
  PimStatus pimRotateElementsLeft(PimObjId src);
  PimStatus pimShiftElementsRight(PimObjId src);
  PimStatus pimShiftElementsLeft(PimObjId src);
  PimStatus pimShiftBitsRight(PimObjId src, PimObjId dest, unsigned shiftAmount);
  PimStatus pimShiftBitsLeft(PimObjId src, PimObjId dest, unsigned shiftAmount);
  PimStatus pimAesSbox(PimObjId src, PimObjId dest, const std::vector<uint8_t>& lut);
  PimStatus pimAesInverseSbox(PimObjId src, PimObjId dest, const std::vector<uint8_t>& lut);
  PimStatus pimPrefixSum(PimObjId src, PimObjId dest);
  PimStatus pimMAC(PimObjId src1, PimObjId src2, void* dest);

  // PIM API Fusion
  PimStatus pimFuse(PimProg prog);

  // BitSIMD v-layout low-level APIs
  PimStatus pimOpReadRowToSa(PimObjId objId, unsigned ofst);
  PimStatus pimOpWriteSaToRow(PimObjId objId, unsigned ofst);
  PimStatus pimOpTRA(PimObjId src1, unsigned ofst1, PimObjId src2, unsigned ofst2, PimObjId src3, unsigned ofst3);
  PimStatus pimOpMove(PimObjId objId, PimRowReg src, PimRowReg dest);
  PimStatus pimOpSet(PimObjId objId, PimRowReg dest, bool val);
  PimStatus pimOpNot(PimObjId objId, PimRowReg src, PimRowReg dest);
  PimStatus pimOpAnd(PimObjId objId, PimRowReg src1, PimRowReg src2, PimRowReg dest);
  PimStatus pimOpOr(PimObjId objId, PimRowReg src1, PimRowReg src2, PimRowReg dest);
  PimStatus pimOpNand(PimObjId objId, PimRowReg src1, PimRowReg src2, PimRowReg dest);
  PimStatus pimOpNor(PimObjId objId, PimRowReg src1, PimRowReg src2, PimRowReg dest);
  PimStatus pimOpXor(PimObjId objId, PimRowReg src1, PimRowReg src2, PimRowReg dest);
  PimStatus pimOpXnor(PimObjId objId, PimRowReg src1, PimRowReg src2, PimRowReg dest);
  PimStatus pimOpMaj(PimObjId objId, PimRowReg src1, PimRowReg src2, PimRowReg src3, PimRowReg dest);
  PimStatus pimOpSel(PimObjId objId, PimRowReg cond, PimRowReg src1, PimRowReg src2, PimRowReg dest);
  PimStatus pimOpRotateRH(PimObjId objId, PimRowReg src);
  PimStatus pimOpRotateLH(PimObjId objId, PimRowReg src);

  // SIMDRAM commands
  PimStatus pimOpAP(int numSrc, va_list args);
  PimStatus pimOpAAP(int numSrc, int numDest, va_list args);

private:
  static pimSim* s_instance;

  PimStatus createDeviceCommon();

  std::unique_ptr<pimSimConfig> m_config;

  // support one device for now
  std::unique_ptr<pimDevice> m_device;
  std::unique_ptr<pimParamsDram> m_paramsDram;
  std::unique_ptr<pimStatsMgr> m_statsMgr;
  std::unique_ptr<pimUtils::threadPool> m_threadPool;

};

#endif
