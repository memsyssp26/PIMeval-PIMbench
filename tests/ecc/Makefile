PROJ_ROOT = ../..
include ${PROJ_ROOT}/Makefile.common

UNIT_SRC = unit/test-ecc-math.cpp
INTEG_SRC = integration/test-ecc-system.cpp
HARDEN_SRC = hardening/test-ecc-hardening.cpp

UNIT_EXE = unit/test-ecc-math.out
INTEG_EXE = integration/test-ecc-system.out
HARDEN_EXE = hardening/test-ecc-hardening.out

all: debug

debug perf dramsim3_integ: $(UNIT_EXE) $(INTEG_EXE) $(HARDEN_EXE)

$(UNIT_EXE): $(UNIT_SRC) $(LIBPIMEVAL_A)
	$(CXX) $(UNIT_SRC) $(CXXFLAGS) $(LDFLAGS) -o $@

$(INTEG_EXE): $(INTEG_SRC) $(LIBPIMEVAL_A)
	$(CXX) $(INTEG_SRC) $(CXXFLAGS) $(LDFLAGS) -o $@

$(HARDEN_EXE): $(HARDEN_SRC) $(LIBPIMEVAL_A)
	$(CXX) $(HARDEN_SRC) $(CXXFLAGS) $(LDFLAGS) -o $@

run-unit: $(UNIT_EXE)
	./$(UNIT_EXE)

run-integ: $(INTEG_EXE)
	./$(INTEG_EXE)

run-harden: $(HARDEN_EXE)
	./$(HARDEN_EXE)

run-functional:
	cd functional && chmod +x run-regression.sh && ./run-regression.sh

test-all: run-unit run-integ run-harden run-functional
	@echo "--------------------------------------------------"
	@echo " HARD PROOF: All Stability & ECC Tests Passed! "
	@echo "--------------------------------------------------"

clean:
	rm -f $(UNIT_EXE) $(INTEG_EXE) $(HARDEN_EXE)
