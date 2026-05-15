// File: pimSimConfig.cpp
// PIMeval Simulator - PIM Simulator Configurations
// Copyright (c) 2024 University of Virginia
// This file is licensed under the MIT License.
// See the LICENSE file in the root of this repository for more details.

#include "pimSimConfig.h"
#include "pimUtils.h"
#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <string>
#include <thread>
#include <map>

pimSimConfig::pimSimConfig()
  : m_isInit(false)
{
  registerParams();
  reset();
}

void
pimSimConfig::registerParams()
{
  m_registry[m_simConfigFile.getName()] = &m_simConfigFile;
  m_registry[m_memConfigFile.getName()] = &m_memConfigFile;
  m_registry[m_deviceType.getName()] = &m_deviceType;
  m_registry[m_simTarget.getName()] = &m_simTarget;
  m_registry[m_memoryProtocol.getName()] = &m_memoryProtocol;
  m_registry[m_numRanks.getName()] = &m_numRanks;
  m_registry[m_numBankPerRank.getName()] = &m_numBankPerRank;
  m_registry[m_numSubarrayPerBank.getName()] = &m_numSubarrayPerBank;
  m_registry[m_numRowPerSubarray.getName()] = &m_numRowPerSubarray;
  m_registry[m_numColPerSubarray.getName()] = &m_numColPerSubarray;
  m_registry[m_numThreads.getName()] = &m_numThreads;
  m_registry[m_bufferSize.getName()] = &m_bufferSize;
  m_registry[m_analysisMode.getName()] = &m_analysisMode;
  m_registry[m_debug.getName()] = &m_debug;
  m_registry[m_loadBalanced.getName()] = &m_loadBalanced;
  m_registry[m_eccEnabled.getName()] = &m_eccEnabled;
  m_registry[m_eccReadoutOnly.getName()] = &m_eccReadoutOnly;
  m_registry[m_eccGranularity.getName()] = &m_eccGranularity;
  m_registry[m_eccType.getName()] = &m_eccType;
  m_registry[m_eccLayers.getName()] = &m_eccLayers;
  m_registry[m_eccLatencyNs.getName()] = &m_eccLatencyNs;
  m_registry[m_eccEnergyPj.getName()] = &m_eccEnergyPj;
  m_registry[m_odeccEnabled.getName()] = &m_odeccEnabled;
  m_registry[m_odeccDataWidth.getName()] = &m_odeccDataWidth;
  m_registry[m_odeccParityWidth.getName()] = &m_odeccParityWidth;
  m_registry[m_odeccLatencyNs.getName()] = &m_odeccLatencyNs;
  m_registry[m_odeccEnergyPj.getName()] = &m_odeccEnergyPj;
  m_registry[m_scratchpadEnabled.getName()] = &m_scratchpadEnabled;
  m_registry[m_scratchpadSizeKb.getName()] = &m_scratchpadSizeKb;
  m_registry[m_scratchpadWordBits.getName()] = &m_scratchpadWordBits;
  m_registry[m_scratchpadEcc.getName()] = &m_scratchpadEcc;
  m_registry[m_scratchpadEccType.getName()] = &m_scratchpadEccType;
  m_registry[m_scratchpadEccLatencyNs.getName()] = &m_scratchpadEccLatencyNs;
  m_registry[m_scratchpadEccEnergyPj.getName()] = &m_scratchpadEccEnergyPj;
  m_registry[m_scratchpadEccOutputOnly.getName()] = &m_scratchpadEccOutputOnly;
  m_registry[m_dramBer.getName()] = &m_dramBer;
  m_registry[m_sramBer.getName()] = &m_sramBer;
  m_registry[m_preset.getName()] = &m_preset;
}

//! @brief  Init PIMeval simulation configuration parameters at device creation
bool
pimSimConfig::init(PimDeviceEnum deviceType,
    unsigned numRanks, unsigned numBankPerRank, unsigned numSubarrayPerBank,
    unsigned numRowPerSubarray, unsigned numColPerSubarray, unsigned bufferSize)
{
  reset();  // always reset before init
  return deriveConfig(deviceType, "",
                      numRanks, numBankPerRank, numSubarrayPerBank,
                      numRowPerSubarray, numColPerSubarray, bufferSize);
}

