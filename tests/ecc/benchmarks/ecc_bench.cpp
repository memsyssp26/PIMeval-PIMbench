// ecc_bench.cpp — ECC overhead benchmark for PIMeval-PIMbench
//
// Runs representative PIM workloads on a compact analytical device
// (no functional simulation, fast) and reports per-tier ECC overhead via
// pimGetStats().  Each workload is run once; the ECC mode is set through
// environment variables before invoking this binary (see run-ecc-comparison.sh).
//
// Workloads
//   1. vec-add          : INT32 element-wise addition      (H2D × 2, ADD, D2H)
//   2. gemv             : matrix-vector multiply           (H2D loop + ScaledAdd loop, D2H)
//   3. relu             : element-wise max(x,0)            (H2D, MAX, D2H)
//   4. axpy             : y = alpha*x + y (BLAS-1)         (H2D × 2, ScaledAdd, D2H)
//   5. brightness       : clamp(x + c, 0, 255)             (H2D, AddScalar+MaxScalar+MinScalar, D2H)
//   6. histogram        : 16-bin int32 histogram           (H2D, 16×(EQ+RedSum), D2H)
//   7. linear-regression: ΣX, ΣX², ΣY, ΣXY from vectors  (H2D × 2, 2×Mul+4×RedSum, D2H × 0)
//   8. prefix-sum       : inclusive prefix sum             (H2D, PrefixSum, D2H)
//   9. select           : cond select on threshold         (H2D × 2, GTScalar+CondSelect, D2H)
//
// Device: PIM_DEVICE_BITSIMD_V, 1 rank, 1 bank, 8 subarrays, 1024 rows,
//         1024 cols — 8 cores, 32K INT32 capacity.  All ops analytical.
//
// Usage:
//   ./ecc_bench.out [workload|all]   default: all
//   Workload names: vec-add gemv relu axpy brightness histogram
//                   linear-regression prefix-sum select
//   Env vars (set by sweep script):
//     PIMEVAL_ODECC, PIMEVAL_ECC, PIMEVAL_ECC_READOUT_ONLY, PIMEVAL_SCRATCHPAD, ...

#include "libpimeval.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string>
#include <cassert>
#include <cstdint>

// ── Device geometry (compact, analytical) ────────────────────────────────────
static const unsigned kRanks       = 1;
static const unsigned kBanks       = 4;
static const unsigned kSubarrays   = 4;
static const unsigned kRows        = 1024;
static const unsigned kCols        = 1024;
static const unsigned kElements    = 8192;   // fits comfortably in 8 cores

// ── Helpers ───────────────────────────────────────────────────────────────────

static bool createDev() {
    PimStatus s = pimCreateDevice(PIM_DEVICE_BITSIMD_V,
                                  kRanks, kBanks, kSubarrays, kRows, kCols);
    return s == PIM_OK;
}

// Print one PimStats row
static void printStatsRow(const char* label, const PimStats& s) {
    double totalMs = s.copyH2DMs + s.copyD2HMs + s.copyD2DMs + s.computeMs;
    double totalMj = s.copyH2DMj + s.copyD2HMj + s.copyD2DMj + s.computeMj;
    double eccMs   = s.odeccMs + s.controllerEccMs + s.scratchpadEccMs;
    double eccMj   = s.odeccMj + s.controllerEccMj + s.scratchpadEccMj;

    printf("%-18s | H2D:%9.4f D2H:%9.4f Compute:%9.4f | Total:%9.4f ms"
           " | ECC(od/ctrl/sp): %9.6f / %9.6f / %9.6f ms\n",
           label,
           s.copyH2DMs, s.copyD2HMs, s.computeMs, totalMs,
           s.odeccMs, s.controllerEccMs, s.scratchpadEccMs);
    (void)eccMs; (void)eccMj; (void)totalMj;
}

// ── Workload 1: vec-add ───────────────────────────────────────────────────────
static void run_vecadd() {
    if (!createDev()) { fprintf(stderr, "vec-add: device create failed\n"); return; }

    std::vector<int32_t> a(kElements, 3), b(kElements, 7), c(kElements, 0);

    PimObjId src1 = pimAlloc(PIM_ALLOC_AUTO, kElements, PIM_INT32);
    PimObjId src2 = pimAllocAssociated(src1, PIM_INT32);
    assert(src1 != -1 && src2 != -1);

    pimResetStats();
    pimCopyHostToDevice(a.data(), src1);
    pimCopyHostToDevice(b.data(), src2);
    pimAdd(src1, src2, src1);
    pimCopyDeviceToHost(src1, c.data());

    PimStats stats{};
    pimGetStats(&stats);
    printStatsRow("vec-add", stats);

    pimShowStats();
    pimFree(src1); pimFree(src2);
    pimDeleteDevice();
}

