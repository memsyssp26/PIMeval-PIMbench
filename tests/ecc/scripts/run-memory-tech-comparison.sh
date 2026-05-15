#!/usr/bin/env bash
# run-memory-tech-comparison.sh — Run a benchmark under DDR5, HBM3, and LPDDR5
# memory-technology presets and print a side-by-side ECC overhead comparison.
#
# Usage:
#   ./run-memory-tech-comparison.sh <benchmark_binary> [benchmark_args...]
#
# Example:
#   ./run-memory-tech-comparison.sh ../benchmarks/ecc_bench.out vec-add
#
# Each preset uses Mode 3 (all three ECC tiers: ODECC + controller + scratchpad).
# The key differences across columns are ODECC params mandated by the JEDEC spec:
#   DDR5  : 128+8  SECDED, 1.0 ns, 0.50 pJ  (JESD79-5)
#   HBM3  : 256+16 SECDED, 1.5 ns, 0.80 pJ  (JESD235C)
#   LPDDR5: 128+8  SECDED, 0.8 ns, 0.35 pJ  (JESD209-5)
#
# Environment:
#   SWEEP_TIMEOUT   — per-preset timeout in seconds (default: 120, 0 = disabled)
#   SWEEP_DEVICE    — PIMEVAL_DEVICE_TYPE override (default: PIM_DEVICE_BITSIMD_V)
#   SWEEP_NUM_RANKS / SWEEP_NUM_BANKS / SWEEP_NUM_SUBARRAYS — geometry overrides

set -euo pipefail

# ── Timeout detection ─────────────────────────────────────────────────────────
SWEEP_TIMEOUT="${SWEEP_TIMEOUT:-120}"
_TIMEOUT_CMD=""
if [[ "$SWEEP_TIMEOUT" -gt 0 ]]; then
    if   command -v timeout  &>/dev/null; then _TIMEOUT_CMD="timeout";
    elif command -v gtimeout &>/dev/null; then _TIMEOUT_CMD="gtimeout";
    else echo "Warning: no 'timeout' command found; install GNU coreutils for hang protection." >&2
    fi
fi

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <benchmark_binary> [benchmark_args...]" >&2
    exit 1
fi

BENCH="$1"; shift
BENCH_ARGS=("$@")
BENCH_NAME=$(basename "$BENCH" .out)

if [[ ! -x "$BENCH" ]]; then
    echo "Error: '$BENCH' not found or not executable." >&2
    exit 1
fi

# ── Device geometry (same for all presets) ────────────────────────────────────
SWEEP_DEVICE="${SWEEP_DEVICE:-PIM_DEVICE_BITSIMD_V}"
SWEEP_NUM_RANKS="${SWEEP_NUM_RANKS:-1}"
SWEEP_NUM_BANKS="${SWEEP_NUM_BANKS:-4}"
SWEEP_NUM_SUBARRAYS="${SWEEP_NUM_SUBARRAYS:-4}"

_device_env=(
    PIMEVAL_DEVICE_TYPE="$SWEEP_DEVICE"
    PIMEVAL_NUM_RANKS="$SWEEP_NUM_RANKS"
    PIMEVAL_NUM_BANKS="$SWEEP_NUM_BANKS"
    PIMEVAL_NUM_SUBARRAYS="$SWEEP_NUM_SUBARRAYS"
)

# ── Per-preset env (Mode 3: all tiers; ODECC params come from the preset) ─────
# Each preset sets its own odecc_data_width / odecc_parity_width / latency / energy.
# We then layer on controller + scratchpad ECC on top.
_ecc_common=(
    PIMEVAL_ECC=1
    PIMEVAL_ECC_TYPE=secded
    PIMEVAL_ECC_READOUT_ONLY=1
    PIMEVAL_SCRATCHPAD=1
    PIMEVAL_SCRATCHPAD_ECC=1
    PIMEVAL_SCRATCHPAD_ECC_TYPE=secded
)

ddr5_env=(
    "${_device_env[@]}"
    PIMEVAL_PRESET=ddr5
    "${_ecc_common[@]}"
)

hbm3_env=(
    "${_device_env[@]}"
    PIMEVAL_PRESET=hbm3
    "${_ecc_common[@]}"
)

