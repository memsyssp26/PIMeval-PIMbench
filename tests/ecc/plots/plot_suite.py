#!/usr/bin/env python3
"""
PIMeval ECC Benchmark Suite — Publication-Quality Plots
Nine workloads × four ECC mode configurations.

ECC Modes:
  Mode 1 — ODECC only         (JEDEC 128+8 SECDED on every DRAM row activation)
  Mode 2 — ODECC + Controller (Mode 1 + SECDED on D2H copy boundary)
  Mode 3 — All tiers, per-op  (Mode 2 + Scratchpad ECC charged per logic step)
  Mode 4 — All tiers, output  (Mode 2 + Scratchpad ECC charged per output element)

Figures:
  fig_a_ecc_overhead_abs.*   — Grouped stacked bar: absolute ECC overhead per tier (log scale)
  fig_b_ecc_overhead_pct.*   — Total runtime normalized to Mode 1 = 1.0×
  fig_c_runtime_breakdown.*  — Mode 3 total runtime breakdown (100% stacked by tier)

Usage:
  python3 plot_suite.py           # re-run benchmarks then plot
  python3 plot_suite.py --no-run  # use hardcoded data (fast, no binary needed)
"""

import sys
import os
import subprocess
import io
import csv

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import numpy as np

# ── paths ─────────────────────────────────────────────────────────────────────
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
BENCH_EXE  = os.path.join(SCRIPT_DIR, "..", "benchmarks", "ecc_bench.out")
SWEEP_SH   = os.path.join(SCRIPT_DIR, "..", "scripts", "run-ecc-comparison.sh")
OUT_DIR    = SCRIPT_DIR

# ── style ─────────────────────────────────────────────────────────────────────
C_ODECC  = "#DD8452"
C_CTRL   = "#C44E52"
C_SPAD   = "#55A868"
C_COPY   = "#4C72B0"
C_COMP   = "#8172B2"

MODE_COLORS  = ["#4878CF", "#6ACC65", "#D65F5F", "#E88C3B"]
MODE_LABELS  = ["Mode 1 (ODECC only)", "Mode 2 (+Controller)",
                "Mode 3 (+S.Pad, per-op)", "Mode 4 (+S.Pad, output-only)"]
MODE_HATCHES = ["", "//", "xx", ".."]

plt.rcParams.update({
    "font.family":      "serif",
    "font.size":        10,
    "axes.titlesize":   11,
    "axes.labelsize":   10,
    "xtick.labelsize":  9,
    "ytick.labelsize":  9,
    "legend.fontsize":  9,
    "figure.dpi":       150,
    "savefig.dpi":      300,
    "savefig.bbox":     "tight",
})

# ── benchmark data ─────────────────────────────────────────────────────────────
# Hardcoded from verified run (2026-05-06), device: BITSIMD_V 1×4×4×1024×1024.
# Keys: copy[4], compute[4], odecc[4], ctrl[4], spad[4] — indices 0/1/2/3 = Mode1/2/3/4
WORKLOADS = [
    "vec-add", "gemv", "relu", "axpy",
    "brightness", "histogram", "linear-reg", "prefix-sum", "select",
]