//! @brief  Init PIMeval simulation configuration parameters at device creation
bool
pimSimConfig::init(PimDeviceEnum deviceType, const std::string& configFilePath)
{
  reset();  // always reset before init
  return deriveConfig(deviceType, configFilePath);
}

bool
pimSimConfig::init(int* argc, char*** argv)
{
  reset();
  if (argc && argv) {
    int newArgc = 1;
    for (int i = 1; i < *argc; ++i) {
      std::string arg = (*argv)[i];
      if (arg.find("--pim-") == 0) {
        size_t pos = arg.find('=');
        if (pos != std::string::npos) {
          std::string key = arg.substr(6, pos - 6);
          std::string val = arg.substr(pos + 1);
          m_cliParams[key] = val;
        } else if (i + 1 < *argc) {
          std::string key = arg.substr(6);
          std::string val = (*argv)[++i];
          m_cliParams[key] = val;
        }
      } else {
        (*argv)[newArgc++] = (*argv)[i];
      }
    }
    *argc = newArgc;
  }
  return deriveConfig(PIM_FUNCTIONAL, "");
}

void
pimSimConfig::reset()
{
  for (auto const& [name, param] : m_registry) {
    param->reset();
  }
  m_eccStrategy = nullptr;
  m_odeccModel = nullptr;
  m_scratchpadModel = nullptr;
  // Preserve m_cliParams across reset — they represent explicit user intent
  // from pimInit(argc, argv) and should persist across device creation cycles.
  m_envParams.clear();
  m_cfgParams.clear();
  m_presetParams.clear();
  m_isInit = false;
}

std::string
pimSimConfig::getParam(const std::string& key, const std::string& envVar, bool& hasVal) const
{
  if (m_cliParams.find(key) != m_cliParams.end()) {
    hasVal = true;
    return m_cliParams.at(key);
  }
  if (m_cfgParams.find(key) != m_cfgParams.end()) {
    hasVal = true;
    return m_cfgParams.at(key);
  }
  if (m_envParams.find(envVar) != m_envParams.end()) {
    hasVal = true;
    return m_envParams.at(envVar);
  }
  if (m_presetParams.find(key) != m_presetParams.end()) {
    hasVal = true;
    return m_presetParams.at(key);
  }
  hasVal = false;
  return "";
}

