#!/bin/bash

# Configuration
BENCHMARK_DIR="PIMbench/vec-add/PIM"
BENCHMARK_EXE="./vec-add.out"
NUM_ELEMENTS=1000000
PIM_SIM_TARGET="PIM_DEVICE_BITSIMD_V"

echo "Comparing PIMeval performance with and without SECDED ECC"
echo "Benchmark: vec-add (Size: $NUM_ELEMENTS)"
echo "Simulation Target: $PIM_SIM_TARGET"
echo "--------------------------------------------------------"

# Run without ECC
echo "Running WITHOUT ECC..."
export PIMEVAL_ECC=0
export PIMEVAL_SIM_TARGET=$PIM_SIM_TARGET
WITHOUT_ECC_OUT=$(cd $BENCHMARK_DIR && $BENCHMARK_EXE -l $NUM_ELEMENTS 2>&1)

# Run with ECC
echo "Running WITH ECC..."
export PIMEVAL_ECC=1
export PIMEVAL_SIM_TARGET=$PIM_SIM_TARGET
WITH_ECC_OUT=$(cd $BENCHMARK_DIR && $BENCHMARK_EXE -l $NUM_ELEMENTS 2>&1)

# Function to extract numeric value from stats
extract_copy_runtime() { echo "$1" | grep "TOTAL ---------" | head -n 1 | grep -oE "[0-9]+\.[0-9]+ ms" | awk '{print $1}'; }
extract_copy_energy()  { echo "$1" | grep "TOTAL ---------" | head -n 1 | grep -oE "[0-9]+\.[0-9]+ mj" | awk '{print $1}'; }
# Columns: "TOTAL" (1), "---------" (2), ":" (3), CNT (4), Runtime (5), Energy (6)
extract_cmd_runtime()  { echo "$1" | grep "TOTAL ---------" | tail -n 1 | awk '{print $5}'; }
extract_cmd_energy()   { echo "$1" | grep "TOTAL ---------" | tail -n 1 | awk '{print $6}'; }

NO_ECC_COPY_RT=$(extract_copy_runtime "$WITHOUT_ECC_OUT")
NO_ECC_COPY_EN=$(extract_copy_energy "$WITHOUT_ECC_OUT")
NO_ECC_CMD_RT=$(extract_cmd_runtime "$WITHOUT_ECC_OUT")
NO_ECC_CMD_EN=$(extract_cmd_energy "$WITHOUT_ECC_OUT")

ECC_COPY_RT=$(extract_copy_runtime "$WITH_ECC_OUT")
ECC_COPY_EN=$(extract_copy_energy "$WITH_ECC_OUT")
ECC_CMD_RT=$(extract_cmd_runtime "$WITH_ECC_OUT")
ECC_CMD_EN=$(extract_cmd_energy "$WITH_ECC_OUT")

echo ""
echo "Results Comparison:"
echo "--------------------------------------------------------"
printf "%-25s | %-15s | %-15s | %-10s\n" "Metric" "Without ECC" "With ECC" "Increase %"
echo "--------------------------------------------------------"

calculate_increase() {
    local old=$1
    local new=$2
    if [[ -z "$old" || -z "$new" || "$old" == "0" ]]; then
        echo "N/A"
    else
        python3 -c "print(round(($new - $old) / $old * 100, 2))" 2>/dev/null || echo "N/A"
    fi
}

printf "%-25s | %-15s | %-15s | %-10s%%\n" "Data Copy Runtime (ms)" "$NO_ECC_COPY_RT" "$ECC_COPY_RT" "$(calculate_increase $NO_ECC_COPY_RT $ECC_COPY_RT)"
printf "%-25s | %-15s | %-15s | %-10s%%\n" "Data Copy Energy (mJ)" "$NO_ECC_COPY_EN" "$ECC_COPY_EN" "$(calculate_increase $NO_ECC_COPY_EN $ECC_COPY_EN)"
printf "%-25s | %-15s | %-15s | %-10s%%\n" "PIM CMD Runtime (ms)" "$NO_ECC_CMD_RT" "$ECC_CMD_RT" "$(calculate_increase $NO_ECC_CMD_RT $ECC_CMD_RT)"
printf "%-25s | %-15s | %-15s | %-10s%%\n" "PIM CMD Energy (mJ)" "$NO_ECC_CMD_EN" "$ECC_CMD_EN" "$(calculate_increase $NO_ECC_CMD_EN $ECC_CMD_EN)"
echo "--------------------------------------------------------"
echo "Note: SECDED ECC for 32-bit data adds 7 parity bits, increasing bit width from 32 to 39 bits (~21.8% overhead)."
echo "For bit-serial PIM, this translates to proportional increase in runtime and energy."
