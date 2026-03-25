// File: pimSimConfig.h
// PIMeval Simulator - PIM Simulator Configurations
// Copyright (c) 2024 University of Virginia
// This file is licensed under the MIT License.
// See the LICENSE file in the root of this repository for more details.

#ifndef LAVA_PIM_SIM_CONFIG_H
#define LAVA_PIM_SIM_CONFIG_H

#include "libpimeval.h"
#include "pimEccStrategy.h"
#include "pimUtils.h"
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <functional>
#include <algorithm>

/**
 * @class pimConfigParamBase
 * @brief Base class for type-erased parameter access.
 */
class pimConfigParamBase {
public:
  virtual ~pimConfigParamBase() {}
  virtual const std::string& getName() const = 0;
  virtual const std::string& getDescription() const = 0;
  virtual std::string getEnvName() const = 0;
  virtual bool fromString(const std::string& val) = 0;
  virtual std::string toString() const = 0;
  virtual void reset() = 0;
};

/**
 * @class pimConfigParam
 * @brief Template class for a configuration parameter with validation and metadata.
 */
template <typename T>
class pimConfigParam : public pimConfigParamBase {
public:
  using Validator = std::function<bool(const T&)>;

  pimConfigParam(const std::string& name, T defaultValue, const std::string& description = "", Validator validator = nullptr)
    : m_name(name), m_defaultValue(defaultValue), m_value(defaultValue), m_description(description), m_validator(validator) {}

  const std::string& getName() const override { return m_name; }
  const std::string& getDescription() const override { return m_description; }
  std::string getEnvName() const override {
    std::string envName = "PIMEVAL_" + m_name;
    std::transform(envName.begin(), envName.end(), envName.begin(), ::toupper);
    return envName;
  }
  const T& getValue() const { return m_value; }

  bool setValue(T value) {
    if (m_validator && !m_validator(value)) {
      std::fprintf(stderr, "PIM-Error: Invalid value for parameter '%s'\n", m_name.c_str());
      return false;
    }
    m_value = value;
    return true;
  }

  void reset() override { m_value = m_defaultValue; }

  bool fromString(const std::string& val) override {
    if constexpr (std::is_same_v<T, double>) {
      try {
        return setValue(std::stod(val));
      } catch (...) { return false; }
    } else if constexpr (std::is_same_v<T, std::string>) {
      return setValue(val);
    } else if constexpr (std::is_same_v<T, unsigned>) {
      try {
        return setValue(static_cast<unsigned>(std::stoul(val)));
      } catch (...) { return false; }
    } else if constexpr (std::is_same_v<T, bool>) {
      std::string lval = val;
      std::transform(lval.begin(), lval.end(), lval.begin(), ::tolower);
      return setValue(lval == "1" || lval == "true" || lval == "on" || lval == "yes");
    } else if constexpr (std::is_same_v<T, PimDeviceEnum>) {
      try {
        if (!val.empty() && std::isdigit(val[0])) {
          return setValue(static_cast<PimDeviceEnum>(std::stoi(val)));
        } else {
          PimDeviceEnum dev = pimUtils::strToPimDeviceEnum(val);
          return setValue(dev);
        }
      } catch (...) {
        return false;
      }
    } else if constexpr (std::is_same_v<T, PimDeviceProtocolEnum>) {
      try {
        if (!val.empty() && std::isdigit(val[0])) {
          return setValue(static_cast<PimDeviceProtocolEnum>(std::stoi(val)));
        } else {
          return setValue(pimUtils::strToPimProtocolEnum(val));
        }
      } catch (...) { return false; }
    }
    return false;
  }

  std::string toString() const override {
    if constexpr (std::is_same_v<T, std::string>) return m_value;
    else if constexpr (std::is_same_v<T, bool>) return m_value ? "1" : "0";
    else if constexpr (std::is_same_v<T, double>) {
      char buf[32];
      std::snprintf(buf, sizeof(buf), "%.2f", m_value);
      return buf;
    }
    else return std::to_string(m_value);
  }

private:
  std::string m_name;
  T m_defaultValue;
  T m_value;
  std::string m_description;
  Validator m_validator;
};

