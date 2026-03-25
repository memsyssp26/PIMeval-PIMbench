// File: pimEcc.h
// PIMeval Simulator - ECC Implementation
// Copyright (c) 2024 University of Virginia
// This file is licensed under the MIT License.
// See the LICENSE file in the root of this repository for more details.

#ifndef LAVA_PIM_ECC_H
#define LAVA_PIM_ECC_H

#include <cstdint>
#include <vector>
#include <cmath>
#include <string>
#include <algorithm>

/**
 * @class pimEcc
 * @brief Provides Error Correction Code (ECC) mechanisms for PIM simulation.
 */
class pimEcc
{
public:
  /**
   * @brief Get number of ECC bits for a given data width using SECDED
   */
  static unsigned getNumEccBits(unsigned dataWidth) {
    if (dataWidth == 0) return 0;
    unsigned k = 0;
    while ((1U << k) < dataWidth + k + 1) {
      k++;
    }
    return k + 1; // +1 for DED (overall parity)
  }

  /**
   * @brief Get total bits with ECC for given data width and layers
   */
  static unsigned getTotalBitsWithEcc(unsigned dataWidth, unsigned layers, const std::string& type = "secded") {
    unsigned totalBits = dataWidth;
    for (unsigned i = 0; i < layers; ++i) {
      if (type == "secded") {
        totalBits += getNumEccBits(totalBits);
      } else if (type == "rs") {
        // RS simulation model: 8 check bits per 64 data bits (1 check symbol per block)
        totalBits += (totalBits + 63) / 64 * 8;
      } else if (type == "crc32") {
        totalBits += 32;
      }
    }
    return totalBits;
  }

  //! @brief  Encode data bits with SECDED ECC
  static std::vector<bool> encode(const std::vector<bool>& data) {
    unsigned dataWidth = data.size();
    unsigned k = 0;
    while ((1U << k) < dataWidth + k + 1) {
      k++;
    }
    
    unsigned totalBitsWithoutDED = dataWidth + k;
    std::vector<bool> encoded(totalBitsWithoutDED + 1, false);
    
    unsigned dataBitPos = 0;
    // Position Hamming bits at powers of 2 (1, 2, 4, 8, ...)
    for (unsigned i = 1; i <= totalBitsWithoutDED; ++i) {
      if ((i & (i - 1)) == 0) {
        continue;
      }
      if (dataBitPos < dataWidth && data[dataBitPos]) {
        encoded[i - 1] = true;
      }
      dataBitPos++;
    }
    
    // Calculate parity bits
    for (unsigned j = 0; j < k; ++j) {
      unsigned parityBitPos = (1U << j);
      bool parity = false;
      for (unsigned i = 1; i <= totalBitsWithoutDED; ++i) {
        if (i & parityBitPos) {
          if (encoded[i - 1]) {
            parity = !parity;
          }
        }
      }
      encoded[parityBitPos - 1] = parity;
    }
    
    // Calculate overall parity bit (DED)
    bool overallParity = false;
    for (unsigned i = 0; i < totalBitsWithoutDED; ++i) {
      if (encoded[i]) {
        overallParity = !overallParity;
      }
    }
    encoded[totalBitsWithoutDED] = overallParity;
    
    return encoded;
  }

  //! @brief  Decode data bits with SECDED ECC
  static std::vector<bool> decode(const std::vector<bool>& encoded, int& status) {
    unsigned totalBitsWithDED = encoded.size();
    if (totalBitsWithDED == 0) {
      status = 0;
      return {};
    }
    unsigned totalBitsWithoutDED = totalBitsWithDED - 1;
    
    // Find k
    unsigned k = 0;
    while ((1U << k) < totalBitsWithoutDED + 1) {
      k++;
    }
    unsigned dataWidth = totalBitsWithoutDED - k;
    
    // Check overall parity
    bool overallParity = false;
    for (unsigned i = 0; i < totalBitsWithDED; ++i) {
      if (encoded[i]) {
        overallParity = !overallParity;
      }
    }
    
    // Check syndrome
    unsigned syndrome = 0;
    for (unsigned j = 0; j < k; ++j) {
      unsigned parityBitPos = (1U << j);
      bool parity = false;
      for (unsigned i = 1; i <= totalBitsWithoutDED; ++i) {
        if (i & parityBitPos) {
          if (encoded[i - 1]) {
            parity = !parity;
          }
        }
      }
      if (parity) {
        syndrome |= parityBitPos;
      }
    }
    
    std::vector<bool> corrected = encoded;
    if (syndrome == 0) {
      if (overallParity == false) {
        status = 0; // OK
      } else {
        status = 1; // Single error in overall parity bit
      }
    } else {
      if (overallParity == true) {
        status = 1; // Single error
        if (syndrome <= totalBitsWithoutDED) {
          corrected[syndrome - 1] = !corrected[syndrome - 1];
        }
      } else {
        status = 2; // Double error
      }
    }
    
    // Extract data bits
    std::vector<bool> data(dataWidth);
    unsigned dataBitPos = 0;
    for (unsigned i = 1; i <= totalBitsWithoutDED; ++i) {
      if ((i & (i - 1)) == 0) {
        continue;
      }
      if (dataBitPos < dataWidth) {
        data[dataBitPos++] = corrected[i - 1];
      }
    }
    
    return data;
  }

