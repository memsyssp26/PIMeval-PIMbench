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
#include <unordered_map>


//! @brief  Init PIMeval simulation configuration parameters at device creation
bool
pimSimConfig::init(PimDeviceEnum deviceType,
    unsigned numRanks, unsigned numBankPerRank, unsigned numSubarrayPerBank,
    unsigned numRowPerSubarray, unsigned numColPerSubarray, unsigned bufferSize)
{
  std::fprintf(stderr, "PIM-Debug: pimSimConfig::init start\n");
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

//! @brief  Show all configuration parameters
void
pimSimConfig::show() const
{
  std::printf("----------------------------------------\n");
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
            m_numRanks.getValue(), m_numBankPerRank.getValue(), m_numSubarrayPerBank.getValue(), m_numRowPerSubarray.getValue(), m_numColPerSubarray.getValue());
  if (m_bufferSize.getValue() > 0) std::printf(", bufferSize = %uB", m_bufferSize.getValue());
  std::printf("\n");

  std::printf("PIM-Config: Number of Threads = %u\n", m_numThreads.getValue());
  std::printf("PIM-Config: Load Balanced = %s\n", m_loadBalanced.getValue() ? "1" : "0");
  std::printf("PIM-Config: ECC Enabled = %s, Granularity = %u, Type = %s, Layers = %u\n", 
            m_eccEnabled.getValue() ? "1" : "0", m_eccGranularity.getValue(), m_eccType.getValue().c_str(), m_eccLayers.getValue());
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

  // Derive debug flags first
  ok = ok & deriveDebug();

  // Read environment variables
  m_envParams = readEnvVars();

  // Derive simulator config file
  ok = ok & deriveSimConfigFile(configFilePath);

  // Read config file parameters
  m_cfgParams = readSimConfigFileParams();

  // Derive other configuration parameters in order
  ok = ok & deriveDeviceType(deviceType);
  ok = ok & deriveSimTarget();
  ok = ok & deriveMemConfigFile();
  ok = ok & deriveDimensions(numRanks, numBankPerRank, numSubarrayPerBank, numRowPerSubarray, numColPerSubarray, bufferSize);
  ok = ok & deriveNumThreads();
  ok = ok & deriveMiscEnvVars();
  ok = ok & deriveLoadBalance();
  ok = ok & deriveEcc();

  // Show summary
  show();
  if (!ok) {
    std::printf("PIM-Error: Please resolve incorrect PIMeval configuration.\n");
  }
  m_isInit = true;
  return ok;
}

//! @brief  Derive Params: Debug Flags
bool
pimSimConfig::deriveDebug()
{
  unsigned debug = 0;
  std::string envVal;
  bool hasEnv = pimUtils::getEnvVar(m_envVarDebug, envVal);
  if (hasEnv && !envVal.empty()) {
    bool ok = pimUtils::convertStringToUnsigned(envVal, debug);
    if (!ok) {
      std::printf("PIM-Error: Incorrect environment variable: %s = %s\n", m_envVarDebug.c_str(), envVal.c_str());
      return false;
    }
  }
  m_debug.setValue(debug);
  return true;
}

//! @brief  Read config env vars
std::unordered_map<std::string, std::string>
pimSimConfig::readEnvVars() const
{
  std::unordered_map<std::string, std::string> params;
  params = pimUtils::readParamsFromEnvVars(m_envVarList);

  if (m_debug.getValue() & pimSimConfig::DEBUG_PARAMS) {
    for (const auto& [key, val] : params) {
      std::printf("PIM-Debug: Environment variable: %s = %s\n", key.c_str(), val.c_str());
    }
  }

  return params;
}

//! @brief  Derive Params: Simulator Configuration File
bool
pimSimConfig::deriveSimConfigFile(const std::string& configFilePath)
{
  m_simConfigFile.reset();

  // If a config file is specified through APIs, use it. Otherwise check env var
  if (!configFilePath.empty()) {
    m_simConfigFile.setValue(configFilePath);
  } else if (m_envParams.find(m_envVarSimConfig) != m_envParams.end()) {
    m_simConfigFile.setValue(m_envParams.at(m_envVarSimConfig));
  }
  if (!m_simConfigFile.getValue().empty()) {
    if (!std::filesystem::exists(m_simConfigFile.getValue())) {
      std::printf("PIM-Error: Cannot find simulator config file: %s\n", m_simConfigFile.getValue().c_str());
      return false;
    }
  }
  return true;
}