/**
 * @class pimSimConfig
 * @brief Manage PIMeval simulator configurations.
 *
 * Uses a registry-based approach: parameters are loaded from CLI args (--pim-<key>=<val>),
 * config files, and environment variables (PIMEVAL_<KEY>) with CLI > config > env precedence.
 * Post-processing handles special cases like sim target fallback, memory protocol detection,
 * dimension validation, and thread auto-detection.
 */
class pimSimConfig
{
public:
  pimSimConfig();
  ~pimSimConfig() {}

  pimSimConfig(const pimSimConfig&) = delete;
  pimSimConfig& operator=(const pimSimConfig&) = delete;

  // Update PIMeval simulation configuration parameters at device creation
  bool init(PimDeviceEnum deviceType, unsigned numRanks, unsigned numBankPerRank,
      unsigned numSubarrayPerBank, unsigned numRowPerSubarray, unsigned numColPerSubarray, unsigned bufferSize);
  bool init(PimDeviceEnum deviceType, const std::string& configFilePath);
  bool init(int* argc, char*** argv);

  void uninit() { reset(); }
  void reset();
  bool isInit() const { return m_isInit; }
  bool isConfigured() const { return m_isInit; }
  void show() const;

  std::string getParam(const std::string& key, const std::string& envVar, bool& hasVal) const;

  // Getters
  std::string getSimConfigFile() const { return m_simConfigFile.getValue(); }
  std::string getMemConfigFile() const { return m_memConfigFile.getValue(); }
  PimDeviceEnum getDeviceType() const { return m_deviceType.getValue(); }
  PimDeviceEnum getSimTarget() const { return m_simTarget.getValue(); }
  PimDeviceProtocolEnum getMemoryProtocol() const { return m_memoryProtocol.getValue(); }
  unsigned getNumRanks() const { return m_numRanks.getValue(); }
  unsigned getNumBankPerRank() const { return m_numBankPerRank.getValue(); }
  unsigned getNumSubarrayPerBank() const { return m_numSubarrayPerBank.getValue(); }
  unsigned getNumRowPerSubarray() const { return m_numRowPerSubarray.getValue(); }
  unsigned getNumColPerSubarray() const { return m_numColPerSubarray.getValue(); }
  unsigned getNumThreads() const { return m_numThreads.getValue(); }
  unsigned getBufferSize() const { return m_bufferSize.getValue(); }
  bool isAnalysisMode() const { return m_analysisMode.getValue(); }
  unsigned getDebug() const { return m_debug.getValue(); }
  bool isLoadBalanced() const { return m_loadBalanced.getValue(); }
  bool isEccEnabled() const { return m_eccEnabled.getValue(); }
  unsigned getEccGranularity() const { return m_eccGranularity.getValue(); }
  std::string getEccType() const { return m_eccType.getValue(); }
  unsigned getEccLayers() const { return m_eccLayers.getValue(); }
  double getEccLatencyNs() const { return m_eccLatencyNs.getValue(); }
  double getEccEnergyPj() const { return m_eccEnergyPj.getValue(); }
  const pimEccStrategy* getEccStrategy() const { return m_eccStrategy.get(); }

  enum pimDebugFlags
  {
    DEBUG_PARAMS      = 0x0001,
    DEBUG_API_CALLS   = 0x0002,
    DEBUG_CMDS        = 0x0004,
    DEBUG_ALLOC       = 0x0008,
    DEBUG_PERF        = 0x0010,
  };

private:
  bool deriveConfig(PimDeviceEnum deviceType,
      const std::string& configFilePath = "",
      unsigned numRanks = 0,
      unsigned numBankPerRank = 0,
      unsigned numSubarrayPerBank = 0,
      unsigned numRowPerSubarray = 0,
      unsigned numColPerSubarray = 0,
      unsigned bufferSize = 0);

  std::map<std::string, std::string> readEnvVars() const;
  bool deriveSimConfigFile(const std::string& configFilePath);
  std::map<std::string, std::string> readSimConfigFileParams() const;