  //! @brief  Simple CRC-32 implementation for error detection
  static uint32_t calculateCRC32(const std::vector<bool>& data) {
    uint32_t crc = 0xFFFFFFFF;
    for (bool bit : data) {
      uint32_t msb = (crc >> 31) ^ (bit ? 1 : 0);
      crc <<= 1;
      if (msb) {
        crc ^= 0x04C11DB7; // CRC-32 polynomial
      }
    }
    return crc;
  }

  //! @brief  Encode with CRC-32
  static std::vector<bool> encodeCRC32(const std::vector<bool>& data) {
    uint32_t crc = calculateCRC32(data);
    std::vector<bool> encoded = data;
    for (int i = 31; i >= 0; --i) {
      encoded.push_back((crc >> i) & 1);
    }
    return encoded;
  }

  //! @brief  Decode with CRC-32 (Detection only)
  static std::vector<bool> decodeCRC32(const std::vector<bool>& encoded, int& status) {
    if (encoded.size() < 32) {
      status = 2; // Error
      return encoded;
    }
    unsigned dataWidth = encoded.size() - 32;
    std::vector<bool> data(encoded.begin(), encoded.begin() + dataWidth);
    uint32_t expectedCrc = 0;
    for (unsigned i = 0; i < 32; ++i) {
      expectedCrc = (expectedCrc << 1) | (encoded[dataWidth + i] ? 1 : 0);
    }
    uint32_t actualCrc = calculateCRC32(data);
    if (actualCrc == expectedCrc) {
      status = 0;
    } else {
      status = 2; // Uncorrectable error (detection)
    }
    return data;
  }

  //! @brief  Compute RS check byte for a 64-bit data block using rotational XOR.
  //! Each data byte is rotated by its position index before XOR, making the check
  //! sensitive to both value and position (approximates RS symbol-level protection).
  static uint8_t computeRsCheckByte(const std::vector<bool>& block, unsigned blockStart, unsigned blockLen) {
    uint8_t check = 0;
    unsigned numBytes = (blockLen + 7) / 8;
    for (unsigned byteIdx = 0; byteIdx < numBytes; ++byteIdx) {
      uint8_t dataByte = 0;
      for (unsigned bit = 0; bit < 8; ++bit) {
        unsigned pos = blockStart + byteIdx * 8 + bit;
        if (pos < blockStart + blockLen && pos < block.size() && block[pos]) {
          dataByte |= (1U << bit);
        }
      }
      // Rotate left by byte position to make check position-dependent
      unsigned rot = byteIdx % 8;
      uint8_t rotated = (dataByte << rot) | (dataByte >> (8 - rot));
      check ^= rotated;
    }
    return check;
  }

  //! @brief  Reed-Solomon simulation model for PIM ECC.
  //! Uses 8 check bits per 64 data bits (1 check symbol per block).
  //! This models RS symbol-level error detection at the byte granularity.
  //! With 1 check symbol, the scheme can detect single-symbol errors but
  //! cannot correct them (correction requires n-k >= 2t check symbols).
  static std::vector<bool> encodeRS(const std::vector<bool>& data) {
    unsigned dataSize = data.size();
    unsigned numBlocks = (dataSize + 63) / 64;
    std::vector<bool> encoded = data;
    // Compute and append check byte for each 64-bit block
    for (unsigned b = 0; b < numBlocks; ++b) {
      unsigned blockStart = b * 64;
      unsigned blockLen = std::min(64u, dataSize - blockStart);
      uint8_t check = computeRsCheckByte(data, blockStart, blockLen);
      for (unsigned bit = 0; bit < 8; ++bit) {
        encoded.push_back((check >> bit) & 1);
      }
    }
    return encoded;
  }

