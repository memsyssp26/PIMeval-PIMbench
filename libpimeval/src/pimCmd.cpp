// File: pimCmd.cpp
// PIMeval Simulator - PIM Commands
// Copyright (c) 2024 University of Virginia
// This file is licensed under the MIT License.
// See the LICENSE file in the root of this repository for more details.

#include "pimCmd.h"
#include "pimSim.h"
#include "pimSimConfig.h"
#include "pimDevice.h"
#include "pimCore.h"
#include "pimResMgr.h"
#include "libpimeval.h"
#include "pimStats.h"
#include <cstdio>
#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <climits>
#include <cinttypes>

std::string
pimCmd::getName(PimCmdEnum cmdType, const std::string& suffix)
{
  static const std::unordered_map<PimCmdEnum, std::string> cmdNames = {
    { PimCmdEnum::NOOP, "noop" },
    { PimCmdEnum::COPY_H2D, "copy_h2d" },
    { PimCmdEnum::COPY_D2H, "copy_d2h" },
    { PimCmdEnum::COPY_D2D, "copy_d2d" },
    { PimCmdEnum::COPY_O2O, "copy_o2o" },
    { PimCmdEnum::ABS, "abs" },
    { PimCmdEnum::POPCOUNT, "popcount" },
    { PimCmdEnum::SHIFT_BITS_R, "shift_bits_r" },
    { PimCmdEnum::SHIFT_BITS_L, "shift_bits_l" },
    { PimCmdEnum::BROADCAST, "broadcast" },
    { PimCmdEnum::ADD, "add" },
    { PimCmdEnum::SUB, "sub" },
    { PimCmdEnum::MUL, "mul" },
    { PimCmdEnum::SCALED_ADD, "scaled_add" },
    { PimCmdEnum::DIV, "div" },
    { PimCmdEnum::NOT, "not" },
    { PimCmdEnum::AND, "and" },
    { PimCmdEnum::OR, "or" },
    { PimCmdEnum::XOR, "xor" },
    { PimCmdEnum::XNOR, "xnor" },
    { PimCmdEnum::GT, "gt" },
    { PimCmdEnum::LT, "lt" },
    { PimCmdEnum::EQ, "eq" },
    { PimCmdEnum::NE, "ne" },
    { PimCmdEnum::MIN, "min" },
    { PimCmdEnum::MAX, "max" },
    { PimCmdEnum::ADD_SCALAR, "add_scalar" },
    { PimCmdEnum::SUB_SCALAR, "sub_scalar" },
    { PimCmdEnum::MUL_SCALAR, "mul_scalar" },
    { PimCmdEnum::DIV_SCALAR, "div_scalar" },
    { PimCmdEnum::AND_SCALAR, "and_scalar" },
    { PimCmdEnum::OR_SCALAR, "or_scalar" },
    { PimCmdEnum::XOR_SCALAR, "xor_scalar" },
    { PimCmdEnum::XNOR_SCALAR, "xnor_scalar" },
    { PimCmdEnum::GT_SCALAR, "gt_scalar" },
    { PimCmdEnum::LT_SCALAR, "lt_scalar" },
    { PimCmdEnum::EQ_SCALAR, "eq_scalar" },
    { PimCmdEnum::NE_SCALAR, "ne_scalar" },
    { PimCmdEnum::MIN_SCALAR, "min_scalar" },
    { PimCmdEnum::MAX_SCALAR, "max_scalar" },
    { PimCmdEnum::CONVERT_TYPE, "convert_type" },
    { PimCmdEnum::BIT_SLICE_EXTRACT, "bit_slice_extract" },
    { PimCmdEnum::BIT_SLICE_INSERT, "bit_slice_insert" },
    { PimCmdEnum::COND_COPY, "cond_copy" },
    { PimCmdEnum::COND_BROADCAST, "cond_broadcast" },
    { PimCmdEnum::COND_SELECT, "cond_select" },
    { PimCmdEnum::COND_SELECT_SCALAR, "cond_select_scalar" },
    { PimCmdEnum::AES_SBOX, "aes_sbox" },
    { PimCmdEnum::AES_INVERSE_SBOX, "aes_inverse_sbox" },
    { PimCmdEnum::PREFIX_SUM, "prefix_sum"},
    { PimCmdEnum::REDSUM, "redsum" },
    { PimCmdEnum::REDSUM_RANGE, "redsum_range" },
    { PimCmdEnum::REDMIN, "redmin" },
    { PimCmdEnum::REDMIN_RANGE, "redmin_range" },
    { PimCmdEnum::REDMAX, "redmax" },
    { PimCmdEnum::REDMAX_RANGE, "redmax_range" },
    { PimCmdEnum::MAC, "mac" },
    { PimCmdEnum::ROTATE_ELEM_R, "rotate_elem_r" },
    { PimCmdEnum::ROTATE_ELEM_L, "rotate_elem_l" },
    { PimCmdEnum::SHIFT_ELEM_R, "shift_elem_r" },
    { PimCmdEnum::SHIFT_ELEM_L, "shift_elem_l" },
    { PimCmdEnum::ROW_R, "row_r" },
    { PimCmdEnum::ROW_W, "row_w" },
    { PimCmdEnum::RREG_MOV, "rreg.mov" },
    { PimCmdEnum::RREG_SET, "rreg.set" },
    { PimCmdEnum::RREG_NOT, "rreg.not" },
    { PimCmdEnum::RREG_AND, "rreg.and" },
    { PimCmdEnum::RREG_OR, "rreg.or" },
    { PimCmdEnum::RREG_NAND, "rreg.nand" },
    { PimCmdEnum::RREG_NOR, "rreg.nor" },
    { PimCmdEnum::RREG_XOR, "rreg.xor" },
    { PimCmdEnum::RREG_XNOR, "rreg.xnor" },
    { PimCmdEnum::RREG_MAJ, "rreg.maj" },
    { PimCmdEnum::RREG_SEL, "rreg.sel" },
    { PimCmdEnum::RREG_ROTATE_R, "rreg.rotate_r" },
    { PimCmdEnum::RREG_ROTATE_L, "rreg.rotate_l" },
    { PimCmdEnum::ROW_AP, "row_ap" },
    { PimCmdEnum::ROW_AAP, "row_aap" },
  };
  auto it = cmdNames.find(cmdType);
  return it != cmdNames.end() ? it->second + suffix : "unknown";
}