// ── Workload 2: gemv (matrix-vector, col-major) ───────────────────────────────
static void run_gemv() {
    if (!createDev()) { fprintf(stderr, "gemv: device create failed\n"); return; }

    // Small GEMV: 256-element vector, 64-column matrix → 64 iterations
    const unsigned row = 256, col = 64;
    std::vector<int32_t> vec(col, 1);
    std::vector<int32_t> matCol(row, 2);
    std::vector<int32_t> result(row, 0);

    PimObjId matObj = pimAlloc(PIM_ALLOC_AUTO, row, PIM_INT32);
    PimObjId dstObj = pimAllocAssociated(matObj, PIM_INT32);
    assert(matObj != -1 && dstObj != -1);

    pimBroadcastInt(dstObj, 0);
    pimResetStats();

    for (unsigned i = 0; i < col; ++i) {
        pimCopyHostToDevice(matCol.data(), matObj);
        pimScaledAdd(matObj, dstObj, dstObj, vec[i]);
    }
    pimCopyDeviceToHost(dstObj, result.data());

    PimStats stats{};
    pimGetStats(&stats);
    printStatsRow("gemv", stats);

    pimShowStats();
    pimFree(matObj); pimFree(dstObj);
    pimDeleteDevice();
}

// ── Workload 3: relu (element-wise max(x, 0)) ────────────────────────────────
static void run_relu() {
    if (!createDev()) { fprintf(stderr, "relu: device create failed\n"); return; }

    std::vector<int32_t> src(kElements, -5), dst(kElements, 0);

    PimObjId srcObj = pimAlloc(PIM_ALLOC_AUTO, kElements, PIM_INT32);
    PimObjId dstObj = pimAllocAssociated(srcObj, PIM_INT32);
    PimObjId zeroObj = pimAllocAssociated(srcObj, PIM_INT32);
    assert(srcObj != -1 && dstObj != -1 && zeroObj != -1);

    pimBroadcastInt(zeroObj, 0);
    pimResetStats();
    pimCopyHostToDevice(src.data(), srcObj);
    pimMax(srcObj, zeroObj, dstObj);
    pimCopyDeviceToHost(dstObj, dst.data());

    PimStats stats{};
    pimGetStats(&stats);
    printStatsRow("relu", stats);

    pimShowStats();
    pimFree(srcObj); pimFree(dstObj); pimFree(zeroObj);
    pimDeleteDevice();
}

// ── Workload 4: axpy (y = alpha*x + y, BLAS-1) ───────────────────────────────
static void run_axpy() {
    if (!createDev()) { fprintf(stderr, "axpy: device create failed\n"); return; }

    const int32_t alpha = 3;
    std::vector<int32_t> x(kElements, 2), y(kElements, 5);

    PimObjId xObj = pimAlloc(PIM_ALLOC_AUTO, kElements, PIM_INT32);
    PimObjId yObj = pimAllocAssociated(xObj, PIM_INT32);
    assert(xObj != -1 && yObj != -1);

    pimResetStats();
    pimCopyHostToDevice(x.data(), xObj);
    pimCopyHostToDevice(y.data(), yObj);
    pimScaledAdd(xObj, yObj, yObj, static_cast<uint64_t>(alpha));
    pimCopyDeviceToHost(yObj, y.data());

    PimStats stats{};
    pimGetStats(&stats);
    printStatsRow("axpy", stats);

    pimShowStats();
    pimFree(xObj); pimFree(yObj);
    pimDeleteDevice();
}

