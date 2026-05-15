#!/usr/bin/env bash
# run-ecc-comparison.sh — Run a PIMBench binary under three ECC modes and
# print a side-by-side comparison of runtime, energy, and per-tier ECC overhead.
#
# Usage:
#   ./run-ecc-comparison.sh [--csv] <benchmark_binary> [benchmark_args...]
#
# Example:
#   ./run-ecc-comparison.sh ../../PIMbench/vec-add/PIM/vec-add.out -l 65536
#   ./run-ecc-comparison.sh ../../PIMbench/gemv/PIM/gemv.out -r 2048 -d 64
#   ./run-ecc-comparison.sh --csv ./benchmarks/ecc_bench.out relu >> results.csv
#
# ECC Modes:
#   Mode 1 — ODECC only       : 128+8 SECDED on every DRAM row activation (JEDEC mandatory)
#   Mode 2 — ODECC + readout  : Mode 1 + controller SECDED on D2H boundary only
#   Mode 3 — All three tiers  : Mode 2 + scratchpad SECDED on every SRAM word access
#
# Options:
#   --csv   Emit one CSV row per mode (header printed on first use; no human table)
#
# Environment:
#   SWEEP_TIMEOUT   — per-mode timeout in seconds (default: 120). Set to 0 to disable.
#   SWEEP_CSV       — set to 1 to enable CSV output (same as --csv flag)
#   SWEEP_CSV_HEADER— set to 0 to suppress CSV header row (useful when appending)
#
# Output: human-readable table + per-mode pimShowStats block appended at end,
#         or CSV rows if --csv / SWEEP_CSV=1.
# Requires: awk, grep (POSIX)

set -euo pipefail

# ── Parse --csv flag ──────────────────────────────────────────────────────────
SWEEP_CSV="${SWEEP_CSV:-0}"
SWEEP_CSV_HEADER="${SWEEP_CSV_HEADER:-1}"
_remaining_args=()
for _arg in "$@"; do
    if [[ "$_arg" == "--csv" ]]; then
        SWEEP_CSV=1
    else
        _remaining_args+=("$_arg")
    fi
done
set -- "${_remaining_args[@]}"

# ── Timeout detection (macOS: no built-in timeout; use gtimeout if available) ─
SWEEP_TIMEOUT="${SWEEP_TIMEOUT:-120}"
_TIMEOUT_CMD=""
if [[ "$SWEEP_TIMEOUT" -gt 0 ]]; then
    if command -v timeout  &>/dev/null; then _TIMEOUT_CMD="timeout";
    elif command -v gtimeout &>/dev/null; then _TIMEOUT_CMD="gtimeout";
    else
        echo "Warning: no 'timeout' command found; install GNU coreutils for hang protection." >&2
    fi
fi

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <benchmark_binary> [benchmark_args...]" >&2
    exit 1
fi