pimCmd::pimCmd(PimCmdEnum cmdType)
  : m_cmdType(cmdType)
{
  m_debugCmds = pimSim::get()->isDebug(pimSimConfig::DEBUG_CMDS);
}

pimResMgr*
pimCmd::getResMgr() const { return m_device->getResMgr(); }

pimCore&
pimCmd::getCore(PimCoreId coreId) const { return m_device->getCore(coreId); }

PimStatus
pimCmd::isValidObjId(pimResMgr* resMgr, PimObjId objId) const

{
  if (!resMgr->isValidObjId(objId)) {
    return PIM_ERR_OBJ_INVALID;
  }
  return PIM_OK;
}

PimStatus
pimCmd::isAssociated(const pimObjInfo& obj1, const pimObjInfo& obj2) const
{
  if (obj1.getAssocObjId() != obj2.getAssocObjId()) {
    return PIM_ERR_LAYOUT_MISMATCH;
  }
  return PIM_OK;
}

PimStatus
pimCmd::isCompatibleType(const pimObjInfo& obj1, const pimObjInfo& obj2) const
{
  if (obj1.getDataType() != obj2.getDataType()) {
    return PIM_ERR_CONFIG_INVALID;
  }
  return PIM_OK;
}

PimStatus
pimCmd::isConvertibleType(const pimObjInfo& src, const pimObjInfo& dest) const
{
  if (src.getDataType() != dest.getDataType()) {
    return PIM_ERR_CONFIG_INVALID;
  }
  return PIM_OK;
}

PimStatus
pimCmd::computeAllRegions(unsigned numRegions)
{
  if (pimSim::get()->isAnalysisMode()) return PIM_OK;
  if (pimSim::get()->getNumThreads() > 1) {
    for (unsigned i = 0; i < numRegions; ++i) {
      pimSim::get()->getThreadPool()->enqueue([this, i] {
        this->computeRegion(i);
      });
    }
    pimSim::get()->getThreadPool()->wait();
  } else {
    for (unsigned i = 0; i < numRegions; ++i) {
      computeRegion(i);
    }
  }
  return PIM_OK;
}

PimStatus
pimCmdCopy::execute()
{
  PimStatus status = sanityCheck();
  if (status != PIM_OK) {
    std::printf("PIM-Debug: pimCmdCopy::sanityCheck failed with status %d\n", (int)status);
    return status;
  }

  if (m_device->getDeviceType() != PIM_FUNCTIONAL) {
    if (m_cmdType == PimCmdEnum::COPY_D2H || m_cmdType == PimCmdEnum::COPY_D2D) {
      pimObjInfo &objSrc = m_device->getResMgr()->getObjInfo(m_src);
      objSrc.syncFromSimulatedMem();
    }
  }

  if (m_cmdType == PimCmdEnum::COPY_H2D) {
    pimObjInfo &objDest = m_device->getResMgr()->getObjInfo(m_dest);
    objDest.copyFromHost(m_ptr, m_idxBegin, m_idxEnd);
  } else if (m_cmdType == PimCmdEnum::COPY_D2H) {
    pimObjInfo &objSrc = m_device->getResMgr()->getObjInfo(m_src);
    objSrc.copyToHost(m_ptr, m_idxBegin, m_idxEnd);
  } else if (m_cmdType == PimCmdEnum::COPY_D2D) {
    pimObjInfo &objSrc = m_device->getResMgr()->getObjInfo(m_src);
    pimObjInfo &objDest = m_device->getResMgr()->getObjInfo(m_dest);
    objSrc.copyToObj(objDest, m_idxBegin, m_idxEnd);
  }

  if (m_device->getDeviceType() != PIM_FUNCTIONAL) {
    if (m_cmdType == PimCmdEnum::COPY_H2D || m_cmdType == PimCmdEnum::COPY_D2D) {
      pimObjInfo &objDest = m_device->getResMgr()->getObjInfo(m_dest);
      objDest.syncToSimulatedMem();
      objDest.markDirtyBitMapped();
    }
  }

  updateStats();
  return PIM_OK;
}

