// File: pimSim.cpp
// PIMeval Simulator - PIM Simulator Main Entry
// Copyright (c) 2024 University of Virginia
// This file is licensed under the MIT License.
// See the LICENSE file in the root of this repository for more details.

#include "pimSim.h"
#include "pimCmd.h"
#include "pimCmdFuse.h"
#include "pimParamsDram.h"
#include "pimStats.h"
#include "pimUtils.h"
#include <cstdio>
#include <memory>
#include <algorithm>
#include <string>

// The pimSim singleton
pimSim* pimSim::s_instance = nullptr;

//! @brief  Get or create the pimSim singleton
pimSim*
pimSim::get()
{
  if (!s_instance) {
    s_instance = new pimSim();
  }
  return s_instance;
}

void
pimSim::destroy()
{
  if (s_instance) {
    delete s_instance;
    s_instance = nullptr;
  }
}

//! @brief  pimSim ctor
pimSim::pimSim()
{
  m_config = std::make_unique<pimSimConfig>();
}

//! @brief  pimSim dtor
pimSim::~pimSim()
{
  uninit();
}

//! @brief  Uninitialize pimSim member classes
void
pimSim::uninit()
{
  if (m_device && m_device->getResMgr()) {
    m_device->getResMgr()->reset();
  }
  m_device.reset();
  m_threadPool.reset();
  m_statsMgr.reset();
  m_paramsDram.reset();
  if (m_config) {
    m_config->reset();
  }
}

bool
pimSim::isValidDevice(bool showMsg) const
{
  bool isValid = m_device && m_device->isValid();
  if (!isValid && showMsg) {
    std::printf("PIM-Error: Invalid PIM device\n");
  }
  return isValid;
}

PimStatus
pimSim::createDevice(PimDeviceEnum deviceType, unsigned numRanks, unsigned numBankPerRank, unsigned numSubarrayPerBank, unsigned numRows, unsigned numCols, unsigned bufferSize)
{
  if (isInit()) return PIM_ERR_ALREADY_INITIALIZED;
  pimPerfMon perfMon("createDevice");
  uninit();
  if (!m_config) {
    m_config = std::make_unique<pimSimConfig>();
  }
  if (numRanks == 0 || numBankPerRank == 0 || numSubarrayPerBank == 0 || numRows == 0 || numCols == 0) {
    return PIM_ERR_INVALID_ARG;
  }
  if (!m_config->init(deviceType, numRanks, numBankPerRank, numSubarrayPerBank, numRows, numCols, bufferSize)) {
    return PIM_ERR_CONFIG_INVALID;
  }
  return createDeviceCommon();
}

PimStatus
pimSim::createDeviceFromConfig(PimDeviceEnum deviceType, const char* configFilePath)
{
  if (isInit()) return PIM_ERR_ALREADY_INITIALIZED;
  pimPerfMon perfMon("createDeviceFromConfig");
  uninit();
  if (!configFilePath) return PIM_ERR_INVALID_ARG;
  if (!m_config->init(deviceType, configFilePath)) {
    return PIM_ERR_CONFIG_INVALID;
  }
  return createDeviceCommon();
}

PimStatus
pimSim::createDeviceCommon()
{
  // Create memory params, which is needed before creating pimDevice
  if (!m_config->getMemConfigFile().empty()) {
    try {
      m_paramsDram = pimParamsDram::createFromConfig(m_config->getMemConfigFile());
    } catch (...) {
      return PIM_ERR_CONFIG_INVALID;
    }
  } else {
    m_paramsDram = pimParamsDram::create(m_config->getMemoryProtocol());
  }

  if (!m_paramsDram) {
    return PIM_ERR_CONFIG_INVALID;
  }

  // Create PIM device
  m_device = pimDeviceFactory::create(*m_config, *m_paramsDram);

  if (!m_device) {
    uninit();
    return PIM_ERROR;
  }

  // Create stats mgr if not exists
  if (!m_statsMgr) {
    m_statsMgr = std::make_unique<pimStatsMgr>();
  }

  // Create thread pool
  if (getNumThreads() > 1) {
    m_threadPool = std::make_unique<pimUtils::threadPool>(getNumThreads());
  }
  return PIM_OK;
}

