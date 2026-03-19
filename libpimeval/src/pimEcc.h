// File: pimEcc.h
// PIMeval Simulator - ECC (SECDED) Implementation
// Copyright (c) 2024 University of Virginia
// This file is licensed under the MIT License.
// See the LICENSE file in the root of this repository for more details.

#ifndef LAVA_PIM_ECC_H
#define LAVA_PIM_ECC_H

#include <cstdint>
#include <vector>
#include <cmath>

//! @class  pimEcc
//! @brief  ECC (SECDED) implementation for PIM simulator
class pimEcc
{
public:
  //! @brief  Get number of ECC bits for a given data width
  static unsigned getNumEccBits(unsigned dataWidth) {
    if (dataWidth == 0) return 0;
    unsigned k = 0;
    while ((1U << k) < dataWidth + k + 1) {
      k++;
    }
    return k + 1; // +1 for DED (overall parity)
  }

  //! @brief  Encode data with SECDED ECC
  //! Returns (data | ecc_bits)
  static uint64_t encode(uint64_t data, unsigned dataWidth) {
    unsigned k = 0;
    while ((1U << k) < dataWidth + k + 1) {
      k++;
    }
    
    uint64_t encoded = 0;
    unsigned dataBitPos = 0;
    unsigned totalBits = dataWidth + k;
    
    // Position Hamming bits at powers of 2 (1, 2, 4, 8, ...)
    for (unsigned i = 1; i <= totalBits; ++i) {
      if ((i & (i - 1)) == 0) {
        // Skip parity bit positions for now
        continue;
      }
      if ((data >> dataBitPos) & 1) {
        encoded |= (1ULL << (i - 1));
      }
      dataBitPos++;
    }
    
    // Calculate parity bits
    for (unsigned j = 0; j < k; ++j) {
      unsigned parityBitPos = (1U << j);
      bool parity = 0;
      for (unsigned i = 1; i <= totalBits; ++i) {
        if (i & parityBitPos) {
          if ((encoded >> (i - 1)) & 1) {
            parity = !parity;
          }
        }
      }
      if (parity) {
        encoded |= (1ULL << (parityBitPos - 1));
      }
    }
    
    // Calculate overall parity bit (DED)
    bool overallParity = 0;
    for (unsigned i = 0; i < totalBits; ++i) {
      if ((encoded >> i) & 1) {
        overallParity = !overallParity;
      }
    }
    if (overallParity) {
      encoded |= (1ULL << totalBits);
    }
    
    return encoded;
  }

  //! @brief  Decode data with SECDED ECC
  //! Returns corrected data. Reports status: 0: OK, 1: Single Error Corrected, 2: Double Error Detected
  static uint64_t decode(uint64_t encoded, unsigned dataWidth, int& status) {
    unsigned k = 0;
    while ((1U << k) < dataWidth + k + 1) {
      k++;
    }
    unsigned totalBits = dataWidth + k;
    
    // Check overall parity
    bool overallParity = 0;
    for (unsigned i = 0; i <= totalBits; ++i) {
      if ((encoded >> i) & 1) {
        overallParity = !overallParity;
      }
    }
    
    // Check syndrome
    unsigned syndrome = 0;
    for (unsigned j = 0; j < k; ++j) {
      unsigned parityBitPos = (1U << j);
      bool parity = 0;
      for (unsigned i = 1; i <= totalBits; ++i) {
        if (i & parityBitPos) {
          if ((encoded >> (i - 1)) & 1) {
            parity = !parity;
          }
        }
      }
      if (parity) {
        syndrome |= parityBitPos;
      }
    }
    
    if (syndrome == 0) {
      if (overallParity == 0) {
        status = 0; // OK
      } else {
        status = 1; // Single error in overall parity bit
        // No correction needed for data
      }
    } else {
      if (overallParity == 1) {
        status = 1; // Single error
        if (syndrome <= totalBits) {
          encoded ^= (1ULL << (syndrome - 1)); // Correct bit
        }
      } else {
        status = 2; // Double error
      }
    }
    
    // Extract data bits
    uint64_t data = 0;
    unsigned dataBitPos = 0;
    for (unsigned i = 1; i <= totalBits; ++i) {
      if ((i & (i - 1)) == 0) {
        continue;
      }
      if ((encoded >> (i - 1)) & 1) {
        data |= (1ULL << dataBitPos);
      }
      dataBitPos++;
    }
    
    return data;
  }
};

#endif