PimStatus
pimCmdCopy::sanityCheck() const
{
  pimResMgr* resMgr = m_device->getResMgr();
  switch (m_cmdType) {
  case PimCmdEnum::COPY_H2D:
    if (!m_ptr) return PIM_ERR_INVALID_ARG;
    if (!resMgr->isValidObjId(m_dest)) return PIM_ERR_OBJ_INVALID;
    break;
  case PimCmdEnum::COPY_D2H:
    if (!resMgr->isValidObjId(m_src)) return PIM_ERR_OBJ_INVALID;
    if (!m_ptr) return PIM_ERR_INVALID_ARG;
    break;
  case PimCmdEnum::COPY_D2D:
    if (!resMgr->isValidObjId(m_src)) return PIM_ERR_OBJ_INVALID;
    if (!resMgr->isValidObjId(m_dest)) return PIM_ERR_OBJ_INVALID;
    { PimStatus status = isAssociated(resMgr->getObjInfo(m_src), resMgr->getObjInfo(m_dest));
      if (status != PIM_OK) return status; }
    break;
  default:
    break;
  }
  return PIM_OK;
}

PimStatus
pimCmdCopy::updateStats() const {
  pimResMgr* resMgr = m_device->getResMgr();
  pimStatsMgr* statsMgr = pimSim::get()->getStatsMgr();
  pimPerfEnergyBase* perfModel = m_device->getPerfEnergyModel();
  uint64_t numElements = (m_idxEnd == 0) ? 0 : (m_idxEnd - m_idxBegin);
  if (m_cmdType == PimCmdEnum::COPY_H2D) {
    if (numElements == 0) numElements = resMgr->getObjInfo(m_dest).getNumElements();
    unsigned bitsPerElem = resMgr->getObjInfo(m_dest).getBitsPerElement(PimBitWidth::HOST);
    uint64_t numBytes = (numElements * bitsPerElem + 7) / 8;
    statsMgr->recordCopyMainToDevice(numBytes * 8, perfModel->getPerfEnergyForBytesTransfer(m_cmdType, numBytes));
  } else if (m_cmdType == PimCmdEnum::COPY_D2H) {
    if (numElements == 0) numElements = resMgr->getObjInfo(m_src).getNumElements();
    unsigned bitsPerElem = resMgr->getObjInfo(m_src).getBitsPerElement(PimBitWidth::HOST);
    uint64_t numBytes = (numElements * bitsPerElem + 7) / 8;
    statsMgr->recordCopyDeviceToMain(numBytes * 8, perfModel->getPerfEnergyForBytesTransfer(m_cmdType, numBytes));
  } else if (m_cmdType == PimCmdEnum::COPY_D2D) {
    if (numElements == 0) numElements = resMgr->getObjInfo(m_src).getNumElements();
    unsigned bitsPerElem = resMgr->getObjInfo(m_src).getBitsPerElement(PimBitWidth::HOST);
    uint64_t numBytes = (numElements * bitsPerElem + 7) / 8;
    statsMgr->recordCopyDeviceToDevice(numBytes * 8, perfModel->getPerfEnergyForBytesTransfer(m_cmdType, numBytes));
  }
  return PIM_OK;
}

PimStatus
pimCmdFunc1::execute()
{
  PimStatus status = sanityCheck();
  if (status != PIM_OK) return status;

  if (m_device->getDeviceType() != PIM_FUNCTIONAL) {
    pimObjInfo &objSrc = m_device->getResMgr()->getObjInfo(m_src);
    objSrc.syncFromSimulatedMem();
    if (m_cmdType == PimCmdEnum::BIT_SLICE_INSERT) {
      pimObjInfo &objDest = m_device->getResMgr()->getObjInfo(m_dest);
      objDest.syncFromSimulatedMem();
    }
  }

  pimObjInfo &objSrc = m_device->getResMgr()->getObjInfo(m_src);
  computeAllRegions(objSrc.getRegions().size());

  if (m_device->getDeviceType() != PIM_FUNCTIONAL) {
    pimObjInfo &objDest = m_device->getResMgr()->getObjInfo(m_dest);
    objDest.markDirtyFunctional();
  }

  updateStats();
  return PIM_OK;
}

PimStatus
pimCmdFunc1::sanityCheck() const
{
  pimResMgr* resMgr = m_device->getResMgr();
  PimStatus status;
  if ((status = isValidObjId(resMgr, m_src)) != PIM_OK) return status;
  if ((status = isValidObjId(resMgr, m_dest)) != PIM_OK) return status;
  if ((status = isAssociated(resMgr->getObjInfo(m_src), resMgr->getObjInfo(m_dest))) != PIM_OK) return status;
  return PIM_OK;
}