PimStatus
pimSim::deleteDevice()
{
  if (!m_device) return PIM_ERR_UNINITIALIZED;
  uninit();
  return PIM_OK;
}

PimStatus
pimSim::getDeviceProperties(PimDeviceProperties* deviceProperties) {
  pimPerfMon perfMon("getDeviceProperties");
  if (!m_device) {
    return PIM_ERR_UNINITIALIZED;
  }
  deviceProperties->deviceType = m_device->getDeviceType();
  deviceProperties->simTarget = m_device->getSimTarget();
  deviceProperties->numRanks = m_device->getNumRanks();
  deviceProperties->numBankPerRank = m_device->getNumBankPerRank();
  deviceProperties->numSubarrayPerBank = m_device->getNumSubarrayPerBank();
  deviceProperties->numRowPerSubarray = m_device->getNumRowPerSubarray();
  deviceProperties->numColPerSubarray = m_device->getNumColPerSubarray();
  deviceProperties->isHLayoutDevice = m_device->isHLayoutDevice();
  deviceProperties->numPIMCores = m_device->getNumCores();
  deviceProperties->numRowPerCore = m_device->getNumRows();
  return PIM_OK;
}

PimStatus
pimSim::pimInit(int* argc, char*** argv)
{
  return m_config->init(argc, argv) ? PIM_OK : PIM_ERR_CONFIG_INVALID;
}

unsigned pimSim::getNumCores() const { return m_device ? m_device->getNumCores() : 0; }
unsigned pimSim::getNumRows() const { return m_device ? m_device->getNumRows() : 0; }
unsigned pimSim::getNumCols() const { return m_device ? m_device->getNumCols() : 0; }

void pimSim::startKernelTimer() const { if (m_statsMgr) m_statsMgr->startKernelTimer(); }
void pimSim::endKernelTimer() const { if (m_statsMgr) m_statsMgr->endKernelTimer(); }
void pimSim::showStats() const { if (m_statsMgr) m_statsMgr->showStats(); }
void pimSim::resetStats() const { if (m_statsMgr) m_statsMgr->resetStats(); }

pimPerfEnergyBase* pimSim::getPerfEnergyModel() { return m_device ? m_device->getPerfEnergyModel() : nullptr; }

PimObjId
pimSim::pimAlloc(PimAllocEnum allocType, uint64_t numElements, PimDataType dataType)
{
  pimPerfMon perfMon("pimAlloc");
  if (!isValidDevice()) return -1;
  return m_device->pimAlloc(allocType, numElements, dataType);
}

PimObjId
pimSim::pimAllocAssociated(PimObjId assocId, PimDataType dataType)
{
  pimPerfMon perfMon("pimAllocAssociated");
  if (!isValidDevice()) return -1;
  return m_device->pimAllocAssociated(assocId, dataType);
}

PimObjId
pimSim::pimAllocBuffer(uint32_t numElements, PimDataType dataType)
{
  pimPerfMon perfMon("pimAllocBuffer");
  if (!isValidDevice()) return -1;
  return m_device->pimAllocBuffer(numElements, dataType);
}

PimStatus
pimSim::pimFree(PimObjId obj)
{
  pimPerfMon perfMon("pimFree");
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  return m_device->pimFree(obj);
}

PimStatus
pimSim::pimInjectError(PimObjId obj, uint64_t elemIdx, unsigned bitIdx)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  return m_device->pimInjectError(obj, elemIdx, bitIdx);
}

PimStatus
pimSim::pimInjectBurstError(PimObjId obj, uint64_t elemIdx, unsigned bitIdx, unsigned length)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  return m_device->pimInjectBurstError(obj, elemIdx, bitIdx, length);
}

PimObjId
pimSim::pimCreateRangedRef(PimObjId refId, uint64_t idxBegin, uint64_t idxEnd)
{
  pimPerfMon perfMon("pimCreateRangedRef");
  if (!isValidDevice()) return -1;
  return m_device->pimCreateRangedRef(refId, idxBegin, idxEnd);
}

PimObjId
pimSim::pimCreateDualContactRef(PimObjId refId)
{
  pimPerfMon perfMon("pimCreateDualContactRef");
  if (!isValidDevice()) return -1;
  return m_device->pimCreateDualContactRef(refId);
}

