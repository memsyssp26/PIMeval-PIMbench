// File: pimSimConfig.h
// PIMeval Simulator - PIM Simulator Configurations
// Copyright (c) 2024 University of Virginia
// This file is licensed under the MIT License.
// See the LICENSE file in the root of this repository for more details.

#ifndef LAVA_PIM_SIM_CONFIG_H
#define LAVA_PIM_SIM_CONFIG_H

#include "libpimeval.h"
#include "pimEccStrategy.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>


/**
 * @class pimConfigParam
 * @brief A type-safe configuration parameter with validation and defaults.
 */
template <typename T>
class pimConfigParam {
public:
    pimConfigParam(const std::string& name, T defaultVal)
        : m_name(name), m_value(defaultVal), m_default(defaultVal) {}

    void setValue(T val) { m_value = val; m_isSet = true; }
    T getValue() const { return m_value; }
    const std::string& getName() const { return m_name; }
    bool isSet() const { return m_isSet; }
    void reset() { m_value = m_default; m_isSet = false; }

private:
    std::string m_name;
    T m_value;
    T m_default;
    bool m_isSet = false;
};

//! @class  pimSimConfig
//! @brief  PIM simulator configurations
//! This class manages global static PIMeval configurations from config files, env vars, or API parameters,
//! which are for controlling PIMeval library behavior once it is linked with PIM application executables.
//!
//! Supported configuration file parameters:
//!   memory_config_file = <ini-file>            // memory config file, e.g., DDR4_8Gb_x16_3200.ini
//!   simulation_target = <PimDeviceEnum>        // simulation target, e.g., PIM_DEVICE_BITSIMD_V
//!   num_ranks = <int>                          // number of ranks
//!   num_bank_per_rank = <int>                  // number of banks per rank
//!   num_subarray_per_bank = <int>              // number of subarrays per bank
//!   num_row_per_subarray = <int>               // number of rows per subarray
//!   num_col_per_subarray = <int>               // number of columns per subarray
//!   max_num_threads = <int>                    // maximum number of threads used by simulation
//!   should_load_balance = <0|1>                // distribute data evenly among all cores
//!
//! Supported environment variables:
//!   PIMEVAL_SIM_CONFIG <abs-path/cfg-file>     // PIMeval config file, e.g., abs-path/PIMeval_BitSimdV.cfg
//!   PIMEVAL_MEM_CONFIG <abs-path/ini-file>     // memory config file, e.g., DDR4_8Gb_x16_3200.ini
//!   PIMEVAL_SIM_TARGET <PimDeviceEnum>         // simulation target, e.g., PIM_DEVICE_BITSIMD_V
//!   PIMEVAL_NUM_RANKS <int>                    // number of ranks
//!   PIMEVAL_NUM_BANK_PER_RANK <int>            // number of banks per rank
//!   PIMEVAL_NUM_SUBARRAY_PER_BANK <int>        // number of subarrays per bank
//!   PIMEVAL_NUM_ROW_PER_SUBARRAY <int>         // number of rows per subarray
//!   PIMEVAL_NUM_COL_PER_SUBARRAY <int>         // number of columns per subarray
//!   PIMEVAL_MAX_NUM_THREADS <int>              // maximum number of threads used by simulation
//!   PIMEVAL_ANALYSIS_MODE <0|1>                // PIMeval analysis mode
//!   PIMEVAL_DEBUG <int>                        // PIMeval debug flags (see enum pimDebugFlags)
//!   PIMEVAL_LOAD_BALANCE <0|1>                 // distribute data evenly among all cores
//!
//! Precedence rules (highest to lowest priority):
//! * Config file: Either from -c command-line argument or from PIMEVAL_SIM_CONFIG
//! * Environment variables
//! * Parameters from pimCreateDevice API or C++ macros
//!
//! About config file paths:
//! * Simulator config file
//!   - If passing it through -c command line argument, it's already a valid absolute or relative path
//!   - If using the PIMEVAL_SIM_CONFIG env variable, its value needs to be a valid absolute path
//! * Memory config file
//!   - If it is in the same directory as the simulator config file, specifying a file name is sufficient
//!   - Otherwise, use absolute path
//!
//! How to add a new configuration parameter:
//! * Define a key string below as m_cfgVar* and/or m_envVar*, and update the documentation above
//! * Define a member variable, and update the uninit and show function
//! * Add a private derive function (can reuse deriveMiscEnvVars if it's env only)
//! * Add a public getter function
//! * Add env var to readEnvVars function
//!
class pimSimConfig
{
public:
  pimSimConfig() { reset(); }
  ~pimSimConfig() {}