lpddr5_env=(
    "${_device_env[@]}"
    PIMEVAL_PRESET=lpddr5
    "${_ecc_common[@]}"
)

# ── Runner ────────────────────────────────────────────────────────────────────
run_to_file() {
    local outfile="$1" label="$2"; shift 2
    local env_str=""
    for pair in "$@"; do env_str="$env_str $pair"; done

    if [[ -n "$_TIMEOUT_CMD" ]]; then
        $_TIMEOUT_CMD "$SWEEP_TIMEOUT" env $env_str "$BENCH" "${BENCH_ARGS[@]}" \
            >"$outfile" 2>&1 \
            || echo "[${label}: TIMEOUT or ERROR after ${SWEEP_TIMEOUT}s]" >>"$outfile"
    else
        env $env_str "$BENCH" "${BENCH_ARGS[@]}" >"$outfile" 2>&1 || true
    fi
}

# ── Parser helpers (shared with run-ecc-comparison.sh) ───────────────────────
extract_copy_ms() {
    echo "$1" | awk '/TOTAL ---------/ && /ms Estimated Runtime/ {
        n = split($0, parts, " ms Estimated Runtime")
        m = split(parts[1], tok, " ")
        print tok[m]; exit
    }'
}
extract_copy_mj() {
    echo "$1" | awk '/TOTAL ---------/ && /mj Estimated Energy/ {
        n = split($0, parts, " mj Estimated Energy")
        m = split(parts[1], tok, " ")
        print tok[m]; exit
    }'
}
extract_compute_ms() {
    echo "$1" | awk '/PIM Command Stats:/{ in_cmd=1 } in_cmd && /TOTAL ---------/{ print $5; exit }'
}
extract_compute_mj() {
    echo "$1" | awk '/PIM Command Stats:/{ in_cmd=1 } in_cmd && /TOTAL ---------/{ print $6; exit }'
}
extract_ecc_field() {
    local out="$1" label="$2" field="$3"
    echo "$out" | awk -v lbl="$label" -v f="$field" '
        /ECC Reliability Stats:/ { in_ecc=1 }
        in_ecc && $0 ~ lbl {
            split($0, parts, ":")
            n = split(parts[2], vals, " ")
            if (n >= f) { print vals[f]; exit }
        }
    '
}

fmt() { printf "%14s" "${1:-N/A}"; }

# ── Run all three presets in parallel ─────────────────────────────────────────
echo "=========================================================="
echo "  Memory-Technology Comparison: $BENCH_NAME"
echo "=========================================================="
echo "  DDR5   — JESD79-5:  128+8  SECDED, 1.0ns, 0.50pJ/access"
echo "  HBM3   — JESD235C:  256+16 SECDED, 1.5ns, 0.80pJ/access"
echo "  LPDDR5 — JESD209-5: 128+8  SECDED, 0.8ns, 0.35pJ/access"
echo "  (All presets use Mode 3: ODECC + controller + scratchpad)"
echo "=========================================================="
echo ""

_TMP=$(mktemp -d)
trap 'rm -rf "$_TMP"' EXIT

TDDR5="$_TMP/ddr5" THBM3="$_TMP/hbm3" TLPDDR5="$_TMP/lpddr5"

echo "Running DDR5, HBM3, LPDDR5 in parallel (timeout=${SWEEP_TIMEOUT}s each)..." >&2
run_to_file "$TDDR5"   "DDR5"   "${ddr5_env[@]}"   &
run_to_file "$THBM3"   "HBM3"   "${hbm3_env[@]}"   &
run_to_file "$TLPDDR5" "LPDDR5" "${lpddr5_env[@]}" &
wait
echo "All presets complete." >&2

ODDR5=$(cat "$TDDR5") OHBM3=$(cat "$THBM3") OLPDDR5=$(cat "$TLPDDR5")

for label in "DDR5:$ODDR5" "HBM3:$OHBM3" "LPDDR5:$OLPDDR5"; do
    tag="${label%%:*}"; body="${label#*:}"
    if echo "$body" | grep -q "TIMEOUT or ERROR"; then
        echo "ERROR: $tag timed out. Is the binary using a large PIM_FUNCTIONAL device?" >&2
        exit 1
    fi
