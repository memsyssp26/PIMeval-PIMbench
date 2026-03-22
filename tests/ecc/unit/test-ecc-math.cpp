#include "../../../libpimeval/src/pimEcc.h"
#include <iostream>
#include <vector>
#include <cassert>

void test_secded_math() {
    std::cout << "Running Unit Test: SECDED Math..." << std::endl;
    std::vector<bool> data = {1, 0, 1, 1, 0, 1, 1, 0}; // 8-bit data
    auto encoded = pimEcc::encode(data);
    
    // Check bits: 8 data + 4 Hamming + 1 DED = 13 bits
    assert(encoded.size() == 13);

    int status = 0;
    auto decoded = pimEcc::decode(encoded, status);
    assert(status == 0);
    assert(decoded == data);

    // Test Single Bit Correction
    for (size_t i = 0; i < encoded.size(); ++i) {
        auto corrupted = encoded;
        corrupted[i] = !corrupted[i];
        int s = 0;
        auto fixed = pimEcc::decode(corrupted, s);
        assert(s == 1);
        assert(fixed == data);
    }

    // Test Double Bit Detection
    auto double_corrupted = encoded;
    double_corrupted[0] = !double_corrupted[0];
    double_corrupted[5] = !double_corrupted[5];
    int s2 = 0;
    pimEcc::decode(double_corrupted, s2);
    assert(s2 == 2);
    
    std::cout << "  -> SECDED Math Passed." << std::endl;
}

void test_crc_math() {
    std::cout << "Running Unit Test: CRC-32 Math..." << std::endl;
    std::vector<bool> data = {1, 1, 0, 0, 1, 0, 1, 0};
    auto encoded = pimEcc::encodeCRC32(data);
    assert(encoded.size() == data.size() + 32);

    int status = 0;
    auto decoded = pimEcc::decodeCRC32(encoded, status);
    assert(status == 0);
    assert(decoded == data);

    // Test Detection
    encoded[5] = !encoded[5];
    int s = 0;
    pimEcc::decodeCRC32(encoded, s);
    assert(s == 2);
    
    std::cout << "  -> CRC-32 Math Passed." << std::endl;
}

int main() {
    test_secded_math();
    test_crc_math();
    std::cout << "All ECC Unit Tests Passed!" << std::endl;
    return 0;
}