PimStatus
pimSim::pimCopyMainToDevice(void* src, PimObjId dest, uint64_t idxBegin, uint64_t idxEnd)
{
  pimPerfMon perfMon("pimCopyMainToDevice");
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  return m_device->pimCopyMainToDevice(src, dest, idxBegin, idxEnd);
}

PimStatus
pimSim::pimCopyDeviceToMain(PimObjId src, void* dest, uint64_t idxBegin, uint64_t idxEnd)
{
  pimPerfMon perfMon("pimCopyDeviceToMain");
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  return m_device->pimCopyDeviceToMain(src, dest, idxBegin, idxEnd);
}

PimStatus
pimSim::pimCopyMainToDeviceWithType(PimCopyEnum copyType, void* src, PimObjId dest, uint64_t idxBegin, uint64_t idxEnd)
{
  pimPerfMon perfMon("pimCopyMainToDevice");
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  return m_device->pimCopyMainToDeviceWithType(copyType, src, dest, idxBegin, idxEnd);
}

PimStatus
pimSim::pimCopyDeviceToMainWithType(PimCopyEnum copyType, PimObjId src, void* dest, uint64_t idxBegin, uint64_t idxEnd)
{
  pimPerfMon perfMon("pimCopyDeviceToMain");
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  return m_device->pimCopyDeviceToMainWithType(copyType, src, dest, idxBegin, idxEnd);
}

PimStatus
pimSim::pimCopyDeviceToDevice(PimObjId src, PimObjId dest, uint64_t idxBegin, uint64_t idxEnd)
{
  pimPerfMon perfMon("pimCopyDeviceToDevice");
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  return m_device->pimCopyDeviceToDevice(src, dest, idxBegin, idxEnd);
}