RAW = {
    "vec-add": dict(
        copy    = [0.009720, 0.017912, 0.017912, 0.017912],
        compute = [0.012088, 0.013277, 0.031837, 0.019421],
        odecc   = [0.012288, 0.012288, 0.012288, 0.012288],
        ctrl    = [0.000000, 0.008192, 0.008192, 0.008192],
        spad    = [0.000000, 0.000000, 0.018560, 0.006144],
    ),
    "gemv": dict(
        copy    = [0.006581, 0.006837, 0.006837, 0.006837],
        compute = [4.946928, 5.933856, 10.088224, 5.942048],
        odecc   = [0.016448, 0.016448, 0.016448,  0.016448],
        ctrl    = [0.000000, 0.000256, 0.000256,  0.000256],
        spad    = [0.000000, 0.000000, 4.154368,  0.008192],
    ),
    "relu": dict(
        copy    = [0.006480, 0.014672, 0.014672, 0.014672],
        compute = [0.016048, 0.018028, 0.048877, 0.022124],
        odecc   = [0.010240, 0.010240, 0.010240, 0.010240],
        ctrl    = [0.000000, 0.008192, 0.008192, 0.008192],
        spad    = [0.000000, 0.000000, 0.030848, 0.004096],
    ),
    "axpy": dict(
        copy    = [0.009720, 0.017912, 0.017912, 0.017912],
        compute = [0.083248, 0.098669, 0.617965, 0.102765],
        odecc   = [0.012288, 0.012288, 0.012288, 0.012288],
        ctrl    = [0.000000, 0.008192, 0.008192, 0.008192],
        spad    = [0.000000, 0.000000, 0.519296, 0.004096],
    ),
    "brightness": dict(
        copy    = [0.006480, 0.014672, 0.014672, 0.014672],
        compute = [0.026770, 0.027610, 0.109914, 0.039898],
        odecc   = [0.016384, 0.016384, 0.016384, 0.016384],
        ctrl    = [0.000000, 0.008192, 0.008192, 0.008192],
        spad    = [0.000000, 0.000000, 0.082304, 0.012288],
    ),
    "histogram": dict(
        copy    = [0.003240, 0.003240, 0.003240, 0.003240],
        compute = [0.104026, 0.111724, 0.511084, 0.177260],
        odecc   = [0.067584, 0.067584, 0.067584, 0.067584],
        ctrl    = [0.000000, 0.000000, 0.000000, 0.000000],
        spad    = [0.000000, 0.000000, 0.399360, 0.065536],
    ),
    "linear-reg": dict(
        copy    = [0.006480, 0.006480, 0.006480, 0.006480],
        compute = [0.212028, 0.251976, 1.050696, 0.260168],
        odecc   = [0.016384, 0.016384, 0.016384, 0.016384],
        ctrl    = [0.000000, 0.000000, 0.000000, 0.000000],
        spad    = [0.000000, 0.000000, 0.798720, 0.008192],
    ),
    "prefix-sum": dict(
        copy    = [0.006480, 0.014672, 0.014672, 0.014672],
        compute = [0.100734, 0.120061, 0.306301, 0.126205],
        odecc   = [0.008192, 0.008192, 0.008192, 0.008192],
        ctrl    = [0.000000, 0.008192, 0.008192, 0.008192],
        spad    = [0.000000, 0.000000, 0.186240, 0.006144],
    ),
    "select": dict(
        copy    = [0.009720, 0.017912, 0.017912, 0.017912],
        compute = [0.006498, 0.006978, 0.031682, 0.011074],
        odecc   = [0.010240, 0.010240, 0.010240, 0.010240],
        ctrl    = [0.000000, 0.008192, 0.008192, 0.008192],
        spad    = [0.000000, 0.000000, 0.024704, 0.004096],
    ),
}

