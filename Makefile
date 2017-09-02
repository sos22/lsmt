objects = lsmt.o randomise.o serialise.o order.o tests/basic.o

all: lsmt tests/order tests/compare runtimeperf

runtests: tests/compare
	./tests/testorder.sh
	./tests/compare

lsmt: meta.H $(objects)
	clang++ -std=gnu++1y -Wall -g $(objects) -o $@

runtimeperf: meta.H serialise.o runtimeperf.o
	clang++ -std=gnu++1y -Wall -g serialise.o runtimeperf.o -o $@

tests/order: tests/order.C order.o *.H
	clang++ -I. -O3 -std=gnu++1y -Wall -g $< order.o -o $@

tests/compare: tests/compare.C order.o *.H
	clang++ -I. -O3 -std=gnu++1y -Wall -g $< order.o -o $@

%.o: %.C *.H tests/*.H
	clang++ -I. -O3 -std=gnu++1y -Wall -c -g $< -o $@

clean:
	git clean -fX
