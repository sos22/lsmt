objects = lsmt.o randomise.o serialise.o tests/basic.o

lsmt: meta.H $(objects)
	clang++ -std=gnu++1y -Wall -g $(objects) -o $@

tests/order: tests/order.C *.H
	clang++ -I. -O3 -std=gnu++1y -Wall -g $< -o $@

%.o: %.C *.H tests/*.H
	clang++ -I. -O3 -std=gnu++1y -Wall -c -g $< -o $@

clean:
	git clean -fX
