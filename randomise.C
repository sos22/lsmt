#include "randomise.H"

#include <sys/syscall.h>
#include <assert.h>
#include <unistd.h>

void randomiser::randombytes(void * ptr, size_t sz) {
    int r = syscall(SYS_getrandom, ptr, sz, 0);
    assert(r >= 0); }

template <> void nonmetatypes::randomise(std::string & x, randomiser & r) {
    unsigned l;
    r.randomise(l);
    switch (l % 4) {
    case 0:
        x = "";
        break;
    case 1: {
        char c;
        r.randomise(c);
            x.clear();
            x.append(1, c);
            break; }
    case 2: {
        r.randomise(l);
        l %= 10;
        x.resize(l % 10);
        r.randombytes((void *)x.data(), x.size());
        break; }
    case 3: {
        r.randomise(l);
        l %= 10;
        l = 1 << l;
        int l2;
        r.randomise(l2);
        l2 = l2 % 5 - 2;
        if (l2 < -l) l2 = 0;
        l += l2;
        x.resize(l);
        r.randombytes((void *)x.data(), x.size());
        break; } } }
