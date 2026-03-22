PROJ_ROOT = ../..
include ${PROJ_ROOT}/Makefile.common

UNIT_SRC = unit/test-ecc-math.cpp
INTEG_SRC = integration/test-ecc-system.cpp
HARDEN_SRC = hardening/test-ecc-hardening.cpp

UNIT_EXE = unit/test-ecc-math.out
INTEG_EXE = integration/test-ecc-system.out
HARDEN_EXE = hardening/test-ecc-hardening.out

all: $(UNIT_EXE) $(INTEG_EXE) $(HARDEN_EXE)

$(UNIT_EXE): $(UNIT_SRC)
	$(CXX) $(UNIT_SRC) $(CXXFLAGS) -o $@

$(INTEG_EXE): $(INTEG_SRC)
	$(CXX) $(INTEG_SRC) $(CXXFLAGS) -o $@

$(HARDEN_EXE): $(HARDEN_SRC)
	$(CXX) $(HARDEN_SRC) $(CXXFLAGS) -o $@

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
	rm -f $(UNIT_EXE) $(INTEG_EXE)
