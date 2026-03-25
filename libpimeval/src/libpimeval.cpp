// File: libpimeval.cpp
// PIMeval Simulator - Library Interface
// Copyright (c) 2024 University of Virginia
// This file is licensed under the MIT License.
// See the LICENSE file in the root of this repository for more details.

#include "libpimeval.h"
#include "pimSim.h"
#include "pimUtils.h"
#include <cstdio>
#include <cstdarg>

//! @brief  Create a PIM device
PimStatus
pimCreateDevice(PimDeviceEnum deviceType, unsigned numRanks, unsigned numBankPerRank, unsigned numSubarrayPerBank, unsigned numRowPerSubarray, unsigned numColPerSubarray, unsigned bufferSize)
{
  return pimSim::get()->createDevice(deviceType, numRanks, numBankPerRank, numSubarrayPerBank, numRowPerSubarray, numColPerSubarray, bufferSize);
}

//! @brief  Create a PIM device from a config file
PimStatus
pimCreateDeviceFromConfig(PimDeviceEnum deviceType, const char* configFilePath)
{
  return pimSim::get()->createDeviceFromConfig(deviceType, configFilePath);
}

//! @brief  Delete PIM device
PimStatus
pimDeleteDevice()
{
  return pimSim::get()->deleteDevice();
}

//! @brief  Check if device is initialized
bool
pimIsInit()
{
  return pimSim::get()->isInit();
}

PimStatus
pimInit(int* argc, char*** argv)
{
  return pimSim::get()->pimInit(argc, argv);
}

//! @brief  Get device properties
PimStatus
pimGetDeviceProperties(PimDeviceProperties* deviceProperties)
{
  return pimSim::get()->getDeviceProperties(deviceProperties);
}

//! @brief  Check if in analysis mode
bool
pimIsAnalysisMode()
{
  return pimSim::get()->isAnalysisMode();
}

//! @brief  Allocate a PIM resource
PimObjId
pimAlloc(PimAllocEnum allocType, uint64_t numElements, PimDataType dataType)
{
  return pimSim::get()->pimAlloc(allocType, numElements, dataType);
}

//! @brief  Allocate a PIM resource, with an associated object as reference
PimObjId
pimAllocAssociated(PimObjId assocId, PimDataType dataType)
{
  return pimSim::get()->pimAllocAssociated(assocId, dataType);
}

//! @brief  Allocate a PIM object of type buffer
PimObjId
pimAllocBuffer(uint32_t numElements, PimDataType dataType)
{
  return pimSim::get()->pimAllocBuffer(numElements, dataType);
}

//! @brief  Free a PIM resource
PimStatus
pimFree(PimObjId obj)
{
  return pimSim::get()->pimFree(obj);
}

//! @brief  Create a PIM resource as a ranged reference to another PIM resource
PimObjId
pimCreateRangedRef(PimObjId refId, uint64_t idxBegin, uint64_t idxEnd)
{
  return pimSim::get()->pimCreateRangedRef(refId, idxBegin, idxEnd);
}

//! @brief  Create a PIM resource as a dual-contact reference to another PIM resource
PimObjId
pimCreateDualContactRef(PimObjId refId)
{
  return pimSim::get()->pimCreateDualContactRef(refId);
}

//! @brief  Copy data from main memory to PIM device for a range of elements within the PIM object
PimStatus
pimCopyHostToDevice(void* src, PimObjId dest, uint64_t idxBegin, uint64_t idxEnd)
{
  return pimSim::get()->pimCopyMainToDevice(src, dest, idxBegin, idxEnd);
}

//! @brief  Copy data from PIM device to main memory for a range of elements within the PIM object
PimStatus
pimCopyDeviceToHost(PimObjId src, void* dest, uint64_t idxBegin, uint64_t idxEnd)
{
  return pimSim::get()->pimCopyDeviceToMain(src, dest, idxBegin, idxEnd);
}

