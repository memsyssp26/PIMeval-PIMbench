#include "libpimeval.h"
#include <iostream>
#include <vector>
#include <cassert>

void test_hardening(const std::string& eccType, bool expectCorrection) {
    std::cout << "\n>>> Hardening Test: ECC=" << eccType << " <<<" << std::endl;
    setenv("PIMEVAL_ECC", "1", 1);
    setenv("PIMEVAL_ECC_TYPE", eccType.c_str(), 1);
    setenv("PIMEVAL_ECC_GRANULARITY", "64", 1);

    pimCreateDevice(PIM_DEVICE_BITSIMD_V, 1, 1, 2, 1024, 1024);
    
    unsigned numElements = 64;
    PimObjId obj = pimAlloc(PIM_ALLOC_AUTO, numElements, PIM_INT32);
    
    std::vector<int32_t> src(numElements, 0x12345678);
    pimCopyHostToDevice(src.data(), obj);

    // Inject 1-bit error (Should be corrected by SECDED and RS)
    std::cout << "Injecting 1-bit error at element 0..." << std::endl;
    pimInjectError(obj, 0, 0);

    // Inject 4-bit burst error at element 10 (Should be UNCORRECTABLE by SECDED, but detectable by CRC/RS)
    std::cout << "Injecting 4-bit burst error at element 10..." << std::endl;
    pimInjectBurstError(obj, 10, 0, 4);

    std::vector<int32_t> dest(numElements);
    pimCopyDeviceToHost(obj, dest.data());

    pimShowStats();
    pimDeleteDevice();
}

int main() {
    test_hardening("secded", true);
    test_hardening("crc32", false);
    test_hardening("rs", true);
    return 0;
}