//! @brief  Read config file params
std::unordered_map<std::string, std::string>
pimSimConfig::readSimConfigFileParams() const
{
  std::unordered_map<std::string, std::string> params;
  if (!m_simConfigFile.getValue().empty()) {
    params = pimUtils::readParamsFromConfigFile(m_simConfigFile.getValue());

    if (m_debug.getValue() & pimSimConfig::DEBUG_PARAMS) {
      for (const auto& [key, val] : params) {
        std::printf("PIM-Debug: Simulator config file parameter: %s = %s\n", key.c_str(), val.c_str());
      }
    }
  }
  return params;
}

//! @brief  Derive Params: Device Type
bool
pimSimConfig::deriveDeviceType(PimDeviceEnum deviceType)
{
  m_deviceType.setValue(deviceType);
  return true;
}

//! @brief  Derive Params: Simulation Target
bool
pimSimConfig::deriveSimTarget()
{
  // If device type is not functional, always use it as simulation target
  m_simTarget.setValue(m_deviceType.getValue());

  if (m_deviceType.getValue() == PIM_FUNCTIONAL) {
    bool hasVal = false;
    std::string val;
    // Check simulator config file
    if (m_simTarget.getValue() == PIM_DEVICE_NONE || m_simTarget.getValue() == PIM_FUNCTIONAL) {
      val = pimUtils::getOptionalParam(m_cfgParams, m_cfgVarSimTarget, hasVal);
      if (hasVal) {
        m_simTarget.setValue(pimUtils::strToPimDeviceEnum(val));
        if (m_simTarget.getValue() == PIM_DEVICE_NONE) {
          std::printf("PIM-Error: Incorrect config file parameter: %s=%s\n", m_cfgVarSimTarget.c_str(), val.c_str());
          return false;
        }
      }
    }
    // Check env var
    if (m_simTarget.getValue() == PIM_DEVICE_NONE || m_simTarget.getValue() == PIM_FUNCTIONAL) {
      val = pimUtils::getOptionalParam(m_envParams, m_envVarSimTarget, hasVal);
      if (hasVal) {
        m_simTarget.setValue(pimUtils::strToPimDeviceEnum(val));
        if (m_simTarget.getValue() == PIM_DEVICE_NONE) {
          std::printf("PIM-Error: Incorrect environment variable: %s=%s\n", m_envVarSimTarget.c_str(), val.c_str());
          return false;
        }
      }
    }
    // Check macro
    if (m_simTarget.getValue() == PIM_DEVICE_NONE || m_simTarget.getValue() == PIM_FUNCTIONAL) {
      // from 'make PIM_SIM_TARGET=...'
      #if defined(PIM_SIM_TARGET)
      m_simTarget.setValue(PIM_SIM_TARGET);
      #endif
    }
    // Use default
    if (m_simTarget.getValue() == PIM_DEVICE_NONE || m_simTarget.getValue() == PIM_FUNCTIONAL) {
      m_simTarget.setValue(DEFAULT_SIM_TARGET);
    }
  }

  return true;
}

