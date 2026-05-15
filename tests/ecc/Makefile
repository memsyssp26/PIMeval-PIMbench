PROJ_ROOT = ../..
include ${PROJ_ROOT}/Makefile.common

UNIT_SRC       = unit/test-ecc-math.cpp
INTEG_SRC      = integration/test-ecc-system.cpp
HARDEN_SRC     = hardening/test-ecc-hardening.cpp
SCRATCHPAD_SRC = scratchpad/test-ecc-scratchpad.cpp
BENCH_SRC      = plots/ecc_benchmark.cpp
DS3_SRC        = dramsim3/test-dramsim3.cpp
ECCBENCH_SRC   = benchmarks/ecc_bench.cpp

UNIT_EXE       = unit/test-ecc-math.out
INTEG_EXE      = integration/test-ecc-system.out
HARDEN_EXE     = hardening/test-ecc-hardening.out
SCRATCHPAD_EXE = scratchpad/test-ecc-scratchpad.out
BENCH_EXE      = plots/ecc_benchmark.out
DS3_EXE        = dramsim3/test-dramsim3.out
ECCBENCH_EXE   = benchmarks/ecc_bench.out

SWEEP_SCRIPT   = scripts/run-ecc-comparison.sh
MEMTECH_SCRIPT  = scripts/run-memory-tech-comparison.sh
SENSIT_SCRIPT   = scripts/run-sensitivity-sweep.sh

all: debug

debug perf: $(UNIT_EXE) $(INTEG_EXE) $(HARDEN_EXE) $(SCRATCHPAD_EXE) $(BENCH_EXE) $(ECCBENCH_EXE)

dramsim3_integ: $(UNIT_EXE) $(INTEG_EXE) $(HARDEN_EXE) $(SCRATCHPAD_EXE) $(BENCH_EXE) $(ECCBENCH_EXE) $(DS3_EXE)

$(UNIT_EXE): $(UNIT_SRC) $(LIBPIMEVAL_A)
	$(CXX) $(UNIT_SRC) $(CXXFLAGS) $(LDFLAGS) -o $@

$(INTEG_EXE): $(INTEG_SRC) $(LIBPIMEVAL_A)
	$(CXX) $(INTEG_SRC) $(CXXFLAGS) $(LDFLAGS) -o $@

$(HARDEN_EXE): $(HARDEN_SRC) $(LIBPIMEVAL_A)
	$(CXX) $(HARDEN_SRC) $(CXXFLAGS) $(LDFLAGS) -o $@

$(SCRATCHPAD_EXE): $(SCRATCHPAD_SRC) $(LIBPIMEVAL_A)
	$(CXX) $(SCRATCHPAD_SRC) $(CXXFLAGS) $(LDFLAGS) -o $@

$(BENCH_EXE): $(BENCH_SRC) $(LIBPIMEVAL_A)
	$(CXX) $(BENCH_SRC) $(CXXFLAGS) $(LDFLAGS) -o $@

$(DS3_EXE): $(DS3_SRC) $(LIBPIMEVAL_A)
	$(CXX) $(DS3_SRC) $(CXXFLAGS) $(LDFLAGS) -o $@

$(ECCBENCH_EXE): $(ECCBENCH_SRC) $(LIBPIMEVAL_A)
	$(CXX) $(ECCBENCH_SRC) $(CXXFLAGS) $(LDFLAGS) -o $@

run-unit: $(UNIT_EXE)
	./$(UNIT_EXE)

run-integ: $(INTEG_EXE)
	./$(INTEG_EXE)

run-harden: $(HARDEN_EXE)
	./$(HARDEN_EXE)

run-scratchpad: $(SCRATCHPAD_EXE)
	./$(SCRATCHPAD_EXE)

run-functional:
	cd functional && chmod +x run-regression.sh && ./run-regression.sh

test-all: run-unit run-integ run-harden run-scratchpad run-functional
	@echo "--------------------------------------------------"
	@echo " HARD PROOF: All Stability & ECC Tests Passed! "
	@echo "--------------------------------------------------"

# Run the three-mode ECC comparison sweep for all workloads
sweep: $(ECCBENCH_EXE)
	@chmod +x $(SWEEP_SCRIPT)
	@for wl in vec-add gemv relu; do \
	    echo ""; \
	    $(SWEEP_SCRIPT) $(ECCBENCH_EXE) $$wl; \
	done

# Run memory-technology preset comparison (DDR5 vs HBM3 vs LPDDR5)
memtech: $(ECCBENCH_EXE)
	@chmod +x $(MEMTECH_SCRIPT)
	@for wl in vec-add gemv relu; do \
	    echo ""; \
	    $(MEMTECH_SCRIPT) $(ECCBENCH_EXE) $$wl; \
	done

# Run ODECC latency sensitivity sweep across all workloads (outputs CSV to stdout)
sensitivity: $(ECCBENCH_EXE)
	@chmod +x $(SENSIT_SCRIPT)
	@$(SENSIT_SCRIPT) PIMEVAL_ODECC_LATENCY_NS 0.5,1.0,1.5,2.0,4.0 $(ECCBENCH_EXE) vec-add
	@SWEEP_CSV_HEADER=0 $(SENSIT_SCRIPT) PIMEVAL_ODECC_LATENCY_NS 0.5,1.0,1.5,2.0,4.0 $(ECCBENCH_EXE) gemv
	@SWEEP_CSV_HEADER=0 $(SENSIT_SCRIPT) PIMEVAL_ODECC_LATENCY_NS 0.5,1.0,1.5,2.0,4.0 $(ECCBENCH_EXE) relu

clean:
	rm -f $(UNIT_EXE) $(INTEG_EXE) $(HARDEN_EXE) $(SCRATCHPAD_EXE) $(BENCH_EXE) $(DS3_EXE) $(ECCBENCH_EXE)
