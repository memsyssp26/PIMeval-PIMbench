/**
 * @file hello_world_pim.cpp
 * @brief A simple example demonstrating the PIMeval API.
 */
#include "libpimeval.h"
#include <iostream>
#include <vector>

int main() {
    std::cout << "--- PIMeval Hello World ---" << std::endl;

    // 1. Initialize PIM device (BitSIMD-V, 1 rank, 1 bank, 2 subarrays, 1024 rows, 1024 cols)
    // ECC is enabled via environment variables for this example
    setenv("PIMEVAL_ECC", "1", 1);
    setenv("PIMEVAL_ECC_TYPE", "secded", 1);
    
    PimStatus status = pimCreateDevice(PIM_DEVICE_BITSIMD_V, 1, 1, 2, 1024, 1024);
    if (status != PIM_OK) {
        std::cerr << "Failed to create PIM device" << std::endl;
        return 1;
    }

    // 2. Allocate PIM objects (Vectors of 128 integers)
    unsigned numElements = 128;
    PimObjId objA = pimAlloc(PIM_ALLOC_AUTO, numElements, PIM_INT32);
    PimObjId objB = pimAlloc(PIM_ALLOC_AUTO, numElements, PIM_INT32);
    PimObjId objC = pimAllocAssociated(objA, PIM_INT32);

    // 3. Prepare host data and copy to device
    std::vector<int32_t> hostA(numElements, 10);
    std::vector<int32_t> hostB(numElements, 20);
    pimCopyHostToDevice(hostA.data(), objA);
    pimCopyHostToDevice(hostB.data(), objB);

    // 4. Perform PIM addition: C = A + B
    status = pimAdd(objA, objB, objC);
    if (status != PIM_OK) {
        std::cerr << "PIM operation failed" << std::endl;
        return 1;
    }

    // 5. Demonstrate Error Injection & Correction
    std::cout << "Injecting a correctable error into Obj C..." << std::endl;
    pimInjectError(objC, 0, 0); // Flip bit 0 of element 0

    // 6. Copy results back to host (ECC will automatically correct the error here)
    std::vector<int32_t> hostC(numElements);
    pimCopyDeviceToHost(objC, hostC.data());

    // 7. Verify (10 + 20 = 30)
    std::cout << "Result at index 0: " << hostC[0] << " (Expected: 30)" << std::endl;

    // 8. Clean up
    pimShowStats();
    pimDeleteDevice();

    return 0;
}