// ── Workload 5: brightness (clamp(x + c, 0, 255)) ────────────────────────────
// Models an image brightness adjustment: add constant, clamp to [0, 255].
static void run_brightness() {
    if (!createDev()) { fprintf(stderr, "brightness: device create failed\n"); return; }

    const uint64_t kBrightness = 50;
    std::vector<int32_t> img(kElements, 128), out(kElements, 0);

    PimObjId srcObj = pimAlloc(PIM_ALLOC_AUTO, kElements, PIM_INT32);
    PimObjId dstObj = pimAllocAssociated(srcObj, PIM_INT32);
    assert(srcObj != -1 && dstObj != -1);

    pimResetStats();
    pimCopyHostToDevice(img.data(), srcObj);
    pimAddScalar(srcObj, dstObj, kBrightness);
    pimMaxScalar(dstObj, dstObj, 0);
    pimMinScalar(dstObj, dstObj, 255);
    pimCopyDeviceToHost(dstObj, out.data());

    PimStats stats{};
    pimGetStats(&stats);
    printStatsRow("brightness", stats);

    pimShowStats();
    pimFree(srcObj); pimFree(dstObj);
    pimDeleteDevice();
}

// ── Workload 6: histogram (16 bins) ──────────────────────────────────────────
// For each bin value [0,15]: compare data == bin, then sum the 0/1 mask.
// Models analytics workloads with repeated scan-and-reduce over DRAM data.
static void run_histogram() {
    if (!createDev()) { fprintf(stderr, "histogram: device create failed\n"); return; }

    const unsigned kBins = 16;
    std::vector<int32_t> data(kElements);
    for (unsigned i = 0; i < kElements; ++i) data[i] = static_cast<int32_t>(i % kBins);

    PimObjId dataObj = pimAlloc(PIM_ALLOC_AUTO, kElements, PIM_INT32);
    PimObjId maskObj = pimAllocAssociated(dataObj, PIM_INT32);
    assert(dataObj != -1 && maskObj != -1);

    pimResetStats();
    pimCopyHostToDevice(data.data(), dataObj);

    int32_t counts[kBins] = {};
    for (unsigned bin = 0; bin < kBins; ++bin) {
        pimEQScalar(dataObj, maskObj, static_cast<uint64_t>(bin));
        pimRedSum(maskObj, &counts[bin]);
    }

    PimStats stats{};
    pimGetStats(&stats);
    printStatsRow("histogram", stats);

    pimShowStats();
    pimFree(dataObj); pimFree(maskObj);
    pimDeleteDevice();
}

// ── Workload 7: linear-regression (compute ΣX, ΣX², ΣY, ΣXY) ────────────────
// Computes the four sufficient statistics for OLS regression: uses two element-
// wise multiplications and four reductions — high scratchpad utilization.
static void run_linreg() {
    if (!createDev()) { fprintf(stderr, "linear-regression: device create failed\n"); return; }

    std::vector<int32_t> x(kElements), y(kElements), tmp(kElements);
    for (unsigned i = 0; i < kElements; ++i) {
        x[i] = static_cast<int32_t>(i);
        y[i] = static_cast<int32_t>(2 * i + 1);
    }

    PimObjId xObj   = pimAlloc(PIM_ALLOC_AUTO, kElements, PIM_INT32);
    PimObjId yObj   = pimAllocAssociated(xObj, PIM_INT32);
    PimObjId tmpObj = pimAllocAssociated(xObj, PIM_INT32);
    assert(xObj != -1 && yObj != -1 && tmpObj != -1);

    pimResetStats();
    pimCopyHostToDevice(x.data(), xObj);
    pimCopyHostToDevice(y.data(), yObj);

    int64_t sumX = 0, sumX2 = 0, sumY = 0, sumXY = 0;
    pimRedSum(xObj, &sumX);
    pimMul(xObj, xObj, tmpObj);
    pimRedSum(tmpObj, &sumX2);
    pimRedSum(yObj, &sumY);
    pimMul(xObj, yObj, tmpObj);
    pimRedSum(tmpObj, &sumXY);

    PimStats stats{};
    pimGetStats(&stats);
    printStatsRow("linear-regression", stats);

    pimShowStats();
    pimFree(xObj); pimFree(yObj); pimFree(tmpObj);
    pimDeleteDevice();
}

// ── Workload 8: prefix-sum ────────────────────────────────────────────────────
// Inclusive prefix sum — heavily uses the scratchpad for carry propagation.
static void run_prefixsum() {
    if (!createDev()) { fprintf(stderr, "prefix-sum: device create failed\n"); return; }

    std::vector<int32_t> src(kElements, 1), dst(kElements, 0);

    PimObjId srcObj = pimAlloc(PIM_ALLOC_AUTO, kElements, PIM_INT32);
    PimObjId dstObj = pimAllocAssociated(srcObj, PIM_INT32);
    assert(srcObj != -1 && dstObj != -1);

    pimResetStats();
    pimCopyHostToDevice(src.data(), srcObj);
    pimPrefixSum(srcObj, dstObj);
    pimCopyDeviceToHost(dstObj, dst.data());

    PimStats stats{};
    pimGetStats(&stats);
    printStatsRow("prefix-sum", stats);

    pimShowStats();
    pimFree(srcObj); pimFree(dstObj);
    pimDeleteDevice();
}

