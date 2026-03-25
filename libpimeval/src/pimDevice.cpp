// File: pimDevice.cpp
#include "pimDevice.h"
#include "pimResMgr.h"
#include "pimSim.h"
#include "libpimeval.h"
#include "pimUtils.h"
#include <cstdio>
#include <memory>
#include <cassert>

/**
 * @brief Subarray-level Bit-Serial PIM (Vertical Layout)
 */
class pimDeviceBitSIMD_V : public pimDevice {
public:
    pimDeviceBitSIMD_V(const pimSimConfig& config, const pimParamsDram& paramsDram) : pimDevice(config, paramsDram) {}
    bool isVLayoutDevice() const override { return true; }
    bool isHLayoutDevice() const override { return false; }
    bool init() override {
        unsigned numRanks = m_config.getNumRanks();
        unsigned numBankPerRank = m_config.getNumBankPerRank();
        unsigned numSubarrayPerBank = m_config.getNumSubarrayPerBank();
        unsigned numRowsPerSubarray = m_config.getNumRowPerSubarray();
        unsigned numCols = m_config.getNumColPerSubarray();
        if (numSubarrayPerBank % 2 != 0) {
            std::printf("PIM-Error: Please config even number of subarrays in each bank\n");
            return false;
        }
        unsigned numCores = numRanks * numBankPerRank * (numSubarrayPerBank / 2);
        unsigned totalRows = numCores * numRowsPerSubarray * 2;
        return commonInit(numCores, totalRows, numCols, m_config.getBufferSize());
    }
};

/**
 * @brief SIMDRAM (Vertical Layout)
 */
class pimDeviceSimdram : public pimDevice {
public:
    pimDeviceSimdram(const pimSimConfig& config, const pimParamsDram& paramsDram) : pimDevice(config, paramsDram) {}
    bool isVLayoutDevice() const override { return true; }
    bool isHLayoutDevice() const override { return false; }
    bool init() override {
        unsigned numRanks = m_config.getNumRanks();
        unsigned numBankPerRank = m_config.getNumBankPerRank();
        unsigned numSubarrayPerBank = m_config.getNumSubarrayPerBank();
        unsigned numRowsPerSubarray = m_config.getNumRowPerSubarray();
        unsigned numCols = m_config.getNumColPerSubarray();
        unsigned numCores = numRanks * numBankPerRank * numSubarrayPerBank;
        unsigned totalRows = numCores * numRowsPerSubarray;
        return commonInit(numCores, totalRows, numCols, m_config.getBufferSize());
    }
};

/**
 * @brief Subarray-level Bit-Serial PIM (Horizontal Layout)
 */
class pimDeviceBitSIMD_H : public pimDevice {
public:
    pimDeviceBitSIMD_H(const pimSimConfig& config, const pimParamsDram& paramsDram) : pimDevice(config, paramsDram) {}
    bool isVLayoutDevice() const override { return false; }
    bool isHLayoutDevice() const override { return true; }
    bool init() override {
        unsigned numRanks = m_config.getNumRanks();
        unsigned numBankPerRank = m_config.getNumBankPerRank();
        unsigned numSubarrayPerBank = m_config.getNumSubarrayPerBank();
        unsigned numRowsPerSubarray = m_config.getNumRowPerSubarray();
        unsigned numCols = m_config.getNumColPerSubarray();
        unsigned numCores = numRanks * numBankPerRank * numSubarrayPerBank;
        unsigned totalRows = numCores * numRowsPerSubarray;
        return commonInit(numCores, totalRows, numCols, m_config.getBufferSize());
    }
};

/**
 * @brief Bank-Level PIM (Fulcrum, Bank-Level, etc.)
 */
class pimDeviceBankLevel : public pimDevice {
public:
    pimDeviceBankLevel(const pimSimConfig& config, const pimParamsDram& paramsDram) : pimDevice(config, paramsDram) {}
    bool isVLayoutDevice() const override { return false; }
    bool isHLayoutDevice() const override { return true; }
    bool init() override {
        unsigned numRanks = m_config.getNumRanks();
        unsigned numBankPerRank = m_config.getNumBankPerRank();
        unsigned numSubarrayPerBank = m_config.getNumSubarrayPerBank();
        unsigned numRowsPerSubarray = m_config.getNumRowPerSubarray();
        unsigned numCols = m_config.getNumColPerSubarray();
        unsigned numCores = numRanks * numBankPerRank;
        unsigned totalRows = numCores * numRowsPerSubarray * numSubarrayPerBank;
        return commonInit(numCores, totalRows, numCols, m_config.getBufferSize());
    }
};

