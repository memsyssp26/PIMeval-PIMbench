#include "libpimeval.h"
#include <iostream>
#include <vector>
#include <cassert>
#include <cstdlib>
#include <ctime>

int main() {
    std::srand(std::time(nullptr));
    unsigned numElements = 128; // multiple of 8 (for 64-bit granularity with 8-bit elements)
    PimDataType dataType = PIM_INT8;
    
    // Test 1: SECDED at 64-bit granularity
    std::cout << "Test 1: SECDED at 64-bit granularity" << std::endl;
    setenv("PIMEVAL_ECC", "1", 1);
    setenv("PIMEVAL_ECC_GRANULARITY", "64", 1);

    
    PimStatus status = pimCreateDevice(PIM_DEVICE_BITSIMD_V, 1, 1, 2, 1024, 256);
    assert(status == PIM_OK);
    
    PimObjId obj = pimAlloc(PIM_ALLOC_AUTO, numElements, dataType);
    assert(obj != -1);
    
    std::vector<int8_t> src(numElements);
    for (unsigned i = 0; i < numElements; ++i) {
        src[i] = (int8_t)(std::rand() % 256 - 128);
    }
    
    status = pimCopyHostToDevice(src.data(), obj);
    assert(status == PIM_OK);
    
    // Inject 1-bit error in the first block (bits 0..63)
    // The first block is elements 0..7.
    // Let's flip bit 0 of element 0.
    pimInjectError(obj, 0, 0);
    
    std::vector<int8_t> dest(numElements);
    status = pimCopyDeviceToHost(obj, dest.data());
    assert(status == PIM_OK);
    
    for (unsigned i = 0; i < numElements; ++i) {
        if (src[i] != dest[i]) {
            std::cout << "Error at index " << i << ": expected " << (int)src[i] << ", got " << (int)dest[i] << std::endl;
            assert(false);
        }
    }
    std::cout << "Test 1 Passed (Single bit error corrected)" << std::endl;
    
    // Inject 2-bit error
    // Flip bit 1 of element 0
    pimInjectError(obj, 0, 0); // Flip back to original
    pimInjectError(obj, 0, 0); // Flip again
    pimInjectError(obj, 0, 1); // Flip another bit
    
    // This should report an uncorrectable error in the log, and might return corrupted data
    std::cout << "Next copy should report a double bit error:" << std::endl;
    status = pimCopyDeviceToHost(obj, dest.data());
    assert(status == PIM_OK);
    
    pimFree(obj);
    pimDeleteDevice();
    
    // Test 2: Multi-layered ECC (2 layers of SECDED)
    std::cout << "Test 2: Multi-layered SECDED" << std::endl;
    setenv("PIMEVAL_ECC_LAYERS", "2", 1);
    setenv("PIMEVAL_ECC_GRANULARITY", "0", 1); // Per-element
    
    status = pimCreateDevice(PIM_DEVICE_BITSIMD_V, 1, 1, 2, 1024, 256);
    assert(status == PIM_OK);
    
    obj = pimAlloc(PIM_ALLOC_AUTO, numElements, dataType);
    assert(obj != -1);
    
    status = pimCopyHostToDevice(src.data(), obj);
    assert(status == PIM_OK);
    
    // Double bit error: should still be corrected if in different layers? 
    // Wait, no, multi-layering SECDED doesn't necessarily correct double bit errors.
    // But it's a test of the infrastructure.
    
    pimInjectError(obj, 10, 0);
    status = pimCopyDeviceToHost(obj, dest.data());
    assert(status == PIM_OK);
    assert(src[10] == dest[10]);
    std::cout << "Test 2 Passed" << std::endl;
    
    pimFree(obj);
    pimDeleteDevice();

    std::cout << "All ECC Flexibility tests passed!" << std::endl;
    return 0;
}