  //! @brief  Decode RS-encoded data. Detects symbol-level corruption via check bytes.
  //! Returns status: 0=OK, 2=error detected (uncorrectable with 1 check symbol).
  static std::vector<bool> decodeRS(const std::vector<bool>& encoded, int& status) {
    // Recover data width from encoded size: dataWidth + ceil(dataWidth/64)*8 == encoded.size()
    unsigned dataWidth = (unsigned)(encoded.size() / 1.125);
    while (dataWidth + (dataWidth + 63) / 64 * 8 < encoded.size()) dataWidth++;
    while (dataWidth + (dataWidth + 63) / 64 * 8 > encoded.size()) dataWidth--;

    std::vector<bool> data(encoded.begin(), encoded.begin() + dataWidth);
    unsigned numBlocks = (dataWidth + 63) / 64;

    status = 0;
    for (unsigned b = 0; b < numBlocks; ++b) {
      unsigned blockStart = b * 64;
      unsigned blockLen = std::min(64u, dataWidth - blockStart);
      // Recompute check byte from the (possibly corrupted) data portion
      uint8_t recomputed = computeRsCheckByte(encoded, blockStart, blockLen);
      // Extract stored check byte
      unsigned checkStart = dataWidth + b * 8;
      uint8_t stored = 0;
      for (unsigned bit = 0; bit < 8; ++bit) {
        if (checkStart + bit < encoded.size() && encoded[checkStart + bit]) {
          stored |= (1U << bit);
        }
      }
      if (recomputed != stored) {
        // Error detected in this block. With 1 check symbol per block,
        // RS can detect but not correct (would need 2+ check symbols).
        status = 2;
      }
    }
    return data;
  }

  //! @brief  Encode with multi-layers
  static std::vector<bool> encodeMultiLayer(const std::vector<bool>& data, unsigned layers, const std::string& type = "secded") {
    std::vector<bool> result = data;
    for (unsigned i = 0; i < layers; ++i) {
      if (type == "secded") {
        result = encode(result);
      } else if (type == "rs") {
        result = encodeRS(result);
      } else if (type == "crc32") {
        result = encodeCRC32(result);
      }
    }
    return result;
  }

  //! @brief  Decode with multi-layers
  static std::vector<bool> decodeMultiLayer(const std::vector<bool>& encoded, unsigned layers, int& status, const std::string& type = "secded") {
    std::vector<bool> result = encoded;
    status = 0;
    for (int i = (int)layers - 1; i >= 0; --i) {
      int layerStatus = 0;
      if (type == "secded") {
        result = decode(result, layerStatus);
      } else if (type == "rs") {
        result = decodeRS(result, layerStatus);
      } else if (type == "crc32") {
        result = decodeCRC32(result, layerStatus);
      }
      status = std::max(status, layerStatus);
    }
    return result;
  }

  // Legacy uint64_t support
  static uint64_t encode(uint64_t data, unsigned dataWidth) {
    std::vector<bool> dataVec(dataWidth);
    for (unsigned i = 0; i < dataWidth; ++i) {
      dataVec[i] = (data >> i) & 1;
    }
    std::vector<bool> encodedVec = encode(dataVec);
    uint64_t encoded = 0;
    for (unsigned i = 0; i < encodedVec.size() && i < 64; ++i) {
      if (encodedVec[i]) encoded |= (1ULL << i);
    }
    return encoded;
  }

  static uint64_t decode(uint64_t encoded, unsigned dataWidth, int& status) {
    unsigned k = 0;
    while ((1U << k) < dataWidth + k + 1) {
      k++;
    }
    unsigned totalBits = dataWidth + k + 1;
    std::vector<bool> encodedVec(totalBits);
    for (unsigned i = 0; i < totalBits && i < 64; ++i) {
      encodedVec[i] = (encoded >> i) & 1;
    }
    std::vector<bool> decodedVec = decode(encodedVec, status);
    uint64_t decoded = 0;
    for (unsigned i = 0; i < decodedVec.size() && i < 64; ++i) {
      if (decodedVec[i]) decoded |= (1ULL << i);
    }
    return decoded;
  }
};

#endif
