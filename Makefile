# Makefile for PIMeval / PIMbench Framework
# Copyright (c) 2024 University of Virginia
# This file is licensed under the MIT License.
# See the LICENSE file in the root of this repository for more details.

# Directories
LIBDIR       := libpimeval
BITSERIALDIR := bit-serial
APPDIR       := PIMbench
MISCDIR      := misc-bench
TESTDIR      := tests
THIRD_PARTY  := third-party/DRAMsim3

# All standard build directories
SUBDIRS := $(LIBDIR) $(BITSERIALDIR) $(APPDIR) $(MISCDIR) $(TESTDIR)

# Artifacts
LIBPIMEVAL_A := $(LIBDIR)/lib/libpimeval.a
DRAMSIM3_A   := $(THIRD_PARTY)/build/libdramsim3.a

# Configuration Overrides
PIM_SIM_TARGET    ?= PIM_DEVICE_NONE
USE_OPENMP        ?= 0
COMPILE_WITH_JPEG ?= 0
DRAMSIM3_PATH     ?= $(shell pwd)/$(THIRD_PARTY)

# Exports for sub-makes
export PIM_SIM_TARGET USE_OPENMP COMPILE_WITH_JPEG DRAMSIM3_PATH

.PHONY: all debug perf dramsim3 clean check docs third-party $(SUBDIRS)

# Default goal: high-performance functional simulation
all: perf

perf: $(SUBDIRS)
	@echo "\n>>> Built PIMeval (Target: PERF) <<<\n"

debug: $(SUBDIRS)
	@echo "\n>>> Built PIMeval (Target: DEBUG) <<<\n"

# Integrated Cycle-Accurate Mode
dramsim3: third-party dramsim3_integ

dramsim3_integ: $(SUBDIRS)
	@echo "\n>>> Built PIMeval (Target: DRAMSIM3_INTEG) <<<\n"

# Third-party dependencies
third-party: $(DRAMSIM3_A)

$(DRAMSIM3_A):
	@echo ">>> Building DRAMsim3 dependency <<<"
	@mkdir -p $(THIRD_PARTY)/build
	@cd $(THIRD_PARTY)/build && cmake .. && $(MAKE) -j$(sysctl -n hw.ncpu || nproc)

# Directory Orchestration
$(SUBDIRS):
	@$(MAKE) -C $@ $(filter-out all dramsim3,$(MAKECMDGOALS))

# Specific Dependencies
$(BITSERIALDIR) $(APPDIR) $(MISCDIR) $(TESTDIR): $(LIBDIR)

# Verification
check:
	@$(MAKE) perf
	@echo ">>> Verifying PIMeval Build <<<"
	@$(MAKE) -C tests/ecc test-all
	@echo "\n>>> Checking Standard PIMbench Kernel <<<"
	@$(MAKE) -C PIMbench/vec-add/PIM perf > /dev/null
	@cd PIMbench/vec-add/PIM && ./vec-add.out > /dev/null && echo "  vec-add: PASSED"
	@echo "\n>>> All Checks Passed! <<<"

# Documentation
docs:
	@echo ">>> Generating Doxygen Documentation <<<"
	@doxygen Doxyfile || echo "Error: doxygen not found. Please install it."

# Global Cleanup
clean:
	@for dir in $(SUBDIRS); do $(MAKE) -C $$dir clean; done
	@echo ">>> Project Cleaned <<<"

clean-all: clean
	@rm -rf $(THIRD_PARTY)/build
	@echo ">>> All Artifacts (including third-party) Cleaned <<<"
