// File: pimDevice.cpp
#include "pimDevice.h"
#include "pimResMgr.h"
#include "pimSim.h"
#include "libpimeval.h"
#include "pimUtils.h"
#include <cstdio>
#include <memory>
#include <cassert>
#ifdef DRAMSIM3_INTEG
#include "configuration.h"  // dramsim3::Config
#include "memory_system.h"  // dramsim3::MemorySystem::getConfig()
#include <filesystem>
#include <stdexcept>
#include <atomic>
#include <algorithm>
#include <cinttypes>
#endif

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
pimDevice::~pimDevice() {
#ifdef DRAMSIM3_INTEG
  delete m_deviceMemory;
  m_deviceMemory = nullptr;
#endif
}

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

#ifdef DRAMSIM3_INTEG
    initDramSim3();
#endif

    std::printf("PIM-Info: Created polymorphic PIM device with %u cores\n", m_numCores);
    return m_isValid;
}

#ifdef DRAMSIM3_INTEG
void pimDevice::initDramSim3() {
    // Select the DRAMSim3 config file based on the configured memory protocol.
    // DRAMSIM3_CONFIG_DIR is injected at compile time by the Makefile.
    const std::string configDir = DRAMSIM3_CONFIG_DIR;
    std::string configFile;

    switch (m_config.getMemoryProtocol()) {
        case PIM_DEVICE_PROTOCOL_HBM:
            configFile = configDir + "/HBM2_8Gb_x128.ini";
            break;
        case PIM_DEVICE_PROTOCOL_LPDDR:
            configFile = configDir + "/LPDDR4_8Gb_x16_2400.ini";
            break;
        case PIM_DEVICE_PROTOCOL_GDDR:
            configFile = configDir + "/GDDR6_8Gb_x16.ini";
            break;
        default: // DDR
            configFile = configDir + "/DDR4_8Gb_x8_2400.ini";
            break;
    }

    // DRAMSim3 writes per-run stats files to the output directory.
    // Use /tmp so they don't clutter the working directory.
    const std::string outputDir = "/tmp/pimeval_dramsim3";
    std::filesystem::create_directories(outputDir);

    try {
        m_deviceMemory = new dramsim3::PIMCPU(configFile, outputDir);
        std::printf("PIM-Info: DRAMSim3 initialised (config: %s)\n", configFile.c_str());
    } catch (const std::exception& e) {
        std::printf("PIM-Warning: DRAMSim3 init failed (%s); falling back to analytical model\n", e.what());
        delete m_deviceMemory; m_deviceMemory = nullptr;
    }
}

