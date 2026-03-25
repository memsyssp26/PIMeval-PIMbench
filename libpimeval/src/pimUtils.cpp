// File: pimUtils.cpp
// PIMeval Simulator - Utilities
// Copyright (c) 2024 University of Virginia
// This file is licensed under the MIT License.
// See the LICENSE file in the root of this repository for more details.

#include "pimUtils.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cstring>

namespace pimUtils
{
  const std::map<PimStatus, std::string> statusToStrMap = {
    {PIM_ERROR, "PIM_ERROR"},
    {PIM_OK, "PIM_OK"},
    {PIM_ERR_UNINITIALIZED, "PIM_ERR_UNINITIALIZED"},
    {PIM_ERR_ALREADY_INITIALIZED, "PIM_ERR_ALREADY_INITIALIZED"},
    {PIM_ERR_INVALID_ARG, "PIM_ERR_INVALID_ARG"},
    {PIM_ERR_OUT_OF_MEM, "PIM_ERR_OUT_OF_MEM"},
    {PIM_ERR_DEVICE_NOT_FOUND, "PIM_ERR_DEVICE_NOT_FOUND"},
    {PIM_ERR_NOT_SUPPORTED, "PIM_ERR_NOT_SUPPORTED"},
    {PIM_ERR_CONFIG_NOT_FOUND, "PIM_ERR_CONFIG_NOT_FOUND"},
    {PIM_ERR_CONFIG_INVALID, "PIM_ERR_CONFIG_INVALID"},
    {PIM_ERR_OBJ_NOT_FOUND, "PIM_ERR_OBJ_NOT_FOUND"},
    {PIM_ERR_OBJ_ALREADY_EXISTS, "PIM_ERR_OBJ_ALREADY_EXISTS"},
    {PIM_ERR_OBJ_INVALID, "PIM_ERR_OBJ_INVALID"},
    {PIM_ERR_ECC_UNCORRECTABLE, "PIM_ERR_ECC_UNCORRECTABLE"},
    {PIM_ERR_ECC_CORRECTED, "PIM_ERR_ECC_CORRECTED"},
    {PIM_ERR_LAYOUT_MISMATCH, "PIM_ERR_LAYOUT_MISMATCH"},
    {PIM_ERR_CORE_NOT_FOUND, "PIM_ERR_CORE_NOT_FOUND"},
    {PIM_ERR_CORE_BUSY, "PIM_ERR_CORE_BUSY"},
    {PIM_ERR_CORE_OOM, "PIM_ERR_CORE_OOM"},
    {PIM_ERR_UNSPECIFIED, "PIM_ERR_UNSPECIFIED"}
  };

  const std::map<PimDeviceEnum, std::string> deviceToStrMap = {
    {PIM_DEVICE_NONE, "PIM_DEVICE_NONE"},
    {PIM_FUNCTIONAL, "PIM_FUNCTIONAL"},
    {PIM_DEVICE_BITSIMD_V, "PIM_DEVICE_BITSIMD_V"},
    {PIM_DEVICE_BITSIMD_V_NAND, "PIM_DEVICE_BITSIMD_V_NAND"},
    {PIM_DEVICE_BITSIMD_V_MAJ, "PIM_DEVICE_BITSIMD_V_MAJ"},
    {PIM_DEVICE_BITSIMD_V_AP, "PIM_DEVICE_BITSIMD_V_AP"},
    {PIM_DEVICE_DRISA_NOR, "PIM_DEVICE_DRISA_NOR"},
    {PIM_DEVICE_DRISA_MIXED, "PIM_DEVICE_DRISA_MIXED"},
    {PIM_DEVICE_SIMDRAM, "PIM_DEVICE_SIMDRAM"},
    {PIM_DEVICE_BITSIMD_H, "PIM_DEVICE_BITSIMD_H"},
    {PIM_DEVICE_FULCRUM, "PIM_DEVICE_FULCRUM"},
    {PIM_DEVICE_BANK_LEVEL, "PIM_DEVICE_BANK_LEVEL"},
    {PIM_DEVICE_AQUABOLT, "PIM_DEVICE_AQUABOLT"},
    {PIM_DEVICE_AIM, "PIM_DEVICE_AIM"}
  };