//! @brief  Copy data from main memory to PIM device with type for a range of elements within the PIM object
PimStatus
pimCopyHostToDeviceWithType(PimCopyEnum copyType, void* src, PimObjId dest, uint64_t idxBegin, uint64_t idxEnd)
{
  return pimSim::get()->pimCopyMainToDeviceWithType(copyType, src, dest, idxBegin, idxEnd);
}

//! @brief  Copy data from PIM device to main memory with type for a range of elements within the PIM object
PimStatus
pimCopyDeviceToHostWithType(PimCopyEnum copyType, PimObjId src, void* dest, uint64_t idxBegin, uint64_t idxEnd)
{
  return pimSim::get()->pimCopyDeviceToMainWithType(copyType, src, dest, idxBegin, idxEnd);
}

//! @brief  Copy data from one PIM resource to another
PimStatus
pimCopyDeviceToDevice(PimObjId src, PimObjId dest, uint64_t idxBegin, uint64_t idxEnd)
{
  return pimSim::get()->pimCopyDeviceToDevice(src, dest, idxBegin, idxEnd);
}

//! @brief  Copy data between associated memory objects
PimStatus
pimCopyObjectToObject(PimObjId src, PimObjId dest)
{
  return pimSim::get()->pimCopyObjectToObject(src, dest);
}

//! @brief  Inject error to an element of a PIM object
PimStatus
pimInjectError(PimObjId obj, uint64_t elemIdx, unsigned bitIdx)
{
  return pimSim::get()->pimInjectError(obj, elemIdx, bitIdx);
}

//! @brief  Inject burst error to an element of a PIM object
PimStatus
pimInjectBurstError(PimObjId obj, uint64_t elemIdx, unsigned bitIdx, unsigned length)
{
  return pimSim::get()->pimInjectBurstError(obj, elemIdx, bitIdx, length);
}

PimStatus
pimConvertType(PimObjId src, PimObjId dest)
{
  return pimSim::get()->pimConvertType(src, dest);
}

PimStatus
pimAdd(PimObjId src1, PimObjId src2, PimObjId dest)
{
  return pimSim::get()->pimAdd(src1, src2, dest);
}

PimStatus
pimSub(PimObjId src1, PimObjId src2, PimObjId dest)
{
  return pimSim::get()->pimSub(src1, src2, dest);
}

PimStatus
pimDiv(PimObjId src1, PimObjId src2, PimObjId dest)
{
  return pimSim::get()->pimDiv(src1, src2, dest);
}

PimStatus
pimAbs(PimObjId src, PimObjId dest)
{
  return pimSim::get()->pimAbs(src, dest);
}

PimStatus
pimMul(PimObjId src1, PimObjId src2, PimObjId dest)
{
  return pimSim::get()->pimMul(src1, src2, dest);
}

PimStatus
pimNot(PimObjId src, PimObjId dest)
{
  return pimSim::get()->pimNot(src, dest);
}

PimStatus
pimOr(PimObjId src1, PimObjId src2, PimObjId dest)
{
  return pimSim::get()->pimOr(src1, src2, dest);
}

PimStatus
pimAnd(PimObjId src1, PimObjId src2, PimObjId dest)
{
  return pimSim::get()->pimAnd(src1, src2, dest);
}

PimStatus
pimXor(PimObjId src1, PimObjId src2, PimObjId dest)
{
  return pimSim::get()->pimXor(src1, src2, dest);
}

PimStatus
pimXnor(PimObjId src1, PimObjId src2, PimObjId dest)
{
  return pimSim::get()->pimXnor(src1, src2, dest);
}

PimStatus
pimGT(PimObjId src1, PimObjId src2, PimObjId dest)
{
  return pimSim::get()->pimGT(src1, src2, dest);
}

PimStatus
pimLT(PimObjId src1, PimObjId src2, PimObjId dest)
{
  return pimSim::get()->pimLT(src1, src2, dest);
}

PimStatus
pimEQ(PimObjId src1, PimObjId src2, PimObjId dest)
{
  return pimSim::get()->pimEQ(src1, src2, dest);
}

