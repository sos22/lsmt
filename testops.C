#include "meta.H"

#include "testops.H"

struct withopers : meta<withopers> {
    structA a;
    structB b;
    withopers(int _a, int _b) : a(_a), b(_b) {}
    template <typename visitor> bool visit(visitor && v) {
        return v("a", a) && v("b", b); } };

void dotest() {
    for (unsigned a = 0; a < 3; a++) {
        for (unsigned b = 0; b < 3; b++) {
            withopers x(a, b);
            for (unsigned c = 0; c < 3; c++) {
                for (unsigned d = 0; d < 3; d++) {
                    withopers y(c, d);
                    if (a < c) {
                        assert(x <= y);
                        assert(x < y);
                        assert(!(x == y));
                        assert(x != y);
                        assert(!(x > y));
                        assert(!(x >= y)); }
                    else if (a > c) {
                        assert(!(x <= y));
                        assert(!(x < y));
                        assert(!(x == y));
                        assert(x != y);
                        assert(x > y);
                        assert(x >= y); }
                    else {
                        if (b < d) {
                            assert(x <= y);
                            assert(x < y);
                            assert(!(x == y));
                            assert(x != y);
                            assert(!(x > y));
                            assert(!(x >= y)); }
                        else if (b == d) {
                            assert(x <= y);
                            assert(!(x < y));
                            assert(x == y);
                            assert(!(x != y));
                            assert(!(x > y));
                            assert(x >= y); }
                        else {
                            assert(!(x <= y));
                            assert(!(x < y));
                            assert(!(x == y));
                            assert(x != y);
                            assert(x > y);
                            assert(x >= y); } } } } } } }

struct simplecompare : meta<simplecompare> {
    inlineable a;
    inlineable b;
    template <typename visitor> bool visit(visitor && v) {
        return v("a", a) && v("b", b); } };

bool docompare(simplecompare &a, simplecompare &b) {
    return a == b; }