  const std::map<std::string, PimDeviceEnum> strToDeviceMap = {
    {"PIM_DEVICE_NONE", PIM_DEVICE_NONE},
    {"PIM_FUNCTIONAL", PIM_FUNCTIONAL},
    {"PIM_DEVICE_BITSIMD_V", PIM_DEVICE_BITSIMD_V},
    {"PIM_DEVICE_BITSIMD_V_NAND", PIM_DEVICE_BITSIMD_V_NAND},
    {"PIM_DEVICE_BITSIMD_V_MAJ", PIM_DEVICE_BITSIMD_V_MAJ},
    {"PIM_DEVICE_BITSIMD_V_AP", PIM_DEVICE_BITSIMD_V_AP},
    {"PIM_DEVICE_DRISA_NOR", PIM_DEVICE_DRISA_NOR},
    {"PIM_DEVICE_DRISA_MIXED", PIM_DEVICE_DRISA_MIXED},
    {"PIM_DEVICE_SIMDRAM", PIM_DEVICE_SIMDRAM},
    {"PIM_DEVICE_BITSIMD_H", PIM_DEVICE_BITSIMD_H},
    {"PIM_DEVICE_FULCRUM", PIM_DEVICE_FULCRUM},
    {"PIM_DEVICE_BANK_LEVEL", PIM_DEVICE_BANK_LEVEL},
    {"PIM_DEVICE_AQUABOLT", PIM_DEVICE_AQUABOLT},
    {"PIM_DEVICE_AIM", PIM_DEVICE_AIM}
  };

  std::string pimStatusEnumToStr(PimStatus status) {
    auto it = statusToStrMap.find(status);
    return (it != statusToStrMap.end()) ? it->second : "UNKNOWN";
  }

  std::string pimDeviceEnumToStr(PimDeviceEnum deviceType) {
    auto it = deviceToStrMap.find(deviceType);
    return (it != deviceToStrMap.end()) ? it->second : "UNKNOWN";
  }

  PimDeviceEnum strToPimDeviceEnum(const std::string& deviceTypeStr) {
    auto it = strToDeviceMap.find(deviceTypeStr);
    return (it != strToDeviceMap.end()) ? it->second : PIM_DEVICE_NONE;
  }

  std::string pimAllocEnumToStr(PimAllocEnum allocType) {
    switch (allocType) {
      case PIM_ALLOC_AUTO: return "PIM_ALLOC_AUTO";
      case PIM_ALLOC_H: return "PIM_ALLOC_H";
      case PIM_ALLOC_V: return "PIM_ALLOC_V";
      default: return "UNKNOWN";
    }
  }

  std::string pimCopyEnumToStr(PimCopyEnum copyType) {
    switch (copyType) {
      case PIM_COPY_H: return "PIM_COPY_H";
      case PIM_COPY_V: return "PIM_COPY_V";
      default: return "UNKNOWN";
    }
  }

  std::string pimDataTypeEnumToStr(PimDataType dataType) {
    switch (dataType) {
      case PIM_BOOL: return "PIM_BOOL";
      case PIM_INT8: return "PIM_INT8";
      case PIM_INT16: return "PIM_INT16";
      case PIM_INT32: return "PIM_INT32";
      case PIM_INT64: return "PIM_INT64";
      case PIM_UINT8: return "PIM_UINT8";
      case PIM_UINT16: return "PIM_UINT16";
      case PIM_UINT32: return "PIM_UINT32";
      case PIM_UINT64: return "PIM_UINT64";
      case PIM_FP32: return "PIM_FP32";
      case PIM_FP16: return "PIM_FP16";
      case PIM_BF16: return "PIM_BF16";
      case PIM_FP8: return "PIM_FP8";
      default: return "UNKNOWN";
    }
  }

  unsigned getNumBitsOfDataType(PimDataType dataType, PimBitWidth bitWidthType) {
    (void)bitWidthType;
    unsigned bits = 0;
    switch (dataType) {
      case PIM_BOOL: bits = 1; break;
      case PIM_INT8: case PIM_UINT8: bits = 8; break;
      case PIM_INT16: case PIM_UINT16: bits = 16; break;
      case PIM_INT32: case PIM_UINT32: bits = 32; break;
      case PIM_INT64: case PIM_UINT64: bits = 64; break;
      case PIM_FP32: bits = 32; break;
      case PIM_FP16: case PIM_BF16: bits = 16; break;
      case PIM_FP8: bits = 8; break;
      default: break;
    }
    return bits;
  }

  bool isSigned(PimDataType dataType) {
    return (dataType == PIM_INT8 || dataType == PIM_INT16 || dataType == PIM_INT32 || dataType == PIM_INT64);
  }
  bool isUnsigned(PimDataType dataType) {
    return (dataType == PIM_UINT8 || dataType == PIM_UINT16 || dataType == PIM_UINT32 || dataType == PIM_UINT64 || dataType == PIM_BOOL);
  }
  bool isFP(PimDataType dataType) {
    return (dataType == PIM_FP32 || dataType == PIM_FP16 || dataType == PIM_BF16 || dataType == PIM_FP8);
  }

