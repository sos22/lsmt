deps = $(srcs:.C=.d)

CXX=clang++
CXXFLAGS=-std=gnu++1y -fno-strict-aliasing -fno-operator-names -fno-exceptions -g -O3 -Iinclude

# Warnings:
CXXFLAGS += -Wall -Werror -Wextra -Wshadow -Winit-self -Wswitch-enum 

# clang sometimes generates this one spuriously if a function can
# change template resolution and is never called (SFINAE) and we have
# -Werror on. Turn it off.
CXXFLAGS += -Wno-unneeded-internal-declaration

all: runtimeperf runtests

%.d: %.C
	$(CXX) $(CXXFLAGS) -MM -MT "$@ $(@:.d=.o)" -o $(@:.d=.o) -c -MF $@.tmp $< && mv $@.tmp $@

-include $(deps)

include lib/mk

.PHONY: runtests
runtests: tests/compare tests/split tests/smoke
	./tests/testorder.sh
	./tests/compare
	./tests/split

lsmt: lsmt.C lib.a
	$(CXX) $(CXXFLAGS) $^ -o $@

tests/smoke: tests/smoke.o tests/basic.o lib.a
	$(CXX) $(CXXFLAGS) $^ -o $@

runtimeperf: lib.a runtimeperf.o
	$(CXX) $(CXXFLAGS) lib/serialise.o runtimeperf.o -o $@

tests/order: tests/order.o lib.a
	$(CXX) $(CXXFLAGS) $^ -o $@

tests/compare: lib.a tests/compare.o
	$(CXX) $(CXXFLAGS) -g $^ -o $@

tests/split: tests/split1.o tests/split2.o lib.a
	$(CXX) $(CXXFLAGS) $^ -o $@

%.o: %.C
	$(CXX) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	git clean -fX
