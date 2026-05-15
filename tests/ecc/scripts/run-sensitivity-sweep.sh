#!/usr/bin/env bash
# run-sensitivity-sweep.sh — Sweep one PIMEVAL_* parameter over a list of values
# and collect ECC overhead data across all three modes in CSV format.
#
# Usage:
#   ./run-sensitivity-sweep.sh <param_name> <val1,val2,...> <benchmark_binary> [bench_args...]
#
# Examples:
#   # Sweep ODECC latency from 0.5 to 4.0 ns on vec-add
#   ./run-sensitivity-sweep.sh PIMEVAL_ODECC_LATENCY_NS 0.5,1.0,1.5,2.0,4.0 \
#       ./benchmarks/ecc_bench.out vec-add
#
#   # Sweep ECC granularity on gemv
#   ./run-sensitivity-sweep.sh PIMEVAL_ECC_GRANULARITY 8,16,32,64,128 \
#       ./benchmarks/ecc_bench.out gemv
#
#   # Sweep ODECC energy on relu, redirect output to CSV file
#   ./run-sensitivity-sweep.sh PIMEVAL_ODECC_ENERGY_PJ 0.1,0.5,1.0,2.0 \
#       ./benchmarks/ecc_bench.out relu > results/odecc_energy_sweep.csv
#
# Output: CSV with columns:
#   param_name, param_value, workload, mode,
#   copy_ms, copy_mj, compute_ms, compute_mj,
#   odecc_ms, odecc_mj, controller_ms, controller_mj,
#   scratchpad_ms, scratchpad_mj, total_ms, total_mj
#
# Environment:
#   SWEEP_TIMEOUT    — per-mode timeout in seconds (default: 120, 0 = disabled)
#   SWEEP_DEVICE     — PIMEVAL_DEVICE_TYPE (default: PIM_DEVICE_BITSIMD_V)
#   SWEEP_NUM_RANKS / SWEEP_NUM_BANKS / SWEEP_NUM_SUBARRAYS — geometry overrides
#   SWEEP_BASE_ENV   — space-separated "KEY=VAL" pairs applied to all runs
#                      (e.g. to set a preset before sweeping one param over it)
#
# Requires: awk, grep (POSIX)

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

