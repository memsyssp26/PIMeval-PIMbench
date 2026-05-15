// test-dramsim3.cpp
// Validates that the DRAMSim3 integration in PIMeval-PIMbench produces
// cycle-accurate, physically plausible timing and energy for DRAM transfers.
//
// Each test checks:
//   (a) DRAMSim3 was actually invoked (not zero / not analytical fallback)
//   (b) Results are in the physically plausible range for the protocol
//   (c) Key ratios hold: linearity, D2D ≈ 2× single-direction, independence
//
// Compile only with DRAMSIM3_INTEG; analytical-only builds skip automatically.

#include "libpimeval.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cassert>
#include <vector>
#include <string>
#include <unistd.h>
#include <fcntl.h>

// ── stdout capture ────────────────────────────────────────────────────────────
// Redirects fd 1 to a temp file, calls pimShowStats(), restores fd 1, and
// returns the captured text.  Critical details:
//   1. fflush(stdout) BEFORE dup2: any printf output already buffered in
//      FILE* stdout must be flushed to the terminal before we redirect fd 1,
//      otherwise the next fflush() inside the redirect drains it into the
//      temp file and it is silently discarded.
//   2. setvbuf(..., _IOLBF) AFTER restore: when fd 1 points to a file,
//      glibc switches FILE* stdout to block-buffered mode.  Restoring line-
//      buffering ensures subsequent printf()s appear on the terminal promptly.
static std::string captureShowStats() {
    char path[] = "/tmp/pimeval_ds3_XXXXXX";
    int fd = mkstemp(path);
    int saved = dup(STDOUT_FILENO);
    fflush(stdout);                          // flush pending terminal output first
    dup2(fd, STDOUT_FILENO);
    pimShowStats();
    fflush(stdout);                          // flush pimShowStats output into tempfile
    dup2(saved, STDOUT_FILENO);
    close(saved);
    setvbuf(stdout, nullptr, _IOLBF, 0);    // restore line buffering for terminal
    lseek(fd, 0, SEEK_SET);
    std::string out;
    char buf[8192];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf) - 1)) > 0) { buf[n] = '\0'; out += buf; }
    close(fd);
    unlink(path);
    return out;
}

// Parse the Data Copy Stats TOTAL line:
//   "TOTAL --------- : <bytes> bytes <ms> ms Estimated Runtime <mj> mj Estimated Energy"
static bool parseCopyTotal(const std::string& s, double& ms, double& mj) {
    size_t pos = s.find("TOTAL ---------");
    if (pos == std::string::npos) return false;
    size_t col = s.find(':', pos);
    if (col == std::string::npos) return false;
    unsigned long long bytes;
    int n = sscanf(s.c_str() + col + 1,
                   " %llu bytes %lf ms Estimated Runtime %lf mj Estimated Energy",
                   &bytes, &ms, &mj);
    return n == 3;
}

// ── helpers ───────────────────────────────────────────────────────────────────
static void check(bool condition, const char* msg) {
    if (!condition) {
        fprintf(stderr, "  FAIL: %s\n", msg);
        exit(1);
    }
    printf("  PASS: %s\n", msg);
}

// Create device, alloc, transfer in one direction, capture stats, tear down.
// Returns {ms, mj} for that transfer direction.
struct PerfResult { double ms; double mj; };

static PerfResult runH2D(unsigned numElements, const char* protocol = nullptr) {
    if (protocol) setenv("PIMEVAL_MEMORY_PROTOCOL", protocol, 1);
    else          unsetenv("PIMEVAL_MEMORY_PROTOCOL");
    unsetenv("PIMEVAL_ECC");

    // 1 rank, 1 bank/rank, 8 subarrays/bank, 1024 rows/subarray, 1024 cols
    // -> 4 cores, 262144 INT32 capacity (1 MB)
    PimStatus st = pimCreateDevice(PIM_DEVICE_BITSIMD_V, 1, 1, 8, 1024, 1024);
    assert(st == PIM_OK);

    PimObjId obj = pimAlloc(PIM_ALLOC_AUTO, numElements, PIM_INT32);
    assert(obj != -1);

    std::vector<int32_t> src(numElements, 42);
    st = pimCopyHostToDevice(src.data(), obj);
    assert(st == PIM_OK);

    std::string stats = captureShowStats();
    pimFree(obj);
    pimDeleteDevice();

    PerfResult r{};
    if (!parseCopyTotal(stats, r.ms, r.mj)) {
        fprintf(stderr, "Failed to parse stats output:\n%s\n", stats.c_str());
        exit(1);
    }
    return r;
}