BENCH="$1"; shift
BENCH_ARGS=("$@")
BENCH_NAME="${SWEEP_WORKLOAD_NAME:-$(basename "$BENCH" .out)}"
# If benchmark args are provided and SWEEP_WORKLOAD_NAME is not set, use first arg as workload name
if [[ -z "${SWEEP_WORKLOAD_NAME:-}" && ${#BENCH_ARGS[@]} -gt 0 ]]; then
    BENCH_NAME="${BENCH_ARGS[0]}"
fi

if [[ ! -x "$BENCH" ]]; then
    echo "Error: '$BENCH' not found or not executable." >&2
    exit 1
fi

# ── Device override (applied to all modes) ───────────────────────────────────
# Force analytical BITSIMD_V device with a compact geometry so any PIMbench
# binary that hardcodes PIM_FUNCTIONAL + huge geometry runs without OOM/hang.
# Geometry: 1 rank × 4 banks × 4 subarrays = 16 cores; rows/cols unchanged.
# Override these via env: SWEEP_DEVICE, SWEEP_NUM_RANKS, SWEEP_NUM_BANKS, etc.
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

# ── ECC env-var presets ───────────────────────────────────────────────────────

# Mode 1: ODECC only (JEDEC-mandated 128+8 SECDED on every DRAM row activation)
mode1_env=(
    "${_device_env[@]}"
    PIMEVAL_ODECC=1
    PIMEVAL_ODECC_DATA_WIDTH=128
    PIMEVAL_ODECC_PARITY_WIDTH=8
    PIMEVAL_ECC=0
    PIMEVAL_SCRATCHPAD=0
)

# Mode 2: ODECC + controller ECC on readout (D2H) only
mode2_env=(
    "${_device_env[@]}"
    PIMEVAL_ODECC=1
    PIMEVAL_ODECC_DATA_WIDTH=128
    PIMEVAL_ODECC_PARITY_WIDTH=8
    PIMEVAL_ECC=1
    PIMEVAL_ECC_TYPE=secded
    PIMEVAL_ECC_READOUT_ONLY=1
    PIMEVAL_SCRATCHPAD=0
)

# Mode 3: All three tiers (conservative: scratchpad ECC per logic op)
mode3_env=(
    "${_device_env[@]}"
    PIMEVAL_ODECC=1
    PIMEVAL_ODECC_DATA_WIDTH=128
    PIMEVAL_ODECC_PARITY_WIDTH=8
    PIMEVAL_ECC=1
    PIMEVAL_ECC_TYPE=secded
    PIMEVAL_ECC_READOUT_ONLY=1
    PIMEVAL_SCRATCHPAD=1
    PIMEVAL_SCRATCHPAD_ECC=1
    PIMEVAL_SCRATCHPAD_ECC_TYPE=secded
)

# Mode 4: All three tiers (output-only: scratchpad ECC per output element)
mode4_env=(
    "${_device_env[@]}"
    PIMEVAL_ODECC=1
    PIMEVAL_ODECC_DATA_WIDTH=128
    PIMEVAL_ODECC_PARITY_WIDTH=8
    PIMEVAL_ECC=1
    PIMEVAL_ECC_TYPE=secded
    PIMEVAL_ECC_READOUT_ONLY=1
    PIMEVAL_SCRATCHPAD=1
    PIMEVAL_SCRATCHPAD_ECC=1
    PIMEVAL_SCRATCHPAD_ECC_TYPE=secded
    PIMEVAL_SCRATCHPAD_ECC_OUTPUT_ONLY=1
)

# ── Runner ────────────────────────────────────────────────────────────────────

# run_mode_to_file <outfile> <label> [env_pair ...]
# Runs the benchmark with the given env vars, writing stdout+stderr to outfile.
# Honours SWEEP_TIMEOUT; appends a TIMEOUT marker on expiry.
run_mode_to_file() {
    local outfile="$1" label="$2"; shift 2
    local env_str=""
    for pair in "$@"; do env_str="$env_str $pair"; done

    if [[ -n "$_TIMEOUT_CMD" ]]; then
        $_TIMEOUT_CMD "$SWEEP_TIMEOUT" env $env_str "$BENCH" "${BENCH_ARGS[@]}" \
            >"$outfile" 2>&1 \
            || echo "[${label}: TIMEOUT or ERROR after ${SWEEP_TIMEOUT}s — check binary]" >>"$outfile"
    else
        env $env_str "$BENCH" "${BENCH_ARGS[@]}" >"$outfile" 2>&1 || true
    fi
}

# ── Parser helpers ────────────────────────────────────────────────────────────

# Extract a named field from pimShowStats lines.
# Fields format examples:
#   "TOTAL --------- : 262144 bytes    0.123456 ms Estimated Runtime    0.456789 mj Estimated Energy"
#   "ECC TOTAL --------- :     0.001234     0.000000001"
#   "On-Die ECC (ODECC)  :     0.001234     0.000000002  [128+8 SECDED ...]"

extract_copy_ms() {
    # Data Copy Stats TOTAL line: "... TOTAL --------- : <bytes> bytes  <ms> ms Estimated Runtime ..."
    # Use POSIX awk: split on " ms Estimated Runtime", grab last token of left part.
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
    # PIM Command Stats TOTAL line: "TOTAL --------- : <cnt> <ms> <mj> ..."
    # Fields after awk tokenisation: $1=TOTAL $2=--------- $3=: $4=cnt $5=ms $6=mj
    echo "$1" | awk '/PIM Command Stats:/{ in_cmd=1 } in_cmd && /TOTAL ---------/{ print $5; exit }'
}

extract_compute_mj() {
    echo "$1" | awk '/PIM Command Stats:/{ in_cmd=1 } in_cmd && /TOTAL ---------/{ print $6; exit }'
}

extract_ecc_field() {
    # $1=output, $2=label pattern, $3=field index (1=ms, 2=mj)
    # Line format: "  <label> : <ms> <mj> [...]"
    # Only search inside the "ECC Reliability Stats:" section to avoid
    # false matches against PIM-Config lines with the same label names.
    local out="$1" label="$2" field="$3"
    echo "$out" | LC_ALL=C awk -v lbl="$label" -v f="$field" '
        /ECC Reliability Stats:/ { in_ecc=1 }
        in_ecc && $0 ~ lbl {
            split($0, parts, ":")
            n = split(parts[2], vals, " ")
            if (n >= f) { print vals[f]; exit }
        }
    '
}

fmt() {
    # Print value right-aligned in 14 chars, or "N/A" if empty
    local v="${1:-N/A}"
    printf "%14s" "$v"
}

# ── Run all three modes (in parallel) ────────────────────────────────────────

if [[ "$SWEEP_CSV" != "1" ]]; then
echo "=========================================================="
echo "  ECC Mode Comparison: $BENCH_NAME"
echo "=========================================================="
echo "  Mode 1 — ODECC only (JEDEC 128+8)"
echo "  Mode 2 — ODECC + controller ECC on readout (D2H)"
echo "  Mode 3 — All tiers: scratchpad ECC per logic op (conservative)"
echo "  Mode 4 — All tiers: scratchpad ECC per output element (output-only)"
echo "=========================================================="
echo ""
fi

_SWEEP_TMP=$(mktemp -d)
trap 'rm -rf "$_SWEEP_TMP"' EXIT

TMP1="$_SWEEP_TMP/mode1" TMP2="$_SWEEP_TMP/mode2"
TMP3="$_SWEEP_TMP/mode3" TMP4="$_SWEEP_TMP/mode4"

echo "Running all four modes in parallel (timeout=${SWEEP_TIMEOUT}s each)..." >&2
run_mode_to_file "$TMP1" "Mode 1" "${mode1_env[@]}" &
run_mode_to_file "$TMP2" "Mode 2" "${mode2_env[@]}" &
run_mode_to_file "$TMP3" "Mode 3" "${mode3_env[@]}" &
run_mode_to_file "$TMP4" "Mode 4" "${mode4_env[@]}" &
wait
echo "All modes complete." >&2

OUT1=$(cat "$TMP1") OUT2=$(cat "$TMP2") OUT3=$(cat "$TMP3") OUT4=$(cat "$TMP4")

# Abort early if any mode timed out
for _m in 1 2 3 4; do
    _var="OUT${_m}"
    if echo "${!_var}" | grep -q "TIMEOUT or ERROR"; then
        echo "ERROR: Mode ${_m} timed out. Is the binary using a large PIM_FUNCTIONAL device?" >&2
        echo "Hint: use ecc_bench.out (BITSIMD_V analytical) instead of PIMbench binaries." >&2
        exit 1
    fi
done

# ── Extract numbers ───────────────────────────────────────────────────────────

CM1=$(extract_copy_ms  "$OUT1"); CMJ1=$(extract_copy_mj  "$OUT1")
CM2=$(extract_copy_ms  "$OUT2"); CMJ2=$(extract_copy_mj  "$OUT2")
CM3=$(extract_copy_ms  "$OUT3"); CMJ3=$(extract_copy_mj  "$OUT3")
CM4=$(extract_copy_ms  "$OUT4"); CMJ4=$(extract_copy_mj  "$OUT4")

CO1=$(extract_compute_ms "$OUT1"); COJ1=$(extract_compute_mj "$OUT1")
CO2=$(extract_compute_ms "$OUT2"); COJ2=$(extract_compute_mj "$OUT2")
CO3=$(extract_compute_ms "$OUT3"); COJ3=$(extract_compute_mj "$OUT3")
CO4=$(extract_compute_ms "$OUT4"); COJ4=$(extract_compute_mj "$OUT4")

OD1_MS=$(extract_ecc_field "$OUT1" "On-Die ECC" 1);  OD1_MJ=$(extract_ecc_field "$OUT1" "On-Die ECC" 2)
OD2_MS=$(extract_ecc_field "$OUT2" "On-Die ECC" 1);  OD2_MJ=$(extract_ecc_field "$OUT2" "On-Die ECC" 2)
OD3_MS=$(extract_ecc_field "$OUT3" "On-Die ECC" 1);  OD3_MJ=$(extract_ecc_field "$OUT3" "On-Die ECC" 2)
OD4_MS=$(extract_ecc_field "$OUT4" "On-Die ECC" 1);  OD4_MJ=$(extract_ecc_field "$OUT4" "On-Die ECC" 2)

CE1_MS=$(extract_ecc_field "$OUT1" "Controller ECC" 1); CE1_MJ=$(extract_ecc_field "$OUT1" "Controller ECC" 2)
CE2_MS=$(extract_ecc_field "$OUT2" "Controller ECC" 1); CE2_MJ=$(extract_ecc_field "$OUT2" "Controller ECC" 2)
CE3_MS=$(extract_ecc_field "$OUT3" "Controller ECC" 1); CE3_MJ=$(extract_ecc_field "$OUT3" "Controller ECC" 2)
CE4_MS=$(extract_ecc_field "$OUT4" "Controller ECC" 1); CE4_MJ=$(extract_ecc_field "$OUT4" "Controller ECC" 2)

SP1_MS=$(extract_ecc_field "$OUT1" "Scratchpad ECC" 1); SP1_MJ=$(extract_ecc_field "$OUT1" "Scratchpad ECC" 2)
SP2_MS=$(extract_ecc_field "$OUT2" "Scratchpad ECC" 1); SP2_MJ=$(extract_ecc_field "$OUT2" "Scratchpad ECC" 2)
SP3_MS=$(extract_ecc_field "$OUT3" "Scratchpad ECC" 1); SP3_MJ=$(extract_ecc_field "$OUT3" "Scratchpad ECC" 2)
SP4_MS=$(extract_ecc_field "$OUT4" "Scratchpad ECC" 1); SP4_MJ=$(extract_ecc_field "$OUT4" "Scratchpad ECC" 2)

# ── Compute totals (needed for both CSV and human table) ─────────────────────
total_ms() { awk -v a="${1:-0}" -v b="${2:-0}" 'BEGIN{printf "%.6f", a+b}'; }
total_mj() { awk -v a="${1:-0}" -v b="${2:-0}" 'BEGIN{printf "%.9f", a+b}'; }

TM1=$(total_ms "$CM1" "$CO1"); TMJ1=$(total_mj "$CMJ1" "$COJ1")
TM2=$(total_ms "$CM2" "$CO2"); TMJ2=$(total_mj "$CMJ2" "$COJ2")
TM3=$(total_ms "$CM3" "$CO3"); TMJ3=$(total_mj "$CMJ3" "$COJ3")
TM4=$(total_ms "$CM4" "$CO4"); TMJ4=$(total_mj "$CMJ4" "$COJ4")

# ── Print comparison table (human-readable) ───────────────────────────────────

if [[ "$SWEEP_CSV" == "1" ]]; then
    _CSV_COLS="workload,mode,copy_ms,copy_mj,compute_ms,compute_mj,odecc_ms,odecc_mj,controller_ms,controller_mj,scratchpad_ms,scratchpad_mj,total_ms,total_mj"
    if [[ "$SWEEP_CSV_HEADER" != "0" ]]; then
        echo "$_CSV_COLS"
    fi
    echo "${BENCH_NAME},mode1,${CM1:-},${CMJ1:-},${CO1:-},${COJ1:-},${OD1_MS:-},${OD1_MJ:-},${CE1_MS:-},${CE1_MJ:-},${SP1_MS:-},${SP1_MJ:-},${TM1:-},${TMJ1:-}"
    echo "${BENCH_NAME},mode2,${CM2:-},${CMJ2:-},${CO2:-},${COJ2:-},${OD2_MS:-},${OD2_MJ:-},${CE2_MS:-},${CE2_MJ:-},${SP2_MS:-},${SP2_MJ:-},${TM2:-},${TMJ2:-}"
    echo "${BENCH_NAME},mode3,${CM3:-},${CMJ3:-},${CO3:-},${COJ3:-},${OD3_MS:-},${OD3_MJ:-},${CE3_MS:-},${CE3_MJ:-},${SP3_MS:-},${SP3_MJ:-},${TM3:-},${TMJ3:-}"
    echo "${BENCH_NAME},mode4,${CM4:-},${CMJ4:-},${CO4:-},${COJ4:-},${OD4_MS:-},${OD4_MJ:-},${CE4_MS:-},${CE4_MJ:-},${SP4_MS:-},${SP4_MJ:-},${TM4:-},${TMJ4:-}"
    exit 0
fi

COL="%-38s"
HDR=$(printf "$COL %13s %13s %13s %13s\n" "Metric" "Mode1(ODECC)" "Mode2(+Ctrl)" "Mode3(+Spad)" "Mode4(OutOnly)")
SEP=$(printf '%0.s-' {1..93})

echo "$HDR"
echo "$SEP"
printf "$COL %13s %13s %13s %13s\n" "Copy runtime (ms)"       "$(fmt $CM1)"  "$(fmt $CM2)"  "$(fmt $CM3)"  "$(fmt $CM4)"
printf "$COL %13s %13s %13s %13s\n" "Compute runtime (ms)"    "$(fmt $CO1)"  "$(fmt $CO2)"  "$(fmt $CO3)"  "$(fmt $CO4)"
echo "$SEP"
printf "$COL %13s %13s %13s %13s\n" "  ECC: ODECC overhead (ms)"      "$(fmt $OD1_MS)" "$(fmt $OD2_MS)" "$(fmt $OD3_MS)" "$(fmt $OD4_MS)"
printf "$COL %13s %13s %13s %13s\n" "  ECC: Controller overhead (ms)" "$(fmt $CE1_MS)" "$(fmt $CE2_MS)" "$(fmt $CE3_MS)" "$(fmt $CE4_MS)"
printf "$COL %13s %13s %13s %13s\n" "  ECC: Scratchpad overhead (ms)" "$(fmt $SP1_MS)" "$(fmt $SP2_MS)" "$(fmt $SP3_MS)" "$(fmt $SP4_MS)"
echo "$SEP"
printf "$COL %13s %13s %13s %13s\n" "Total runtime (ms)"  "$(fmt $TM1)"  "$(fmt $TM2)"  "$(fmt $TM3)"  "$(fmt $TM4)"

echo ""
echo "=========================================================="
echo "  Incremental ECC overhead vs Mode 1 baseline"
echo "=========================================================="
delta_ms() { awk -v a="${1:-0}" -v b="${2:-0}" 'BEGIN{d=a-b; printf "%+.6f", d}'; }
delta_pct() { awk -v a="${1:-0}" -v b="${2:-0}" 'BEGIN{ if(b==0) print "N/A"; else printf "%+.1f%%", (a-b)/b*100 }'; }

printf "%-38s %13s %13s %13s\n" "Metric" "Mode2 delta" "Mode3 delta" "Mode4 delta"
echo "$(printf '%0.s-' {1..80})"
printf "%-38s %13s %13s %13s\n" "Total runtime delta (ms)" \
    "$(delta_ms $TM2 $TM1)" "$(delta_ms $TM3 $TM1)" "$(delta_ms $TM4 $TM1)"
printf "%-38s %13s %13s %13s\n" "Total runtime delta (%)" \
    "$(delta_pct $TM2 $TM1)" "$(delta_pct $TM3 $TM1)" "$(delta_pct $TM4 $TM1)"

echo ""

# ── Append raw pimShowStats output for each mode ──────────────────────────────
echo "=========================================================="
echo "  Raw pimShowStats output per mode"
echo "=========================================================="
for mode_num in 1 2 3 4; do
    echo ""
    echo "--- Mode $mode_num ---"
    case $mode_num in
        1) echo "$OUT1" | grep -A 200 "^PIM Params:" | head -60 ;;
        2) echo "$OUT2" | grep -A 200 "^PIM Params:" | head -60 ;;
        3) echo "$OUT3" | grep -A 200 "^PIM Params:" | head -60 ;;
        4) echo "$OUT4" | grep -A 200 "^PIM Params:" | head -60 ;;
    esac
done