PimStatus
pimCmdFunc1::computeRegion(unsigned index)
{
  pimResMgr* resMgr = m_device->getResMgr();
  pimObjInfo& objSrc = resMgr->getObjInfo(m_src);
  pimObjInfo& objDest = resMgr->getObjInfo(m_dest);
  const pimRegion& region =
 objSrc.getRegions()[index];
  
  PimDataType dataType = objSrc.getDataType();
  int bitsPerElementSrc = objSrc.getBitsPerElement(PimBitWidth::ACTUAL);

  for (uint64_t i = region.getElemIdxBegin(); i < region.getElemIdxEnd(); ++i) {
    uint64_t srcBits = 0;
    objSrc.getFunctionalTier()->read(i, srcBits);
    uint64_t destBits = 0;
    
    if (pimUtils::isFP(dataType)) {
      float srcVal = pimUtils::castBitsToFloat(dataType, srcBits);
      float scalarVal = pimUtils::castBitsToFloat(dataType, m_scalarValue);
      float destVal = 0;
      computeResultFP<float>(srcVal, m_cmdType, scalarVal, destVal);
      if (objDest.getDataType() == PIM_BOOL) {
        destBits = (destVal != 0);
      } else {
        destBits = pimUtils::castFloatToBits(objDest.getDataType(), destVal);
      }
    } else if (pimUtils::isUnsigned(dataType)) {
      uint64_t srcVal = srcBits;
      uint64_t scalarVal = m_scalarValue;
      uint64_t destVal = 0;
      if (m_cmdType == PimCmdEnum::BIT_SLICE_INSERT) {
        uint64_t existingDestBits = 0;
        objDest.getFunctionalTier()->read(i, existingDestBits);
        uint64_t bitVal = srcVal & 1;
        uint64_t bitIdx = scalarVal;
        if (bitVal) existingDestBits |= (1ULL << bitIdx);
        else existingDestBits &= ~(1ULL << bitIdx);
        destBits = existingDestBits;
      } else {
        computeResult<uint64_t>(srcVal, m_cmdType, scalarVal, destVal, bitsPerElementSrc);
        destBits = destVal;
      }
    } else {
      int64_t srcVal = pimUtils::castBitsToType<int64_t>(srcBits);
      int64_t scalarVal = pimUtils::castBitsToType<int64_t>(m_scalarValue);
      int64_t destVal = 0;
      if (m_cmdType == PimCmdEnum::BIT_SLICE_INSERT) {
        uint64_t existingDestBits = 0;
        objDest.getFunctionalTier()->read(i, existingDestBits);
        int64_t bitVal = srcVal & 1;
        uint64_t bitIdx = static_cast<uint64_t>(scalarVal);
        if (bitVal) existingDestBits |= (1ULL << bitIdx);
        else existingDestBits &= ~(1ULL << bitIdx);
        destBits = existingDestBits;
      } else {
        computeResult<int64_t>(srcVal, m_cmdType, scalarVal, destVal, bitsPerElementSrc);
        destBits = pimUtils::castTypeToBits<int64_t>(destVal);
      }
    }
    objDest.getFunctionalTier()->write(i, destBits);
  }
  return PIM_OK;
}
PimStatus
pimCmdFunc1::updateStats() const {
  pimResMgr* resMgr = m_device->getResMgr();
  pimStatsMgr* statsMgr = pimSim::get()->getStatsMgr();
  pimPerfEnergyBase* perfModel = m_device->getPerfEnergyModel();
  const pimObjInfo& objSrc = resMgr->getObjInfo(m_src);
  const pimObjInfo& objDest = resMgr->getObjInfo(m_dest);
  auto pe = perfModel->getPerfEnergyForFunc1(m_cmdType, objSrc, objDest);
  // ODECC fires on every row activation during PIM compute: read src + write dest
  // Use raw data width (ACTUAL), not padded width, since ODECC operates on raw data inside the die
  unsigned rawBits = pimUtils::getNumBitsOfDataType(objSrc.getDataType(), PimBitWidth::ACTUAL);
  uint64_t bytesTouched = (objSrc.getNumElements() + objDest.getNumElements())
                          * (uint64_t)rawBits / 8;
  perfModel->addOdeccOverhead(pe, bytesTouched);
  statsMgr->recordCmd(getName(), pe);
  return PIM_OK;
}

PimStatus
pimCmdFunc2::execute()
{
  PimStatus status = sanityCheck();
  if (status != PIM_OK) return status;

  if (m_device->getDeviceType() != PIM_FUNCTIONAL) {
    pimObjInfo &objSrc1 = m_device->getResMgr()->getObjInfo(m_src1);
    pimObjInfo &objSrc2 = m_device->getResMgr()->getObjInfo(m_src2);
    objSrc1.syncFromSimulatedMem();
    objSrc2.syncFromSimulatedMem();
  }

  pimObjInfo &objSrc1 = m_device->getResMgr()->getObjInfo(m_src1);
  computeAllRegions(objSrc1.getRegions().size());

  if (m_device->getDeviceType() != PIM_FUNCTIONAL) {
    pimObjInfo &objDest = m_device->getResMgr()->getObjInfo(m_dest);
    objDest.markDirtyFunctional();
  }

  updateStats();
  return PIM_OK;
}

PimStatus
pimCmdFunc2::sanityCheck() const
{
  pimResMgr* resMgr = m_device->getResMgr();
  PimStatus status;
  if ((status = isValidObjId(resMgr, m_src1)) != PIM_OK) {
    return status;
  }
  if ((status = isValidObjId(resMgr, m_src2)) != PIM_OK) {
    return status;
  }
  if ((status = isValidObjId(resMgr, m_dest)) != PIM_OK) {
    return status;
  }
  const pimObjInfo& o1 = resMgr->getObjInfo(m_src1);
  if ((status = isAssociated(o1, resMgr->getObjInfo(m_src2))) != PIM_OK) {
    return status;
  }
  if ((status = isAssociated(o1, resMgr->getObjInfo(m_dest))) != PIM_OK) {
    return status;
  }
  return PIM_OK;
}