pimeval::perfEnergy pimDevice::dramsim3SimulateTransfer(uint64_t numBytes,
                                                        bool     isWrite) {
    if (!m_deviceMemory || numBytes == 0) return pimeval::perfEnergy();

    // ── Get config up front — needed to size transactions correctly ────────
    // Drive MemorySystem directly instead of through PIMCPU's submission queue.
    // PIMCPU::ClockTick() has a bug: the last item popped from pendingReq is
    // never passed to AddTransaction because the while-loop guard checks
    // !pendingReq.empty() AFTER the pop, causing the loop to exit with a
    // dangling transaction.  Bypassing the queue avoids that hang entirely.
    dramsim3::MemorySystem* mem = m_deviceMemory->getMemorySystem();
    dramsim3::Config*       cfg = mem->getConfig();

    // DRAMSim3 transaction granularity = request_size_bytes (bus_width/8 * BL).
    // This is 64 B for DDR4/HBM2, 128 B for LPDDR4, 256 B for GDDR6.
    // We MUST stride addresses by this amount: using a smaller stride (e.g.
    // CPU cache line = 64 B) maps multiple CPU transactions onto the SAME
    // DRAMSim3 transaction address, producing aliased duplicate requests and
    // overcounting READ/WRITE commands by request_size_bytes/64.
    uint64_t txSize = static_cast<uint64_t>(cfg->request_size_bytes);
    if (txSize < 64) txSize = 64;   // guard: never smaller than a CPU line

    // Use a monotonically advancing synthetic address so that consecutive
    // independent transfers (e.g. H2D followed by D2H) never alias the same
    // DRAMSim3 addresses.  If they did, the OPEN_PAGE row buffers left open
    // by the first transfer would give the second transfer free row-buffer hits,
    // systematically underestimating its latency.
    // Align to txSize boundary before use.
    uint64_t alignedBase = (m_dramNextAddr + txSize - 1) & ~(txSize - 1);

    // With m_config.getNumRanks() parallel PIMeval ranks, each rank handles
    // 1/N of the data.  Simulate ONE rank's workload via one DRAMSim3 channel;
    // since all ranks run in parallel, channel time = total transfer time.
    uint64_t numRanks  = (m_config.getNumRanks() > 0) ? m_config.getNumRanks() : 1;
    uint64_t totalTx   = (numBytes + txSize - 1) / txSize;
    uint64_t txPerRank = (totalTx + numRanks - 1) / numRanks;

    // Register completion callbacks so we can count finished transactions.
    std::atomic<uint64_t> completed{0};
    mem->RegisterCallbacks(
        [&completed](uint64_t) { ++completed; },   // read done
        [&completed](uint64_t) { ++completed; }    // write done
    );
    // Note: we do NOT call mem->ResetStats() here because DRAMSim3's epoch-JSON
    // output is written incrementally; calling Reset without a matching
    // PrintStats first would corrupt the output file on subsequent transfers.
    // Our energy computation is entirely from config parameters + our own
    // elapsed-cycle count, so we don't rely on DRAMSim3's internal stat counters.

    uint64_t nextAddr      = alignedBase;
    uint64_t submitted     = 0;
    uint64_t elapsedCycles = 0;

    // Safety cap: tRC is typically ≤ 100 cycles; allow 50 k cycles per transaction.
    static constexpr uint64_t kMaxCyclesPerTx = 50000;
    const uint64_t kMaxCycles = txPerRank * kMaxCyclesPerTx;

    while (completed < txPerRank && elapsedCycles < kMaxCycles) {
        // Feed transactions whenever the controller queue has room.
        while (submitted < txPerRank &&
               mem->WillAcceptTransaction(nextAddr, isWrite)) {
            mem->AddTransaction(nextAddr, isWrite);
            nextAddr += txSize;
            ++submitted;
        }
        mem->ClockTick();
        ++elapsedCycles;
    }

    if (completed < txPerRank) {
        std::printf("PIM-Warning: DRAMSim3 transfer timed out after %" PRIu64
                    " cycles (%" PRIu64 "/%" PRIu64 " tx completed); "
                    "falling back to analytical model\n",
                    elapsedCycles, completed.load(), txPerRank);
        mem->RegisterCallbacks([](uint64_t){}, [](uint64_t){});
        // Advance past the timed-out region so subsequent calls start fresh.
        m_dramNextAddr = alignedBase + txPerRank * txSize;
        return pimeval::perfEnergy();
    }

    // Restore no-op callbacks before returning.
    mem->RegisterCallbacks([](uint64_t){}, [](uint64_t){});

    // Advance the address cursor past the region we just simulated.
    // Next call will start at fresh rows, preventing row-buffer state bleed.
    m_dramNextAddr = alignedBase + txPerRank * txSize;

    // Wrap around if we approach the end of the DRAMSim3 address space
    // (channel_size is in MB; leave a generous margin).
    const uint64_t kAddrSpaceBytes = static_cast<uint64_t>(cfg->channel_size) * 1024ULL * 1024ULL;
    if (kAddrSpaceBytes > 0 && m_dramNextAddr >= kAddrSpaceBytes / 2) {
        m_dramNextAddr = 0;
    }

    // ── Timing ────────────────────────────────────────────────────────────
    double tCK_ns    = m_deviceMemory->GetTCK();
    double msRuntime = static_cast<double>(elapsedCycles) * tCK_ns / 1.0e6;

    // ── Energy from DRAMSim3 IDD/VDD parameters ───────────────────────────
    // All *_energy_inc fields are in V×mA×cycles (= mW×cycles).
    // Multiply by tCK_ns [ns/cycle] to get pJ.
    //
    //   act_energy_inc   — net activation energy per ACT command
    //                      = VDD*(IDD0*tRC - IDD3N*tRAS - IDD2N*tRP)*devices_per_rank
    //   read/write_energy_inc — incremental I/O energy per burst
    //                      = VDD*(IDD4R/W - IDD3N)*burst_cycles*devices_per_rank
    //   act_stb_energy_inc — active-row standby power  [mW per rank, not mW*cycles]
    //                      = VDD*IDD3N*devices_per_rank
    //   pre_stb_energy_inc — precharged standby power  [mW per rank, not mW*cycles]
    //                      = VDD*IDD2N*devices_per_rank

    // ── ACT command count ─────────────────────────────────────────────────
    // All configs use OPEN_PAGE and column bits at the LSB (after shift_bits).
    // Consecutive txSize-byte transactions increment only the column field, so
    // (columns / BL) consecutive transactions share one open row → one ACT.
    uint64_t txsPerRow = (cfg->BL > 0)
                         ? static_cast<uint64_t>(cfg->columns) / cfg->BL
                         : 1;
    if (txsPerRow == 0) txsPerRow = 1;
    uint64_t numACTs = (txPerRank + txsPerRow - 1) / txsPerRow;

    // Command energy — independent of cfg->ranks (total ACTs and bursts are
    // distributed across internal ranks, but the counts themselves don't change).
    double actEnergyPJ = static_cast<double>(numACTs) * cfg->act_energy_inc * tCK_ns;
    double ioEnergyPJ  = static_cast<double>(txPerRank)
                         * (isWrite ? cfg->write_energy_inc : cfg->read_energy_inc)
                         * tCK_ns;

    // Background energy — act_stb_energy_inc is the standby power for ONE rank.
    // DRAMSim3 may model cfg->ranks internal ranks simultaneously (e.g. DDR4 and
    // LPDDR4 get ranks=2 from channel_size/megs_per_rank).  All cfg->ranks ranks
    // consume standby power for the full elapsedCycles, so multiply by cfg->ranks.
    // With OPEN_PAGE, rows stay open for the whole burst → activeCycles = elapsedCycles.
    double activeCycles = static_cast<double>(elapsedCycles);
    double actStbPJ = activeCycles * cfg->ranks * cfg->act_stb_energy_inc * tCK_ns;

    // ── Scale to all PIMeval ranks ────────────────────────────────────────
    // rankEnergyPJ represents the energy of one DRAMSim3 channel (= one PIMeval
    // rank's share of the device).  Multiply by the number of PIMeval ranks.
    double rankEnergyPJ  = actEnergyPJ + ioEnergyPJ + actStbPJ;
    double totalEnergyMJ = rankEnergyPJ * static_cast<double>(m_config.getNumRanks()) / 1.0e9;

    return pimeval::perfEnergy(msRuntime, totalEnergyMJ, 0.0, 0.0, 0.0, 0);
}
#endif

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