PimStatus
pimNE(PimObjId src1, PimObjId src2, PimObjId dest)
{
  return pimSim::get()->pimNE(src1, src2, dest);
}

PimStatus
pimMin(PimObjId src1, PimObjId src2, PimObjId dest)
{
  return pimSim::get()->pimMin(src1, src2, dest);
}

PimStatus
pimMax(PimObjId src1, PimObjId src2, PimObjId dest)
{
  return pimSim::get()->pimMax(src1, src2, dest);
}

PimStatus
pimAddScalar(PimObjId src, PimObjId dest, uint64_t scalarValue)
{
  return pimSim::get()->pimAdd(src, dest, scalarValue);
}

PimStatus
pimSubScalar(PimObjId src, PimObjId dest, uint64_t scalarValue)
{
  return pimSim::get()->pimSub(src, dest, scalarValue);
}

PimStatus
pimMulScalar(PimObjId src, PimObjId dest, uint64_t scalarValue)
{
  return pimSim::get()->pimMul(src, dest, scalarValue);
}

PimStatus
pimDivScalar(PimObjId src, PimObjId dest, uint64_t scalarValue)
{
  return pimSim::get()->pimDiv(src, dest, scalarValue);
}

PimStatus
pimAndScalar(PimObjId src, PimObjId dest, uint64_t scalarValue)
{
  return pimSim::get()->pimAnd(src, dest, scalarValue);
}

PimStatus
pimOrScalar(PimObjId src, PimObjId dest, uint64_t scalarValue)
{
  return pimSim::get()->pimOr(src, dest, scalarValue);
}

PimStatus
pimXorScalar(PimObjId src, PimObjId dest, uint64_t scalarValue)
{
  return pimSim::get()->pimXor(src, dest, scalarValue);
}

PimStatus
pimXnorScalar(PimObjId src, PimObjId dest, uint64_t scalarValue)
{
  return pimSim::get()->pimXnor(src, dest, scalarValue);
}

PimStatus
pimGTScalar(PimObjId src, PimObjId dest, uint64_t scalarValue)
{
  return pimSim::get()->pimGT(src, dest, scalarValue);
}

PimStatus
pimLTScalar(PimObjId src, PimObjId dest, uint64_t scalarValue)
{
  return pimSim::get()->pimLT(src, dest, scalarValue);
}

PimStatus
pimEQScalar(PimObjId src, PimObjId dest, uint64_t scalarValue)
{
  return pimSim::get()->pimEQ(src, dest, scalarValue);
}

PimStatus
pimNEScalar(PimObjId src, PimObjId dest, uint64_t scalarValue)
{
  return pimSim::get()->pimNE(src, dest, scalarValue);
}

PimStatus
pimMinScalar(PimObjId src, PimObjId dest, uint64_t scalarValue)
{
  return pimSim::get()->pimMin(src, dest, scalarValue);
}

PimStatus
pimMaxScalar(PimObjId src, PimObjId dest, uint64_t scalarValue)
{
  return pimSim::get()->pimMax(src, dest, scalarValue);
}

PimStatus
pimScaledAdd(PimObjId src1, PimObjId src2, PimObjId dest, uint64_t scalarValue)
{
  return pimSim::get()->pimScaledAdd(src1, src2, dest, scalarValue);
}

PimStatus
pimPopCount(PimObjId src, PimObjId dest)
{
  return pimSim::get()->pimPopCount(src, dest);
}

PimStatus
pimRedSum(PimObjId src, void* sum, uint64_t idxBegin, uint64_t idxEnd)
{
  return pimSim::get()->pimRedSum(src, sum, idxBegin, idxEnd);
}

PimStatus
pimRedMin(PimObjId src, void* min, uint64_t idxBegin, uint64_t idxEnd)
{
  return pimSim::get()->pimRedMin(src, min, idxBegin, idxEnd);
}