// ── Workload 9: select (conditional select on threshold) ─────────────────────
// dest[i] = (a[i] > threshold) ? a[i] : b[i] — models filter-by-key / masked copy.
static void run_select() {
    if (!createDev()) { fprintf(stderr, "select: device create failed\n"); return; }

    const uint64_t kThreshold = 100;
    std::vector<int32_t> a(kElements), b(kElements, 0), out(kElements, 0);
    for (unsigned i = 0; i < kElements; ++i) a[i] = static_cast<int32_t>(i % 200);

    PimObjId aObj    = pimAlloc(PIM_ALLOC_AUTO, kElements, PIM_INT32);
    PimObjId bObj    = pimAllocAssociated(aObj, PIM_INT32);
    PimObjId maskObj = pimAllocAssociated(aObj, PIM_INT32);
    PimObjId outObj  = pimAllocAssociated(aObj, PIM_INT32);
    assert(aObj != -1 && bObj != -1 && maskObj != -1 && outObj != -1);

    pimResetStats();
    pimCopyHostToDevice(a.data(), aObj);
    pimCopyHostToDevice(b.data(), bObj);
    pimGTScalar(aObj, maskObj, kThreshold);
    pimCondSelect(maskObj, aObj, bObj, outObj);
    pimCopyDeviceToHost(outObj, out.data());

    PimStats stats{};
    pimGetStats(&stats);
    printStatsRow("select", stats);

    pimShowStats();
    pimFree(aObj); pimFree(bObj); pimFree(maskObj); pimFree(outObj);
    pimDeleteDevice();
}

// ── main ──────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {
    const char* which = (argc >= 2) ? argv[1] : "all";

    bool doAll        = (strcmp(which, "all")               == 0);
    bool doVecAdd     = doAll || (strcmp(which, "vec-add")           == 0);
    bool doGemv       = doAll || (strcmp(which, "gemv")              == 0);
    bool doRelu       = doAll || (strcmp(which, "relu")              == 0);
    bool doAxpy       = doAll || (strcmp(which, "axpy")              == 0);
    bool doBrightness = doAll || (strcmp(which, "brightness")        == 0);
    bool doHistogram  = doAll || (strcmp(which, "histogram")         == 0);
    bool doLinreg     = doAll || (strcmp(which, "linear-regression") == 0);
    bool doPrefixSum  = doAll || (strcmp(which, "prefix-sum")        == 0);
    bool doSelect     = doAll || (strcmp(which, "select")            == 0);

    if (!doVecAdd && !doGemv && !doRelu && !doAxpy && !doBrightness &&
        !doHistogram && !doLinreg && !doPrefixSum && !doSelect) {
        fprintf(stderr,
            "Usage: %s [workload|all]\n"
            "  workloads: vec-add gemv relu axpy brightness histogram"
            " linear-regression prefix-sum select\n",
            argv[0]);
        return 1;
    }

    printf("=== ECC Benchmark (%s) ===\n", which);
    printf("Device: %u rank, %u bank, %u subarrays, %u rows, %u cols"
           " = BITSIMD_V analytical\n\n",
           kRanks, kBanks, kSubarrays, kRows, kCols);

    if (doVecAdd)     { printf("\n[vec-add]\n");           run_vecadd();     }
    if (doGemv)       { printf("\n[gemv]\n");              run_gemv();       }
    if (doRelu)       { printf("\n[relu]\n");              run_relu();       }
    if (doAxpy)       { printf("\n[axpy]\n");              run_axpy();       }
    if (doBrightness) { printf("\n[brightness]\n");        run_brightness(); }
    if (doHistogram)  { printf("\n[histogram]\n");         run_histogram();  }
    if (doLinreg)     { printf("\n[linear-regression]\n"); run_linreg();     }
    if (doPrefixSum)  { printf("\n[prefix-sum]\n");        run_prefixsum();  }
    if (doSelect)     { printf("\n[select]\n");            run_select();     }

    return 0;
}
