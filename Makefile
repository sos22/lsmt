srcs = lsmt.C lib/randomise.C lib/serialise.C lib/order.C tests/basic.C lib/json.C
objects = $(srcs:.C=.o)
deps = $(srcs:.C=.d)

CXX=clang++
CXXFLAGS=-std=gnu++1y -fno-strict-aliasing -fno-operator-names -fno-exceptions -g -I.

# Warnings:
CXXFLAGS += -Wall -Werror -Wextra -Wshadow -Winit-self -Wswitch-enum 

# clang sometimes generates this one spuriously if a function can
# change template resolution and is never called (SFINAE) and we have
# -Werror on. Turn it off.
CXXFLAGS += -Wno-unneeded-internal-declaration

all: lsmt tests/order tests/compare runtimeperf

%.d: %.C
	$(CXX) $(CXXFLAGS) -MM -MT "$@ $(@:.d=.o)" -o $(@:.d=.o) -c -MF $@.tmp $< && mv $@.tmp $@

-include $(deps)

runtests: tests/compare tests/split
	./tests/testorder.sh
	./tests/compare
	./tests/split

lsmt: $(objects)
	clang++ $(objects) -o $@

runtimeperf: lib/serialise.o runtimeperf.o
	clang++ -lm -std=gnu++1y -Wall -g lib/serialise.o runtimeperf.o -o $@

tests/order: tests/order.C lib/order.o
	clang++ -I. -O3 -std=gnu++1y -Wall -g $< lib/order.o -o $@

tests/compare: tests/compare.C lib/order.o
	clang++ -I. -O3 -std=gnu++1y -Wall -g $< lib/order.o -o $@

tests/split: tests/split1.o tests/split2.o lib/serialise.o lib/order.o lib/randomise.o lib/json.o
	clang++ -I. $^ -o $@

%.o: %.C
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	git clean -fX