PimStatus
pimRedMax(PimObjId src, void* max, uint64_t idxBegin, uint64_t idxEnd)
{
  return pimSim::get()->pimRedMax(src, max, idxBegin, idxEnd);
}

PimStatus
pimBitSliceExtract(PimObjId src, PimObjId destBool, unsigned bitIdx)
{
  return pimSim::get()->pimBitSliceExtract(src, destBool, bitIdx);
}

PimStatus
pimBitSliceInsert(PimObjId srcBool, PimObjId dest, unsigned bitIdx)
{
  return pimSim::get()->pimBitSliceInsert(srcBool, dest, bitIdx);
}

PimStatus
pimCondCopy(PimObjId condBool, PimObjId src, PimObjId dest)
{
  return pimSim::get()->pimCondCopy(condBool, src, dest);
}

PimStatus
pimCondBroadcast(PimObjId condBool, uint64_t scalarBits, PimObjId dest)
{
  return pimSim::get()->pimCondBroadcast(condBool, scalarBits, dest);
}

PimStatus
pimCondSelect(PimObjId condBool, PimObjId src1, PimObjId src2, PimObjId dest)
{
  return pimSim::get()->pimCondSelect(condBool, src1, src2, dest);
}

PimStatus
pimCondSelectScalar(PimObjId condBool, PimObjId src1, uint64_t scalarBits, PimObjId dest)
{
  return pimSim::get()->pimCondSelectScalar(condBool, src1, scalarBits, dest);
}

PimStatus
pimBroadcastInt(PimObjId dest, long long value) { return pimSim::get()->pimBroadcast(dest, (uint64_t)value); }
PimStatus
pimBroadcastUInt(PimObjId dest, unsigned long long value) { return pimSim::get()->pimBroadcast(dest, value); }
PimStatus
pimBroadcastFP(PimObjId dest, float value) { return pimSim::get()->pimBroadcast(dest, value); }

PimStatus
pimRotateElementsRight(PimObjId src)
{
  return pimSim::get()->pimRotateElementsRight(src);
}

PimStatus
pimRotateElementsLeft(PimObjId src)
{
  return pimSim::get()->pimRotateElementsLeft(src);
}

PimStatus
pimShiftElementsRight(PimObjId src)
{
  return pimSim::get()->pimShiftElementsRight(src);
}

PimStatus
pimShiftElementsLeft(PimObjId src)
{
  return pimSim::get()->pimShiftElementsLeft(src);
}

PimStatus
pimShiftBitsRight(PimObjId src, PimObjId dest, unsigned shiftAmount)
{
  return pimSim::get()->pimShiftBitsRight(src, dest, shiftAmount);
}

PimStatus
pimShiftBitsLeft(PimObjId src, PimObjId dest, unsigned shiftAmount)
{
  return pimSim::get()->pimShiftBitsLeft(src, dest, shiftAmount);
}

PimStatus
pimAesSbox(PimObjId src, PimObjId dest, const std::vector<uint8_t>& lut)
{
  return pimSim::get()->pimAesSbox(src, dest, lut);
}

PimStatus
pimAesInverseSbox(PimObjId src, PimObjId dest, const std::vector<uint8_t>& lut)
{
  return pimSim::get()->pimAesInverseSbox(src, dest, lut);
}

PimStatus
pimPrefixSum(PimObjId src, PimObjId dest)
{
  return pimSim::get()->pimPrefixSum(src, dest);
}

PimStatus
pimMAC(PimObjId src1, PimObjId src2, void* dest)
{
  return pimSim::get()->pimMAC(src1, src2, dest);
}

//! @brief  Start timer for a PIM kernel to measure CPU runtime and DRAM refresh
void
pimStartTimer()
{
  pimSim::get()->startKernelTimer();
}

//! @brief  End timer for a PIM kernel
void
pimEndTimer()
{
  pimSim::get()->endKernelTimer();
}

//! @brief  Show PIM simulator stats
void
pimShowStats()
{
  pimSim::get()->showStats();
}

