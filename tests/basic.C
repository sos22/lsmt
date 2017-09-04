#include "tests/basic.H"

#include <assert.h>

#include "meta.H"
#include "order.H"
#include "serialise.H"

namespace {
enum class eclass;
struct structA;
struct structB; }

template <> void nonmetatypes::serialise(eclass const & ec, serialiser & s);
template <> void nonmetatypes::deserialise(eclass & ec, deserialiser & s);
template <> order nonmetatypes::compare(structA const &, structA const &);
template <> order nonmetatypes::compare(structB const &, structB const &);

namespace {

static std::string mkstr(std::string const & what) { return what; }
static std::string mkstr(int what) {
    char buf[128];
    sprintf(buf, "%d", what);
    return buf; }

struct testkey : meta<testkey> {
    std::string what;
    int number{0};

    explicit testkey(char const * what_, size_t len) : what{what_, len} {}

    explicit testkey(char const * what_) : what(what_) { }

    explicit testkey() : testkey("") {}

    template <typename state, typename visitor>
    static void visit(state s, visitor && v) {
        v(s, "what", &testkey::what) &&
            v(s, "number", &testkey::number); } };

struct threeints : meta<threeints> {
    int a{0};
    int b{0};
    int c{0};
    explicit threeints(int _a, int _b, int _c) : a(_a), b(_b), c(_c) {}
    template <typename state, typename visitor>
    static void visit(state s, visitor && v) {
        v(s, "a", &threeints::a) &&
            v(s, "b", &threeints::b) &&
            v(s, "c", &threeints::c); } };

static void testserialise() {
    testkey t{"FOOOO"};
    t.visit(nullptr, [&](void *, char const * name, auto val){
            printf("name %s, val %s\n", name, mkstr(t.*val).c_str());
            return true; });
    t.number = 0x1234567;
    std::string json(mkjson(t));
    printf("jsonised %s\n", json.c_str());
    serialiser ser;
    ser.serialise(t);
    printf("ser buffer size %lx, contents %lx %lx\n",
           ser.cursor,
           ((unsigned long *)ser.stage)[0],
           ((unsigned long *)ser.stage)[1]);

    deserialiser deser(ser.stage, ser.cursor);
    testkey t2;
    deser.deserialise(t2);

    std::string json2(mkjson(t2));
    printf("jsonised %s\n", json2.c_str());

    randomiser rand;
    testkey t3;
    rand.randomise(t3);
    std::string json3(mkjson(t3));
    printf("randomised %s\n", json3.c_str());

    for (unsigned x = 0; x < 100000; x++) {
        testkey a;
        rand.randomise(a);
        serialiser ser;
        ser.serialise(a);
        deserialiser deser(ser.stage, ser.cursor);
        testkey b;
        deser.deserialise(b);
        assert(!deser.failed());
        assert(a == b); } }

static void testoperators() {
    assert(threeints(0, 0, 0) < threeints(0,0,1));
    assert(threeints(0, 0, 0) < threeints(10,0,1));
    assert(threeints(0, 0, 0) < threeints(10,0,1)); }

struct structA {
    int v;
    explicit structA(int _v = 0) : v(_v) {}
};
typestyles::nonmeta gettypestyle(structA ***);

struct structB {
    int v;
    explicit structB(int _v = 0) : v(_v) {} };
typestyles::nonmeta gettypestyle(structB ***);

static void test1() {
    threeints a(1,2,3);
    randomiser rand;
    rand.randomise(a);
    serialiser ser;
    ser.serialise(a);
    deserialiser deser(ser.stage, ser.cursor);
    threeints b(5,6,7);
    deser.deserialise(b);
    assert(!deser.failed());
    assert(a == b); }

struct withopers : meta<withopers> {
    structA a;
    structB b;
    withopers(int _a, int _b) : a(_a), b(_b) {}
    template <typename state, typename visitor>
    static void visit(state s, visitor && v) {
        v(s, "a", &withopers::a) && v(s, "b", &withopers::b); } };

static void dotest() {
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

struct inlineable {
    int v;
    explicit inlineable(int _v) : v(_v) {}
    bool operator<(inlineable const & o) const { return v < o.v; } };

struct simplecompare : meta<simplecompare> {
    inlineable a;
    inlineable b;
    template <typename visitor> static void visit(visitor && v) {
        v("a", &simplecompare::a) && v("b", &simplecompare::b); } };

static void test2() { test1(); }

enum class eclass {
    one = 1,
    two = 2,
    three = 3
};
typestyles::nonmeta gettypestyle(eclass ***);

struct witheclass : meta<witheclass> {
    eclass inner;
    template <typename state, typename visitor>
    static void visit(state s, visitor && v) {
        v(s, "inner", &witheclass::inner); } };

static void test3() {
    serialiser ser;
    witheclass we;
    we.inner = eclass::two;
    ser.serialise(we);
    deserialiser der(ser.stage, ser.cursor);
    witheclass we2;
    der.deserialise(we2);
    assert(we.inner == we2.inner); } }

template <> void nonmetatypes::serialise(eclass const & ec, serialiser & s) {
    s.serialise((int)ec); }
template <> void nonmetatypes::deserialise(eclass & ec, deserialiser & s) {
    int e;
    s.deserialise(e);
    if (e < 1 || e > 3) s.fail();
    else ec = (eclass)e; }

template <> order nonmetatypes::compare(structA const & a, structA const & b) {
    if (a.v < b.v) return order::lt;
    else if (a.v == b.v) return order::eq;
    else return order::gt; }
template <> order nonmetatypes::compare(structB const & a, structB const & b) {
    if (a.v < b.v) return order::lt;
    else if (a.v == b.v) return order::eq;
    else return order::gt; }

void basictest() {
    testserialise();
    testoperators();
    dotest();
    test2();
    test3(); }
