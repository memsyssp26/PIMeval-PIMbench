PROJ_ROOT = ../..
include ${PROJ_ROOT}/Makefile.common

EXEC := test-ecc-flexibility.out
SRCS := $(wildcard *.cpp) 

debug perf dramsim3_integ: $(EXEC)

$(EXEC): $(SRCS) $(DEPS)
	$(CXX) $(SRCS) $(CXXFLAGS) -o $@

clean:
	rm -rf $(EXEC) *.dSYM
