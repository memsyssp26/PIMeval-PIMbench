#!/bin/bash
# PIMeval Comprehensive Setup Script
# Copyright (c) 2024 University of Virginia

set -e

# Configuration
PROJ_ROOT=$(pwd)
DRAMSIM3_DIR="${PROJ_ROOT}/third-party/DRAMsim3"
LIBPIMEVAL_DIR="${PROJ_ROOT}/libpimeval"

echo "=================================================="
echo "   PIMeval Environment Setup & Automated Build    "
echo "=================================================="

# 1. Dependency Check
echo "[1/5] Checking system dependencies..."
TOOLS=("g++" "make" "python3" "git" "cmake")
for tool in "${TOOLS[@]}"; do
    if ! command -v $tool &> /dev/null; then
        echo "  Error: $tool is not installed. Please install it."
        exit 1
    else
        echo "  Found: $tool"
    fi
done

# 2. Submodule Initialization
echo "[2/5] Initializing git submodules..."
git submodule update --init --recursive
echo "  Submodules ready."

# 3. Automated DRAMsim3 Build
echo "[3/5] Building DRAMsim3 dependency (required for cycle-accurate mode)..."
mkdir -p "${DRAMSIM3_DIR}/build"
cd "${DRAMSIM3_DIR}/build"
# Check if already built
if [ ! -f "libdramsim3.a" ]; then
    cmake .. -DCMAKE_BUILD_TYPE=Release
    make -j$(sysctl -n hw.ncpu || nproc)
    echo "  DRAMsim3 built successfully."
else
    echo "  DRAMsim3 already built. Skipping."
fi
cd "${PROJ_ROOT}"

# 4. Compiler Verification
echo "[4/5] Verifying compiler support (C++17)..."
GXX_VERSION=$(g++ -dumpversion | cut -f1 -d.)
if [ "$GXX_VERSION" -lt 7 ]; then
    echo "  Warning: Your g++ version ($GXX_VERSION) might be too old for C++17."
else
    echo "  Compiler version $GXX_VERSION is sufficient."
fi

# 5. Final Initialization
echo "[5/5] Finalizing project structure..."
make -C "${LIBPIMEVAL_DIR}" create_link > /dev/null
echo "  Project headers linked."

echo "--------------------------------------------------"
echo " SETUP COMPLETE!"
echo "--------------------------------------------------"
echo " Quick Start Commands:"
echo "  1. Build Library & Benchmarks:  make -j"
echo "  2. Cycle-Accurate Mode:         make dramsim3"
echo "  3. Verify Everything:           make check"
echo "--------------------------------------------------"
echo " Binaries will be located in their respective source folders."
echo " Use 'make clean' to wipe local build artifacts."
