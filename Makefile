objects = lsmt.o randomise.o serialise.o testops.o 

lsmt: meta.H $(objects)
	clang++ -std=gnu++1y -Wall -g $(objects) -o $@

%.o: %.C *.H
	clang++ -O3 -std=gnu++1y -Wall -c -g $<

clean:
	git clean -fX