/**
 * @brief AiM (Bank-Level with 16-bit float/int support)
 */
class pimDeviceAim : public pimDevice {
public:
    pimDeviceAim(const pimSimConfig& config, const pimParamsDram& paramsDram) : pimDevice(config, paramsDram) {}
    bool isVLayoutDevice() const override { return false; }
    bool isHLayoutDevice() const override { return true; }
    bool init() override {
        unsigned numRanks = m_config.getNumRanks();
        unsigned numBankPerRank = m_config.getNumBankPerRank();
        unsigned numSubarrayPerBank = m_config.getNumSubarrayPerBank();
        unsigned numRowsPerSubarray = m_config.getNumRowPerSubarray();
        unsigned numCols = m_config.getNumColPerSubarray();
        unsigned numCores = numRanks * numBankPerRank;
        unsigned totalRows = numCores * numRowsPerSubarray * numSubarrayPerBank;
        return commonInit(numCores, totalRows, numCols, m_config.getBufferSize());
    }
};

/**
 * @brief Aquabolt (Bank-Level)
 */
class pimDeviceAquabolt : public pimDevice {
public:
    pimDeviceAquabolt(const pimSimConfig& config, const pimParamsDram& paramsDram) : pimDevice(config, paramsDram) {}
    bool isVLayoutDevice() const override { return false; }
    bool isHLayoutDevice() const override { return true; }
    bool init() override {
        unsigned numRanks = m_config.getNumRanks();
        unsigned numBankPerRank = m_config.getNumBankPerRank();
        unsigned numSubarrayPerBank = m_config.getNumSubarrayPerBank();
        unsigned numRowsPerSubarray = m_config.getNumRowPerSubarray();
        unsigned numCols = m_config.getNumColPerSubarray();
        unsigned numCores = numRanks * numBankPerRank;
        unsigned totalRows = numCores * numRowsPerSubarray * numSubarrayPerBank;
        return commonInit(numCores, totalRows, numCols, m_config.getBufferSize());
    }
};

/**
 * @brief Factory Implementation
 */
std::unique_ptr<pimDevice> pimDeviceFactory::create(const pimSimConfig& config, const pimParamsDram& paramsDram) {
    PimDeviceEnum target = config.getSimTarget();
    std::unique_ptr<pimDevice> device;

    switch (target) {
        case PIM_DEVICE_BITSIMD_V:
        case PIM_DEVICE_BITSIMD_V_AP:
        case PIM_DEVICE_BITSIMD_V_MAJ:
        case PIM_DEVICE_BITSIMD_V_NAND:
            device = std::make_unique<pimDeviceBitSIMD_V>(config, paramsDram);
            break;
        case PIM_DEVICE_SIMDRAM:
            device = std::make_unique<pimDeviceSimdram>(config, paramsDram);
            break;
        case PIM_DEVICE_BITSIMD_H:
            device = std::make_unique<pimDeviceBitSIMD_H>(config, paramsDram);
            break;
        case PIM_DEVICE_FULCRUM:
        case PIM_DEVICE_BANK_LEVEL:
            device = std::make_unique<pimDeviceBankLevel>(config, paramsDram);
            break;
        case PIM_DEVICE_AIM:
            device = std::make_unique<pimDeviceAim>(config, paramsDram);
            break;
        case PIM_DEVICE_AQUABOLT:
            device = std::make_unique<pimDeviceAquabolt>(config, paramsDram);
            break;
        default:
            device = std::make_unique<pimDeviceBitSIMD_V>(config, paramsDram);
            break;
    }

    if (device && !device->init()) {
        return nullptr;
    }
    return device;
}

// Implement base class methods moved from old pimDevice
pimDevice::~pimDevice() {}

bool pimDevice::commonInit(unsigned numCores, unsigned numRows, unsigned numCols, unsigned bufferSize) {
    m_numCores = numCores;
    m_numRows = numRows;
    m_numCols = numCols;
    m_bufferSize = bufferSize;
    m_isValid = (m_numCores > 0 && m_numRows > 0 && m_numCols > 0);

    pimPerfEnergyModelParams params(getSimTarget(), m_config.getNumRanks(), m_paramsDram);
    m_perfEnergyModel = pimPerfEnergyFactory::createPerfEnergyModel(params);

    m_resMgr = std::make_unique<pimResMgr>(this);

    if (getSimTarget() != PIM_DEVICE_NONE && getSimTarget() != PIM_FUNCTIONAL) {
        m_cores.resize(m_numCores, pimCore(m_numRows, m_numCols));
    }

    std::printf("PIM-Info: Created polymorphic PIM device with %u cores\n", m_numCores);
    return m_isValid;
}