PimStatus
pimCmdFunc2::computeRegion(unsigned index)
{
  pimResMgr* resMgr = m_device->getResMgr();
  pimObjInfo& objSrc1 = resMgr->getObjInfo(m_src1);
  pimObjInfo& objSrc2 = resMgr->getObjInfo(m_src2);
  pimObjInfo& objDest = resMgr->getObjInfo(m_dest);
  const pimRegion& region =
 objSrc1.getRegions()[index];
  
  PimDataType dataType = objSrc1.getDataType();

  for (uint64_t i = region.getElemIdxBegin(); i < region.getElemIdxEnd(); ++i) {
    uint64_t srcBits1 = 0;
    uint64_t srcBits2 = 0;
    objSrc1.getFunctionalTier()->read(i, srcBits1);
    objSrc2.getFunctionalTier()->read(i, srcBits2);
    uint64_t destBits = 0;
    
    if (pimUtils::isFP(dataType)) {
      float srcVal1 = pimUtils::castBitsToFloat(dataType, srcBits1);
      float srcVal2 = pimUtils::castBitsToFloat(dataType, srcBits2);
      float scalarVal = pimUtils::castBitsToFloat(dataType, m_scalarValue);
      float destVal = 0;
      computeResultFP<float>(srcVal1, srcVal2, m_cmdType, scalarVal, destVal);
      if (objDest.getDataType() == PIM_BOOL) {
        destBits = (destVal != 0);
      } else {
        destBits = pimUtils::castFloatToBits(objDest.getDataType(), destVal);
      }
    } else if (pimUtils::isUnsigned(dataType)) {
      uint64_t srcVal1 = srcBits1;
      uint64_t srcVal2 = srcBits2;
      uint64_t scalarVal = m_scalarValue;
      uint64_t destVal = 0;
      computeResult<uint64_t>(srcVal1, srcVal2, m_cmdType, scalarVal, destVal);
      destBits = destVal;
    } else {
      int64_t srcVal1 = pimUtils::castBitsToType<int64_t>(srcBits1);
      int64_t srcVal2 = pimUtils::castBitsToType<int64_t>(srcBits2);
      int64_t scalarVal = pimUtils::castBitsToType<int64_t>(m_scalarValue);
      int64_t destVal = 0;
      computeResult<int64_t>(srcVal1, srcVal2, m_cmdType, scalarVal, destVal);
      destBits = pimUtils::castTypeToBits<int64_t>(destVal);
    }
    objDest.getFunctionalTier()->write(i, destBits);
  }
  return PIM_OK;
}
PimStatus
pimCmdFunc2::updateStats() const {
  pimResMgr* resMgr = m_device->getResMgr();
  pimStatsMgr* statsMgr = pimSim::get()->getStatsMgr();
  pimPerfEnergyBase* perfModel = m_device->getPerfEnergyModel();
  const pimObjInfo& objSrc1 = resMgr->getObjInfo(m_src1);
  const pimObjInfo& objSrc2 = resMgr->getObjInfo(m_src2);
  const pimObjInfo& objDest = resMgr->getObjInfo(m_dest);
  auto pe = perfModel->getPerfEnergyForFunc2(m_cmdType, objSrc1, objSrc2, objDest);
  // ODECC fires on every row activation: read src1 + read src2 + write dest
  unsigned rawBits = pimUtils::getNumBitsOfDataType(objSrc1.getDataType(), PimBitWidth::ACTUAL);
  uint64_t bytesTouched = (objSrc1.getNumElements() + objSrc2.getNumElements() + objDest.getNumElements())
                          * (uint64_t)rawBits / 8;
  perfModel->addOdeccOverhead(pe, bytesTouched);
  statsMgr->recordCmd(getName(), pe);
  return PIM_OK;
}

PimStatus
pimCmdCond::execute()
{
  PimStatus status = sanityCheck();
  if (status != PIM_OK) return status;
  
  if (m_device->getDeviceType() != PIM_FUNCTIONAL) {
    m_device->getResMgr()->getObjInfo(m_condBool).syncFromSimulatedMem();
    if (m_cmdType == PimCmdEnum::COND_COPY || m_cmdType == PimCmdEnum::COND_SELECT || m_cmdType == PimCmdEnum::COND_SELECT_SCALAR) {
      m_device->getResMgr()->getObjInfo(m_src1).syncFromSimulatedMem();
    }
    if (m_cmdType == PimCmdEnum::COND_SELECT) {
      m_device->getResMgr()->getObjInfo(m_src2).syncFromSimulatedMem();
    }
  }

  pimObjInfo &objCond = m_device->getResMgr()->getObjInfo(m_condBool);
  computeAllRegions(objCond.getRegions().size());
  
  if (m_device->getDeviceType() != PIM_FUNCTIONAL) {
    pimObjInfo &objDest = m_device->getResMgr()->getObjInfo(m_dest);
    objDest.markDirtyFunctional();
  }
  
  updateStats();
  return PIM_OK;
}