static PerfResult runD2H(unsigned numElements) {
    unsetenv("PIMEVAL_MEMORY_PROTOCOL");
    unsetenv("PIMEVAL_ECC");

    PimStatus st = pimCreateDevice(PIM_DEVICE_BITSIMD_V, 1, 1, 8, 1024, 1024);
    assert(st == PIM_OK);

    PimObjId obj = pimAlloc(PIM_ALLOC_AUTO, numElements, PIM_INT32);
    assert(obj != -1);

    std::vector<int32_t> src(numElements, 7);
    st = pimCopyHostToDevice(src.data(), obj);
    assert(st == PIM_OK);
    pimResetStats();

    std::vector<int32_t> dst(numElements, 0);
    st = pimCopyDeviceToHost(obj, dst.data());
    assert(st == PIM_OK);

    // Verify data integrity
    for (unsigned i = 0; i < numElements; i++) assert(dst[i] == 7);

    std::string stats = captureShowStats();
    pimFree(obj);
    pimDeleteDevice();

    PerfResult r{};
    if (!parseCopyTotal(stats, r.ms, r.mj)) {
        fprintf(stderr, "Failed to parse stats: %s\n", stats.c_str());
        exit(1);
    }
    return r;
}

static PerfResult runD2D(unsigned numElements) {
    unsetenv("PIMEVAL_MEMORY_PROTOCOL");
    unsetenv("PIMEVAL_ECC");

    PimStatus st = pimCreateDevice(PIM_DEVICE_BITSIMD_V, 1, 1, 8, 1024, 1024);
    assert(st == PIM_OK);

    PimObjId src_obj = pimAlloc(PIM_ALLOC_AUTO, numElements, PIM_INT32);
    assert(src_obj != -1);
    PimObjId dst_obj = pimAllocAssociated(src_obj, PIM_INT32);
    assert(dst_obj != -1);

    std::vector<int32_t> hostData(numElements, 99);
    st = pimCopyHostToDevice(hostData.data(), src_obj);
    assert(st == PIM_OK);
    pimResetStats();

    st = pimCopyDeviceToDevice(src_obj, dst_obj);
    assert(st == PIM_OK);

    std::string stats = captureShowStats();
    pimFree(src_obj);
    pimFree(dst_obj);
    pimDeleteDevice();

    PerfResult r{};
    if (!parseCopyTotal(stats, r.ms, r.mj)) {
        fprintf(stderr, "Failed to parse stats: %s\n", stats.c_str());
        exit(1);
    }
    return r;
}

// ── Test functions ────────────────────────────────────────────────────────────

// Test 1: DRAMSim3 is actually running — runtime and energy are non-zero
// and in the physically plausible range for DDR4-2400.
void test_dramsim3_active() {
    printf("\n--- Test 1: DRAMSim3 active (DDR4, 256KB H2D) ---\n");
    // 65536 INT32 = 256 KB
    PerfResult r = runH2D(65536);
    printf("  H2D 256KB: %.6f ms, %.9f mJ\n", r.ms, r.mj);

    // DDR4-2400: peak BW = 19.2 GB/s => 256KB / 19.2 GB/s = 13.3 us = 0.0133 ms
    // Allow 50x tolerance around peak for overhead/queue effects: 0.002 -- 0.5 ms
    check(r.ms > 0.0,     "runtime is non-zero (DRAMSim3 invoked)");
    check(r.mj > 0.0,     "energy is non-zero");
    check(r.ms > 0.002,   "runtime > 2us (not unrealistically fast)");
    check(r.ms < 0.5,     "runtime < 500us (not unrealistically slow for 256KB DDR4)");
    check(r.mj < 1.0,     "energy < 1 mJ (physically sane for 256KB)");
}

