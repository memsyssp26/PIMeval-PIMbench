// File: vec-add.cpp
#include <iostream>
#include <vector>
#include <getopt.h>
#include <stdint.h>
#include <iomanip>
#include <chrono>

#include "libpimeval.h"
#include "util.h"

using namespace std;

typedef int32_t data_t;

struct Params
{
  uint64_t vectorLength;
  char *configFile;
  char *inputFile;
  bool shouldVerify;
};

void usage()
{
  cout << "Usage: ./vec-add.out [-l vector_length] [-c config_file] [-i input_file] [-v]" << endl;
  cout << "    -l: vector length (default: 65536)" << endl;
  cout << "    -c: config file" << endl;
  cout << "    -i: input file" << endl;
  cout << "    -v: should verify (default: false)" << endl;
}

struct Params getInputParams(int argc, char **argv)
{
  struct Params p;
  p.vectorLength = 65536;
  p.configFile = nullptr;
  p.inputFile = nullptr;
  p.shouldVerify = false;

  int c;
  while ((c = getopt(argc, argv, "l:c:i:v")) != -1)
  {
    switch (c)
    {
    case 'l':
      p.vectorLength = strtoull(optarg, NULL, 0);
      break;
    case 'c':
      p.configFile = optarg;
      break;
    case 'i':
      p.inputFile = optarg;
      break;
    case 'v':
      p.shouldVerify = true;
      break;
    default:
      fprintf(stderr, "\nUnrecognized option!\n");
      usage();
      exit(0);
    }
  }
  return p;
}

void vectorAddition(uint64_t vectorLength, std::vector<int> &src1, std::vector<int> &src2, std::vector<int> &dst)
{
  PimObjId srcObj1 = pimAlloc(PIM_ALLOC_AUTO, vectorLength, PIM_INT32);
  if (srcObj1 == -1)
  {
    std::cout << "Abort" << std::endl;
    return;
  }
  PimObjId srcObj2 = pimAllocAssociated(srcObj1, PIM_INT32);
  if (srcObj2 == -1)
  {
    std::cout << "Abort" << std::endl;
    return;
  }

  PimStatus status = pimCopyHostToDevice((void *)src1.data(), srcObj1);
  if (status != PIM_OK)
  {
    std::cout << "Abort" << std::endl;
    return;
  }

  status = pimCopyHostToDevice((void *)src2.data(), srcObj2);
  if (status != PIM_OK)
  {
    std::cout << "Abort" << std::endl;
    return;
  }

  status = pimAdd(srcObj1, srcObj2, srcObj1);
  if (status != PIM_OK)
  {
    std::cout << "Abort" << std::endl;
    return;
  }

  dst.resize(vectorLength);
  status = pimCopyDeviceToHost(srcObj1, (void *)dst.data());
  if (status != PIM_OK)
  {
    std::cout << "Abort" << std::endl;
  }
  pimFree(srcObj1);
  pimFree(srcObj2);
}

int main(int argc, char **argv)
{
  // Parse --pim-* args first so they're stripped before getopt sees them
  pimInit(&argc, &argv);
  struct Params params = getInputParams(argc, argv);
  std::cout << "Running Vector Add on PIM for vector length: " << params.vectorLength << "\n\n";
  std::vector<int> src1(params.vectorLength, 1), src2(params.vectorLength, 2), dst;
  if (params.shouldVerify) {  
    if (params.inputFile == nullptr)
    {
      getVector(params.vectorLength, src1);
      getVector(params.vectorLength, src2);
    } else {
      std::cout << "Reading from input file is not implemented yet." << std::endl;
      return 1;
    }
  }
  if (!createDevice(params.configFile)) return 1;
  //TODO: Check if vector can fit in one iteration. Otherwise need to run addition in multiple iteration.
  vectorAddition(params.vectorLength, src1, src2, dst);
  if (params.shouldVerify) {
    // verify result
    #pragma omp parallel for
    for (unsigned i = 0; i < params.vectorLength; ++i)
    {
      int sum = src1[i] + src2[i];
      if (dst[i] != sum)
      {
        std::cout << "Wrong answer for addition: " << src1[i] << " + " << src2[i] << " = " << dst[i] << " (expected " << sum << ")" << std::endl;
      }
    }
  }

  pimShowStats();

  return 0;
}