PimStatus
pimSim::pimCopyObjectToObject(PimObjId src, PimObjId dest)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc1>(PimCmdEnum::COPY_O2O, src, dest);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimConvertType(PimObjId src, PimObjId dest)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc1>(PimCmdEnum::CONVERT_TYPE, src, dest);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimAdd(PimObjId src1, PimObjId src2, PimObjId dest)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc2>(PimCmdEnum::ADD, src1, src2, dest);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimSub(PimObjId src1, PimObjId src2, PimObjId dest)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc2>(PimCmdEnum::SUB, src1, src2, dest);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimDiv(PimObjId src1, PimObjId src2, PimObjId dest)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc2>(PimCmdEnum::DIV, src1, src2, dest);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimAbs(PimObjId src, PimObjId dest)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc1>(PimCmdEnum::ABS, src, dest);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimMul(PimObjId src1, PimObjId src2, PimObjId dest)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc2>(PimCmdEnum::MUL, src1, src2, dest);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimNot(PimObjId src, PimObjId dest)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc1>(PimCmdEnum::NOT, src, dest);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimOr(PimObjId src1, PimObjId src2, PimObjId dest)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc2>(PimCmdEnum::OR, src1, src2, dest);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimAnd(PimObjId src1, PimObjId src2, PimObjId dest)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc2>(PimCmdEnum::AND, src1, src2, dest);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimXor(PimObjId src1, PimObjId src2, PimObjId dest)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc2>(PimCmdEnum::XOR, src1, src2, dest);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimXnor(PimObjId src1, PimObjId src2, PimObjId dest)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc2>(PimCmdEnum::XNOR, src1, src2, dest);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimGT(PimObjId src1, PimObjId src2, PimObjId dest)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc2>(PimCmdEnum::GT, src1, src2, dest);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimLT(PimObjId src1, PimObjId src2, PimObjId dest)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc2>(PimCmdEnum::LT, src1, src2, dest);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimEQ(PimObjId src1, PimObjId src2, PimObjId dest)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc2>(PimCmdEnum::EQ, src1, src2, dest);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimNE(PimObjId src1, PimObjId src2, PimObjId dest)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc2>(PimCmdEnum::NE, src1, src2, dest);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimMin(PimObjId src1, PimObjId src2, PimObjId dest)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc2>(PimCmdEnum::MIN, src1, src2, dest);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimMax(PimObjId src1, PimObjId src2, PimObjId dest)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc2>(PimCmdEnum::MAX, src1, src2, dest);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimAdd(PimObjId src, PimObjId dest, uint64_t scalarValue)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc1>(PimCmdEnum::ADD_SCALAR, src, dest, scalarValue);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimSub(PimObjId src, PimObjId dest, uint64_t scalarValue)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc1>(PimCmdEnum::SUB_SCALAR, src, dest, scalarValue);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimMul(PimObjId src, PimObjId dest, uint64_t scalarValue)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc1>(PimCmdEnum::MUL_SCALAR, src, dest, scalarValue);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimDiv(PimObjId src, PimObjId dest, uint64_t scalarValue)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc1>(PimCmdEnum::DIV_SCALAR, src, dest, scalarValue);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimAnd(PimObjId src, PimObjId dest, uint64_t scalarValue)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc1>(PimCmdEnum::AND_SCALAR, src, dest, scalarValue);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimOr(PimObjId src, PimObjId dest, uint64_t scalarValue)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc1>(PimCmdEnum::OR_SCALAR, src, dest, scalarValue);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimXor(PimObjId src, PimObjId dest, uint64_t scalarValue)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc1>(PimCmdEnum::XOR_SCALAR, src, dest, scalarValue);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimXnor(PimObjId src, PimObjId dest, uint64_t scalarValue)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc1>(PimCmdEnum::XNOR_SCALAR, src, dest, scalarValue);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimGT(PimObjId src, PimObjId dest, uint64_t scalarValue)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc1>(PimCmdEnum::GT_SCALAR, src, dest, scalarValue);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimLT(PimObjId src, PimObjId dest, uint64_t scalarValue)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc1>(PimCmdEnum::LT_SCALAR, src, dest, scalarValue);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimEQ(PimObjId src, PimObjId dest, uint64_t scalarValue)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc1>(PimCmdEnum::EQ_SCALAR, src, dest, scalarValue);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimNE(PimObjId src, PimObjId dest, uint64_t scalarValue)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc1>(PimCmdEnum::NE_SCALAR, src, dest, scalarValue);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimMin(PimObjId src, PimObjId dest, uint64_t scalarValue)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc1>(PimCmdEnum::MIN_SCALAR, src, dest, scalarValue);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimMax(PimObjId src, PimObjId dest, uint64_t scalarValue)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc1>(PimCmdEnum::MAX_SCALAR, src, dest, scalarValue);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimScaledAdd(PimObjId src1, PimObjId src2, PimObjId dest, uint64_t scalarValue)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc2>(PimCmdEnum::SCALED_ADD, src1, src2, dest, scalarValue);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimPopCount(PimObjId src, PimObjId dest)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc1>(PimCmdEnum::POPCOUNT, src, dest);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimRedSum(PimObjId src, void* sum, uint64_t idxBegin, uint64_t idxEnd)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  const PimDataType dataType = m_device->getResMgr()->getObjInfo(src).getDataType();
  std::unique_ptr<pimCmd> cmd;
  PimCmdEnum cmdType = (idxBegin < idxEnd && idxEnd > 0) ? PimCmdEnum::REDSUM_RANGE : PimCmdEnum::REDSUM;
  switch (dataType) {
    case PIM_INT8:
    case PIM_INT16:
    case PIM_INT32:
    case PIM_INT64:
      cmd = std::make_unique<pimCmdReduction<int64_t>>(cmdType, src, sum, idxBegin, idxEnd);
      break;
    case PIM_BOOL:
    case PIM_UINT8:
    case PIM_UINT16:
    case PIM_UINT32:
    case PIM_UINT64:
      cmd = std::make_unique<pimCmdReduction<uint64_t>>(cmdType, src, sum, idxBegin, idxEnd);
      break;
    case PIM_FP32:
      cmd = std::make_unique<pimCmdReduction<float>>(cmdType, src, sum, idxBegin, idxEnd);
      break;
    default: return PIM_ERR_CONFIG_INVALID;
  }
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimRedMin(PimObjId src, void* min, uint64_t idxBegin, uint64_t idxEnd)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  const PimDataType dataType = m_device->getResMgr()->getObjInfo(src).getDataType();
  std::unique_ptr<pimCmd> cmd;
  PimCmdEnum cmdType = (idxBegin < idxEnd && idxEnd > 0) ? PimCmdEnum::REDMIN_RANGE : PimCmdEnum::REDMIN;
  switch (dataType) {
    case PIM_INT8: cmd = std::make_unique<pimCmdReduction<int8_t>>(cmdType, src, min, idxBegin, idxEnd); break;
    case PIM_INT16: cmd = std::make_unique<pimCmdReduction<int16_t>>(cmdType, src, min, idxBegin, idxEnd); break;
    case PIM_INT32: cmd = std::make_unique<pimCmdReduction<int32_t>>(cmdType, src, min, idxBegin, idxEnd); break;
    case PIM_INT64: cmd = std::make_unique<pimCmdReduction<int64_t>>(cmdType, src, min, idxBegin, idxEnd); break;
    case PIM_UINT8: cmd = std::make_unique<pimCmdReduction<uint8_t>>(cmdType, src, min, idxBegin, idxEnd); break;
    case PIM_UINT16: cmd = std::make_unique<pimCmdReduction<uint16_t>>(cmdType, src, min, idxBegin, idxEnd); break;
    case PIM_UINT32: cmd = std::make_unique<pimCmdReduction<uint32_t>>(cmdType, src, min, idxBegin, idxEnd); break;
    case PIM_UINT64: cmd = std::make_unique<pimCmdReduction<uint64_t>>(cmdType, src, min, idxBegin, idxEnd); break;
    case PIM_FP32: cmd = std::make_unique<pimCmdReduction<float>>(cmdType, src, min, idxBegin, idxEnd); break;
    default: return PIM_ERR_CONFIG_INVALID;
  }
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimRedMax(PimObjId src, void* max, uint64_t idxBegin, uint64_t idxEnd)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  const PimDataType dataType = m_device->getResMgr()->getObjInfo(src).getDataType();
  std::unique_ptr<pimCmd> cmd;
  PimCmdEnum cmdType = (idxBegin < idxEnd && idxEnd > 0) ? PimCmdEnum::REDMAX_RANGE : PimCmdEnum::REDMAX;
  switch (dataType) {
    case PIM_INT8: cmd = std::make_unique<pimCmdReduction<int8_t>>(cmdType, src, max, idxBegin, idxEnd); break;
    case PIM_INT16: cmd = std::make_unique<pimCmdReduction<int16_t>>(cmdType, src, max, idxBegin, idxEnd); break;
    case PIM_INT32: cmd = std::make_unique<pimCmdReduction<int32_t>>(cmdType, src, max, idxBegin, idxEnd); break;
    case PIM_INT64: cmd = std::make_unique<pimCmdReduction<int64_t>>(cmdType, src, max, idxBegin, idxEnd); break;
    case PIM_UINT8: cmd = std::make_unique<pimCmdReduction<uint8_t>>(cmdType, src, max, idxBegin, idxEnd); break;
    case PIM_UINT16: cmd = std::make_unique<pimCmdReduction<uint16_t>>(cmdType, src, max, idxBegin, idxEnd); break;
    case PIM_UINT32: cmd = std::make_unique<pimCmdReduction<uint32_t>>(cmdType, src, max, idxBegin, idxEnd); break;
    case PIM_UINT64: cmd = std::make_unique<pimCmdReduction<uint64_t>>(cmdType, src, max, idxBegin, idxEnd); break;
    case PIM_FP32: cmd = std::make_unique<pimCmdReduction<float>>(cmdType, src, max, idxBegin, idxEnd); break;
    default: return PIM_ERR_CONFIG_INVALID;
  }
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimBitSliceExtract(PimObjId src, PimObjId destBool, unsigned bitIdx)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc1>(PimCmdEnum::BIT_SLICE_EXTRACT, src, destBool, bitIdx);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimBitSliceInsert(PimObjId srcBool, PimObjId dest, unsigned bitIdx)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc1>(PimCmdEnum::BIT_SLICE_INSERT, srcBool, dest, bitIdx);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimCondCopy(PimObjId condBool, PimObjId src, PimObjId dest)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdCond>(PimCmdEnum::COND_COPY, condBool, src, dest);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimCondBroadcast(PimObjId condBool, uint64_t scalarBits, PimObjId dest)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdCond>(PimCmdEnum::COND_BROADCAST, condBool, scalarBits, dest);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimCondSelect(PimObjId condBool, PimObjId src1, PimObjId src2, PimObjId dest)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdCond>(PimCmdEnum::COND_SELECT, condBool, src1, src2, dest);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimCondSelectScalar(PimObjId condBool, PimObjId src1, uint64_t scalarBits, PimObjId dest)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdCond>(PimCmdEnum::COND_SELECT_SCALAR, condBool, src1, scalarBits, dest);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimRotateElementsRight(PimObjId src)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdRotate>(PimCmdEnum::ROTATE_ELEM_R, src);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimRotateElementsLeft(PimObjId src)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdRotate>(PimCmdEnum::ROTATE_ELEM_L, src);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimShiftElementsRight(PimObjId src)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdRotate>(PimCmdEnum::SHIFT_ELEM_R, src);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimShiftElementsLeft(PimObjId src)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdRotate>(PimCmdEnum::SHIFT_ELEM_L, src);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimShiftBitsRight(PimObjId src, PimObjId dest, unsigned shiftAmount)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc1>(PimCmdEnum::SHIFT_BITS_R, src, dest, shiftAmount);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimShiftBitsLeft(PimObjId src, PimObjId dest, unsigned shiftAmount)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc1>(PimCmdEnum::SHIFT_BITS_L, src, dest, shiftAmount);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimAesSbox(PimObjId src, PimObjId dest, const std::vector<uint8_t>& lut)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc1>(PimCmdEnum::AES_SBOX, src, dest, lut);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimAesInverseSbox(PimObjId src, PimObjId dest, const std::vector<uint8_t>& lut)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFunc1>(PimCmdEnum::AES_INVERSE_SBOX, src, dest, lut);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimPrefixSum(PimObjId src, PimObjId dest)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdPrefixSum>(PimCmdEnum::PREFIX_SUM, src, dest);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimMAC(PimObjId src1, PimObjId src2, void* dest)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  const PimDataType dataType = m_device->getResMgr()->getObjInfo(src1).getDataType();
  std::unique_ptr<pimCmd> cmd;
  PimCmdEnum cmdType = PimCmdEnum::MAC;
  switch (dataType) {
    case PIM_INT8: cmd = std::make_unique<pimCmdMAC<int8_t>>(cmdType, src1, src2, dest); break;
    case PIM_INT16: cmd = std::make_unique<pimCmdMAC<int16_t>>(cmdType, src1, src2, dest); break;
    case PIM_INT32: cmd = std::make_unique<pimCmdMAC<int32_t>>(cmdType, src1, src2, dest); break;
    case PIM_INT64: cmd = std::make_unique<pimCmdMAC<int64_t>>(cmdType, src1, src2, dest); break;
    case PIM_UINT8: cmd = std::make_unique<pimCmdMAC<uint8_t>>(cmdType, src1, src2, dest); break;
    case PIM_UINT16: cmd = std::make_unique<pimCmdMAC<uint16_t>>(cmdType, src1, src2, dest); break;
    case PIM_UINT32: cmd = std::make_unique<pimCmdMAC<uint32_t>>(cmdType, src1, src2, dest); break;
    case PIM_UINT64: cmd = std::make_unique<pimCmdMAC<uint64_t>>(cmdType, src1, src2, dest); break;
    case PIM_FP32: cmd = std::make_unique<pimCmdMAC<float>>(cmdType, src1, src2, dest); break;
    default: return PIM_ERR_CONFIG_INVALID;
  }
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimFuse(PimProg prog)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdFuse>(prog);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimOpReadRowToSa(PimObjId objId, unsigned ofst)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdReadRowToSa>(PimCmdEnum::ROW_R, objId, ofst);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimOpWriteSaToRow(PimObjId objId, unsigned ofst)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdWriteSaToRow>(PimCmdEnum::ROW_W, objId, ofst);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimOpTRA(PimObjId src1, unsigned ofst1, PimObjId src2, unsigned ofst2, PimObjId src3, unsigned ofst3)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::vector<std::pair<PimObjId, unsigned>> srcRows = {{src1, ofst1}, {src2, ofst2}, {src3, ofst3}};
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdAnalogAAP>(PimCmdEnum::ROW_AP, srcRows, std::vector<std::pair<PimObjId, unsigned>>{});
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimOpMove(PimObjId objId, PimRowReg src, PimRowReg dest)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdRRegOp>(PimCmdEnum::RREG_MOV, objId, dest, src);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimOpSet(PimObjId objId, PimRowReg dest, bool val)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdRRegOp>(PimCmdEnum::RREG_SET, objId, dest, val);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimOpNot(PimObjId objId, PimRowReg src, PimRowReg dest)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdRRegOp>(PimCmdEnum::RREG_NOT, objId, dest, src);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimOpAnd(PimObjId objId, PimRowReg src1, PimRowReg src2, PimRowReg dest)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdRRegOp>(PimCmdEnum::RREG_AND, objId, dest, src1, src2);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimOpOr(PimObjId objId, PimRowReg src1, PimRowReg src2, PimRowReg dest)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdRRegOp>(PimCmdEnum::RREG_OR, objId, dest, src1, src2);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimOpNand(PimObjId objId, PimRowReg src1, PimRowReg src2, PimRowReg dest)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdRRegOp>(PimCmdEnum::RREG_NAND, objId, dest, src1, src2);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimOpNor(PimObjId objId, PimRowReg src1, PimRowReg src2, PimRowReg dest)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdRRegOp>(PimCmdEnum::RREG_NOR, objId, dest, src1, src2);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimOpXor(PimObjId objId, PimRowReg src1, PimRowReg src2, PimRowReg dest)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdRRegOp>(PimCmdEnum::RREG_XOR, objId, dest, src1, src2);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimOpXnor(PimObjId objId, PimRowReg src1, PimRowReg src2, PimRowReg dest)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdRRegOp>(PimCmdEnum::RREG_XNOR, objId, dest, src1, src2);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimOpMaj(PimObjId objId, PimRowReg src1, PimRowReg src2, PimRowReg src3, PimRowReg dest)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdRRegOp>(PimCmdEnum::RREG_MAJ, objId, dest, src1, src2, src3);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimOpSel(PimObjId objId, PimRowReg cond, PimRowReg src1, PimRowReg src2, PimRowReg dest)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdRRegOp>(PimCmdEnum::RREG_SEL, objId, dest, cond, src1, src2);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimOpRotateRH(PimObjId objId, PimRowReg src)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdRRegRotate>(PimCmdEnum::RREG_ROTATE_R, objId, src);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimOpRotateLH(PimObjId objId, PimRowReg src)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdRRegRotate>(PimCmdEnum::RREG_ROTATE_L, objId, src);
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimOpAP(int numSrc, va_list args)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::vector<std::pair<PimObjId, unsigned>> srcRows;
  for (int i = 0; i < numSrc; ++i) {
    PimObjId objId = va_arg(args, PimObjId);
    unsigned ofst = va_arg(args, unsigned);
    srcRows.push_back({objId, ofst});
  }
  // PIM_RREG_SA is implicit destination for AP
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdAnalogAAP>(PimCmdEnum::ROW_AP, srcRows, std::vector<std::pair<PimObjId, unsigned>>{});
  return m_device->executeCmd(std::move(cmd));
}

