#include <sys/syscall.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <string>
#include <type_traits>

#include "meta.H"

#include "testops.H"

const char deserialiser::_zeroes[128] = {};

std::string mkjson(std::string const & what) {
    return "\"" + what + "\""; }

std::string mkjson(int val) {
    char buf[32];
    sprintf(buf, "%d", val);
    return buf; }

void randomiser::randomise(std::string & x) {
    unsigned l;
    randomise(l);
    switch (l % 4) {
    case 0:
        x = "";
        break;
    case 1: {
        char c;
        randomise(c);
            x.clear();
            x.append(1, c);
            break; }
    case 2: {
        randomise(l);
        l %= 10;
        x.resize(l % 10);
        randombytes((void *)x.data(), x.size());
        break; }
    case 3: {
        randomise(l);
        l %= 10;
        l = 1 << l;
        int l2;
        randomise(l2);
        l2 = l2 % 5 - 2;
        if (l2 < -l) l2 = 0;
        l += l2;
        x.resize(l);
        randombytes((void *)x.data(), x.size());
        break; } } }

void randomiser::randombytes(void * ptr, size_t sz) {
    int r = syscall(SYS_getrandom, ptr, sz, 0);
    assert(r >= 0); }

struct testkey : meta<testkey> {
    std::string what;
    int number{0};

    explicit testkey(char const * what_, size_t len) : what{what_, len} {}

    explicit testkey(char const * what_) : what(what_) { }

    explicit testkey() : testkey("") {}

    template <typename visitor> static bool visit(visitor && v) {
        return v("what", &testkey::what) &&
            v("number", &testkey::number);
    }

};

static std::string mkstr(std::string const & what) { return what; }
static std::string mkstr(int what) {
    char buf[128];
    sprintf(buf, "%d", what);
    return buf; }

struct threeints : meta<threeints> {
    int a{0};
    int b{0};
    int c{0};
    explicit threeints(int _a, int _b, int _c) : a(_a), b(_b), c(_c) {}
    template <typename visitor> static bool visit(visitor && v) {
        return v("a", &threeints::a) &&
            v("b", &threeints::b) &&
            v("c", &threeints::c); } };

void testserialise() {
    testkey t{"FOOOO"};
    t.visit([&](char const * name, auto val){
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
        a.randomise(rand);
        serialiser ser;
        a.serialise(ser);
        deserialiser deser(ser.stage, ser.cursor);
        testkey b;
        b.deserialise(deser);
        assert(!deser.failed());
        assert(a == b); } }

void testoperators() {
    assert(threeints(0, 0, 0) < threeints(0,0,1));
    assert(threeints(0, 0, 0) < threeints(10,0,1));
    assert(threeints(0, 0, 0) < threeints(10,0,1)); }

bool structA::operator<(structA const & o) const { return v < o.v; }

bool structB::operator<(structB const & o) const { return v < o.v; }

void test1() {
    threeints a(1,2,3);
    randomiser rand;
    a.randomise(rand);
    serialiser ser;
    a.serialise(ser);
    deserialiser deser(ser.stage, ser.cursor);
    threeints b(5,6,7);
    b.deserialise(deser);
    assert(!deser.failed());
    assert(a == b); }

enum class eclass {
    one = 1,
    two = 2,
    three = 3
};
typestyles::nonmeta gettypestyle(eclass ***) { return typestyles::nonmeta(); }

template <> void nonmetatypes::serialise(eclass const & ec, serialiser & s) {
    s.serialise((int)ec); }
template <> void nonmetatypes::deserialise(eclass & ec, deserialiser & s) {
    int e;
    s.deserialise(e);
    if (e < 1 || e > 3) s.fail();
    else ec = (eclass)e; }

struct witheclass : meta<witheclass> {
    eclass inner;
    template <typename visitor> static bool visit(visitor && v) {
        return v("inner", &witheclass::inner); } };

void test3() {
    serialiser ser;
    witheclass we;
    we.inner = eclass::two;
    ser.serialise(we);
    deserialiser der(ser.stage, ser.cursor);
    witheclass we2;
    der.deserialise(we2);
    assert(we.inner == we2.inner); }

int main() {
    testserialise();
    testoperators();
    dotest();
    test2();
    test3();
    return 0; }