//! @brief  Derive Params: Memory Config File
bool
pimSimConfig::deriveMemConfigFile()
{
  m_memConfigFile.reset();

  // Read config file and env
  if (m_cfgParams.find(m_cfgVarMemConfig) != m_cfgParams.end()) {
    m_memConfigFile.setValue(m_cfgParams.at(m_cfgVarMemConfig));
  } else if (m_envParams.find(m_envVarMemConfig) != m_envParams.end()) {
    m_memConfigFile.setValue(m_envParams.at(m_envVarMemConfig));
  }
  if (!m_memConfigFile.getValue().empty()) {
    if (!std::filesystem::exists(m_memConfigFile.getValue())) {
      // Try to find it in the same directory of sim config file
      std::string configFilePath = pimUtils::getDirectoryPath(m_simConfigFile.getValue());
      if (std::filesystem::exists(configFilePath + "/" + m_memConfigFile.getValue())) {
        m_memConfigFile.setValue(configFilePath + "/" + m_memConfigFile.getValue());
      } else {
        std::printf("PIM-Error: Cannot find memory config file: %s\n", m_memConfigFile.getValue().c_str());
        return false;
      }
    }

    // Determine memory protocol from memory config file. This is not sim config file.
    std::unordered_map<std::string, std::string> memParams = pimUtils::readParamsFromConfigFile(m_memConfigFile.getValue());
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
        std::printf("PIM-Error: Unknown protocol %s in memory config file: %s\n", protocol.c_str(), m_memConfigFile.getValue().c_str());
        return false;
      }
    } else {
      std::printf("PIM-Error: Missing protocol parameter in memory config file: %s\n", m_memConfigFile.getValue().c_str());
      return false;
    }
  }
  return true;
}

//! @brief  Derive Params: A Specific PIM Memory Dimension
bool
pimSimConfig::deriveDimension(const std::string& cfgVar, const std::string& envVar, const unsigned apiVal, const unsigned defVal, pimConfigParam<unsigned>& retVal)
{
  bool hasVal = false;
  std::string valStr;

  // Check config file. Zero will be ignored
  valStr = pimUtils::getOptionalParam(m_cfgParams, cfgVar, hasVal);
  if (hasVal) {
    unsigned val = 0;
    bool ok = pimUtils::convertStringToUnsigned(valStr, val);
    if (!ok || val == 0) {
      std::printf("PIM-Error: Incorrect config file parameter: %s=%s\n", cfgVar.c_str(), valStr.c_str());
      return false;
    }
    if (val > 0) {
      retVal.setValue(val);
      return true;
    }
  }

  // Check env var. Zero will be ignored
  valStr = pimUtils::getOptionalParam(m_envParams, envVar, hasVal);
  if (hasVal) {
    unsigned val = 0;
    bool ok = pimUtils::convertStringToUnsigned(valStr, val);
    if (!ok) {
      std::printf("PIM-Error: Incorrect environment variable: %s=%s\n", envVar.c_str(), valStr.c_str());
      return false;
    }
    if (val > 0) {
      retVal.setValue(val);
      return true;
    }
  }

  // Check value from APIs
  retVal.setValue((apiVal > 0) ? apiVal : defVal);
  return true;
}

//! @brief  Derive Params: PIM Memory Dimensions
bool
pimSimConfig::deriveDimensions(unsigned numRanks, unsigned numBankPerRank, unsigned numSubarrayPerBank, unsigned numRowPerSubarray, unsigned numColPerSubarray, unsigned bufferSize)
{
  bool ok = true;
  ok = ok & deriveDimension(m_cfgVarNumRanks, m_envVarNumRanks, numRanks, DEFAULT_NUM_RANKS, m_numRanks);
  ok = ok & deriveDimension(m_cfgVarNumBankPerRank, m_envVarNumBankPerRank, numBankPerRank, DEFAULT_NUM_BANK_PER_RANK, m_numBankPerRank);
  ok = ok & deriveDimension(m_cfgVarNumSubarrayPerBank, m_envVarNumSubarrayPerBank, numSubarrayPerBank, DEFAULT_NUM_SUBARRAY_PER_BANK, m_numSubarrayPerBank);
  ok = ok & deriveDimension(m_cfgVarNumRowPerSubarray, m_envVarNumRowPerSubarray, numRowPerSubarray, DEFAULT_NUM_ROW_PER_SUBARRAY, m_numRowPerSubarray);
  ok = ok & deriveDimension(m_cfgVarNumColPerSubarray, m_envVarNumColPerSubarray, numColPerSubarray, DEFAULT_NUM_COL_PER_SUBARRAY, m_numColPerSubarray);
  ok = ok & deriveDimension(m_cfgVarBufferSize, m_envVarBufferSize, bufferSize, DEFAULT_BUFFER_SIZE, m_bufferSize);
  if (m_numRanks.getValue() == 0 || m_numBankPerRank.getValue() == 0 || m_numSubarrayPerBank.getValue() == 0 || m_numRowPerSubarray.getValue() == 0 || m_numColPerSubarray.getValue() == 0) {
    std::printf("PIM-Error: Memory dimension parameter cannot be 0\n");
    ok = false;
  }
  if (m_simTarget.getValue() != PIM_DEVICE_AIM && m_bufferSize.getValue() > 0) {
    std::printf("PIM-Error: PIM Device %s does not support any on-chip buffer.\n", pimUtils::pimDeviceEnumToStr(m_simTarget.getValue()).c_str());
    ok = false;
  }
  return ok;
}

