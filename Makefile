objects = lsmt.o randomise.o serialise.o tests/basic.o

lsmt: meta.H $(objects)
	clang++ -std=gnu++1y -Wall -g $(objects) -o $@

%.o: %.C *.H tests/*.H
	../llvm-build/bin/clang++ -I. -O3 -std=gnu++1y -Wall -c -g $< -o $@

clean:
	git clean -fX