def reload_from_benchmark():
    """Re-run all workloads and update RAW dict from live benchmark output."""
    if not os.path.isfile(BENCH_EXE):
        print(f"ERROR: binary not found at {BENCH_EXE}. Build first: cd tests/ecc && make")
        sys.exit(1)

    mode_envs = [
        # Mode 1: ODECC only
        {"PIMEVAL_ODECC": "1", "PIMEVAL_ODECC_DATA_WIDTH": "128",
         "PIMEVAL_ODECC_PARITY_WIDTH": "8", "PIMEVAL_ECC": "0", "PIMEVAL_SCRATCHPAD": "0"},
        # Mode 2: ODECC + Controller ECC on readout
        {"PIMEVAL_ODECC": "1", "PIMEVAL_ODECC_DATA_WIDTH": "128",
         "PIMEVAL_ODECC_PARITY_WIDTH": "8", "PIMEVAL_ECC": "1",
         "PIMEVAL_ECC_TYPE": "secded", "PIMEVAL_ECC_READOUT_ONLY": "1",
         "PIMEVAL_SCRATCHPAD": "0"},
        # Mode 3: All tiers, scratchpad ECC per logic op (conservative)
        {"PIMEVAL_ODECC": "1", "PIMEVAL_ODECC_DATA_WIDTH": "128",
         "PIMEVAL_ODECC_PARITY_WIDTH": "8", "PIMEVAL_ECC": "1",
         "PIMEVAL_ECC_TYPE": "secded", "PIMEVAL_ECC_READOUT_ONLY": "1",
         "PIMEVAL_SCRATCHPAD": "1", "PIMEVAL_SCRATCHPAD_ECC": "1",
         "PIMEVAL_SCRATCHPAD_ECC_TYPE": "secded"},
        # Mode 4: All tiers, scratchpad ECC per output element (output-only)
        {"PIMEVAL_ODECC": "1", "PIMEVAL_ODECC_DATA_WIDTH": "128",
         "PIMEVAL_ODECC_PARITY_WIDTH": "8", "PIMEVAL_ECC": "1",
         "PIMEVAL_ECC_TYPE": "secded", "PIMEVAL_ECC_READOUT_ONLY": "1",
         "PIMEVAL_SCRATCHPAD": "1", "PIMEVAL_SCRATCHPAD_ECC": "1",
         "PIMEVAL_SCRATCHPAD_ECC_TYPE": "secded",
         "PIMEVAL_SCRATCHPAD_ECC_OUTPUT_ONLY": "1"},
    ]

    def parse_stats(output):
        """Return (copy_ms, compute_ms, odecc_ms, ctrl_ms, spad_ms) from pimShowStats text."""
        copy_ms = compute_ms = odecc_ms = ctrl_ms = spad_ms = 0.0
        in_copy = in_cmd = in_ecc = False
        for line in output.splitlines():
            if "Data Copy Stats:" in line:    in_copy = True; in_cmd = in_ecc = False
            if "PIM Command Stats:" in line:  in_cmd  = True; in_copy = in_ecc = False
            if "ECC Reliability Stats:" in line: in_ecc = True; in_copy = in_cmd = False
            if in_copy and "TOTAL ---------" in line and "ms Estimated Runtime" in line:
                toks = line.split()
                for i, t in enumerate(toks):
                    if t == "ms" and i > 0:
                        try: copy_ms = float(toks[i - 1]); break
                        except ValueError: pass
            if in_cmd and "TOTAL ---------" in line:
                toks = line.split()
                # format: TOTAL --------- : <cnt> <ms> <mj> ...
                if len(toks) >= 6:
                    try: compute_ms = float(toks[4])
                    except ValueError: pass
            if in_ecc:
                if "On-Die ECC" in line:
                    p = line.split(":"); vals = p[1].split() if len(p) > 1 else []
                    if vals:
                        try: odecc_ms = float(vals[0])
                        except ValueError: pass
                if "Controller ECC" in line:
                    p = line.split(":"); vals = p[1].split() if len(p) > 1 else []
                    if vals:
                        try: ctrl_ms = float(vals[0])
                        except ValueError: pass
                if "Scratchpad ECC" in line:
                    p = line.split(":"); vals = p[1].split() if len(p) > 1 else []
                    if vals:
                        try: spad_ms = float(vals[0])
                        except ValueError: pass
        return copy_ms, compute_ms, odecc_ms, ctrl_ms, spad_ms

    for wl in WORKLOADS:
        wl_key = wl
        d = RAW[wl_key]
        # Ensure lists are long enough for 4 modes
        for k in ("copy", "compute", "odecc", "ctrl", "spad"):
            while len(d[k]) < len(mode_envs):
                d[k].append(0.0)
        for mi, env_extra in enumerate(mode_envs):
            env = dict(os.environ)
            env.update(env_extra)
            r = subprocess.run([BENCH_EXE, wl], capture_output=True, text=True, env=env)
            cp, co, od, ct, sp = parse_stats(r.stdout)
            d["copy"][mi]    = cp
            d["compute"][mi] = co
            d["odecc"][mi]   = od
            d["ctrl"][mi]    = ct
            d["spad"][mi]    = sp
        print(f"  {wl_key}: OK")


# ── Figure A: grouped stacked bar — absolute ECC overhead ────────────────────

