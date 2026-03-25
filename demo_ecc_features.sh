#!/bin/bash
#
# demo_ecc_features.sh
# Interactive demonstration of PIMeval's ECC framework
# Shows all configuration methods and ECC schemes
#

set -e

BOLD="\033[1m"
DIM="\033[2m"
GREEN="\033[32m"
CYAN="\033[36m"
YELLOW="\033[33m"
RESET="\033[0m"

section() {
    echo ""
    echo -e "${BOLD}${CYAN}=== $1 ===${RESET}"
    echo ""
}

run_bench() {
    local label=$1
    shift
    local args="$@"
    echo -e "${BOLD}$label${RESET}"
    echo -e "${DIM}  Command: vec-add.out $args -l 2048${RESET}"
    echo ""

    # Run and capture full output
    local output
    output=$($VEC_ADD $args -l 2048 2>&1)

    # Show ECC config
    echo "$output" | grep -E "ECC Enabled|ECC Type" | tail -1 | sed 's/^/  /'

    # Show data copy stats: "TOTAL --------- : 3072 bytes  0.000796 ms Estimated Runtime  0.001987 mj Estimated Energy"
    local copy_line
    copy_line=$(echo "$output" | grep "TOTAL ---------" | head -1)
    if [ -n "$copy_line" ]; then
        local runtime=$(echo "$copy_line" | grep -oE '[0-9]+\.[0-9]+ ms' | head -1 | awk '{print $1}')
        local energy=$(echo "$copy_line" | grep -oE '[0-9]+\.[0-9]+ mj' | head -1 | awk '{print $1}')
        echo "  Data Copy:  Runtime = ${runtime:-0} ms,  Energy = ${energy:-0} mJ"
    fi

    # Show PIM command stats: "TOTAL --------- :  1  11978.932  29899.415  0.000  2.94  2.94  94.12"
    local cmd_line
    cmd_line=$(echo "$output" | grep "TOTAL ---------" | tail -1)
    if [ -n "$cmd_line" ]; then
        local runtime=$(echo "$cmd_line" | awk '{print $5}')
        local energy=$(echo "$cmd_line" | awk '{print $6}')
        echo "  PIM Cmds:   Runtime = ${runtime} ms,  Energy = ${energy} mJ"
    fi
    echo ""
}

# ──────────────────────────────────────────────────────────────
section "1. Build"

echo "Building libpimeval and vec-add..."
make -C libpimeval clean > /dev/null 2>&1
make -C libpimeval perf -j$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4) > /dev/null 2>&1
make -C PIMbench/vec-add/PIM clean > /dev/null 2>&1
make -C PIMbench/vec-add/PIM perf > /dev/null 2>&1
echo -e "${GREEN}Build successful.${RESET}"

VEC_ADD="./PIMbench/vec-add/PIM/vec-add.out"

# Clear env vars so only CLI args / config files drive behavior
unset PIMEVAL_ECC PIMEVAL_ECC_TYPE PIMEVAL_ECC_GRANULARITY
unset PIMEVAL_ECC_LATENCY_NS PIMEVAL_ECC_ENERGY_PJ

# ──────────────────────────────────────────────────────────────
section "2. Baseline (No ECC)"
echo "Running vec-add without ECC for comparison."
echo ""
run_bench "No ECC" "--pim-ecc=0"

# ──────────────────────────────────────────────────────────────
section "3. ECC via CLI Arguments"
echo "Passing ECC parameters directly on the command line."
echo "These --pim-* flags work with any benchmark that calls pimInit()."
echo ""

run_bench "SECDED (1-bit correct, 2-bit detect)" \
    "--pim-ecc=1 --pim-ecc_type=secded --pim-ecc_granularity=64"

run_bench "CRC-32 (detection only, low overhead)" \
    "--pim-ecc=1 --pim-ecc_type=crc32 --pim-ecc_granularity=128"

run_bench "Reed-Solomon (symbol-level detection)" \
    "--pim-ecc=1 --pim-ecc_type=rs --pim-ecc_granularity=64"

# ──────────────────────────────────────────────────────────────
section "4. ECC via Config File Profiles"
echo "Using pre-built profiles from configs/ecc_profiles/."
echo ""

run_bench "Profile: high_perf.cfg (SECDED, balanced)" \
    "--pim-sim_config=configs/ecc_profiles/high_perf.cfg"

run_bench "Profile: max_reliability.cfg (RS, max protection)" \
    "--pim-sim_config=configs/ecc_profiles/max_reliability.cfg"

run_bench "Profile: detect_only.cfg (CRC-32, lightweight)" \
    "--pim-sim_config=configs/ecc_profiles/detect_only.cfg"

# ──────────────────────────────────────────────────────────────
section "5. Latency/Energy Overrides"
echo "Override the default per-scheme latency and energy costs."
echo "Useful for modeling different hardware implementations."
echo ""

run_bench "SECDED with default costs (2 ns, 2 pJ)" \
    "--pim-ecc=1 --pim-ecc_type=secded"

run_bench "SECDED with custom costs (50 ns, 25 pJ)" \
    "--pim-ecc=1 --pim-ecc_type=secded --pim-ecc_latency_ns=50.0 --pim-ecc_energy_pj=25.0"

# ──────────────────────────────────────────────────────────────
section "6. Profile + CLI Override"
echo "Load a profile, then override specific parameters via CLI."
echo "CLI args take precedence over config file values."
echo ""

run_bench "RS profile with default latency (20 ns)" \
    "--pim-sim_config=configs/ecc_profiles/max_reliability.cfg"

run_bench "RS profile with overridden latency (50 ns)" \
    "--pim-sim_config=configs/ecc_profiles/max_reliability.cfg --pim-ecc_latency_ns=50.0"

# ──────────────────────────────────────────────────────────────
section "7. Environment Variable Method"
echo "ECC can also be configured via PIMEVAL_* environment variables."
echo "This works with any benchmark, no code changes needed."
echo ""

echo -e "${DIM}  export PIMEVAL_ECC=1"
echo -e "  export PIMEVAL_ECC_TYPE=secded"
echo -e "  ./vec-add.out -l 2048${RESET}"
echo ""

PIMEVAL_ECC=1 PIMEVAL_ECC_TYPE=secded $VEC_ADD -l 2048 2>&1 | grep -E "ECC Enabled|ECC Type" | tail -1 | sed 's/^/  /'
echo ""

# ──────────────────────────────────────────────────────────────
section "8. ECC Test Suite"
echo "Running the ECC test suite..."
echo ""

make -C tests/ecc clean > /dev/null 2>&1
make -C tests/ecc perf > /dev/null 2>&1

echo -n "  Unit tests:        "
tests/ecc/unit/test-ecc-math.out 2>&1 | tail -1

echo -n "  Integration tests: "
tests/ecc/integration/test-ecc-system.out 2>&1 | tail -1

echo ""

# ──────────────────────────────────────────────────────────────
section "Summary"

echo "Configuration precedence:  CLI args > Config file > Env vars > Defaults"
echo ""
echo "ECC Schemes:"
echo "  secded  — Corrects 1-bit, detects 2-bit errors  (default: 2 ns, 2 pJ)"
echo "  crc32   — Detects errors, no correction          (default: 5 ns, 5 pJ)"
echo "  rs      — Symbol-level error detection            (default: 20 ns, 20 pJ)"
echo ""
echo "Quick start:"
echo "  ./vec-add.out --pim-ecc=1 --pim-ecc_type=secded -l 65536"
echo ""
echo -e "${GREEN}Demo complete.${RESET}"
