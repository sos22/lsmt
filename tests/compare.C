#include "meta.H"
#include "order.H"

#include <assert.h>

struct eq_only {
    int f;
    explicit eq_only(int _f) : f(_f) {}
    bool operator==(eq_only a) const { return f == a.f; } };
typestyles::nonmeta gettypestyle(eq_only ***);

struct meta_eq : meta<meta_eq> {
    int a;
    eq_only b;
    meta_eq(int _a, eq_only _b) : a(_a), b(_b) {}
    meta_eq(int _a, int _b) : a(_a), b(_b) {}
    template <typename state, typename t> static void visit(state s, t && v) {
        v(s, "a", &meta_eq::a) &&
            v(s, "b", &meta_eq::b); }
    bool operator==(const meta_eq &o) const {
        return *static_cast<const meta<meta_eq> *>(this) == o; }
};

int main() {
    assert(eq_only(3) == eq_only(3));
    assert(!(eq_only(3) == eq_only(4)));
    assert(eq_only(3) != eq_only(4));

    assert(meta_eq(1, 2) == meta_eq(1, 2));
    assert(!(meta_eq(1, 2) != meta_eq(1, 2)));
    assert(meta_eq(1, 2) != meta_eq(1, 3));
    assert(!(meta_eq(1, 2) == meta_eq(1, 3)));

    return 0; }
