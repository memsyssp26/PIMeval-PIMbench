#!/usr/bin/env python3
"""
PIMeval ECC Overhead — Publication-Quality Plots
Generates four figures for the Q3 scratchpad/register-file ECC report.

Figures produced:
  Fig 1 — Absolute runtime per ECC tier configuration (stacked bar)
  Fig 2 — Per-tier overhead contribution as % of baseline (waterfall bar)
  Fig 3 — Analytical: SECDED parity bits required vs. data word width
  Fig 4 — Analytical: ECC storage overhead (%) vs. protection granularity
             for SECDED, RS(255,k), and CRC-32

Usage:
  python3 plot_ecc_overhead.py [--no-run]   # --no-run skips benchmark, uses cached CSV
"""

import subprocess
import sys
import os
import csv
import io
import math
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np

# ── paths ────────────────────────────────────────────────────────────────────
SCRIPT_DIR  = os.path.dirname(os.path.abspath(__file__))
BENCH_EXE   = os.path.join(SCRIPT_DIR, "ecc_benchmark.out")
OUT_DIR     = SCRIPT_DIR

# ── style ────────────────────────────────────────────────────────────────────
COLORS = {
    "base":       "#4C72B0",
    "odecc":      "#DD8452",
    "scratchpad": "#55A868",
    "controller": "#C44E52",
}
HATCHES = {
    "base":       "",
    "odecc":      "//",
    "scratchpad": "xx",
    "controller": "..",
}

plt.rcParams.update({
    "font.family":     "serif",
    "font.size":       11,
    "axes.titlesize":  13,
    "axes.labelsize":  12,
    "xtick.labelsize": 10,
    "ytick.labelsize": 10,
    "legend.fontsize": 10,
    "figure.dpi":      150,
    "savefig.dpi":     300,
    "savefig.bbox":    "tight",
})


# ── benchmark helpers ─────────────────────────────────────────────────────────

def run_benchmark(num_elements: int, mode: str) -> str:
    """Run ecc_benchmark.out and return only CSV lines (PIM-Config noise filtered out)."""
    result = subprocess.run(
        [BENCH_EXE, str(num_elements), mode],
        capture_output=True, text=True, check=True
    )
    # PIM-Config/PIM-Info messages go to stdout; keep only lines with commas
    csv_lines = [ln for ln in result.stdout.splitlines()
                 if "," in ln and not ln.startswith("PIM-") and not ln.startswith("-")]
    return "\n".join(csv_lines)


def parse_csv(text: str) -> list[dict]:
    reader = csv.DictReader(io.StringIO(text))
    rows = []
    for row in reader:
        rows.append({k: (float(v) if k != "label" else v) for k, v in row.items()})
    return rows


# ── Figure 1: absolute runtime stacked bar ───────────────────────────────────