  float castBitsToFloat(PimDataType dataType, uint64_t bits) {
    switch (dataType) {
      case PIM_FP32: return castBitsToType<float>(bits);
      case PIM_FP16: {
        uint32_t sign = (bits >> 15) & 0x1;
        uint32_t exp = (bits >> 10) & 0x1f;
        uint32_t mant = bits & 0x3ff;
        uint32_t resBits = 0;
        if (exp == 0) { // Zero or subnormal
          if (mant == 0) resBits = sign << 31;
          else { // Subnormal to normal
            exp = 127 - 15;
            while (!(mant & 0x400)) { mant <<= 1; exp--; }
            mant &= 0x3ff;
            resBits = (sign << 31) | (exp << 23) | (mant << 13);
          }
        } else if (exp == 0x1f) { // Inf or NaN
          resBits = (sign << 31) | (0xff << 23) | (mant << 13);
        } else {
          resBits = (sign << 31) | ((exp + 127 - 15) << 23) | (mant << 13);
        }
        return castBitsToType<float>(resBits);
      }
      case PIM_BF16: {
        uint32_t resBits = bits << 16;
        return castBitsToType<float>(resBits);
      }
      case PIM_FP8: { // E4M3
        uint32_t sign = (bits >> 7) & 0x1;
        uint32_t exp = (bits >> 3) & 0xf;
        uint32_t mant = bits & 0x7;
        uint32_t resBits = 0;
        if (exp == 0) {
          if (mant == 0) resBits = sign << 31;
          else {
            exp = 127 - 7;
            while (!(mant & 0x8)) { mant <<= 1; exp--; }
            mant &= 0x7;
            resBits = (sign << 31) | (exp << 23) | (mant << 20);
          }
        } else if (exp == 0xf && mant == 0x7) { // NaN
          resBits = 0x7fffffff;
        } else {
          resBits = (sign << 31) | ((exp + 127 - 7) << 23) | (mant << 20);
        }
        return castBitsToType<float>(resBits);
      }
      default: return 0.0f;
    }
  }

  uint64_t castFloatToBits(PimDataType dataType, float val) {
    uint32_t fBits = castTypeToBits<float>(val);
    switch (dataType) {
      case PIM_FP32: return fBits;
      case PIM_FP16: {
        uint32_t sign = (fBits >> 31) & 0x1;
        uint32_t exp = (fBits >> 23) & 0xff;
        uint32_t mant = fBits & 0x7fffff;
        uint16_t res = 0;
        if (exp == 0) res = sign << 15;
        else if (exp == 0xff) res = (sign << 15) | (0x1f << 10) | (mant ? 0x200 : 0);
        else {
          int newExp = (int)exp - 127 + 15;
          if (newExp >= 31) res = (sign << 15) | (0x1f << 10);
          else if (newExp <= 0) res = sign << 15;
          else res = (sign << 15) | (newExp << 10) | (mant >> 13);
        }
        return res;
      }
      case PIM_BF16: return fBits >> 16;
      case PIM_FP8: { // E4M3
        uint32_t sign = (fBits >> 31) & 0x1;
        uint32_t exp = (fBits >> 23) & 0xff;
        uint32_t mant = fBits & 0x7fffff;
        uint8_t res = 0;
        if (exp == 0) res = sign << 7;
        else if (exp == 0xff) res = (sign << 7) | (0xf << 3) | 0x7;
        else {
          int newExp = (int)exp - 127 + 7;
          if (newExp >= 15) res = (sign << 7) | (0xf << 3) | 0x6; // max representable
          else if (newExp <= 0) res = sign << 7;
          else res = (sign << 7) | (newExp << 3) | (mant >> 20);
        }
        return res;
      }
      default: return 0;
    }
  }

  std::string pimProtocolEnumToStr(PimDeviceProtocolEnum protocol) {
    switch (protocol) {
      case PIM_DEVICE_PROTOCOL_DDR: return "DDR";
      case PIM_DEVICE_PROTOCOL_HBM: return "HBM";
      case PIM_DEVICE_PROTOCOL_LPDDR: return "LPDDR";
      case PIM_DEVICE_PROTOCOL_GDDR: return "GDDR";
      default: return "UNKNOWN";
    }
  }

  PimDeviceProtocolEnum strToPimProtocolEnum(const std::string& protocolStr) {
    if (protocolStr == "PIM_DEVICE_PROTOCOL_DDR" || protocolStr == "DDR") return PIM_DEVICE_PROTOCOL_DDR;
    if (protocolStr == "PIM_DEVICE_PROTOCOL_LPDDR" || protocolStr == "LPDDR") return PIM_DEVICE_PROTOCOL_LPDDR;
    if (protocolStr == "PIM_DEVICE_PROTOCOL_HBM" || protocolStr == "HBM") return PIM_DEVICE_PROTOCOL_HBM;
    if (protocolStr == "PIM_DEVICE_PROTOCOL_GDDR" || protocolStr == "GDDR") return PIM_DEVICE_PROTOCOL_GDDR;
    return PIM_DEVICE_PROTOCOL_DDR;
  }

