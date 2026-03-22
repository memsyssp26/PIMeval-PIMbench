#include "libpimeval.h"
#include <iostream>
#include <vector>
#include <cassert>
#include <string>

void run_system_test(const std::string& type, unsigned granularity) {
    std::cout << "Integration Test: " << type << " @ " << granularity << "-bit" << std::endl;
    
    setenv("PIMEVAL_ECC", "1", 1);
    setenv("PIMEVAL_ECC_TYPE", type.c_str(), 1);
    setenv("PIMEVAL_ECC_GRANULARITY", std::to_string(granularity).c_str(), 1);

    PimStatus status = pimCreateDevice(PIM_DEVICE_BITSIMD_V, 1, 1, 2, 1024, 1024);
    assert(status == PIM_OK);

    unsigned numElements = 256;
    PimObjId obj = pimAlloc(PIM_ALLOC_AUTO, numElements, PIM_INT32);
    assert(obj != -1);

    std::vector<int32_t> src(numElements);
    for (unsigned i = 0; i < numElements; ++i) src[i] = i * 2;

    status = pimCopyHostToDevice(src.data(), obj);
    assert(status == PIM_OK);

    // Verify correct restoration
    std::vector<int32_t> dest(numElements, 0);
    status = pimCopyDeviceToHost(obj, dest.data());
    assert(status == PIM_OK);

    for (unsigned i = 0; i < numElements; ++i) {
        if (src[i] != dest[i]) {
            std::cerr << "Data mismatch at index " << i << " expected " << src[i] << " got " << dest[i] << std::endl;
            assert(false);
        }
    }

    pimFree(obj);
    pimDeleteDevice();
    std::cout << "  -> Integration Passed." << std::endl;
}

void run_bank_level_test() {
    std::cout << "Integration Test: Bank Level @ 64-bit" << std::endl;
    setenv("PIMEVAL_ECC", "1", 1);
    setenv("PIMEVAL_ECC_TYPE", "secded", 1);
    setenv("PIMEVAL_ECC_GRANULARITY", "64", 1);

    // Small bank level device
    PimStatus status = pimCreateDevice(PIM_DEVICE_BANK_LEVEL, 1, 1, 1, 1024, 1024);
    assert(status == PIM_OK);
    
    unsigned numElements = 128;
    PimObjId obj = pimAlloc(PIM_ALLOC_AUTO, numElements, PIM_INT32);
    assert(obj != -1);
    pimFree(obj);
    pimDeleteDevice();
    std::cout << "  -> Bank Level Passed." << std::endl;
}

int main() {
    run_system_test("secded", 32);
    run_system_test("secded", 64);
    run_system_test("crc32", 128);
    run_system_test("rs", 64);
    run_bank_level_test();
    std::cout << "All System Integration Tests Passed!" << std::endl;
    return 0;
}
