#!/bin/bash
# PIMeval Setup Script
# Automatically initializes submodules and checks for dependencies.

set -e

echo ">>> PIMeval Environment Setup <<<"

# 1. Check for basic tools
echo "[1/4] Checking system dependencies..."
TOOLS=("g++" "make" "python3" "git")
for tool in "${TOOLS[@]}"; do
    if ! command -v $tool &> /dev/null; then
        echo "  Error: $tool is not installed. Please install it using your package manager."
        exit 1
    else
        echo "  Found: $tool"
    fi
done

# 2. Initialize submodules
echo "[2/4] Initializing git submodules..."
git submodule update --init --recursive
echo "  Submodules ready."

# 3. Verify Compiler Version (Require C++17)
echo "[3/4] Verifying compiler support (C++17)..."
GXX_VERSION=$(g++ -dumpversion | cut -f1 -d.)
if [ "$GXX_VERSION" -lt 7 ]; then
    echo "  Warning: Your g++ version ($GXX_VERSION) might be too old for C++17."
else
    echo "  Compiler version $GXX_VERSION is sufficient."
fi

# 4. Success
echo "[4/4] Setup complete!"
echo "--------------------------------------------------"
echo " To build the project, run:  make -j\$(sysctl -n hw.ncpu || nproc)"
echo " To verify the build, run:   make check"
echo "--------------------------------------------------"