PimStatus
pimCmdCond::sanityCheck() const { return PIM_OK; }
PimStatus
pimCmdCond::computeRegion(unsigned index)
{
  pimResMgr* resMgr = m_device->getResMgr();
  pimObjInfo& objCond = resMgr->getObjInfo(m_condBool);
  pimObjInfo& objDest = resMgr->getObjInfo(m_dest);
  const pimRegion& region =
 objCond.getRegions()[index];

  for (uint64_t i = region.getElemIdxBegin(); i < region.getElemIdxEnd(); ++i) {
    uint64_t condBits = 0;
    objCond.getFunctionalTier()->read(i, condBits);
    bool cond = (condBits != 0);
    uint64_t destBits = 0;

    switch (m_cmdType) {
      case PimCmdEnum::COND_COPY:
        if (cond) {
          resMgr->getObjInfo(m_src1).getFunctionalTier()->read(i, destBits);
        } else {
          objDest.getFunctionalTier()->read(i, destBits);
        }
        break;
      case PimCmdEnum::COND_BROADCAST:
        if (cond) {
          destBits = m_scalarBits;
        } else {
          objDest.getFunctionalTier()->read(i, destBits);
        }
        break;
      case PimCmdEnum::COND_SELECT:
        if (cond) {
          resMgr->getObjInfo(m_src1).getFunctionalTier()->read(i, destBits);
        } else {
          resMgr->getObjInfo(m_src2).getFunctionalTier()->read(i, destBits);
        }
        break;
      case PimCmdEnum::COND_SELECT_SCALAR:
        if (cond) {
          resMgr->getObjInfo(m_src1).getFunctionalTier()->read(i, destBits);
        } else {
          destBits = m_scalarBits;
        }
        break;
      default:
        return PIM_ERR_NOT_SUPPORTED;
    }
    objDest.getFunctionalTier()->write(i, destBits);
  }
  return PIM_OK;
}
PimStatus
pimCmdCond::updateStats() const { return PIM_OK; }

PimStatus
pimCmdPrefixSum::execute() { return computeAllRegions(1); }
PimStatus
pimCmdPrefixSum::sanityCheck() const { return PIM_OK; }
PimStatus
pimCmdPrefixSum::computeRegion(unsigned index)
{
  (void)index; // always use 1 region for prefix sum
  pimResMgr* resMgr = m_device->getResMgr();
  pimObjInfo& objSrc = resMgr->getObjInfo(m_src);
  pimObjInfo& objDest = resMgr->getObjInfo(m_dst);
  
  PimDataType dataType = objSrc.getDataType();
  uint64_t numElements = objSrc.getNumElements();

  if (pimUtils::isFP(dataType)) {
    float runningSum = 0;
    for (uint64_t i = 0; i < numElements; ++i) {
      uint64_t srcBits = 0;
      objSrc.getFunctionalTier()->read(i, srcBits);
      float srcVal = pimUtils::castBitsToFloat(dataType, srcBits);
      runningSum += srcVal;
      uint64_t destBits = pimUtils::castFloatToBits(dataType, runningSum);
      objDest.getFunctionalTier()->write(i, destBits);
    }
  } else {
    int64_t runningSum = 0;
    for (uint64_t i = 0; i < numElements; ++i) {
      uint64_t srcBits = 0;
      objSrc.getFunctionalTier()->read(i, srcBits);
      int64_t srcVal = pimUtils::castBitsToType<int64_t>(srcBits);
      runningSum += srcVal;
      uint64_t destBits = pimUtils::castTypeToBits<int64_t>(runningSum);
      objDest.getFunctionalTier()->write(i, destBits);
    }
  }
  return PIM_OK;
}
PimStatus
pimCmdPrefixSum::updateStats() const {
  pimStatsMgr* statsMgr = pimSim::get()->getStatsMgr();
  pimPerfEnergyBase* perfModel = m_device->getPerfEnergyModel();
  const pimObjInfo& objSrc = m_device->getResMgr()->getObjInfo(m_src);
  auto pe = perfModel->getPerfEnergyForPrefixSum(m_cmdType, objSrc);
  // ODECC: read src + write dest (same size)
  unsigned rawBits = pimUtils::getNumBitsOfDataType(objSrc.getDataType(), PimBitWidth::ACTUAL);
  uint64_t bytesTouched = objSrc.getNumElements() * 2
                          * (uint64_t)rawBits / 8;
  perfModel->addOdeccOverhead(pe, bytesTouched);
  statsMgr->recordCmd(getName(), pe);
  return PIM_OK;
}

PimStatus
pimCmdBroadcast::execute() { return computeAllRegions(1); }
PimStatus
pimCmdBroadcast::sanityCheck() const { return PIM_OK; }
PimStatus
pimCmdBroadcast::computeRegion(unsigned index)
{
  (void)index;
  pimResMgr* resMgr = getResMgr();
  pimObjInfo& objDest = resMgr->getObjInfo(m_dest);
  uint64_t numElements = objDest.getNumElements();

  for (uint64_t i = 0; i < numElements; ++i) {
    objDest.getFunctionalTier()->write(i, m_signExtBits);
  }
  return PIM_OK;
}