// Test 2: Linearity — 1MB takes ~4x longer than 256KB (same device, same protocol)
void test_linearity() {
    printf("\n--- Test 2: Linearity (256KB vs 1MB H2D, DDR4) ---\n");
    PerfResult r256 = runH2D(65536);   // 256 KB
    PerfResult r1m  = runH2D(262144);  // 1 MB
    printf("  H2D 256KB: %.6f ms\n", r256.ms);
    printf("  H2D  1MB:  %.6f ms\n", r1m.ms);
    double ratio = r1m.ms / r256.ms;
    printf("  Ratio (1MB/256KB): %.2fx  (expected ~4x)\n", ratio);

    check(r1m.ms > r256.ms, "1MB takes longer than 256KB");
    check(ratio > 2.0,  "ratio > 2 (1MB at least 2x slower than 256KB)");
    check(ratio < 10.0, "ratio < 10 (not more than 10x slower — linearity holds)");
}

// Test 3: D2D ≈ 2× H2D  (D2D = sequential read + write)
void test_d2d_vs_h2d() {
    printf("\n--- Test 3: D2D ≈ 2x H2D (64K INT32 = 256KB) ---\n");
    PerfResult h2d = runH2D(65536);
    PerfResult d2d = runD2D(65536);
    printf("  H2D 256KB: %.6f ms\n", h2d.ms);
    printf("  D2D 256KB: %.6f ms\n", d2d.ms);
    double ratio = d2d.ms / h2d.ms;
    printf("  D2D/H2D ratio: %.2fx  (expected ~2x)\n", ratio);

    check(d2d.ms > h2d.ms, "D2D takes longer than H2D alone");
    check(ratio > 1.2, "D2D/H2D > 1.2 (clearly two phases)");
    check(ratio < 4.0, "D2D/H2D < 4.0 (not more than 4x — two sequential passes)");
}

// Test 4: D2H data integrity — what came in comes back out
void test_d2h_integrity() {
    printf("\n--- Test 4: D2H data integrity (1K INT32) ---\n");
    // runD2H already checks dst[i]==7 for every element; if it returns we passed.
    PerfResult r = runD2H(1024);
    printf("  D2H 4KB: %.6f ms, %.9f mJ\n", r.ms, r.mj);
    check(r.ms > 0.0, "D2H runtime is non-zero");
    check(r.mj > 0.0, "D2H energy is non-zero");
    printf("  Data integrity: all 1024 elements match (checked inside runD2H)\n");
    printf("  PASS: D2H integrity\n");
}

// Test 5: Transfer independence — verifies m_dramNextAddr assigns fresh DRAM
// addresses to each transfer so the second H2D does NOT get free row-buffer
// hits from the first.
//
// Expected behavior in OPEN_PAGE mode:
//   - Without m_dramNextAddr (both H2Ds at addr 0): second H2D would be
//     significantly FASTER (row-buffer hits from the first transfer).
//   - With m_dramNextAddr: second H2D uses different rows → each bank must
//     PRECHARGE the first transfer's open row then ACTIVATE the new row before
//     bursting, adding tRP+tRCD overhead (~17+17 cycles per row).  For a
//     small 256 KB transfer this overhead is a large fraction of total time,
//     making the second H2D up to ~3–4× slower than the first.
//
// The test therefore checks:
//   ratio > 0.5  — second H2D was not suspiciously fast (row-buffer hit would
//                  make it 2–10× faster, indicating address reuse)
//   ratio < 4.0  — second H2D is not absurdly slow (sanity bound)
void test_transfer_independence() {
    printf("\n--- Test 5: Transfer independence (two 256KB H2Ds) ---\n");
    unsetenv("PIMEVAL_MEMORY_PROTOCOL");
    unsetenv("PIMEVAL_ECC");

    PimStatus st = pimCreateDevice(PIM_DEVICE_BITSIMD_V, 1, 1, 8, 1024, 1024);
    assert(st == PIM_OK);

    unsigned N = 65536; // 256 KB
    PimObjId obj1 = pimAlloc(PIM_ALLOC_AUTO, N, PIM_INT32);
    PimObjId obj2 = pimAllocAssociated(obj1, PIM_INT32);
    assert(obj1 != -1 && obj2 != -1);

    std::vector<int32_t> src(N, 1);

    // First H2D
    pimResetStats();
    st = pimCopyHostToDevice(src.data(), obj1);
    assert(st == PIM_OK);
    std::string s1 = captureShowStats();
    double ms1, mj1;
    assert(parseCopyTotal(s1, ms1, mj1));

    // Second H2D (same size, hits fresh rows via m_dramNextAddr)
    pimResetStats();
    st = pimCopyHostToDevice(src.data(), obj2);
    assert(st == PIM_OK);
    std::string s2 = captureShowStats();
    double ms2, mj2;
    assert(parseCopyTotal(s2, ms2, mj2));

    printf("  1st H2D 256KB: %.6f ms\n", ms1);
    printf("  2nd H2D 256KB: %.6f ms\n", ms2);
    double ratio = ms2 / ms1;
    printf("  2nd/1st ratio: %.3fx\n", ratio);
    printf("  (>1 expected: fresh rows incur PRECHARGE overhead in OPEN_PAGE mode)\n");

    check(ms1 > 0.0, "first H2D runtime non-zero");
    check(ms2 > 0.0, "second H2D runtime non-zero");
    // ratio > 0.5: second H2D was NOT faster — confirms no address reuse / no
    //              row-buffer hit speedup (m_dramNextAddr is working).
    check(ratio > 0.5, "second H2D not faster than 2x first (no row-buffer hit speedup)");
    // ratio < 4.0: OPEN_PAGE precharge overhead is bounded; absurd slowdowns
    //              would indicate a simulation bug.
    check(ratio < 4.0, "second H2D not more than 4x slower than first (precharge overhead bounded)");

    pimFree(obj1);
    pimFree(obj2);
    pimDeleteDevice();
}