# ── Arguments ─────────────────────────────────────────────────────────────────
if [[ $# -lt 3 ]]; then
    echo "Usage: $0 <PIMEVAL_PARAM> <val1,val2,...> <benchmark_binary> [bench_args...]" >&2
    echo ""
    echo "Examples:"  >&2
    echo "  $0 PIMEVAL_ODECC_LATENCY_NS 0.5,1.0,1.5,2.0 ./benchmarks/ecc_bench.out vec-add" >&2
    echo "  $0 PIMEVAL_ECC_GRANULARITY 8,16,32,64 ./benchmarks/ecc_bench.out gemv" >&2
    exit 1
fi

SWEEP_PARAM="$1"; shift
SWEEP_VALUES_STR="$1"; shift
BENCH="$1"; shift
BENCH_ARGS=("$@")

if [[ ! -x "$BENCH" ]]; then
    echo "Error: '$BENCH' not found or not executable." >&2
    exit 1
fi

# Parse comma-separated values into array
IFS=',' read -ra SWEEP_VALUES <<< "$SWEEP_VALUES_STR"
if [[ ${#SWEEP_VALUES[@]} -eq 0 ]]; then
    echo "Error: no values provided for '$SWEEP_PARAM'" >&2
    exit 1
fi

BENCH_NAME=$(basename "$BENCH" .out)

# ── Device geometry (shared across all runs) ──────────────────────────────────
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

# Optional base env from SWEEP_BASE_ENV (e.g. "PIMEVAL_PRESET=ddr5")
_base_env=()
SWEEP_BASE_ENV="${SWEEP_BASE_ENV:-}"
if [[ -n "$SWEEP_BASE_ENV" ]]; then
    read -ra _base_env <<< "$SWEEP_BASE_ENV"
fi

# ── ECC mode env presets ──────────────────────────────────────────────────────
_mode1_base=(
    PIMEVAL_ODECC=1
    PIMEVAL_ECC=0
    PIMEVAL_SCRATCHPAD=0
)
_mode2_base=(
    PIMEVAL_ODECC=1
    PIMEVAL_ECC=1
    PIMEVAL_ECC_TYPE=secded
    PIMEVAL_ECC_READOUT_ONLY=1
    PIMEVAL_SCRATCHPAD=0
)
_mode3_base=(
    PIMEVAL_ODECC=1
    PIMEVAL_ECC=1
    PIMEVAL_ECC_TYPE=secded
    PIMEVAL_ECC_READOUT_ONLY=1
    PIMEVAL_SCRATCHPAD=1
    PIMEVAL_SCRATCHPAD_ECC=1
    PIMEVAL_SCRATCHPAD_ECC_TYPE=secded
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

# ── Parser helpers ────────────────────────────────────────────────────────────
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
total_ms() { awk -v a="${1:-0}" -v b="${2:-0}" 'BEGIN{printf "%.6f", a+b}'; }
total_mj() { awk -v a="${1:-0}" -v b="${2:-0}" 'BEGIN{printf "%.9f", a+b}'; }

# ── Header ────────────────────────────────────────────────────────────────────
echo "param_name,param_value,workload,mode,copy_ms,copy_mj,compute_ms,compute_mj,odecc_ms,odecc_mj,controller_ms,controller_mj,scratchpad_ms,scratchpad_mj,total_ms,total_mj"

# ── Main sweep loop ───────────────────────────────────────────────────────────
_TMP=$(mktemp -d)
trap 'rm -rf "$_TMP"' EXIT

for val in "${SWEEP_VALUES[@]}"; do
    echo "Sweeping ${SWEEP_PARAM}=${val} ..." >&2

    # Build per-mode env: device + base + mode ECC + sweep param override
    # The sweep param is appended last so it wins over any conflicting preset/mode value.
    m1_env=("${_device_env[@]}" ${_base_env[@]+"${_base_env[@]}"} "${_mode1_base[@]}" "${SWEEP_PARAM}=${val}")
    m2_env=("${_device_env[@]}" ${_base_env[@]+"${_base_env[@]}"} "${_mode2_base[@]}" "${SWEEP_PARAM}=${val}")
    m3_env=("${_device_env[@]}" ${_base_env[@]+"${_base_env[@]}"} "${_mode3_base[@]}" "${SWEEP_PARAM}=${val}")

    T1="$_TMP/m1" T2="$_TMP/m2" T3="$_TMP/m3"
    run_to_file "$T1" "Mode1" "${m1_env[@]}" &
    run_to_file "$T2" "Mode2" "${m2_env[@]}" &
    run_to_file "$T3" "Mode3" "${m3_env[@]}" &
    wait

    O1=$(cat "$T1") O2=$(cat "$T2") O3=$(cat "$T3")

    for _label_out in "Mode1:$O1" "Mode2:$O2" "Mode3:$O3"; do
        _lbl="${_label_out%%:*}"; _body="${_label_out#*:}"
        if echo "$_body" | grep -q "TIMEOUT or ERROR"; then
            echo "ERROR: ${SWEEP_PARAM}=${val} ${_lbl} timed out." >&2
            continue
        fi
    done

    for mode_num in 1 2 3; do
        case $mode_num in
            1) _out="$O1" ;;
            2) _out="$O2" ;;
            3) _out="$O3" ;;
        esac

        _cm=$(extract_copy_ms    "$_out"); _cmj=$(extract_copy_mj    "$_out")
        _co=$(extract_compute_ms "$_out"); _coj=$(extract_compute_mj "$_out")
        _od_ms=$(extract_ecc_field "$_out" "On-Die ECC"    1); _od_mj=$(extract_ecc_field "$_out" "On-Die ECC"    2)
        _ce_ms=$(extract_ecc_field "$_out" "Controller ECC" 1); _ce_mj=$(extract_ecc_field "$_out" "Controller ECC" 2)
        _sp_ms=$(extract_ecc_field "$_out" "Scratchpad ECC" 1); _sp_mj=$(extract_ecc_field "$_out" "Scratchpad ECC" 2)
        _tm=$(total_ms "$_cm" "$_co"); _tmj=$(total_mj "$_cmj" "$_coj")

        echo "${SWEEP_PARAM},${val},${BENCH_NAME},mode${mode_num},${_cm},${_cmj},${_co},${_coj},${_od_ms},${_od_mj},${_ce_ms},${_ce_mj},${_sp_ms},${_sp_mj},${_tm},${_tmj}"
    done
done

echo "Sweep complete: ${#SWEEP_VALUES[@]} values × 3 modes = $((${#SWEEP_VALUES[@]} * 3)) runs." >&2