def fig1_runtime_bars(rows: list[dict], out_path: str):
    """
    Stacked bar chart showing absolute runtime for each ECC tier configuration.
    Rows must come from sweep_tiers output.
    """
    configs = [r["label"] for r in rows]
    totals  = [r["total_ms"] for r in rows]
    base    = totals[0]  # No-ECC baseline

    # Derive per-tier overhead components
    # r0=none, r1=odecc, r2=scratch, r3=odecc+scratch, r4=all
    odecc_only_delta    = totals[1] - totals[0]  # pure ODECC cost
    scratch_only_delta  = totals[2] - totals[0]  # pure scratchpad cost
    ctrl_delta          = totals[4] - totals[3]  # controller tier marginal cost

    # Per-config breakdown: base + odecc_component + scratch_component + ctrl_component
    odecc_comp  = [0, odecc_only_delta, 0, odecc_only_delta, odecc_only_delta]
    scratch_comp= [0, 0, scratch_only_delta, scratch_only_delta, scratch_only_delta]
    ctrl_comp   = [0, 0, 0, 0, ctrl_delta]
    base_comp   = [base] * 5

    x     = np.arange(len(configs))
    width = 0.55

    fig, ax = plt.subplots(figsize=(8, 5))

    def bar(bottom, values, label, key):
        bars = ax.bar(x, values, width, bottom=bottom,
                      color=COLORS[key], hatch=HATCHES[key],
                      label=label, edgecolor="white", linewidth=0.6)
        return [b + v for b, v in zip(bottom, values)]

    bot = [0.0] * 5
    bot = bar(bot, base_comp,    "Base (no ECC)",       "base")
    bot = bar(bot, odecc_comp,   "On-Die ECC",          "odecc")
    bot = bar(bot, scratch_comp, "Scratchpad ECC",      "scratchpad")
    bot = bar(bot, ctrl_comp,    "Controller ECC",      "controller")

    # Annotate total ms above each bar
    for i, total in enumerate(totals):
        ax.text(x[i], total + 0.00005, f"{total*1000:.1f} μs",
                ha="center", va="bottom", fontsize=8.5, fontweight="bold")

    ax.set_xticks(x)
    ax.set_xticklabels(configs, rotation=15, ha="right")
    ax.set_ylabel("Runtime (ms)")
    ax.set_title("Fig 1 — PIMeval ADD_SCALAR Runtime per ECC Tier Configuration\n"
                 "(256 INT32 elements, BitSIMD-V, 1 rank × 1 bank × 4 subarrays)")
    ax.legend(loc="upper left", framealpha=0.9)
    ax.set_ylim(0, max(totals) * 1.20)
    ax.yaxis.grid(True, linestyle="--", alpha=0.5)
    ax.set_axisbelow(True)

    fig.tight_layout()
    fig.savefig(out_path)
    plt.close(fig)
    print(f"  Saved {out_path}")


# ── Figure 2: per-tier overhead % waterfall ───────────────────────────────────

def fig2_overhead_pct(rows: list[dict], out_path: str):
    """
    Horizontal bar chart showing each ECC tier's overhead as % of no-ECC runtime.
    """
    base = rows[0]["total_ms"]

    tiers = [
        ("On-Die ECC\n(per-row activation)", rows[1]["total_ms"] - base, "odecc"),
        ("Scratchpad ECC\n(per-word access)",   rows[2]["total_ms"] - base, "scratchpad"),
        ("Controller ECC\n(at copy boundary)",  rows[4]["total_ms"] - rows[3]["total_ms"], "controller"),
    ]

    labels = [t[0] for t in tiers]
    values = [t[1] / base * 100 for t in tiers]
    keys   = [t[2] for t in tiers]

    fig, ax = plt.subplots(figsize=(7, 3.5))

    y = np.arange(len(labels))
    bars = ax.barh(y, values, height=0.5,
                   color=[COLORS[k] for k in keys],
                   edgecolor="white", linewidth=0.8)

    for bar, val in zip(bars, values):
        ax.text(val + 0.3, bar.get_y() + bar.get_height() / 2,
                f"+{val:.1f}%", va="center", fontsize=10, fontweight="bold")

    ax.set_yticks(y)
    ax.set_yticklabels(labels)
    ax.set_xlabel("Runtime Overhead vs. No-ECC Baseline (%)")
    ax.set_title("Fig 2 — Per-Tier ECC Runtime Overhead\n"
                 "(marginal cost of enabling each tier independently)")
    ax.set_xlim(0, max(values) * 1.35)
    ax.xaxis.grid(True, linestyle="--", alpha=0.5)
    ax.set_axisbelow(True)

    fig.tight_layout()
    fig.savefig(out_path)
    plt.close(fig)
    print(f"  Saved {out_path}")


# ── Figure 3: analytical SECDED parity bits ───────────────────────────────────

def secded_parity_bits(data_bits: int) -> int:
    """Minimum parity bits p such that 2^p >= data_bits + p + 1 (SECDED adds +1)."""
    p = 1
    while (1 << p) < data_bits + p + 1:
        p += 1
    return p