PimStatus
pimSim::pimOpAAP(int numSrc, int numDest, va_list args)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  std::vector<std::pair<PimObjId, unsigned>> srcRows, destRows;
  for (int i = 0; i < numSrc; ++i) {
    PimObjId objId = va_arg(args, PimObjId);
    unsigned ofst = va_arg(args, unsigned);
    srcRows.push_back({objId, ofst});
  }
  for (int i = 0; i < numDest; ++i) {
    PimObjId objId = va_arg(args, PimObjId);
    unsigned ofst = va_arg(args, unsigned);
    destRows.push_back({objId, ofst});
  }
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdAnalogAAP>(PimCmdEnum::ROW_AAP, srcRows, destRows);
  return m_device->executeCmd(std::move(cmd));
}

template <typename T>
PimStatus
pimSim::pimBroadcast(PimObjId dest, T value)
{
  if (!isValidDevice()) return PIM_ERR_UNINITIALIZED;
  uint64_t signExtBits = pimUtils::castTypeToBits(value);
  std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdBroadcast>(PimCmdEnum::BROADCAST, dest, signExtBits);
  return m_device->executeCmd(std::move(cmd));
}

// Explicit template instantiations
template PimStatus pimSim::pimBroadcast<uint64_t>(PimObjId dest, uint64_t value);
template PimStatus pimSim::pimBroadcast<int64_t>(PimObjId dest, int64_t value);
template PimStatus pimSim::pimBroadcast<float>(PimObjId dest, float value);
