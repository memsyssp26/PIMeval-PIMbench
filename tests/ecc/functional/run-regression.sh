#!/bin/bash
# Functional Regression for PIMbench with ECC
set -e

APPS=("vec-add" "relu")
ECC_TYPES=("none" "secded" "crc32" "rs")

echo "Starting Functional Regression Suite..."

# Get absolute path to project root
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
PROJ_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"

for app in "${APPS[@]}"; do
    echo "Testing App: $app"
    APP_DIR="$PROJ_ROOT/PIMbench/$app/PIM"
    cd "$APP_DIR"
    make clean > /dev/null
    make -j$(sysctl -n hw.ncpu) > /dev/null
    
    for ecc in "${ECC_TYPES[@]}"; do
        echo -n "  ECC=$ecc : "
        if [ "$ecc" == "none" ]; then
            export PIMEVAL_ECC=0
            unset PIMEVAL_ECC_TYPE
        else
            export PIMEVAL_ECC=1
            export PIMEVAL_ECC_TYPE=$ecc
        fi
        
        # Run app. We use a small input size if the app supports it to speed up tests.
        ./${app}.out > /dev/null 2>&1
        if [ $? -eq 0 ]; then
            echo "PASSED"
        else
            echo "FAILED"
            exit 1
        fi
    done
done

echo "Functional Regression Suite Finished Successfully!"