def fig_a(out_prefix: str):
    """
    For each workload: 4 grouped bars (Mode1/2/3/4), each stacked with
    ODECC (orange) / Controller (red) / Scratchpad (green).
    Y-axis: log scale (ms).
    """
    n = len(WORKLOADS)
    n_modes = 4
    group_width = 0.88
    bar_w = group_width / n_modes
    x = np.arange(n)

    fig, ax = plt.subplots(figsize=(14, 5))

    for mi in range(n_modes):
        offsets = x + (mi - (n_modes - 1) / 2.0) * bar_w

        od_vals = np.array([RAW[wl]["odecc"][mi] for wl in WORKLOADS])
        ct_vals = np.array([RAW[wl]["ctrl"][mi]  for wl in WORKLOADS])
        sp_vals = np.array([RAW[wl]["spad"][mi]  for wl in WORKLOADS])

        base = np.zeros(n)
        kw = dict(width=bar_w, edgecolor="white", linewidth=0.4)

        ax.bar(offsets, od_vals, bottom=base, color=C_ODECC,
               hatch=MODE_HATCHES[mi], alpha=0.85, **kw)
        base = base + od_vals

        ax.bar(offsets, ct_vals, bottom=base, color=C_CTRL,
               hatch=MODE_HATCHES[mi], alpha=0.85, **kw)
        base = base + ct_vals

        ax.bar(offsets, sp_vals, bottom=base, color=C_SPAD,
               hatch=MODE_HATCHES[mi], alpha=0.85, **kw)

    import matplotlib.patches as mpatches
    mode_patches = [
        mpatches.Patch(facecolor=MODE_COLORS[i], hatch=MODE_HATCHES[i],
                       edgecolor="black", linewidth=0.5, label=MODE_LABELS[i])
        for i in range(n_modes)
    ]
    tier_patches = [
        mpatches.Patch(facecolor=C_ODECC, label="On-Die ECC (ODECC)"),
        mpatches.Patch(facecolor=C_CTRL,  label="Controller ECC"),
        mpatches.Patch(facecolor=C_SPAD,  label="Scratchpad ECC"),
    ]
    ax.legend(handles=mode_patches + tier_patches,
              loc="upper left", fontsize=8.0, framealpha=0.9, ncol=2)

    ax.set_yscale("log")
    ax.set_xticks(x)
    ax.set_xticklabels(WORKLOADS, rotation=30, ha="right")
    ax.set_ylabel("ECC Overhead (ms, log scale)")
    ax.set_title("ECC Overhead by Tier and Mode — 9 Workloads × 4 Configurations\n"
                 "(BITSIMD-V analytical, DDR5 baseline, 8192 INT32 elements)")
    ax.yaxis.set_major_formatter(mticker.LogFormatter(labelOnlyBase=False))
    ax.yaxis.grid(True, linestyle="--", alpha=0.4, which="both")
    ax.set_axisbelow(True)

    fig.tight_layout()
    for ext in ("pdf", "png"):
        p = os.path.join(OUT_DIR, f"{out_prefix}.{ext}")
        fig.savefig(p)
        print(f"  Saved {p}")
    plt.close(fig)


# ── Figure B: ECC overhead as % of Mode1 baseline ────────────────────────────

def fig_b(out_prefix: str):
    """
    For each workload: 4 grouped bars (Mode1/2/3/4) showing total runtime
    normalized to Mode 1 = 1.0.  Solid colors, no hatching.
    Mode 4 (output-only) shows how much of Mode 3's scratchpad overhead is
    physically necessary vs conservative per-logic-op overcounting.
    """
    n = len(WORKLOADS)
    n_modes = 4
    group_width = 0.88
    bar_w = group_width / n_modes
    x = np.arange(n)

    fig, ax = plt.subplots(figsize=(14, 5))

    for mi in range(n_modes):
        offsets = x + (mi - (n_modes - 1) / 2.0) * bar_w
        norm_vals = []
        for wl in WORKLOADS:
            d = RAW[wl]
            total_m1 = d["copy"][0] + d["compute"][0] + d["odecc"][0] + d["ctrl"][0] + d["spad"][0]
            total_mi = d["copy"][mi] + d["compute"][mi] + d["odecc"][mi] + d["ctrl"][mi] + d["spad"][mi]
            norm_vals.append(total_mi / total_m1 if total_m1 > 0 else 1.0)

        ax.bar(offsets, norm_vals, width=bar_w,
               color=MODE_COLORS[mi], edgecolor="white", linewidth=0.4,
               alpha=0.9, label=MODE_LABELS[mi])

    ax.axhline(1.0, color="black", linewidth=0.8, linestyle="--", alpha=0.5, label="Mode 1 baseline")

    ax.set_xticks(x)
    ax.set_xticklabels(WORKLOADS, rotation=30, ha="right")
    ax.set_ylabel("Normalized Total Runtime (Mode 1 = 1.0×)")
    ax.set_title("Total Runtime Normalized to Mode 1 Baseline\n"
                 "Four ECC configurations — BITSIMD-V analytical, DDR5; Mode 4 = output-only scratchpad ECC")
    ax.legend(loc="upper right", framealpha=0.9, fontsize=8.5)
    ax.yaxis.grid(True, linestyle="--", alpha=0.5)
    ax.set_axisbelow(True)
    ax.set_ylim(0, None)

    fig.tight_layout()
    for ext in ("pdf", "png"):
        p = os.path.join(OUT_DIR, f"{out_prefix}.{ext}")
        fig.savefig(p)
        print(f"  Saved {p}")
    plt.close(fig)