def fig3_secded_analytical(out_path: str):
    """
    Line plot: parity bits required by SECDED vs data word width.
    Annotate common PIM word widths (8, 32, 64, 128 bits).
    """
    widths = list(range(1, 257))
    parity = [secded_parity_bits(w) for w in widths]
    pct    = [secded_parity_bits(w) / w * 100 for w in widths]

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(10, 4))

    # Left: parity bit count
    ax1.plot(widths, parity, color=COLORS["odecc"], linewidth=2)
    ax1.set_xlabel("Data Word Width (bits)")
    ax1.set_ylabel("Parity Bits Required")
    ax1.set_title("SECDED Parity Bit Count")
    ax1.set_xscale("log", base=2)
    ax1.xaxis.set_major_formatter(matplotlib.ticker.FuncFormatter(
        lambda v, _: f"{int(v)}"))
    ax1.grid(True, linestyle="--", alpha=0.4)

    for w, label in [(8, "8b\n(byte)"), (32, "32b\n(INT32)"),
                     (64, "64b\n(INT64)"), (128, "128b\n(SIMD)")]:
        p = secded_parity_bits(w)
        ax1.annotate(f"p={p}", xy=(w, p), xytext=(w * 1.5, p + 0.3),
                     arrowprops=dict(arrowstyle="->", lw=0.8),
                     fontsize=8.5, color="#333333")

    # Right: overhead %
    ax2.plot(widths, pct, color=COLORS["scratchpad"], linewidth=2)
    ax2.set_xlabel("Data Word Width (bits)")
    ax2.set_ylabel("Parity Overhead (%)")
    ax2.set_title("SECDED Storage Overhead %")
    ax2.set_xscale("log", base=2)
    ax2.xaxis.set_major_formatter(matplotlib.ticker.FuncFormatter(
        lambda v, _: f"{int(v)}"))
    ax2.grid(True, linestyle="--", alpha=0.4)

    for w, _ in [(8, ""), (32, ""), (64, ""), (128, "")]:
        p = secded_parity_bits(w)
        ax2.axvline(w, color="gray", linestyle=":", linewidth=0.8, alpha=0.6)
        ax2.text(w * 1.05, pct[w - 1] + 1,
                 f"{pct[w-1]:.1f}%", fontsize=8, color="#555555")

    fig.suptitle("Fig 3 — Analytical SECDED Hamming Code Properties\n"
                 "(single-error correcting, double-error detecting)",
                 fontsize=12, y=1.02)
    fig.tight_layout()
    fig.savefig(out_path)
    plt.close(fig)
    print(f"  Saved {out_path}")


# ── Figure 4: ECC storage overhead vs granularity ────────────────────────────

def fig4_granularity_overhead(out_path: str):
    """
    Analytical plot: ECC storage overhead % vs protection granularity (bytes)
    for three codes: SECDED, RS(255,k) with 16 check bytes, and CRC-32.
    """
    # Granularities in bytes, log scale
    granularities = [8, 16, 32, 64, 128, 256, 512, 1024]

    def secded_overhead(g_bytes):
        data_bits = g_bytes * 8
        parity    = secded_parity_bits(data_bits)
        return parity / data_bits * 100

    def rs_overhead(g_bytes, check_bytes=16):
        # RS adds check_bytes per g_bytes of data (as long as g_bytes ≤ 255 - check_bytes)
        # For larger granularities, we'd use multiple codewords; show per-codeword overhead
        # capped at RS(255, 255-check_bytes)
        data_bytes = min(g_bytes, 255 - check_bytes)
        return check_bytes / data_bytes * 100

    def crc32_overhead(g_bytes):
        return 4 / g_bytes * 100

    secded_vals = [secded_overhead(g) for g in granularities]
    rs_vals     = [rs_overhead(g)     for g in granularities]
    crc_vals    = [crc32_overhead(g)  for g in granularities]

    fig, ax = plt.subplots(figsize=(8, 4.5))

    ax.plot(granularities, secded_vals, "o-", color=COLORS["odecc"],
            linewidth=2, markersize=6, label="SECDED (Hamming)")
    ax.plot(granularities, rs_vals,     "s-", color=COLORS["scratchpad"],
            linewidth=2, markersize=6, label="Reed-Solomon RS(255,k), 16 check bytes")
    ax.plot(granularities, crc_vals,    "^-", color=COLORS["controller"],
            linewidth=2, markersize=6, label="CRC-32 (4 byte checksum)")

    # Annotate the default 64-byte controller ECC granularity
    ax.axvline(64, color="gray", linestyle="--", linewidth=1.2, alpha=0.7)
    ax.text(66, max(secded_vals) * 0.88, "PIMeval\ndefault\n(64 B)",
            fontsize=8.5, color="gray", va="top")

    ax.set_xscale("log", base=2)
    ax.set_xlabel("Protection Granularity (bytes)")
    ax.set_ylabel("ECC Storage Overhead (%)")
    ax.set_title("Fig 4 — Analytical ECC Storage Overhead vs. Protection Granularity\n"
                 "(controller-tier codes: SECDED, Reed-Solomon, CRC-32)")
    ax.xaxis.set_major_formatter(matplotlib.ticker.FuncFormatter(
        lambda v, _: f"{int(v)}B"))
    ax.legend(framealpha=0.9)
    ax.yaxis.grid(True, linestyle="--", alpha=0.4)
    ax.set_axisbelow(True)

    fig.tight_layout()
    fig.savefig(out_path)
    plt.close(fig)
    print(f"  Saved {out_path}")


