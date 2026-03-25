// File: pimEccStrategy.h
// PIMeval Simulator - ECC Strategy Pattern
// Copyright (c) 2024 University of Virginia
// This file is licensed under the MIT License.

#ifndef LAVA_PIM_ECC_STRATEGY_H
#define LAVA_PIM_ECC_STRATEGY_H

#include <vector>
#include <string>
#include <memory>

/**
 * @class pimEccStrategy
 * @brief Abstract base class for ECC encoding/decoding strategies.
 */
class pimEccStrategy {
public:
    virtual ~pimEccStrategy() = default;

    /**
     * @brief Get the total bits required after encoding a block of data.
     */
    virtual unsigned getTotalBits(unsigned dataWidth) const = 0;

    /**
     * @brief Encode a vector of bits.
     */
    virtual std::vector<bool> encode(const std::vector<bool>& data) const = 0;

    /**
     * @brief Decode a vector of bits and attempt correction.
     * @param status [out] 0=OK, 1=Corrected, 2=Uncorrectable.
     */
    virtual std::vector<bool> decode(const std::vector<bool>& encoded, int& status) const = 0;

    /**
     * @brief Encode a uint64_t value (convenience for dataWidth <= 64).
     */
    virtual uint64_t encode(uint64_t data, unsigned dataWidth) const {
      std::vector<bool> v(dataWidth);
      for (unsigned i = 0; i < dataWidth; ++i) v[i] = (data >> i) & 1;
      std::vector<bool> enc = encode(v);
      uint64_t res = 0;
      for (unsigned i = 0; i < enc.size() && i < 64; ++i) if (enc[i]) res |= (1ULL << i);
      return res;
    }

    /**
     * @brief Decode a uint64_t value (convenience for encodedWidth <= 64).
     */
    virtual uint64_t decode(uint64_t encoded, unsigned dataWidth, int& status) const {
      unsigned encWidth = getTotalBits(dataWidth);
      std::vector<bool> v(encWidth);
      for (unsigned i = 0; i < encWidth; ++i) v[i] = (encoded >> i) & 1;
      std::vector<bool> dec = decode(v, status);
      uint64_t res = 0;
      for (unsigned i = 0; i < dec.size() && i < 64; ++i) if (dec[i]) res |= (1ULL << i);
      return res;
    }

    /**
     * @brief Get the name of the ECC scheme.
     */
    virtual std::string getName() const = 0;

    /**
     * @brief Get hardware overhead estimates.
     */
    virtual double getLatencyNs() const = 0;
    virtual double getEnergyPj() const = 0;
};

/**
 * @class pimEccFactory
 * @brief Factory to create ECC strategies based on configuration.
 */
class pimEccFactory {
public:
    static std::unique_ptr<pimEccStrategy> create(const std::string& type, unsigned layers,
                                                   double latencyOverride = 0.0, double energyOverride = 0.0);
};

#endif