# ── Figure C: Mode 3 total runtime 100% stacked breakdown ────────────────────

def fig_c(out_prefix: str):
    """
    For Mode 3 only: 100% stacked bar per workload showing fraction of total
    runtime from Copy / Compute (base) / ODECC / Controller ECC / Scratchpad ECC.
    Shows where time actually goes at full ECC coverage.
    """
    n = len(WORKLOADS)
    mi = 2  # Mode 3

    totals = []
    for wl in WORKLOADS:
        d = RAW[wl]
        totals.append(d["copy"][mi] + d["compute"][mi] +
                      d["odecc"][mi] + d["ctrl"][mi] + d["spad"][mi])

    # Fractions per tier
    fracs = {k: [] for k in ("copy", "compute", "odecc", "ctrl", "spad")}
    for wl in WORKLOADS:
        d = RAW[wl]; t = totals[WORKLOADS.index(wl)]
        for k in fracs:
            fracs[k].append(d[k][mi] / t * 100 if t > 0 else 0.0)

    x = np.arange(n)
    width = 0.55

    fig, ax = plt.subplots(figsize=(13, 5))

    tiers = [
        ("copy",    C_COPY,  "Copy (H2D + D2H)"),
        ("compute", C_COMP,  "Compute (base)"),
        ("odecc",   C_ODECC, "On-Die ECC"),
        ("ctrl",    C_CTRL,  "Controller ECC"),
        ("spad",    C_SPAD,  "Scratchpad ECC"),
    ]

    bottom = np.zeros(n)
    for key, color, label in tiers:
        vals = np.array(fracs[key])
        bars = ax.bar(x, vals, width, bottom=bottom, color=color,
                      edgecolor="white", linewidth=0.5, label=label)

        # Annotate if fraction is large enough to be readable
        for i, (v, b) in enumerate(zip(vals, bottom)):
            if v > 4.0:
                ax.text(x[i], b + v / 2, f"{v:.0f}%",
                        ha="center", va="center", fontsize=7.5,
                        color="white", fontweight="bold")
        bottom = bottom + vals

    # Absolute total ms above each bar
    for i, t in enumerate(totals):
        ax.text(x[i], 102, f"{t*1000:.0f} μs" if t < 0.5 else f"{t:.2f} ms",
                ha="center", va="bottom", fontsize=8, color="#333333")

    ax.set_xticks(x)
    ax.set_xticklabels(WORKLOADS, rotation=30, ha="right")
    ax.set_ylim(0, 115)
    ax.set_ylabel("Fraction of Total Runtime (%)")
    ax.set_title("Mode 3 Runtime Breakdown per Tier (Conservative Scratchpad ECC, per-op)\n"
                 "Absolute totals shown above bars — BITSIMD-V analytical, DDR5")
    ax.legend(loc="upper right", framealpha=0.9)
    ax.yaxis.grid(True, linestyle="--", alpha=0.4)
    ax.set_axisbelow(True)

    fig.tight_layout()
    for ext in ("pdf", "png"):
        p = os.path.join(OUT_DIR, f"{out_prefix}.{ext}")
        fig.savefig(p)
        print(f"  Saved {p}")
    plt.close(fig)


# ── main ──────────────────────────────────────────────────────────────────────

def main():
    no_run = "--no-run" in sys.argv
    print("PIMeval ECC Benchmark Suite — generating plots (9 workloads × 4 modes)")
    print(f"  Output: {OUT_DIR}")

    if not no_run:
        print("  Re-running benchmarks (pass --no-run to skip)...")
        reload_from_benchmark()
    else:
        print("  Using hardcoded data (--no-run)")

    print("\nGenerating figures:")
    fig_a("fig_a_ecc_overhead_abs")
    fig_b("fig_b_ecc_overhead_pct")
    fig_c("fig_c_runtime_breakdown")

    print("\nDone.")
    print("  fig_a_ecc_overhead_abs.*  — grouped stacked bar: absolute ECC overhead (log), 4 modes")
    print("  fig_b_ecc_overhead_pct.*  — total runtime normalized to Mode1=1.0×, 4 modes")
    print("  fig_c_runtime_breakdown.* — Mode3 100% stacked runtime breakdown")


if __name__ == "__main__":
    main()