// Resource Manager Pass-throughs
PimObjId pimDevice::pimAlloc(PimAllocEnum allocType, uint64_t numElements, PimDataType dataType) { return m_resMgr->pimAlloc(allocType, numElements, dataType); }
PimObjId pimDevice::pimAllocAssociated(PimObjId assocId, PimDataType dataType) { return m_resMgr->pimAllocAssociated(assocId, dataType); }
PimObjId pimDevice::pimAllocBuffer(uint32_t numElements, PimDataType dataType) { return m_resMgr->pimAllocBuffer(numElements, dataType); }
PimStatus pimDevice::pimFree(PimObjId obj) { return m_resMgr->pimFree(obj); }
PimStatus pimDevice::pimInjectError(PimObjId objId, uint64_t elemIdx, unsigned bitIdx) {
    if (!m_resMgr->isValidObjId(objId)) return PIM_ERR_OBJ_INVALID;
    pimObjInfo& obj = m_resMgr->getObjInfo(objId);
    
    // 1. Sync simulated bit-mapped memory to functional host memory (decodes ECC)
    obj.syncFromSimulatedMem();
    
    // 2. Inject error into functional host memory
    uint64_t bits = obj.getElementBits(elemIdx);
    bits ^= (1ULL << bitIdx);
    obj.setElementBits(elemIdx, bits);
    
    // 3. Sync back to simulated bit-mapped memory (encodes ECC)
    obj.syncToSimulatedMem();
    
    return PIM_OK;
}
PimStatus pimDevice::pimInjectBurstError(PimObjId objId, uint64_t elemIdx, unsigned bitIdx, unsigned length) {
    if (!m_resMgr->isValidObjId(objId)) return PIM_ERR_OBJ_INVALID;
    pimObjInfo& obj = m_resMgr->getObjInfo(objId);
    
    // 1. Sync simulated bit-mapped memory to functional host memory (decodes ECC)
    obj.syncFromSimulatedMem();
    
    // 2. Inject burst error into functional host memory
    uint64_t bits = obj.getElementBits(elemIdx);
    uint64_t mask = ((1ULL << length) - 1) << bitIdx;
    bits ^= mask;
    obj.setElementBits(elemIdx, bits);
    
    // 3. Sync back to simulated bit-mapped memory (encodes ECC)
    obj.syncToSimulatedMem();
    
    return PIM_OK;
}
PimObjId pimDevice::pimCreateRangedRef(PimObjId refId, uint64_t idxBegin, uint64_t idxEnd) { return m_resMgr->pimCreateRangedRef(refId, idxBegin, idxEnd); }
PimObjId pimDevice::pimCreateDualContactRef(PimObjId refId) { return m_resMgr->pimCreateDualContactRef(refId); }

