// File: pimUtils.h
// PIMeval Simulator - Utilities
// Copyright (c) 2024 University of Virginia
// This file is licensed under the MIT License.
// See the LICENSE file in the root of this repository for more details.

#ifndef LAVA_PIM_UTILS_H
#define LAVA_PIM_UTILS_H

#include "libpimeval.h"
#include <string>
#include <queue>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <cctype>
#include <locale>
#include <unordered_map>
#include <map>
#include <type_traits>
#include <cstring>
#include <cstdint>


//! @enum   PimBitWidth
//! @brief  Bit width definitions of PIM data types under different usage scenarios
enum class PimBitWidth
{
  ACTUAL = 0,  // bit width of a data type in real hardware
  HOST,        // bit width of a data type used by host for data transfer in PIMeval
  SIM,         // bit width of a data type used for functional compution in PIMeval
  PADDED,      // bit width of a data element with association and padding in PIMeval
};

//! @enum   PimDataLayout
//! @brief  PIM data layout definitions
enum class PimDataLayout
{
  UNKNOWN = 0,  // unknown
  H,            // horizontal data layout
  V,            // vertical data layout
  HYBRID,       // hybrid data layout
};

namespace pimUtils
{
  std::string pimStatusEnumToStr(PimStatus status);
  std::string pimDeviceEnumToStr(PimDeviceEnum deviceType);
  PimDeviceEnum strToPimDeviceEnum(const std::string& deviceTypeStr);
  std::string pimAllocEnumToStr(PimAllocEnum allocType);
  std::string pimCopyEnumToStr(PimCopyEnum copyType);
  std::string pimDataTypeEnumToStr(PimDataType dataType);
  unsigned getNumBitsOfDataType(PimDataType dataType, PimBitWidth bitWidthType);
  bool isSigned(PimDataType dataType);
  bool isUnsigned(PimDataType dataType);
  bool isFP(PimDataType dataType);
  float castBitsToFloat(PimDataType dataType, uint64_t bits);
  uint64_t castFloatToBits(PimDataType dataType, float val);
  std::string pimProtocolEnumToStr(PimDeviceProtocolEnum protocol);
  PimDeviceProtocolEnum strToPimProtocolEnum(const std::string& protocolStr);
  PimDataLayout getDeviceDataLayout(PimDeviceEnum deviceType);

  // Convert raw bits into sign-extended bits based on PIM data type.
  // Input: Raw bits represented as uint64_t
  // Output: Sign-extended bits represented as uint64_t
  inline uint64_t signExt(uint64_t bits, PimDataType dataType) {
    switch (dataType) {
      case PIM_INT8: return static_cast<uint64_t>(static_cast<int64_t>(static_cast<int8_t>(bits)));
      case PIM_INT16: return static_cast<uint64_t>(static_cast<int64_t>(static_cast<int16_t>(bits)));
      case PIM_INT32: return static_cast<uint64_t>(static_cast<int64_t>(static_cast<int32_t>(bits)));
      case PIM_INT64: return static_cast<uint64_t>(static_cast<int64_t>(static_cast<int64_t>(bits)));
      default: break; // no-op
    }
    return bits;
  }

  // Convert sign-extended bits into specific C++ type.
  // Input: Sign-extended bits represented as uint64_t
  // Output: A value in C++ data type T
  template <typename T> T castBitsToType(uint64_t signExtBits) {
    T val;
    std::memcpy(&val, &signExtBits, sizeof(T));
    return val;
  }

  // Convert specific type into sign-extended bits.
  // Input: A value in C++ data type T
  // Output: sign-extended bits represented as uint64_t
  template <typename T> uint64_t castTypeToBits(T val) {
    uint64_t signExtBits = 0;
    if constexpr (std::is_integral<T>::value && std::is_signed<T>::value) {
      signExtBits = static_cast<uint64_t>(static_cast<int64_t>(val)); // sign ext
    } else {
      std::memcpy(&signExtBits, &val, sizeof(T)); // zero padding
    }
    return signExtBits;
  }

  // Service APIs for file system, config files, env vars
  std::string& ltrim(std::string& s);
  std::string& rtrim(std::string& s);
  std::string& trim(std::string& s);
  bool readFileContent(const char* fileName, std::string& fileContent);
  std::string getParam(const std::map<std::string, std::string>& params, const std::string& key);
  std::string getOptionalParam(const std::map<std::string, std::string>& params, const std::string& key, bool& returnStatus);
  std::string removeAfterSemicolon(const std::string &input);

  std::string getDirectoryPath(const std::string& filePath);
  bool getEnvVar(const std::string &varName, std::string &varValue);
  bool convertStringToUnsigned(const std::string& str, unsigned& retVal);
  std::map<std::string, std::string> readParamsFromConfigFile(const std::string& configFilePath);
  std::map<std::string, std::string> readParamsFromEnvVars(const std::vector<std::string>& envVarNames);
/**
 * @class threadWorker
 * @brief A base class for parallel tasks executed by the thread pool.
 */
class threadWorker {
public:
  virtual ~threadWorker() {}
  virtual void execute() = 0;
};

/**
 * @class threadPool
...
   * @brief A simple thread pool for managing parallel execution of tasks.
   */
  class threadPool {
  public:
    threadPool(size_t numThreads);
    ~threadPool();

    template<class F, class... Args>
    void enqueue(F&& f, Args&&... args);

    void wait();

  private:
    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_tasks;
    std::mutex m_queueMutex;
    std::condition_variable m_condition;
    std::condition_variable m_waitCondition;
    size_t m_busyThreads;
    bool m_stop;
  };

  template<class F, class... Args>
  void threadPool::enqueue(F&& f, Args&&... args) {
    {
      std::unique_lock<std::mutex> lock(m_queueMutex);
      m_tasks.emplace(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    }
    m_condition.notify_one();
  }
}

#endif