done

# ── Extract numbers ───────────────────────────────────────────────────────────
CM_DDR5=$(extract_copy_ms   "$ODDR5");   CMJ_DDR5=$(extract_copy_mj   "$ODDR5")
CM_HBM3=$(extract_copy_ms   "$OHBM3");   CMJ_HBM3=$(extract_copy_mj   "$OHBM3")
CM_LPDDR5=$(extract_copy_ms "$OLPDDR5"); CMJ_LPDDR5=$(extract_copy_mj "$OLPDDR5")

CO_DDR5=$(extract_compute_ms   "$ODDR5");   COJ_DDR5=$(extract_compute_mj   "$ODDR5")
CO_HBM3=$(extract_compute_ms   "$OHBM3");   COJ_HBM3=$(extract_compute_mj   "$OHBM3")
CO_LPDDR5=$(extract_compute_ms "$OLPDDR5"); COJ_LPDDR5=$(extract_compute_mj "$OLPDDR5")

OD_DDR5_MS=$(extract_ecc_field "$ODDR5"   "On-Die ECC" 1); OD_DDR5_MJ=$(extract_ecc_field "$ODDR5"   "On-Die ECC" 2)
OD_HBM3_MS=$(extract_ecc_field "$OHBM3"   "On-Die ECC" 1); OD_HBM3_MJ=$(extract_ecc_field "$OHBM3"   "On-Die ECC" 2)
OD_LPDDR5_MS=$(extract_ecc_field "$OLPDDR5" "On-Die ECC" 1); OD_LPDDR5_MJ=$(extract_ecc_field "$OLPDDR5" "On-Die ECC" 2)

CE_DDR5_MS=$(extract_ecc_field "$ODDR5"   "Controller ECC" 1); CE_DDR5_MJ=$(extract_ecc_field "$ODDR5"   "Controller ECC" 2)
CE_HBM3_MS=$(extract_ecc_field "$OHBM3"   "Controller ECC" 1); CE_HBM3_MJ=$(extract_ecc_field "$OHBM3"   "Controller ECC" 2)
CE_LPDDR5_MS=$(extract_ecc_field "$OLPDDR5" "Controller ECC" 1); CE_LPDDR5_MJ=$(extract_ecc_field "$OLPDDR5" "Controller ECC" 2)

SP_DDR5_MS=$(extract_ecc_field "$ODDR5"   "Scratchpad ECC" 1); SP_DDR5_MJ=$(extract_ecc_field "$ODDR5"   "Scratchpad ECC" 2)
SP_HBM3_MS=$(extract_ecc_field "$OHBM3"   "Scratchpad ECC" 1); SP_HBM3_MJ=$(extract_ecc_field "$OHBM3"   "Scratchpad ECC" 2)
SP_LPDDR5_MS=$(extract_ecc_field "$OLPDDR5" "Scratchpad ECC" 1); SP_LPDDR5_MJ=$(extract_ecc_field "$OLPDDR5" "Scratchpad ECC" 2)

# ── Print comparison table ────────────────────────────────────────────────────
COL="%-38s"
SEP=$(printf '%0.s-' {1..80})