# ── main ──────────────────────────────────────────────────────────────────────

def main():
    no_run = "--no-run" in sys.argv

    print("PIMeval ECC Overhead — generating plots")
    print(f"  Output directory: {OUT_DIR}")

    # ── collect simulation data ──────────────────────────────────────────────
    if not no_run:
        if not os.path.isfile(BENCH_EXE):
            print(f"ERROR: benchmark binary not found at {BENCH_EXE}")
            print("  Build with: cd tests/ecc && make")
            sys.exit(1)
        print("  Running benchmark (sweep_tiers, 256 elements)…")
        tier_csv  = run_benchmark(256, "sweep_tiers")
    else:
        # Hardcoded from verified run for --no-run mode
        tier_csv = (
            "label,num_elements,total_ms,scratch_ms\n"
            "No ECC,256,0.003363,0.000000\n"
            "ODECC Only,256,0.004332,0.000000\n"
            "Scratchpad ECC,256,0.005427,0.002064\n"
            "ODECC+Scratchpad,256,0.006908,0.002576\n"
            "All Three Tiers,256,0.008261,0.003088\n"
        )

    tier_rows = parse_csv(tier_csv)

    # ── generate figures ─────────────────────────────────────────────────────
    fig1_runtime_bars(tier_rows,
                      os.path.join(OUT_DIR, "fig1_runtime_bars.pdf"))
    fig1_runtime_bars(tier_rows,
                      os.path.join(OUT_DIR, "fig1_runtime_bars.png"))

    fig2_overhead_pct(tier_rows,
                      os.path.join(OUT_DIR, "fig2_overhead_pct.pdf"))
    fig2_overhead_pct(tier_rows,
                      os.path.join(OUT_DIR, "fig2_overhead_pct.png"))

    fig3_secded_analytical(os.path.join(OUT_DIR, "fig3_secded_analytical.pdf"))
    fig3_secded_analytical(os.path.join(OUT_DIR, "fig3_secded_analytical.png"))

    fig4_granularity_overhead(os.path.join(OUT_DIR, "fig4_granularity_overhead.pdf"))
    fig4_granularity_overhead(os.path.join(OUT_DIR, "fig4_granularity_overhead.png"))

    print("\nAll figures written.")
    print("  fig1_runtime_bars.*       — stacked bar: absolute runtime per tier config")
    print("  fig2_overhead_pct.*       — horizontal bar: marginal overhead per tier (%)")
    print("  fig3_secded_analytical.*  — analytical: SECDED parity bits vs word width")
    print("  fig4_granularity_overhead.* — analytical: ECC storage overhead vs granularity")


if __name__ == "__main__":
    main()