  // Default values if not specified during init
  static constexpr int DEFAULT_NUM_RANKS = 1;
  static constexpr int DEFAULT_NUM_BANK_PER_RANK = 4;
  static constexpr int DEFAULT_NUM_SUBARRAY_PER_BANK = 32;
  static constexpr int DEFAULT_NUM_ROW_PER_SUBARRAY = 1024;
  static constexpr int DEFAULT_NUM_COL_PER_SUBARRAY = 8192;
  static constexpr int DEFAULT_BUFFER_SIZE = 0;
  static constexpr PimDeviceEnum DEFAULT_SIM_TARGET = PIM_DEVICE_BANK_LEVEL;
  static constexpr bool DEFAULT_ECC = false;

  // Registry for automated lookup and validation
  std::map<std::string, pimConfigParamBase*> m_registry;
  void registerParams();

  // PIM sim variables
  pimConfigParam<std::string> m_simConfigFile{"sim_config", "", "Simulator configuration file path"};
  pimConfigParam<std::string> m_memConfigFile{"mem_config", "", "Memory configuration file path"};
  pimConfigParam<PimDeviceEnum> m_deviceType{"device_type", PIM_DEVICE_NONE, "The actual physical device being modeled"};
  pimConfigParam<PimDeviceEnum> m_simTarget{"sim_target", PIM_DEVICE_NONE, "The target architecture for simulation (e.g. BitSIMD-V)"};
  pimConfigParam<PimDeviceProtocolEnum> m_memoryProtocol{"memory_protocol", PIM_DEVICE_PROTOCOL_DDR, "DRAM protocol (DDR, HBM, etc.)"};
  pimConfigParam<unsigned> m_numRanks{"num_ranks", 1, "Number of memory ranks", [](unsigned v){ return v > 0; }};
  pimConfigParam<unsigned> m_numBankPerRank{"num_banks", 4, "Number of banks per rank", [](unsigned v){ return v > 0; }};
  pimConfigParam<unsigned> m_numSubarrayPerBank{"num_subarrays", 32, "Number of subarrays per bank", [](unsigned v){ return v > 0; }};
  pimConfigParam<unsigned> m_numRowPerSubarray{"num_rows", 1024, "Number of rows per subarray", [](unsigned v){ return v > 0; }};
  pimConfigParam<unsigned> m_numColPerSubarray{"num_cols", 8192, "Number of columns per subarray", [](unsigned v){ return v > 0; }};
  pimConfigParam<unsigned> m_numThreads{"num_threads", 1, "Number of worker threads", [](unsigned v){ return v > 0; }};
  pimConfigParam<unsigned> m_bufferSize{"buffer_size", 0, "Size of the global PIM buffer in bytes"};
  pimConfigParam<bool> m_analysisMode{"analysis_mode", false, "If true, skip functional bit-updates for speed"};
  pimConfigParam<unsigned> m_debug{"debug", 0, "Debug flags bitmask"};
  pimConfigParam<bool> m_loadBalanced{"load_balanced", true, "If true, distribute data across all available cores"};
  pimConfigParam<bool> m_eccEnabled{"ecc", false, "Enable/disable Error Correction Code modeling"};
  pimConfigParam<unsigned> m_eccGranularity{"ecc_granularity", 64, "ECC protection block size in bits", [](unsigned v){ return v == 0 || (v >= 8 && (v & (v-1)) == 0); }};
  pimConfigParam<std::string> m_eccType{"ecc_type", "secded", "ECC scheme type (secded, rs, crc32)"};
  pimConfigParam<unsigned> m_eccLayers{"ecc_layers", 1, "Number of recursive ECC layers", [](unsigned v){ return v > 0 && v <= 4; }};
  pimConfigParam<double> m_eccLatencyNs{"ecc_latency_ns", 0.0, "ECC latency override in nanoseconds (0 = use scheme default)"};
  pimConfigParam<double> m_eccEnergyPj{"ecc_energy_pj", 0.0, "ECC energy override in picojoules (0 = use scheme default)"};
  std::unique_ptr<pimEccStrategy> m_eccStrategy;

  // Store original parameters for extension purpose
  std::map<std::string, std::string> m_cliParams;
  std::map<std::string, std::string> m_envParams;
  std::map<std::string, std::string> m_cfgParams;
  bool m_isInit;
};

#endif
