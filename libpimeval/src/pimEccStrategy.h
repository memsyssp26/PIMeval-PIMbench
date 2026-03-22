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
    static std::unique_ptr<pimEccStrategy> create(const std::string& type, unsigned layers);
};

#endif