//! @brief  Derive Params: Max number of threads
bool
pimSimConfig::deriveNumThreads()
{
  bool hasVal = false;
  std::string valStr;

  // Check config file
  if (m_numThreads.getValue() == 0) {
    valStr = pimUtils::getOptionalParam(m_cfgParams, m_cfgVarMaxNumThreads, hasVal);
    if (hasVal) {
      unsigned val = 0;
      bool ok = pimUtils::convertStringToUnsigned(valStr, val);
      if (!ok) {
        std::printf("PIM-Error: Incorrect config file parameter: %s=%s\n", m_cfgVarMaxNumThreads.c_str(), valStr.c_str());
        return false;
      }
      if (val > 0) {
        m_numThreads.setValue(val);
      }
    }
  }

  // Check env var. Zero will be ignored
  if (m_numThreads.getValue() == 0) {
    valStr = pimUtils::getOptionalParam(m_envParams, m_envVarMaxNumThreads, hasVal);
    if (hasVal) {
      unsigned val = 0;
      bool ok = pimUtils::convertStringToUnsigned(valStr, val);
      if (!ok) {
        std::printf("PIM-Error: Incorrect environment variable: %s=%s\n", m_envVarMaxNumThreads.c_str(), valStr.c_str());
        return false;
      }
      if (val > 0) {
        m_numThreads.setValue(val);
      }
    }
  }

  unsigned hwThreads = std::thread::hardware_concurrency();
  unsigned numThreads = m_numThreads.getValue();
  if (m_debug.getValue() & pimSimConfig::DEBUG_PARAMS) {
    std::printf("PIM-Debug: Maximum number of threads = %u, hardware concurrency = %u\n", numThreads, hwThreads);
  }
  if (numThreads == 0) {
    numThreads = hwThreads;
  } else {
    numThreads = std::min(numThreads, hwThreads);
  }
  // Safety check
  if (numThreads < 1) {
    numThreads = 1;
  }
  m_numThreads.setValue(numThreads);
  return true;
}

//! @brief  Derive Params: Misc Env Vars
bool
pimSimConfig::deriveMiscEnvVars()
{
  bool hasVal = false;
  std::string valStr;

  // Analysis Mode
  m_analysisMode.setValue(false);  // off by default
  valStr = pimUtils::getOptionalParam(m_envParams, m_envVarAnalysisMode, hasVal);
  if (hasVal) {
    if (valStr != "0" && valStr != "1") {
      std::printf("PIM-Error: Incorrect environment variable: %s=%s\n", m_envVarAnalysisMode.c_str(), valStr.c_str());
      return false;
    }
    m_analysisMode.setValue(valStr == "1");
  }
  if (m_analysisMode.getValue()) {
    std::printf("PIM-Warning: Running analysis only mode. Ignoring computation for fast performance and energy analysis.\n");
  }

  return true;
}

//! @brief  Derive Params: Load balance - Distribute data evenly among parallel cores during allocation
bool
pimSimConfig::deriveLoadBalance()
{
  m_loadBalanced.setValue(true);  // on by default

  // Check config file then env variable
  bool hasVal = false;
  std::string valStr = pimUtils::getOptionalParam(m_cfgParams, m_cfgVarLoadBalance, hasVal);
  if (hasVal) {
    if (valStr != "0" && valStr != "1") {
      std::printf("PIM-Error: Incorrect config file parameter: %s=%s\n", m_cfgVarLoadBalance.c_str(), valStr.c_str());
      return false;
    }
    m_loadBalanced.setValue(valStr == "1");
  } else {
    valStr = pimUtils::getOptionalParam(m_envParams, m_envVarLoadBalance, hasVal);
    if (hasVal) {
      if (valStr != "0" && valStr != "1") {
        std::printf("PIM-Error: Incorrect environment variable: %s=%s\n", m_envVarLoadBalance.c_str(), valStr.c_str());
        return false;
      }
      m_loadBalanced.setValue(valStr == "1");
    }
  }
  return true;
}

