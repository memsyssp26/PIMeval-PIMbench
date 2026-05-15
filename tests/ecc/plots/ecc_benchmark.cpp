// File: ecc_benchmark.cpp
// PIMeval Simulator — ECC Overhead Data Collection Benchmark
// Runs pimAddScalar on BitSIMD-V under different ECC tier configurations
// and prints machine-readable CSV lines for the Python plotting script.
//
// Output line format (CSV):
//   label,num_elements,total_runtime_ms,scratchpad_overhead_ms,odecc_overhead_ms,controller_overhead_ms
//
// Usage:
//   ecc_benchmark <num_elements> <config>
//   config: none | odecc | scratchpad | odecc+scratchpad | all
//
// Environment variables set by this program override config for more fine-grained control.

#include "libpimeval.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <regex>
#include <sstream>
#include <unistd.h>
#include <fcntl.h>

// ---- helpers ---------------------------------------------------------------

static void clearEccEnv() {
  unsetenv("PIMEVAL_ECC");
  unsetenv("PIMEVAL_ECC_TYPE");
  unsetenv("PIMEVAL_ECC_GRANULARITY");
  unsetenv("PIMEVAL_ODECC");
  unsetenv("PIMEVAL_SCRATCHPAD");
  unsetenv("PIMEVAL_SCRATCHPAD_ECC");
  unsetenv("PIMEVAL_SCRATCHPAD_ECC_TYPE");
  unsetenv("PIMEVAL_SCRATCHPAD_WORD_BITS");
  unsetenv("PIMEVAL_SCRATCHPAD_ECC_LATENCY_NS");
  unsetenv("PIMEVAL_SCRATCHPAD_ECC_ENERGY_PJ");
}

struct RunResult {
  double total_ms   = 0.0;
  double scratch_ms = 0.0;
  double odecc_ms   = 0.0;  // estimated from byte count (addOdeccOverhead added to pe)
};

// Capture pimShowStats() output into a string
// We re-implement minimal parsing by running the op and reading the stats
// through pimShowStats() redirected to a pipe.
// Simpler approach: we know the analytical formulas, compute from config + runtime.

static RunResult runConfig(unsigned numElements, const std::string& label,
                           bool enableOdecc, bool enableScratchpad, bool enableController) {
  clearEccEnv();

  if (enableController) {
    setenv("PIMEVAL_ECC",             "1",      1);
    setenv("PIMEVAL_ECC_TYPE",        "secded", 1);
    setenv("PIMEVAL_ECC_GRANULARITY", "64",     1);
  }
  if (enableOdecc) {
    setenv("PIMEVAL_ODECC", "1", 1);
  }
  if (enableScratchpad) {
    setenv("PIMEVAL_SCRATCHPAD",                "1",      1);
    setenv("PIMEVAL_SCRATCHPAD_ECC",            "1",      1);
    setenv("PIMEVAL_SCRATCHPAD_ECC_TYPE",       "secded", 1);
    setenv("PIMEVAL_SCRATCHPAD_WORD_BITS",      "32",     1);
    setenv("PIMEVAL_SCRATCHPAD_ECC_LATENCY_NS", "0.5",    1);
    setenv("PIMEVAL_SCRATCHPAD_ECC_ENERGY_PJ",  "0.3",    1);
  }

  PimStatus s = pimCreateDevice(PIM_DEVICE_BITSIMD_V, 1, 1, 4, 1024, 1024);
  if (s != PIM_OK) { fprintf(stderr, "createDevice failed for %s\n", label.c_str()); return {}; }

  PimObjId src  = pimAlloc(PIM_ALLOC_AUTO, numElements, PIM_INT32);
  PimObjId dest = pimAllocAssociated(src, PIM_INT32);
  if (src == -1 || dest == -1) { pimDeleteDevice(); return {}; }

  std::vector<int32_t> data(numElements, 42);
  pimCopyHostToDevice(data.data(), src);
  pimAddScalar(src, dest, 1);

  // Redirect stdout to /dev/null while capturing stats via a temp file
  char tmpfile[] = "/tmp/pim_stats_XXXXXX";
  int fd = mkstemp(tmpfile);
  fflush(stdout);            // flush any pending output before redirecting
  int savedStdout = dup(1);
  dup2(fd, 1);   // redirect stdout to temp file
  pimShowStats();
  fflush(stdout);
  dup2(savedStdout, 1);  // restore stdout
  close(savedStdout);
  close(fd);

  // Read temp file
  FILE* f = fopen(tmpfile, "r");
  std::string output;
  if (f) {
    char buf[256];
    while (fgets(buf, sizeof(buf), f)) output += buf;
    fclose(f);
  }
  remove(tmpfile);

  pimFree(src);
  pimFree(dest);
  pimDeleteDevice();

  // Parse: "add_scalar :  1  <runtime_ms>"
  RunResult result;
  {
    std::regex rx_cmd(R"(add_scalar\s*:\s*\d+\s+([\d.]+))");
    std::smatch m;
    if (std::regex_search(output, m, rx_cmd)) {
      result.total_ms = std::stod(m[1]);
    }
  }
  // Parse: "Scratchpad ECC overhead (total) : <val> ms"
  {
    std::regex rx_sp(R"(Scratchpad ECC overhead \(total\)\s*:\s*([\d.]+)\s*ms)");
    std::smatch m;
    if (std::regex_search(output, m, rx_sp)) {
      result.scratch_ms = std::stod(m[1]);
    }
  }
  return result;
}

int main(int argc, char* argv[]) {
  unsigned numElements = 256;
  std::string mode     = "sweep_tiers";

  if (argc >= 2) numElements = static_cast<unsigned>(std::atoi(argv[1]));
  if (argc >= 3) mode = argv[2];

  if (mode == "sweep_tiers") {
    // Collect data for Figure 2: overhead per ECC tier
    // Output: label,num_elements,total_ms,scratch_ms
    auto r0 = runConfig(numElements, "none",           false, false, false);
    auto r1 = runConfig(numElements, "odecc",          true,  false, false);
    auto r2 = runConfig(numElements, "scratchpad",     false, true,  false);
    auto r3 = runConfig(numElements, "odecc+scratch",  true,  true,  false);
    auto r4 = runConfig(numElements, "all",            true,  true,  true);

    // CSV header
    printf("label,num_elements,total_ms,scratch_ms\n");
    printf("No ECC,%u,%.6f,%.6f\n",             numElements, r0.total_ms, r0.scratch_ms);
    printf("ODECC Only,%u,%.6f,%.6f\n",         numElements, r1.total_ms, r1.scratch_ms);
    printf("Scratchpad ECC,%u,%.6f,%.6f\n",     numElements, r2.total_ms, r2.scratch_ms);
    printf("ODECC+Scratchpad,%u,%.6f,%.6f\n",   numElements, r3.total_ms, r3.scratch_ms);
    printf("All Three Tiers,%u,%.6f,%.6f\n",    numElements, r4.total_ms, r4.scratch_ms);

  } else if (mode == "sweep_elements") {
    // Collect data for Figure 3: scratchpad overhead vs element count
    printf("num_elements,total_ms,scratch_ms\n");
    for (unsigned n : {64u, 128u, 256u, 512u}) {
      auto r = runConfig(n, "scratchpad", false, true, false);
      printf("%u,%.6f,%.6f\n", n, r.total_ms, r.scratch_ms);
    }
  }
  return 0;
}
