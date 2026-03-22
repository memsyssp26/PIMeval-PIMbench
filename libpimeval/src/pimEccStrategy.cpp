// File: pimEccStrategy.cpp
#include "pimEccStrategy.h"
#include "pimEcc.h"
#include <algorithm>

/**
 * @brief SECDED Strategy
 */
class pimEccSecDedStrategy : public pimEccStrategy {
public:
    pimEccSecDedStrategy(unsigned layers) : m_layers(layers) {}
    unsigned getTotalBits(unsigned dataWidth) const override {
        return pimEcc::getTotalBitsWithEcc(dataWidth, m_layers, "secded");
    }
    std::vector<bool> encode(const std::vector<bool>& data) const override {
        return pimEcc::encodeMultiLayer(data, m_layers, "secded");
    }
    std::vector<bool> decode(const std::vector<bool>& encoded, int& status) const override {
        return pimEcc::decodeMultiLayer(encoded, m_layers, status, "secded");
    }
    std::string getName() const override { return "secded"; }
    double getLatencyNs() const override { return 2.0 * m_layers; }
    double getEnergyPj() const override { return 2.0 * m_layers; }
private:
    unsigned m_layers;
};

/**
 * @brief CRC32 Strategy
 */
class pimEccCrc32Strategy : public pimEccStrategy {
public:
    pimEccCrc32Strategy(unsigned layers) : m_layers(layers) {}
    unsigned getTotalBits(unsigned dataWidth) const override {
        return pimEcc::getTotalBitsWithEcc(dataWidth, m_layers, "crc32");
    }
    std::vector<bool> encode(const std::vector<bool>& data) const override {
        return pimEcc::encodeMultiLayer(data, m_layers, "crc32");
    }
    std::vector<bool> decode(const std::vector<bool>& encoded, int& status) const override {
        return pimEcc::decodeMultiLayer(encoded, m_layers, status, "crc32");
    }
    std::string getName() const override { return "crc32"; }
    double getLatencyNs() const override { return 5.0 * m_layers; }
    double getEnergyPj() const override { return 5.0 * m_layers; }
private:
    unsigned m_layers;
};

/**
 * @brief Reed-Solomon Strategy
 */
class pimEccRsStrategy : public pimEccStrategy {
public:
    pimEccRsStrategy(unsigned layers) : m_layers(layers) {}
    unsigned getTotalBits(unsigned dataWidth) const override {
        return pimEcc::getTotalBitsWithEcc(dataWidth, m_layers, "rs");
    }
    std::vector<bool> encode(const std::vector<bool>& data) const override {
        return pimEcc::encodeMultiLayer(data, m_layers, "rs");
    }
    std::vector<bool> decode(const std::vector<bool>& encoded, int& status) const override {
        return pimEcc::decodeMultiLayer(encoded, m_layers, status, "rs");
    }
    std::string getName() const override { return "rs"; }
    double getLatencyNs() const override { return 20.0 * m_layers; }
    double getEnergyPj() const override { return 20.0 * m_layers; }
private:
    unsigned m_layers;
};

/**
 * @brief Factory Implementation
 */
std::unique_ptr<pimEccStrategy> pimEccFactory::create(const std::string& type, unsigned layers) {
    if (type == "secded") {
        return std::make_unique<pimEccSecDedStrategy>(layers);
    } else if (type == "crc32") {
        return std::make_unique<pimEccCrc32Strategy>(layers);
    } else if (type == "rs") {
        return std::make_unique<pimEccRsStrategy>(layers);
    }
    // Default to SECDED if unknown
    return std::make_unique<pimEccSecDedStrategy>(layers);
}
