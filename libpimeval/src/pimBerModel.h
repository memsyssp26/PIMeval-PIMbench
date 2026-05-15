// File: pimBerModel.h
// PIMeval Simulator - Bit Error Rate (BER) analytical model
// Copyright (c) 2024 University of Virginia
// This file is licensed under the MIT License.
// See the LICENSE file in the root of this repository for more details.
//
// Models expected corrected and uncorrectable error counts for SECDED-protected
// memory given a raw bit error rate (BER) per bit.
//
// Uses the Poisson approximation to the binomial distribution:
//   For a codeword of `w` bits with BER p, the number of bit errors follows
//   approximately Poisson(lambda = w * p) for small p.
//
//   P(correctable)   = P(exactly 1 bit error)  = lambda * exp(-lambda)   [SECDED corrects 1]
//   P(uncorrectable) = P(2 or more bit errors) = 1 - exp(-lambda) * (1 + lambda)
//
// Typical BER values (approximate):
//   DRAM (DDR5/HBM3 raw, before on-die ECC): ~1e-10 per bit
//   SRAM (scratchpad/register file):         ~1e-15 per bit

#ifndef LAVA_PIM_BER_MODEL_H
#define LAVA_PIM_BER_MODEL_H

#include <cstdint>
#include <cmath>

class pimBerModel
{
public:
  struct EccResult {
    double corrected;      // Expected number of correctable (single-bit) errors
    double uncorrectable;  // Expected number of uncorrectable (multi-bit) errors
  };

  // Compute expected corrected and uncorrectable error counts for SECDED protection.
  //
  // @param ber            Raw bit error rate per bit (e.g., 1e-10 for DRAM)
  // @param numDataBits    Total data bits being protected (not including parity)
  // @param codewordBits   Total codeword width including parity (e.g., 136 for 128+8)
  // @return               Expected corrected and uncorrectable counts
  static EccResult simulate(double ber, uint64_t numDataBits, unsigned codewordBits)
  {
    if (ber <= 0.0 || numDataBits == 0 || codewordBits == 0) return {0.0, 0.0};

    uint64_t numCodewords = (numDataBits + codewordBits - 1) / codewordBits;
    // Poisson rate per codeword: expected number of raw bit errors in one codeword
    double lambda = static_cast<double>(codewordBits) * ber;
    // P(exactly 1 error) = lambda * e^(-lambda)  — correctable by SECDED
    double p_correctable   = lambda * std::exp(-lambda);
    // P(2+ errors) = 1 - P(0) - P(1) = 1 - e^(-lambda) * (1 + lambda)
    double p_uncorrectable = 1.0 - std::exp(-lambda) * (1.0 + lambda);

    return {
      static_cast<double>(numCodewords) * p_correctable,
      static_cast<double>(numCodewords) * p_uncorrectable
    };
  }
};

#endif