  PimDataLayout getDeviceDataLayout(PimDeviceEnum deviceType) {
    switch (deviceType) {
      case PIM_DEVICE_BITSIMD_V:
      case PIM_DEVICE_BITSIMD_V_NAND:
      case PIM_DEVICE_BITSIMD_V_MAJ:
      case PIM_DEVICE_BITSIMD_V_AP:
      case PIM_DEVICE_DRISA_NOR:
      case PIM_DEVICE_DRISA_MIXED:
      case PIM_DEVICE_SIMDRAM:
        return PimDataLayout::V;
      case PIM_DEVICE_BITSIMD_H:
      case PIM_DEVICE_FULCRUM:
      case PIM_DEVICE_BANK_LEVEL:
      case PIM_DEVICE_AQUABOLT:
      case PIM_DEVICE_AIM:
        return PimDataLayout::H;
      default:
        return PimDataLayout::UNKNOWN;
    }
  }

  std::string& ltrim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    return s;
  }
  std::string& rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    return s;
  }
  std::string& trim(std::string& s) { return ltrim(rtrim(s)); }

  bool readFileContent(const char* fileName, std::string& fileContent) {
    std::ifstream ifs(fileName);
    if (!ifs.is_open()) return false;
    std::stringstream ss;
    ss << ifs.rdbuf();
    fileContent = ss.str();
    return true;
  }

  std::string getParam(const std::map<std::string, std::string>& params, const std::string& key) {
    auto it = params.find(key);
    return (it != params.end()) ? it->second : "";
  }

  std::string getOptionalParam(const std::map<std::string, std::string>& params, const std::string& key, bool& returnStatus) {
    auto it = params.find(key);
    if (it != params.end()) {
      returnStatus = true;
      return it->second;
    }
    returnStatus = false;
    return "";
  }

  std::string removeAfterSemicolon(const std::string &input) {
    size_t pos = input.find(';');
    return (pos != std::string::npos) ? input.substr(0, pos) : input;
  }

  std::string getDirectoryPath(const std::string& filePath) {
    size_t pos = filePath.find_last_of("/\\");
    return (pos != std::string::npos) ? filePath.substr(0, pos) : ".";
  }

  bool getEnvVar(const std::string &varName, std::string &varValue) {
    char* val = std::getenv(varName.c_str());
    if (val) {
      varValue = val;
      return true;
    }
    return false;
  }

  bool convertStringToUnsigned(const std::string& str, unsigned& retVal) {
    try {
      retVal = std::stoul(str);
      return true;
    } catch (...) { return false; }
  }

  std::map<std::string, std::string> readParamsFromConfigFile(const std::string& configFilePath) {
    std::map<std::string, std::string> params;
    std::string content;
    if (readFileContent(configFilePath.c_str(), content)) {
      std::stringstream ss(content);
      std::string line;
      while (std::getline(ss, line)) {
        line = removeAfterSemicolon(line);
        trim(line);
        if (line.empty()) continue;
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
          std::string key = line.substr(0, pos);
          std::string val = line.substr(pos + 1);
          params[trim(key)] = trim(val);
        }
      }
    }
    return params;
  }

  std::map<std::string, std::string> readParamsFromEnvVars(const std::vector<std::string>& envVarNames) {
    std::map<std::string, std::string> params;
    for (const auto& name : envVarNames) {
      std::string val;
      if (getEnvVar(name, val)) params[name] = trim(val);
    }
    return params;
  }

  threadPool::threadPool(size_t numThreads) : m_busyThreads(0), m_stop(false) {
    for (size_t i = 0; i < numThreads; ++i) {
      m_workers.emplace_back([this] {
        for (;;) {
          std::function<void()> task;
          {
            std::unique_lock<std::mutex> lock(this->m_queueMutex);
            this->m_condition.wait(lock, [this] { return this->m_stop || !this->m_tasks.empty(); });
            if (this->m_stop && this->m_tasks.empty()) return;
            task = std::move(this->m_tasks.front());
            this->m_tasks.pop();
            m_busyThreads++;
          }
          task();
          {
            std::unique_lock<std::mutex> lock(this->m_queueMutex);
            m_busyThreads--;
          }
          m_waitCondition.notify_all();
        }
      });
    }
  }

  threadPool::~threadPool() {
    {
      std::unique_lock<std::mutex> lock(m_queueMutex);
      m_stop = true;
    }
    m_condition.notify_all();
    for (std::thread &worker : m_workers) worker.join();
  }

  void threadPool::wait() {
    std::unique_lock<std::mutex> lock(m_queueMutex);
    m_waitCondition.wait(lock, [this] { return m_tasks.empty() && m_busyThreads == 0; });
  }
}