//! @brief  Derive Params: ECC
bool
pimSimConfig::deriveEcc()
{
  m_eccEnabled.setValue(DEFAULT_ECC);
  m_eccGranularity.setValue(0);
  m_eccType.setValue("secded");
  m_eccLayers.setValue(1);

  // Check ECC Enabled
  bool hasVal = false;
  std::string valStr = pimUtils::getOptionalParam(m_cfgParams, m_cfgVarEcc, hasVal);
  if (hasVal) {
    if (valStr != "0" && valStr != "1") {
      std::printf("PIM-Error: Incorrect config file parameter: %s=%s\n", m_cfgVarEcc.c_str(), valStr.c_str());
      return false;
    }
    m_eccEnabled.setValue(valStr == "1");
  } else {
    valStr = pimUtils::getOptionalParam(m_envParams, m_envVarEcc, hasVal);
    if (hasVal) {
      if (valStr != "0" && valStr != "1") {
        std::printf("PIM-Error: Incorrect environment variable: %s=%s\n", m_envVarEcc.c_str(), valStr.c_str());
        return false;
      }
      m_eccEnabled.setValue(valStr == "1");
    }
  }

  // Check ECC Granularity
  valStr = pimUtils::getOptionalParam(m_cfgParams, m_cfgVarEccGranularity, hasVal);
  if (hasVal) {
    unsigned val = 0;
    if (!pimUtils::convertStringToUnsigned(valStr, val)) {
      std::printf("PIM-Error: Incorrect config file parameter: %s=%s\n", m_cfgVarEccGranularity.c_str(), valStr.c_str());
      return false;
    }
    m_eccGranularity.setValue(val);
  } else {
    valStr = pimUtils::getOptionalParam(m_envParams, m_envVarEccGranularity, hasVal);
    if (hasVal) {
      unsigned val = 0;
      if (!pimUtils::convertStringToUnsigned(valStr, val)) {
        std::printf("PIM-Error: Incorrect environment variable: %s=%s\n", m_envVarEccGranularity.c_str(), valStr.c_str());
        return false;
      }
      m_eccGranularity.setValue(val);
    }
  }

  // Check ECC Type
  valStr = pimUtils::getOptionalParam(m_cfgParams, m_cfgVarEccType, hasVal);
  if (hasVal) {
    m_eccType.setValue(valStr);
  } else {
    valStr = pimUtils::getOptionalParam(m_envParams, m_envVarEccType, hasVal);
    if (hasVal) {
      m_eccType.setValue(valStr);
    }
  }

  // Check ECC Layers
  valStr = pimUtils::getOptionalParam(m_cfgParams, m_cfgVarEccLayers, hasVal);
  if (hasVal) {
    unsigned val = 0;
    if (!pimUtils::convertStringToUnsigned(valStr, val)) {
      std::printf("PIM-Error: Incorrect config file parameter: %s=%s\n", m_cfgVarEccLayers.c_str(), valStr.c_str());
      return false;
    }
    m_eccLayers.setValue(val);
  } else {
    valStr = pimUtils::getOptionalParam(m_envParams, m_envVarEccLayers, hasVal);
    if (hasVal) {
      unsigned val = 0;
      if (!pimUtils::convertStringToUnsigned(valStr, val)) {
        std::printf("PIM-Error: Incorrect environment variable: %s=%s\n", m_envVarEccLayers.c_str(), valStr.c_str());
        return false;
      }
      m_eccLayers.setValue(val);
    }
  }

  if (m_eccEnabled.getValue()) {
    m_eccStrategy = pimEccFactory::create(m_eccType.getValue(), m_eccLayers.getValue());
    if (!m_eccStrategy) {
      m_eccEnabled.setValue(false);
    }
  }

  return true;
}