  pimSimConfig(const pimSimConfig&) = delete;
  pimSimConfig& operator=(const pimSimConfig&) = delete;

  // Update PIMeval simulation configuration parameters at device creation
  bool init(PimDeviceEnum deviceType, unsigned numRanks, unsigned numBankPerRank,
      unsigned numSubarrayPerBank, unsigned numRowPerSubarray, unsigned numColPerSubarray, unsigned bufferSize);
  bool init(PimDeviceEnum deviceType, const std::string& configFilePath);
  void uninit() { reset(); }
  bool isInit() const { return m_isInit; }
  void show() const;

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

  bool deriveDebug();
  std::unordered_map<std::string, std::string> readEnvVars() const;
  bool deriveSimConfigFile(const std::string& configFilePath);
  std::unordered_map<std::string, std::string> readSimConfigFileParams() const;
  bool deriveDeviceType(PimDeviceEnum deviceType);
  bool deriveSimTarget();
  bool deriveMemConfigFile();
  bool deriveDimension(const std::string& cfgVar, const std::string& envVar, const unsigned apiVal, const unsigned defVal, pimConfigParam<unsigned>& retVal);
  bool deriveDimensions(unsigned numRanks, unsigned numBankPerRank, unsigned numSubarrayPerBank, unsigned numRowPerSubarray, unsigned numColPerSubarray, unsigned bufferSize);
  bool deriveNumThreads();
  bool deriveMiscEnvVars();
  bool deriveLoadBalance();
  bool deriveEcc();

  bool parseConfigFromFile(const std::string& config, unsigned& numRanks, unsigned& numBankPerRank, unsigned& numSubarrayPerBank, unsigned& numRows, unsigned& numCols);

  // Config file parameters
  inline static const std::string m_cfgVarMemConfig = "memory_config_file";
  inline static const std::string m_cfgVarSimTarget = "simulation_target";
  inline static const std::string m_cfgVarNumRanks = "num_ranks";
  inline static const std::string m_cfgVarNumBankPerRank = "num_bank_per_rank";
  inline static const std::string m_cfgVarNumSubarrayPerBank = "num_subarray_per_bank";
  inline static const std::string m_cfgVarNumRowPerSubarray = "num_row_per_subarray";
  inline static const std::string m_cfgVarNumColPerSubarray = "num_col_per_subarray";
  inline static const std::string m_cfgVarMaxNumThreads = "max_num_threads";
  inline static const std::string m_cfgVarLoadBalance = "should_load_balance";
  inline static const std::string m_cfgVarBufferSize = "buffer_size";
  inline static const std::string m_cfgVarEcc = "ecc_enabled";
  inline static const std::string m_cfgVarEccGranularity = "ecc_granularity";
  inline static const std::string m_cfgVarEccType = "ecc_type";
  inline static const std::string m_cfgVarEccLayers = "ecc_layers";

  // Environment variables
  inline static const std::string m_envVarSimConfig = "PIMEVAL_SIM_CONFIG";
  inline static const std::string m_envVarMemConfig = "PIMEVAL_MEM_CONFIG";
  inline static const std::string m_envVarSimTarget = "PIMEVAL_SIM_TARGET";
  inline static const std::string m_envVarNumRanks = "PIMEVAL_NUM_RANKS";
  inline static const std::string m_envVarNumBankPerRank = "PIMEVAL_NUM_BANK_PER_RANK";
  inline static const std::string m_envVarNumSubarrayPerBank = "PIMEVAL_NUM_SUBARRAY_PER_BANK";
  inline static const std::string m_envVarNumRowPerSubarray = "PIMEVAL_NUM_ROW_PER_SUBARRAY";
  inline static const std::string m_envVarNumColPerSubarray = "PIMEVAL_NUM_COL_PER_SUBARRAY";
  inline static const std::string m_envVarBufferSize = "PIMEVAL_BUFFER_SIZE";
  inline static const std::string m_envVarMaxNumThreads = "PIMEVAL_MAX_NUM_THREADS";
  inline static const std::string m_envVarAnalysisMode = "PIMEVAL_ANALYSIS_MODE";
  inline static const std::string m_envVarDebug = "PIMEVAL_DEBUG";
  inline static const std::string m_envVarLoadBalance = "PIMEVAL_LOAD_BALANCE";
  inline static const std::string m_envVarEcc = "PIMEVAL_ECC";
  inline static const std::string m_envVarEccGranularity = "PIMEVAL_ECC_GRANULARITY";
  inline static const std::string m_envVarEccType = "PIMEVAL_ECC_TYPE";
  inline static const std::string m_envVarEccLayers = "PIMEVAL_ECC_LAYERS";