//! @brief  Show all configuration parameters
void
pimSimConfig::show() const
{
  std::printf("----------------------------------------\n");
  if (!m_preset.getValue().empty()) {
    std::printf("PIM-Config: Memory-Tech Preset = %s\n", m_preset.getValue().c_str());
  }
  std::printf("PIM-Config: Debug Flags = 0x%x\n", m_debug.getValue());
  std::printf("PIM-Config: Simulator Config File: %s\n",
            (m_simConfigFile.getValue().empty() ? "<NONE>" : m_simConfigFile.getValue().c_str()));
  std::printf("PIM-Config: Memory Config File: %s\n",
            (m_memConfigFile.getValue().empty() ? "<DEFAULT>" : m_memConfigFile.getValue().c_str()));
  std::printf("PIM-Config: Memory Protocol: %s\n", pimUtils::pimProtocolEnumToStr(m_memoryProtocol.getValue()).c_str());

  std::printf("PIM-Config: Current Device = %s, Simulation Target = %s\n",
            pimUtils::pimDeviceEnumToStr(m_deviceType.getValue()).c_str(),
            pimUtils::pimDeviceEnumToStr(m_simTarget.getValue()).c_str());

  std::printf("PIM-Config: #ranks = %u, #banksPerRank = %u, #subarraysPerBank = %u, #rowsPerSubarray = %u, #colsPerSubarray = %u",
            m_numRanks.getValue(), m_numBankPerRank.getValue(), m_numSubarrayPerBank.getValue(),
            m_numRowPerSubarray.getValue(), m_numColPerSubarray.getValue());
  if (m_bufferSize.getValue() > 0) std::printf(", bufferSize = %uB", m_bufferSize.getValue());
  std::printf("\n");

  std::printf("PIM-Config: Number of Threads = %u\n", m_numThreads.getValue());
  std::printf("PIM-Config: Load Balanced = %s\n", m_loadBalanced.getValue() ? "1" : "0");
  std::printf("PIM-Config: On-Die ECC = %s", m_odeccEnabled.getValue() ? "enabled" : "disabled");
  if (m_odeccEnabled.getValue()) {
    std::printf(" (%u+%u SECDED, %.1fns, %.1fpJ)",
              m_odeccDataWidth.getValue(), m_odeccParityWidth.getValue(),
              m_odeccLatencyNs.getValue(), m_odeccEnergyPj.getValue());
  }
  std::printf("\n");
  std::printf("PIM-Config: Controller ECC = %s", m_eccEnabled.getValue() ? "enabled" : "disabled");
  if (m_eccEnabled.getValue()) {
    std::printf(" (%s, Granularity = %u, Layers = %u%s)",
              m_eccType.getValue().c_str(), m_eccGranularity.getValue(), m_eccLayers.getValue(),
              m_eccReadoutOnly.getValue() ? ", readout-only" : "");
  }
  std::printf("\n");
  if (m_odeccEnabled.getValue() || m_eccEnabled.getValue() || m_scratchpadEnabled.getValue()) {
    std::printf("PIM-Config: BER model — DRAM BER = %.2e, SRAM BER = %.2e\n",
                m_dramBer.getValue(), m_sramBer.getValue());
  }
  std::printf("PIM-Config: Scratchpad = %s", m_scratchpadEnabled.getValue() ? "enabled" : "disabled");
  if (m_scratchpadEnabled.getValue()) {
    std::printf(" (%uKB, %u-bit words, ECC=%s",
              m_scratchpadSizeKb.getValue(), m_scratchpadWordBits.getValue(),
              m_scratchpadEcc.getValue() ? m_scratchpadEccType.getValue().c_str() : "off");
    if (m_scratchpadEcc.getValue()) {
      std::printf(", %.2fns/word, %.2fpJ/word", m_scratchpadEccLatencyNs.getValue(), m_scratchpadEccEnergyPj.getValue());
      if (m_scratchpadEccOutputOnly.getValue()) {
        std::printf(", output-only");
      }
    }
    std::printf(")");
  }
  std::printf("\n");
  std::printf("----------------------------------------\n");
}