PimStatus
pimCmdBroadcast::updateStats() const {
  pimStatsMgr* statsMgr = pimSim::get()->getStatsMgr();
  pimPerfEnergyBase* perfModel = m_device->getPerfEnergyModel();
  const pimObjInfo& objDest = m_device->getResMgr()->getObjInfo(m_dest);
  auto pe = perfModel->getPerfEnergyForBroadcast(m_cmdType, objDest);
  // ODECC: write dest
  unsigned rawBits = pimUtils::getNumBitsOfDataType(objDest.getDataType(), PimBitWidth::ACTUAL);
  uint64_t bytesTouched = objDest.getNumElements()
                          * (uint64_t)rawBits / 8;
  perfModel->addOdeccOverhead(pe, bytesTouched);
  statsMgr->recordCmd(getName(), pe);
  return PIM_OK;
}

PimStatus
pimCmdRotate::execute() { return computeAllRegions(1); }
PimStatus
pimCmdRotate::sanityCheck() const { return PIM_OK; }
PimStatus
pimCmdRotate::computeRegion(unsigned index)
{
  (void)index; // use 1 region for full rotate
  pimResMgr* resMgr = m_device->getResMgr();
  pimObjInfo& obj = resMgr->getObjInfo(m_src);
  uint64_t numElements = obj.getNumElements();
  if (numElements <= 1) return PIM_OK;

  if (m_cmdType == PimCmdEnum::ROTATE_ELEM_R) {
    uint64_t lastBits = 0;
    obj.getFunctionalTier()->read(numElements - 1, lastBits);
    for (uint64_t i = numElements - 1; i > 0; --i) {
      uint64_t prevBits = 0;
      obj.getFunctionalTier()->read(i - 1, prevBits);
      obj.getFunctionalTier()->write(i, prevBits);
    }
    obj.getFunctionalTier()->write(0, lastBits);
  } else if (m_cmdType == PimCmdEnum::ROTATE_ELEM_L) {
    uint64_t firstBits = 0;
    obj.getFunctionalTier()->read(0, firstBits);
    for (uint64_t i = 0; i < numElements - 1; ++i) {
      uint64_t nextBits = 0;
      obj.getFunctionalTier()->read(i + 1, nextBits);
      obj.getFunctionalTier()->write(i, nextBits);
    }
    obj.getFunctionalTier()->write(numElements - 1, firstBits);
  } else if (m_cmdType == PimCmdEnum::SHIFT_ELEM_R) {
    for (uint64_t i = numElements - 1; i > 0; --i) {
      uint64_t prevBits = 0;
      obj.getFunctionalTier()->read(i - 1, prevBits);
      obj.getFunctionalTier()->write(i, prevBits);
    }
    obj.getFunctionalTier()->write(0, 0);
  } else if (m_cmdType == PimCmdEnum::SHIFT_ELEM_L) {
    for (uint64_t i = 0; i < numElements - 1; ++i) {
      uint64_t nextBits = 0;
      obj.getFunctionalTier()->read(i + 1, nextBits);
      obj.getFunctionalTier()->write(i, nextBits);
    }
    obj.getFunctionalTier()->write(numElements - 1, 0);
  }
  return PIM_OK;
}
PimStatus
pimCmdRotate::updateStats() const {
  pimStatsMgr* statsMgr = pimSim::get()->getStatsMgr();
  pimPerfEnergyBase* perfModel = m_device->getPerfEnergyModel();
  const pimObjInfo& objSrc = m_device->getResMgr()->getObjInfo(m_src);
  auto pe = perfModel->getPerfEnergyForRotate(m_cmdType, objSrc);
  // ODECC: read + write all elements
  unsigned rawBits = pimUtils::getNumBitsOfDataType(objSrc.getDataType(), PimBitWidth::ACTUAL);
  uint64_t bytesTouched = objSrc.getNumElements() * 2
                          * (uint64_t)rawBits / 8;
  perfModel->addOdeccOverhead(pe, bytesTouched);
  statsMgr->recordCmd(getName(), pe);
  return PIM_OK;
}

PimStatus
pimCmdReadRowToSa::execute() {
  pimResMgr* resMgr = m_device->getResMgr();
  if (!resMgr->isValidObjId(m_objId)) return PIM_ERR_OBJ_INVALID;
  pimObjInfo& obj = resMgr->getObjInfo(m_objId);
  for (const auto& region : obj.getRegions()) {
    obj.syncToSimulatedMem(region);
    PimCoreId coreId = region.getCoreId();
    unsigned rowIdx = region.getRowIdx() + m_ofst;
    m_device->getCore(coreId).readRowToSa(rowIdx);
  }
  updateStats();
  return PIM_OK;
}
PimStatus
pimCmdReadRowToSa::updateStats() const {
  pimSim::get()->getStatsMgr()->recordCmd("row_r", m_device->getPerfEnergyModel()->getPerfEnergyForRowBitOp(m_cmdType, m_device->getResMgr()->getObjInfo(m_objId)));
  return PIM_OK;
}

PimStatus
pimCmdWriteSaToRow::execute() {
  pimResMgr* resMgr = m_device->getResMgr();
  if (!resMgr->isValidObjId(m_objId)) return PIM_ERR_OBJ_INVALID;
  pimObjInfo& obj = resMgr->getObjInfo(m_objId);
  for (const auto& region : obj.getRegions()) {
    PimCoreId coreId = region.getCoreId();
    unsigned rowIdx = region.getRowIdx() + m_ofst;
    m_device->getCore(coreId).writeSaToRow(rowIdx);
    obj.markDirtyBitMapped(); // Mark this region/object as dirty after write
  }
  updateStats();
  return PIM_OK;
}
PimStatus
pimCmdWriteSaToRow::updateStats() const {
  pimSim::get()->getStatsMgr()->recordCmd("row_w", m_device->getPerfEnergyModel()->getPerfEnergyForRowBitOp(m_cmdType, m_device->getResMgr()->getObjInfo(m_objId)));
  return PIM_OK;
}