  // Add env vars to this list for readEnvVars
  inline static const std::vector<std::string> m_envVarList = {
    m_envVarSimConfig,
    m_envVarMemConfig,
    m_envVarSimTarget,
    m_envVarNumRanks,
    m_envVarNumBankPerRank,
    m_envVarNumSubarrayPerBank,
    m_envVarNumRowPerSubarray,
    m_envVarNumColPerSubarray,
    m_envVarMaxNumThreads,
    m_envVarAnalysisMode,
    m_envVarDebug,
    m_envVarLoadBalance,
    m_envVarBufferSize,
    m_envVarEcc,
    m_envVarEccGranularity,
    m_envVarEccType,
    m_envVarEccLayers,
  };

  // Default values if not specified during init
  static constexpr int DEFAULT_NUM_RANKS = 1;
  static constexpr int DEFAULT_NUM_BANK_PER_RANK = 4;
  static constexpr int DEFAULT_NUM_SUBARRAY_PER_BANK = 32;
  static constexpr int DEFAULT_NUM_ROW_PER_SUBARRAY = 1024;
  static constexpr int DEFAULT_NUM_COL_PER_SUBARRAY = 8192;
  static constexpr int DEFAULT_BUFFER_SIZE = 0;
  static constexpr PimDeviceEnum DEFAULT_SIM_TARGET = PIM_DEVICE_BANK_LEVEL;
  static constexpr bool DEFAULT_ECC = false;

  //! @brief  Reset all member variables to default status
  inline void reset() {
    m_simConfigFile.reset();
    m_memConfigFile.reset();
    m_deviceType.reset();
    m_simTarget.reset();
    m_memoryProtocol.reset();
    m_numRanks.reset();
    m_numBankPerRank.reset();
    m_numSubarrayPerBank.reset();
    m_numRowPerSubarray.reset();
    m_numColPerSubarray.reset();
    m_numThreads.reset();
    m_bufferSize.reset();
    m_analysisMode.reset();
    m_debug.reset();
    m_loadBalanced.reset();
    m_eccEnabled.reset();
    m_eccGranularity.reset();
    m_eccType.reset();
    m_eccLayers.reset();
    m_eccStrategy = nullptr;
    m_envParams.clear();
    m_cfgParams.clear();
    m_isInit = false;
  }

  // PIM sim env variables
  pimConfigParam<std::string> m_simConfigFile{"sim_config_file", ""};
  pimConfigParam<std::string> m_memConfigFile{"mem_config_file", ""};
  pimConfigParam<PimDeviceEnum> m_deviceType{"device_type", PIM_DEVICE_NONE};
  pimConfigParam<PimDeviceEnum> m_simTarget{"sim_target", PIM_DEVICE_NONE};
  pimConfigParam<PimDeviceProtocolEnum> m_memoryProtocol{"memory_protocol", PIM_DEVICE_PROTOCOL_DDR};
  pimConfigParam<unsigned> m_numRanks{"num_ranks", 1};
  pimConfigParam<unsigned> m_numBankPerRank{"num_banks", 4};
  pimConfigParam<unsigned> m_numSubarrayPerBank{"num_subarrays", 32};
  pimConfigParam<unsigned> m_numRowPerSubarray{"num_rows", 1024};
  pimConfigParam<unsigned> m_numColPerSubarray{"num_cols", 8192};
  pimConfigParam<unsigned> m_numThreads{"num_threads", 1};
  pimConfigParam<unsigned> m_bufferSize{"buffer_size", 0};
  pimConfigParam<unsigned> m_debug{"debug", 0};
  pimConfigParam<bool> m_analysisMode{"analysis_mode", false};
  pimConfigParam<bool> m_loadBalanced{"load_balanced", true};
  pimConfigParam<bool> m_eccEnabled{"ecc_enabled", false};
  pimConfigParam<unsigned> m_eccGranularity{"ecc_granularity", 64};
  pimConfigParam<std::string> m_eccType{"ecc_type", "secded"};
  pimConfigParam<unsigned> m_eccLayers{"ecc_layers", 1};
  std::unique_ptr<pimEccStrategy> m_eccStrategy;

  // Store original parameters for extension purpose
  std::unordered_map<std::string, std::string> m_envParams;
  std::unordered_map<std::string, std::string> m_cfgParams;
  bool m_isInit;
};

#endif