PimStatus pimDevice::pimCopyMainToDevice(void* src, PimObjId dest, uint64_t idxBegin, uint64_t idxEnd) {
    PimCopyEnum copyType = m_resMgr->isHLayoutObj(dest) ? PIM_COPY_H : PIM_COPY_V;
    return pimCopyMainToDeviceWithType(copyType, src, dest, idxBegin, idxEnd);
}
PimStatus pimDevice::pimCopyDeviceToMain(PimObjId src, void* dest, uint64_t idxBegin, uint64_t idxEnd) {
    PimCopyEnum copyType = m_resMgr->isHLayoutObj(src) ? PIM_COPY_H : PIM_COPY_V;
    return pimCopyDeviceToMainWithType(copyType, src, dest, idxBegin, idxEnd);
}
PimStatus pimDevice::pimCopyMainToDeviceWithType(PimCopyEnum copyType, void* src, PimObjId dest, uint64_t idxBegin, uint64_t idxEnd) {
    std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdCopy>(PimCmdEnum::COPY_H2D, copyType, src, dest, idxBegin, idxEnd);
    return executeCmd(std::move(cmd));
}
PimStatus pimDevice::pimCopyDeviceToMainWithType(PimCopyEnum copyType, PimObjId src, void* dest, uint64_t idxBegin, uint64_t idxEnd) {
    std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdCopy>(PimCmdEnum::COPY_D2H, copyType, src, dest, idxBegin, idxEnd);
    return executeCmd(std::move(cmd));
}
PimStatus pimDevice::pimCopyDeviceToDevice(PimObjId src, PimObjId dest, uint64_t idxBegin, uint64_t idxEnd) {
    const pimObjInfo& obj = m_resMgr->getObjInfo(src);
    PimCopyEnum copyType = obj.isVLayout() ? PIM_COPY_V : PIM_COPY_H;
    std::unique_ptr<pimCmd> cmd = std::make_unique<pimCmdCopy>(PimCmdEnum::COPY_D2D, copyType, src, dest, idxBegin, idxEnd);
    return executeCmd(std::move(cmd));
}
PimStatus pimDevice::executeCmd(std::unique_ptr<pimCmd> cmd) {
    cmd->setDevice(this);
    PimStatus status = PIM_OK;
    
    // 1. Pre-execution synchronization: Sync functional DIRTY data to bit-mapped for PIM ops
    // Functional ops (ADD, MUL, etc.) and low-level micro-ops (ROW_R, ROW_AP, etc.)
    // need operands in simulated memory.
    if (getDeviceType() != PIM_FUNCTIONAL) {
      // Note: For now, we sync the entire object. 
      // Future optimization: sync only accessed regions.
      switch (cmd->getCmdType()) {
        case PimCmdEnum::COPY_H2D:
        case PimCmdEnum::COPY_D2H:
        case PimCmdEnum::COPY_D2D:
          // Copy command handles its own sync
          break;
        default:
          // For most commands, we assume objects are operands
          // This is a conservative sync for all objects involved in the command
          // The command implementation should ideally list its objects.
          // For now, let's let the command execute() handle it or use a heuristic.
          break;
      }
    }

    status = cmd->execute();
    return status;
}

PimStatus pimDevice::readBitMappedData(PimObjId objId, uint64_t elemIdx, uint64_t& bits)
{
  bits = 0;
  if (!m_resMgr->isValidObjId(objId)) return PIM_ERR_OBJ_INVALID;
  const pimObjInfo& obj = m_resMgr->getObjInfo(objId);
  unsigned numBits = obj.getBitsPerElement(PimBitWidth::SIM);
  
  for (const auto& region : obj.getRegions()) {
    if (elemIdx >= region.getElemIdxBegin() && elemIdx < region.getElemIdxEnd()) {
      PimCoreId coreId = region.getCoreId();
      unsigned rowIdxBase = region.getRowIdx();
      unsigned colIdxBase = region.getColIdx();
      uint64_t idxInRegion = elemIdx - region.getElemIdxBegin();
      
      for (unsigned b = 0; b < numBits; ++b) {
        bool bit = false;
        if (obj.isVLayout()) {
          bit = m_cores[coreId].getBit(rowIdxBase + b, colIdxBase + idxInRegion);
        } else {
          bit = m_cores[coreId].getBit(rowIdxBase, colIdxBase + idxInRegion * numBits + b);
        }
        if (bit) bits |= (1ULL << b);
      }
      return PIM_OK;
    }
  }
  return PIM_ERR_INVALID_ARG;
}

PimStatus pimDevice::writeBitMappedData(PimObjId objId, uint64_t elemIdx, uint64_t bits)
{
  if (!m_resMgr->isValidObjId(objId)) return PIM_ERR_OBJ_INVALID;
  const pimObjInfo& obj = m_resMgr->getObjInfo(objId);
  unsigned numBits = obj.getBitsPerElement(PimBitWidth::SIM);
  
  for (const auto& region : obj.getRegions()) {
    if (elemIdx >= region.getElemIdxBegin() && elemIdx < region.getElemIdxEnd()) {
      PimCoreId coreId = region.getCoreId();
      unsigned rowIdxBase = region.getRowIdx();
      unsigned colIdxBase = region.getColIdx();
      uint64_t idxInRegion = elemIdx - region.getElemIdxBegin();
      
      for (unsigned b = 0; b < numBits; ++b) {
        bool bit = (bits >> b) & 1;
        if (obj.isVLayout()) {
          m_cores[coreId].setBit(rowIdxBase + b, colIdxBase + idxInRegion, bit);
        } else {
          m_cores[coreId].setBit(rowIdxBase, colIdxBase + idxInRegion * numBits + b, bit);
        }
      }
      return PIM_OK;
    }
  }
  return PIM_ERR_INVALID_ARG;
}