//! @brief  Reset PIM simulator stats
void
pimResetStats()
{
  pimSim::get()->resetStats();
}

PimStatus
pimFuse(PimProg prog)
{
  return pimSim::get()->pimFuse(prog);
}

// BitSIMD v-layout low-level APIs
PimStatus
pimOpReadRowToSa(PimObjId objId, unsigned ofst)
{
  return pimSim::get()->pimOpReadRowToSa(objId, ofst);
}

PimStatus
pimOpWriteSaToRow(PimObjId objId, unsigned ofst)
{
  return pimSim::get()->pimOpWriteSaToRow(objId, ofst);
}

PimStatus
pimOpTRA(PimObjId src1, unsigned ofst1, PimObjId src2, unsigned ofst2, PimObjId src3, unsigned ofst3)
{
  return pimSim::get()->pimOpTRA(src1, ofst1, src2, ofst2, src3, ofst3);
}

PimStatus
pimOpMove(PimObjId objId, PimRowReg src, PimRowReg dest)
{
  return pimSim::get()->pimOpMove(objId, src, dest);
}

PimStatus
pimOpSet(PimObjId objId, PimRowReg dest, bool val)
{
  return pimSim::get()->pimOpSet(objId, dest, val);
}

PimStatus
pimOpNot(PimObjId objId, PimRowReg src, PimRowReg dest)
{
  return pimSim::get()->pimOpNot(objId, src, dest);
}

PimStatus
pimOpAnd(PimObjId objId, PimRowReg src1, PimRowReg src2, PimRowReg dest)
{
  return pimSim::get()->pimOpAnd(objId, src1, src2, dest);
}

PimStatus
pimOpOr(PimObjId objId, PimRowReg src1, PimRowReg src2, PimRowReg dest)
{
  return pimSim::get()->pimOpOr(objId, src1, src2, dest);
}

PimStatus
pimOpNand(PimObjId objId, PimRowReg src1, PimRowReg src2, PimRowReg dest)
{
  return pimSim::get()->pimOpNand(objId, src1, src2, dest);
}

PimStatus
pimOpNor(PimObjId objId, PimRowReg src1, PimRowReg src2, PimRowReg dest)
{
  return pimSim::get()->pimOpNor(objId, src1, src2, dest);
}

PimStatus
pimOpXor(PimObjId objId, PimRowReg src1, PimRowReg src2, PimRowReg dest)
{
  return pimSim::get()->pimOpXor(objId, src1, src2, dest);
}

PimStatus
pimOpXnor(PimObjId objId, PimRowReg src1, PimRowReg src2, PimRowReg dest)
{
  return pimSim::get()->pimOpXnor(objId, src1, src2, dest);
}

PimStatus
pimOpMaj(PimObjId objId, PimRowReg src1, PimRowReg src2, PimRowReg src3, PimRowReg dest)
{
  return pimSim::get()->pimOpMaj(objId, src1, src2, src3, dest);
}

PimStatus
pimOpSel(PimObjId objId, PimRowReg cond, PimRowReg src1, PimRowReg src2, PimRowReg dest)
{
  return pimSim::get()->pimOpSel(objId, cond, src1, src2, dest);
}

PimStatus
pimOpRotateRH(PimObjId objId, PimRowReg src)
{
  return pimSim::get()->pimOpRotateRH(objId, src);
}

PimStatus
pimOpRotateLH(PimObjId objId, PimRowReg src)
{
  return pimSim::get()->pimOpRotateLH(objId, src);
}

// SIMDRAM low-level APIs
PimStatus
pimOpAP(int numSrc, ...)
{
  va_list args;
  va_start(args, numSrc);
  PimStatus status = pimSim::get()->pimOpAP(numSrc, args);
  va_end(args);
  return status;
}

PimStatus
pimOpAAP(int numSrc, int numDest, ...)
{
  va_list args;
  va_start(args, numDest);
  PimStatus status = pimSim::get()->pimOpAAP(numSrc, numDest, args);
  va_end(args);
  return status;
}
