lsmt: lsmt.C meta.H testops.o
	clang++ -std=gnu++1y -Wall -g lsmt.C testops.o -o lsmt

testops.o: testops.C meta.H
	clang++ -O3 -std=gnu++1y -Wall -c -g testops.C 

clean:
	git clean -fX
