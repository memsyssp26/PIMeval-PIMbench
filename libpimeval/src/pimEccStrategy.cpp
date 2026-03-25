// File: pimEccStrategy.cpp
#include "pimEccStrategy.h"
#include "pimEcc.h"
#include <algorithm>

/**
 * @brief SECDED Strategy
 */
class pimEccSecDedStrategy : public pimEccStrategy {
public:
    pimEccSecDedStrategy(unsigned layers, double latencyOverride = 0.0, double energyOverride = 0.0)
      : m_layers(layers), m_latencyOverride(latencyOverride), m_energyOverride(energyOverride) {}
    unsigned getTotalBits(unsigned dataWidth) const override {
        return pimEcc::getTotalBitsWithEcc(dataWidth, m_layers, "secded");
    }
    std::vector<bool> encode(const std::vector<bool>& data) const override {
        return pimEcc::encodeMultiLayer(data, m_layers, "secded");
    }
    std::vector<bool> decode(const std::vector<bool>& encoded, int& status) const override {
        return pimEcc::decodeMultiLayer(encoded, m_layers, status, "secded");
    }
    uint64_t encode(uint64_t data, unsigned dataWidth) const override {
        return pimEcc::encode(data, dataWidth); // pimEcc already handles layering if we use this legacy call or we can implement it here
    }
    uint64_t decode(uint64_t encoded, unsigned dataWidth, int& status) const override {
        return pimEcc::decode(encoded, dataWidth, status);
    }
    std::string getName() const override { return "secded"; }
    double getLatencyNs() const override { return m_latencyOverride > 0.0 ? m_latencyOverride : 2.0 * m_layers; }
    double getEnergyPj() const override { return m_energyOverride > 0.0 ? m_energyOverride : 2.0 * m_layers; }
private:
    unsigned m_layers;
    double m_latencyOverride;
    double m_energyOverride;
};

/**
 * @brief CRC32 Strategy
 */
class pimEccCrc32Strategy : public pimEccStrategy {
public:
    pimEccCrc32Strategy(unsigned layers, double latencyOverride = 0.0, double energyOverride = 0.0)
      : m_layers(layers), m_latencyOverride(latencyOverride), m_energyOverride(energyOverride) {}
    unsigned getTotalBits(unsigned dataWidth) const override {
        return pimEcc::getTotalBitsWithEcc(dataWidth, m_layers, "crc32");
    }
    std::vector<bool> encode(const std::vector<bool>& data) const override {
        return pimEcc::encodeMultiLayer(data, m_layers, "crc32");
    }
    std::vector<bool> decode(const std::vector<bool>& encoded, int& status) const override {
        return pimEcc::decodeMultiLayer(encoded, m_layers, status, "crc32");
    }
    uint64_t encode(uint64_t data, unsigned dataWidth) const override {
        std::vector<bool> v(dataWidth);
        for (unsigned i = 0; i < dataWidth; ++i) v[i] = (data >> i) & 1;
        std::vector<bool> enc = encode(v);
        uint64_t res = 0;
        for (unsigned i = 0; i < enc.size() && i < 64; ++i) if (enc[i]) res |= (1ULL << i);
        return res;
    }
    uint64_t decode(uint64_t encoded, unsigned dataWidth, int& status) const override {
        unsigned total = getTotalBits(dataWidth);
        std::vector<bool> v(total);
        for (unsigned i = 0; i < total; ++i) v[i] = (encoded >> i) & 1;
        std::vector<bool> dec = decode(v, status);
        uint64_t res = 0;
        for (unsigned i = 0; i < dec.size() && i < 64; ++i) if (dec[i]) res |= (1ULL << i);
        return res;
    }
    std::string getName() const override { return "crc32"; }
    double getLatencyNs() const override { return m_latencyOverride > 0.0 ? m_latencyOverride : 5.0 * m_layers; }
    double getEnergyPj() const override { return m_energyOverride > 0.0 ? m_energyOverride : 5.0 * m_layers; }
private:
    unsigned m_layers;
    double m_latencyOverride;
    double m_energyOverride;
};

/**
 * @brief Reed-Solomon Strategy
 */
class pimEccRsStrategy : public pimEccStrategy {
public:
    pimEccRsStrategy(unsigned layers, double latencyOverride = 0.0, double energyOverride = 0.0)
      : m_layers(layers), m_latencyOverride(latencyOverride), m_energyOverride(energyOverride) {}
    unsigned getTotalBits(unsigned dataWidth) const override {
        return pimEcc::getTotalBitsWithEcc(dataWidth, m_layers, "rs");
    }
    std::vector<bool> encode(const std::vector<bool>& data) const override {
        return pimEcc::encodeMultiLayer(data, m_layers, "rs");
    }
    std::vector<bool> decode(const std::vector<bool>& encoded, int& status) const override {
        return pimEcc::decodeMultiLayer(encoded, m_layers, status, "rs");
    }
    uint64_t encode(uint64_t data, unsigned dataWidth) const override {
        std::vector<bool> v(dataWidth);
        for (unsigned i = 0; i < dataWidth; ++i) v[i] = (data >> i) & 1;
        std::vector<bool> enc = encode(v);
        uint64_t res = 0;
        for (unsigned i = 0; i < enc.size() && i < 64; ++i) if (enc[i]) res |= (1ULL << i);
        return res;
    }
    uint64_t decode(uint64_t encoded, unsigned dataWidth, int& status) const override {
        unsigned total = getTotalBits(dataWidth);
        std::vector<bool> v(total);
        for (unsigned i = 0; i < total; ++i) v[i] = (encoded >> i) & 1;
        std::vector<bool> dec = decode(v, status);
        uint64_t res = 0;
        for (unsigned i = 0; i < dec.size() && i < 64; ++i) if (dec[i]) res |= (1ULL << i);
        return res;
    }
    std::string getName() const override { return "rs"; }
    double getLatencyNs() const override { return m_latencyOverride > 0.0 ? m_latencyOverride : 20.0 * m_layers; }
    double getEnergyPj() const override { return m_energyOverride > 0.0 ? m_energyOverride : 20.0 * m_layers; }
private:
    unsigned m_layers;
    double m_latencyOverride;
    double m_energyOverride;
};

/**
 * @brief Factory Implementation
 */
std::unique_ptr<pimEccStrategy> pimEccFactory::create(const std::string& type, unsigned layers,
                                                       double latencyOverride, double energyOverride) {
    if (type == "secded") {
        return std::make_unique<pimEccSecDedStrategy>(layers, latencyOverride, energyOverride);
    } else if (type == "crc32") {
        return std::make_unique<pimEccCrc32Strategy>(layers, latencyOverride, energyOverride);
    } else if (type == "rs") {
        return std::make_unique<pimEccRsStrategy>(layers, latencyOverride, energyOverride);
    }
    // Default to SECDED if unknown
    return std::make_unique<pimEccSecDedStrategy>(layers, latencyOverride, energyOverride);
}
