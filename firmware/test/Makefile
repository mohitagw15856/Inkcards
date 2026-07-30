# Host build for the InkCards core unit tests. No board, SDK or external test
# framework required, just a C++17 compiler. CI runs `make -C firmware/test`.
CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2
CORE := ../lib/inkcards_core
INCLUDES := -I$(CORE) -I.
SOURCES := test_core.cpp $(wildcard $(CORE)/*.cpp)
BIN := build/runtests

.PHONY: test clean
test: $(BIN)
	$(BIN)

$(BIN): $(SOURCES) $(wildcard $(CORE)/*.h) $(wildcard *.h)
	@mkdir -p build
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SOURCES) -o $(BIN)

clean:
	rm -rf build