//! @brief  Derive PIMeval simulation configuration parameters with priority rules
bool
pimSimConfig::deriveConfig(PimDeviceEnum deviceType,
    const std::string& configFilePath,
    unsigned numRanks, unsigned numBankPerRank, unsigned numSubarrayPerBank,
    unsigned numRowPerSubarray, unsigned numColPerSubarray,
    unsigned bufferSize)
{
  bool ok = true;

  // Phase 1: Read all parameter sources
  m_envParams = readEnvVars();
  ok = ok & deriveSimConfigFile(configFilePath);
  m_cfgParams = readSimConfigFileParams();

  if (m_debug.getValue() & pimSimConfig::DEBUG_PARAMS) {
    for (const auto& [key, val] : m_envParams) {
      std::printf("PIM-Debug: Environment variable: %s = %s\n", key.c_str(), val.c_str());
    }
    for (const auto& [key, val] : m_cfgParams) {
      std::printf("PIM-Debug: Config file parameter: %s = %s\n", key.c_str(), val.c_str());
    }
  }

  // Phase 1.5: Resolve memory-technology preset (lowest-priority defaults, below env vars)
  // Priority: CLI > config > env > preset.  The preset fills m_presetParams which getParam()
  // returns only when no higher-priority source has the key.
  {
    static const std::map<std::string, std::map<std::string, std::string>> kPresets = {
      {"ddr5", {
        // JEDEC DDR5 DRAM: mandatory 128+8 on-die SECDED ECC per die
        {"odecc",              "1"},
        {"odecc_data_width",   "128"},
        {"odecc_parity_width", "8"},
        {"odecc_latency_ns",   "1.0"},
        {"odecc_energy_pj",    "0.5"},
        {"ecc",                "1"},
        {"ecc_type",           "secded"},
        {"ecc_granularity",    "64"},
        {"ecc_readout_only",   "0"},
        {"memory_protocol",    "DDR"},
      }},
      {"hbm3", {
        // JEDEC HBM3: 256+16 on-die SECDED, wider bus, slightly higher overhead
        {"odecc",              "1"},
        {"odecc_data_width",   "256"},
        {"odecc_parity_width", "16"},
        {"odecc_latency_ns",   "1.5"},
        {"odecc_energy_pj",    "0.8"},
        {"ecc",                "1"},
        {"ecc_type",           "secded"},
        {"ecc_granularity",    "64"},
        {"ecc_readout_only",   "0"},
        {"memory_protocol",    "HBM"},
      }},
      {"lpddr5", {
        // JEDEC LPDDR5: 128+8 on-die SECDED, lower voltage → lower energy
        {"odecc",              "1"},
        {"odecc_data_width",   "128"},
        {"odecc_parity_width", "8"},
        {"odecc_latency_ns",   "0.8"},
        {"odecc_energy_pj",    "0.35"},
        {"ecc",                "1"},
        {"ecc_type",           "secded"},
        {"ecc_granularity",    "64"},
        {"ecc_readout_only",   "0"},
        {"memory_protocol",    "LPDDR"},
      }},
    };

    // Peek at preset name from the highest-priority source that has it
    std::string presetName;
    if (m_cliParams.count("preset"))            presetName = m_cliParams.at("preset");
    else if (m_cfgParams.count("preset"))       presetName = m_cfgParams.at("preset");
    else if (m_envParams.count("PIMEVAL_PRESET")) presetName = m_envParams.at("PIMEVAL_PRESET");

    if (!presetName.empty()) {
      std::string lname = presetName;
      std::transform(lname.begin(), lname.end(), lname.begin(), ::tolower);
      auto it = kPresets.find(lname);
      if (it != kPresets.end()) {
        m_presetParams = it->second;
        m_preset.setValue(lname);
        std::printf("PIM-Config: Memory-tech preset '%s' applied as baseline defaults\n", lname.c_str());
      } else {
        std::printf("PIM-Warning: Unknown preset '%s'. Valid presets: ddr5, hbm3, lpddr5\n", presetName.c_str());
      }
    }
  }

  // Phase 2: Automated derivation from registry using hierarchical lookup (CLI > config > env > preset)
  for (auto const& [name, param] : m_registry) {
    bool hasVal = false;
    std::string envName = param->getEnvName();
    std::string val = getParam(name, envName, hasVal);
    if (hasVal) {
      if (m_debug.getValue() & pimSimConfig::DEBUG_PARAMS) {
        std::printf("PIM-Debug: Setting parameter '%s' to '%s'\n", name.c_str(), val.c_str());
      }
      if (!param->fromString(val)) {
        std::printf("PIM-Error: Failed to set parameter '%s' from value '%s'\n", name.c_str(), val.c_str());
        ok = false;
      } else {
        param->markSetExternally();
      }
    }
  }

  // Phase 3: Handle device type and simulation target (special-case logic)
  // Only apply the explicitly-passed deviceType if env/config/CLI did not already
  // set one (m_deviceType remains PIM_DEVICE_NONE after Phase 2 in that case).
  // This allows PIMEVAL_DEVICE_TYPE=PIM_DEVICE_BITSIMD_V (or similar) to redirect
  // any binary that hardcodes PIM_FUNCTIONAL to an analytical model without source changes.
  if (deviceType != PIM_DEVICE_NONE && m_deviceType.getValue() == PIM_DEVICE_NONE) {
    m_deviceType.setValue(deviceType);
  }

  // Derive simulation target with fallback chain
  if (m_deviceType.getValue() != PIM_FUNCTIONAL) {
    // Non-functional device: always use it as sim target
    m_simTarget.setValue(m_deviceType.getValue());
  } else {
    // For PIM_FUNCTIONAL: check if sim_target was explicitly set
    bool hasTarget = false;
    getParam("sim_target", "PIMEVAL_SIM_TARGET", hasTarget);
    if (!hasTarget) {
      // Check compile-time macro
      #if defined(PIM_SIM_TARGET)
      m_simTarget.setValue(PIM_SIM_TARGET);
      #endif
    }
  }
  if (m_simTarget.getValue() == PIM_DEVICE_NONE || m_simTarget.getValue() == PIM_FUNCTIONAL) {
    m_simTarget.setValue(DEFAULT_SIM_TARGET);
  }

  // Phase 4: API manual overrides for dimensions.
  // Skip if env/config/CLI already set the param (wasSetExternally), so that
  // PIMEVAL_NUM_RANKS=1 etc. can override the hardcoded defaults in util.h::createDevice.
  if (numRanks > 0          && !m_numRanks.wasSetExternally())          m_numRanks.setValue(numRanks);
  if (numBankPerRank > 0    && !m_numBankPerRank.wasSetExternally())    m_numBankPerRank.setValue(numBankPerRank);
  if (numSubarrayPerBank > 0 && !m_numSubarrayPerBank.wasSetExternally()) m_numSubarrayPerBank.setValue(numSubarrayPerBank);
  if (numRowPerSubarray > 0 && !m_numRowPerSubarray.wasSetExternally()) m_numRowPerSubarray.setValue(numRowPerSubarray);
  if (numColPerSubarray > 0 && !m_numColPerSubarray.wasSetExternally()) m_numColPerSubarray.setValue(numColPerSubarray);
  if (bufferSize > 0        && !m_bufferSize.wasSetExternally())        m_bufferSize.setValue(bufferSize);

  // Phase 5: Dimension validation
  if (m_numRanks.getValue() == 0 || m_numBankPerRank.getValue() == 0 ||
      m_numSubarrayPerBank.getValue() == 0 || m_numRowPerSubarray.getValue() == 0 ||
      m_numColPerSubarray.getValue() == 0) {
    std::printf("PIM-Error: Memory dimension parameter cannot be 0\n");
    ok = false;
  }
  if (m_simTarget.getValue() != PIM_DEVICE_AIM && m_bufferSize.getValue() > 0) {
    std::printf("PIM-Error: PIM Device %s does not support any on-chip buffer.\n",
              pimUtils::pimDeviceEnumToStr(m_simTarget.getValue()).c_str());
    ok = false;
  }

  // Phase 6: Derive memory protocol from memory config file
  if (!m_memConfigFile.getValue().empty()) {
    std::string memCfgPath = m_memConfigFile.getValue();
    if (!std::filesystem::exists(memCfgPath)) {
      // Try to find it in the same directory as the simulator config file
      std::string configDir = pimUtils::getDirectoryPath(m_simConfigFile.getValue());
      if (std::filesystem::exists(configDir + "/" + memCfgPath)) {
        memCfgPath = configDir + "/" + memCfgPath;
        m_memConfigFile.setValue(memCfgPath);
      } else {
        std::printf("PIM-Error: Cannot find memory config file: %s\n", memCfgPath.c_str());
        ok = false;
      }
    }
    if (ok && std::filesystem::exists(memCfgPath)) {
      std::map<std::string, std::string> memParams = pimUtils::readParamsFromConfigFile(memCfgPath);
      if (memParams.find("protocol") != memParams.end()) {
        std::string protocol = memParams.at("protocol");
        if (protocol == "DDR3" || protocol == "DDR4" || protocol == "DDR5") {
          m_memoryProtocol.setValue(PIM_DEVICE_PROTOCOL_DDR);
        } else if (protocol == "LPDDR3" || protocol == "LPDDR4") {
          m_memoryProtocol.setValue(PIM_DEVICE_PROTOCOL_LPDDR);
        } else if (protocol == "HBM" || protocol == "HBM2") {
          m_memoryProtocol.setValue(PIM_DEVICE_PROTOCOL_HBM);
        } else if (protocol == "GDDR5" || protocol == "GDDR5X" || protocol == "GDDR6") {
          m_memoryProtocol.setValue(PIM_DEVICE_PROTOCOL_GDDR);
        } else {
          std::printf("PIM-Error: Unknown protocol %s in memory config file: %s\n", protocol.c_str(), memCfgPath.c_str());
          ok = false;
        }
      } else {
        std::printf("PIM-Error: Missing protocol parameter in memory config file: %s\n", memCfgPath.c_str());
        ok = false;
      }
    }
  }

  // Phase 7: Thread count auto-detection
  unsigned hwThreads = std::thread::hardware_concurrency();
  if (m_debug.getValue() & pimSimConfig::DEBUG_PARAMS) {
    std::printf("PIM-Debug: Number of threads = %u, hardware concurrency = %u\n", m_numThreads.getValue(), hwThreads);
  }
  // If thread count was not explicitly set (still at default 1 and no source set it),
  // auto-detect from hardware. Also cap to hardware max.
  {
    bool hasThreads = false;
    getParam("num_threads", "PIMEVAL_NUM_THREADS", hasThreads);
    if (!hasThreads) {
      // Not explicitly set: use hardware concurrency
      m_numThreads.setValue(hwThreads > 0 ? hwThreads : 1);
    } else if (hwThreads > 0 && m_numThreads.getValue() > hwThreads) {
      m_numThreads.setValue(hwThreads);
    }
  }
  if (m_numThreads.getValue() < 1) {
    m_numThreads.setValue(1);
  }

  // Phase 8: Analysis mode warning
  if (m_analysisMode.getValue()) {
    std::printf("PIM-Warning: Running analysis only mode. Ignoring computation for fast performance and energy analysis.\n");
  }

  // Phase 9: ECC strategy creation
  if (m_eccEnabled.getValue()) {
    m_eccStrategy = pimEccFactory::create(m_eccType.getValue(), m_eccLayers.getValue(),
                                          m_eccLatencyNs.getValue(), m_eccEnergyPj.getValue());
    if (!m_eccStrategy) {
      std::printf("PIM-Error: Failed to create ECC strategy for type '%s'\n", m_eccType.getValue().c_str());
      m_eccEnabled.setValue(false);
    }
  }

  // Phase 10: On-Die ECC (ODECC) model creation
  // ODECC defaults to enabled when controller-level ECC is enabled (DDR5/HBM3 mandate it)
  {
    bool hasOdecc = false;
    getParam("odecc", "PIMEVAL_ODECC", hasOdecc);
    if (!hasOdecc && m_eccEnabled.getValue()) {
      m_odeccEnabled.setValue(true);
    }
  }
  if (m_odeccEnabled.getValue()) {
    m_odeccModel = std::make_unique<pimEccOnDie>(
      m_odeccDataWidth.getValue(), m_odeccParityWidth.getValue(),
      m_odeccLatencyNs.getValue(), m_odeccEnergyPj.getValue());
  }

  // Phase 11: Scratchpad / register-file ECC model creation
  if (m_scratchpadEnabled.getValue()) {
    m_scratchpadModel = std::make_unique<pimScratchpad>(
      m_scratchpadSizeKb.getValue(),
      m_scratchpadWordBits.getValue(),
      m_scratchpadEccType.getValue(),
      m_scratchpadEccLatencyNs.getValue(),
      m_scratchpadEccEnergyPj.getValue());
  }

  show();
  if (!ok) {
    std::printf("PIM-Error: Please resolve incorrect PIMeval configuration.\n");
  }
  m_isInit = true;
  return ok;
}

std::map<std::string, std::string>
pimSimConfig::readEnvVars() const
{
  std::vector<std::string> envVarList;
  for (auto const& [name, param] : m_registry) {
    envVarList.push_back(param->getEnvName());
  }
  return pimUtils::readParamsFromEnvVars(envVarList);
}

bool
pimSimConfig::deriveSimConfigFile(const std::string& configFilePath)
{
  bool hasVal = false;
  std::string val = getParam("sim_config", "PIMEVAL_SIM_CONFIG", hasVal);
  if (hasVal) m_simConfigFile.setValue(val);
  else if (!configFilePath.empty()) m_simConfigFile.setValue(configFilePath);

  if (!m_simConfigFile.getValue().empty() && !std::filesystem::exists(m_simConfigFile.getValue())) {
    std::printf("PIM-Error: Cannot find simulator config file: %s\n", m_simConfigFile.getValue().c_str());
    return false;
  }
  return true;
}

std::map<std::string, std::string>
pimSimConfig::readSimConfigFileParams() const
{
  std::map<std::string, std::string> params;
  if (!m_simConfigFile.getValue().empty()) {
    params = pimUtils::readParamsFromConfigFile(m_simConfigFile.getValue());
  }
  return params;
}