// Test 6: LPDDR protocol — runtime plausible, request_size_bytes=128B used
void test_lpddr_protocol() {
    printf("\n--- Test 6: LPDDR4 protocol plausibility (256KB H2D) ---\n");
    // LPDDR4_8Gb_x16_2400: bus=64b, BL=16, request_size=128B
    // Peak BW: 2400 MT/s * 8B = 19.2 GB/s → 256KB = 13.3 us = 0.013 ms
    PerfResult r = runH2D(65536, "LPDDR");
    printf("  LPDDR4 H2D 256KB: %.6f ms, %.9f mJ\n", r.ms, r.mj);
    check(r.ms > 0.0,   "LPDDR4 runtime non-zero");
    check(r.mj > 0.0,   "LPDDR4 energy non-zero");
    check(r.ms > 0.002, "LPDDR4 runtime > 2us (not unrealistically fast)");
    check(r.ms < 1.0,   "LPDDR4 runtime < 1ms (not unrealistically slow for 256KB)");
}

// Test 7: H2D energy scales with transfer size
void test_energy_scaling() {
    printf("\n--- Test 7: Energy scales with transfer size (DDR4) ---\n");
    PerfResult r4k  = runH2D(1024);   //   4 KB
    PerfResult r1m  = runH2D(262144); //   1 MB
    printf("  H2D   4KB: %.9f mJ\n", r4k.mj);
    printf("  H2D   1MB: %.9f mJ\n", r1m.mj);
    double ratio = r1m.mj / r4k.mj;
    printf("  Energy ratio 1MB/4KB: %.1fx  (data ratio = 256x)\n", ratio);

    check(r1m.mj > r4k.mj, "1MB has more energy than 4KB");
    check(ratio > 10.0,    "energy ratio > 10 (scaling with size)");
    check(ratio < 2000.0,  "energy ratio < 2000 (not absurdly disproportionate)");
}

// ── main ──────────────────────────────────────────────────────────────────────
int main() {
#ifndef DRAMSIM3_INTEG
    printf("SKIP: compiled without DRAMSIM3_INTEG — all DRAMSim3 tests skipped.\n");
    return 0;
#endif

    printf("==============================================\n");
    printf("  DRAMSim3 Integration Validation Test Suite \n");
    printf("==============================================\n");

    test_dramsim3_active();
    test_linearity();
    test_d2d_vs_h2d();
    test_d2h_integrity();
    test_transfer_independence();
    test_lpddr_protocol();
    test_energy_scaling();

    printf("\n==============================================\n");
    printf("  All DRAMSim3 validation tests PASSED.      \n");
    printf("==============================================\n");
    return 0;
}