PimStatus
pimCmdRRegOp::execute() {
  pimResMgr* resMgr = m_device->getResMgr();
  if (!resMgr->isValidObjId(m_objId)) return PIM_ERR_OBJ_INVALID;
  pimObjInfo& obj = resMgr->getObjInfo(m_objId);
  for (const auto& region : obj.getRegions()) {
    PimCoreId coreId = region.getCoreId();
    pimCore& core = m_device->getCore(coreId);
    switch (m_cmdType) {
      case PimCmdEnum::RREG_MOV: core.rreg_mov(m_src1, m_dest); break;
      case PimCmdEnum::RREG_SET: core.rreg_set(m_dest, m_val); break;
      case PimCmdEnum::RREG_NOT: core.rreg_not(m_src1, m_dest); break;
      case PimCmdEnum::RREG_AND: core.rreg_and(m_src1, m_src2, m_dest); break;
      case PimCmdEnum::RREG_OR:  core.rreg_or(m_src1, m_src2, m_dest); break;
      case PimCmdEnum::RREG_NAND:core.rreg_nand(m_src1, m_src2, m_dest); break;
      case PimCmdEnum::RREG_NOR: core.rreg_nor(m_src1, m_src2, m_dest); break;
      case PimCmdEnum::RREG_XOR: core.rreg_xor(m_src1, m_src2, m_dest); break;
      case PimCmdEnum::RREG_XNOR:core.rreg_xnor(m_src1, m_src2, m_dest); break;
      case PimCmdEnum::RREG_MAJ: core.rreg_maj(m_src1, m_src2, m_src3, m_dest); break;
      case PimCmdEnum::RREG_SEL: core.rreg_sel(m_src1, m_src2, m_src3, m_dest); break;
      default: return PIM_ERROR;
    }
  }
  updateStats();
  return PIM_OK;
}
PimStatus
pimCmdRRegOp::updateStats() const {
  pimSim::get()->getStatsMgr()->recordCmd(getName(), m_device->getPerfEnergyModel()->getPerfEnergyForRowBitOp(m_cmdType,
 m_device->getResMgr()->getObjInfo(m_objId)));
  return PIM_OK;
}

PimStatus
pimCmdRRegRotate::execute() {
  pimResMgr* resMgr = m_device->getResMgr();
  if (!resMgr->isValidObjId(m_objId)) return PIM_ERR_OBJ_INVALID;
  pimObjInfo& obj = resMgr->getObjInfo(m_objId);
  for (const auto& region : obj.getRegions()) {
    PimCoreId coreId = region.getCoreId();
    pimCore& core = m_device->getCore(coreId);
    if (m_cmdType == PimCmdEnum::RREG_ROTATE_R) core.rreg_rotate_r(m_dest);
    else core.rreg_rotate_l(m_dest);
  }
  updateStats();
  return PIM_OK;
}
PimStatus
pimCmdRRegRotate::updateStats() const {
  pimSim::get()->getStatsMgr()->recordCmd(getName(), m_device->getPerfEnergyModel()->getPerfEnergyForRowBitOp(m_cmdType,
 m_device->getResMgr()->getObjInfo(m_objId)));
  return PIM_OK;
}

PimStatus
pimCmdAnalogAAP::execute() {
  pimResMgr* resMgr = m_device->getResMgr();
  if (m_srcRows.empty() || !resMgr->isValidObjId(m_srcRows[0].first)) return PIM_ERR_OBJ_INVALID;
  
  const pimObjInfo& firstObj = resMgr->getObjInfo(m_srcRows[0].first);
  for (const auto& region : firstObj.getRegions()) {
    PimCoreId coreId = region.getCoreId();
    std::vector<unsigned> srcRows, destRows;
    for (auto p : m_srcRows) {
      pimObjInfo& srcObj = resMgr->getObjInfo(p.first);
      srcObj.syncToSimulatedMem(srcObj.getRegionsOfCore(coreId)[0]);
      srcRows.push_back(srcObj.getRegionsOfCore(coreId)[0].getRowIdx() + p.second);
    }
    for (auto p : m_destRows) {
      pimObjInfo& destObj = resMgr->getObjInfo(p.first);
      destRows.push_back(destObj.getRegionsOfCore(coreId)[0].getRowIdx() + p.second);
      destObj.markDirtyBitMapped();
    }
    m_device->getCore(coreId).aap(srcRows, destRows);
  }
  updateStats();
  return PIM_OK;
}
PimStatus
pimCmdAnalogAAP::updateStats() const {
  pimSim::get()->getStatsMgr()->recordCmd(getName(), m_device->getPerfEnergyModel()->getPerfEnergyForRowBitOp(m_cmdType,
 m_device->getResMgr()->getObjInfo(m_srcRows[0].first)));
  return PIM_OK;
}