printf "$COL %14s %14s %14s\n" "Metric" "DDR5" "HBM3" "LPDDR5"
echo "$SEP"
printf "$COL %14s %14s %14s\n" "Copy runtime (ms)"        "$(fmt $CM_DDR5)"  "$(fmt $CM_HBM3)"  "$(fmt $CM_LPDDR5)"
printf "$COL %14s %14s %14s\n" "Copy energy  (mJ)"        "$(fmt $CMJ_DDR5)" "$(fmt $CMJ_HBM3)" "$(fmt $CMJ_LPDDR5)"
printf "$COL %14s %14s %14s\n" "Compute runtime (ms)"     "$(fmt $CO_DDR5)"  "$(fmt $CO_HBM3)"  "$(fmt $CO_LPDDR5)"
printf "$COL %14s %14s %14s\n" "Compute energy  (mJ)"     "$(fmt $COJ_DDR5)" "$(fmt $COJ_HBM3)" "$(fmt $COJ_LPDDR5)"
echo "$SEP"
printf "$COL %14s %14s %14s\n" "  ODECC overhead (ms)"    "$(fmt $OD_DDR5_MS)"   "$(fmt $OD_HBM3_MS)"   "$(fmt $OD_LPDDR5_MS)"
printf "$COL %14s %14s %14s\n" "  ODECC overhead (mJ)"    "$(fmt $OD_DDR5_MJ)"   "$(fmt $OD_HBM3_MJ)"   "$(fmt $OD_LPDDR5_MJ)"
printf "$COL %14s %14s %14s\n" "  Controller ECC (ms)"    "$(fmt $CE_DDR5_MS)"   "$(fmt $CE_HBM3_MS)"   "$(fmt $CE_LPDDR5_MS)"
printf "$COL %14s %14s %14s\n" "  Controller ECC (mJ)"    "$(fmt $CE_DDR5_MJ)"   "$(fmt $CE_HBM3_MJ)"   "$(fmt $CE_LPDDR5_MJ)"
printf "$COL %14s %14s %14s\n" "  Scratchpad ECC (ms)"    "$(fmt $SP_DDR5_MS)"   "$(fmt $SP_HBM3_MS)"   "$(fmt $SP_LPDDR5_MS)"
printf "$COL %14s %14s %14s\n" "  Scratchpad ECC (mJ)"    "$(fmt $SP_DDR5_MJ)"   "$(fmt $SP_HBM3_MJ)"   "$(fmt $SP_LPDDR5_MJ)"
echo "$SEP"

total_ms() { awk -v a="${1:-0}" -v b="${2:-0}" 'BEGIN{printf "%.6f", a+b}'; }
total_mj() { awk -v a="${1:-0}" -v b="${2:-0}" 'BEGIN{printf "%.9f", a+b}'; }

TM_DDR5=$(total_ms   "$CM_DDR5"   "$CO_DDR5");   TMJ_DDR5=$(total_mj   "$CMJ_DDR5"   "$COJ_DDR5")
TM_HBM3=$(total_ms   "$CM_HBM3"   "$CO_HBM3");   TMJ_HBM3=$(total_mj   "$CMJ_HBM3"   "$COJ_HBM3")
TM_LPDDR5=$(total_ms "$CM_LPDDR5" "$CO_LPDDR5"); TMJ_LPDDR5=$(total_mj "$CMJ_LPDDR5" "$COJ_LPDDR5")

printf "$COL %14s %14s %14s\n" "Total runtime (ms)" "$(fmt $TM_DDR5)"   "$(fmt $TM_HBM3)"   "$(fmt $TM_LPDDR5)"
printf "$COL %14s %14s %14s\n" "Total energy  (mJ)" "$(fmt $TMJ_DDR5)"  "$(fmt $TMJ_HBM3)"  "$(fmt $TMJ_LPDDR5)"

echo ""
echo "=========================================================="
echo "  ODECC overhead as fraction of total runtime (JEDEC impact)"
echo "=========================================================="
odecc_pct() {
    awk -v od="${1:-0}" -v tot="${2:-0}" \
        'BEGIN{ if(tot==0) print "N/A"; else printf "%.1f%%", od/tot*100 }'
}
printf "  DDR5  : ODECC = %s of total runtime\n"   "$(odecc_pct "$OD_DDR5_MS"   "$TM_DDR5")"
printf "  HBM3  : ODECC = %s of total runtime\n"   "$(odecc_pct "$OD_HBM3_MS"   "$TM_HBM3")"
printf "  LPDDR5: ODECC = %s of total runtime\n\n" "$(odecc_pct "$OD_LPDDR5_MS" "$TM_LPDDR5")"

# ── Raw stats (for debugging / deeper inspection) ─────────────────────────────
echo "=========================================================="
echo "  Raw pimShowStats output per memory technology"
echo "=========================================================="
for tech in DDR5 HBM3 LPDDR5; do
    echo ""
    echo "--- $tech ---"
    case $tech in
        DDR5)   echo "$ODDR5"   | grep -A 200 "^PIM Params:" | head -80 ;;
        HBM3)   echo "$OHBM3"   | grep -A 200 "^PIM Params:" | head -80 ;;
        LPDDR5) echo "$OLPDDR5" | grep -A 200 "^PIM Params:" | head -80 ;;
    esac
done
