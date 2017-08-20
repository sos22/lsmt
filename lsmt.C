#include <sys/syscall.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <string>
#include <type_traits>

struct serialiser {
    void * stage {nullptr};
    size_t allocated {0};
    size_t cursor {0};

    void pushbytes(void const * what, size_t sz) {
        if (sz + cursor > allocated) {
            allocated += 4096;
            stage = realloc(stage, allocated);
            assert(stage); }
        memcpy(
            reinterpret_cast<void *>(
                reinterpret_cast<uint64_t>(stage) + cursor),
            what,
            sz);
        cursor += sz; }

    template <typename t> void pushfundamental(t what) {
#if 0
        static_assert(
            !std::is_arithmetic<t>::value,
            "non-arithmetic can't use pushfundamental");
#endif
        pushbytes(&what, sizeof(what)); }

    template <typename t> void serialise(t const & what) {
        what.serialise(*this); }

    void serialise(std::string const & what) {
        pushfundamental<size_t>(what.size());
        pushbytes(what.data(), what.size()); }

    void serialise(int what) { pushfundamental(what); }
    void serialise(size_t x) { pushfundamental(x); } };

struct deserialiser {
    void const * const buf;
    size_t const size;
    size_t offset{0};

    static const char _zeroes[128];

    deserialiser(void const * _buf, size_t _size) : buf(_buf), size(_size) {}

    void const * getbytes(size_t sz) {
        if (offset + sz > size) {
            offset = size + 1;
            return nullptr; }
        else {
            auto res = reinterpret_cast<void const *>(
                reinterpret_cast<uintptr_t>(buf) + offset);
            offset += sz;
            return res; } }
    template <typename t> t const * getfundamental() {
        static_assert(
            std::is_arithmetic<t>::value,
            "non-arithmetic can't use getfundamental");
        static_assert(sizeof(t) < sizeof(_zeroes), "get too big a type");
        auto res = getbytes(sizeof(t));
        if (!res) res = _zeroes;
        return static_cast<t const *>(res); }

    template <typename t> void deserialisefundamental(t & val) {
        val = *getfundamental<t>(); }

    template <typename t> void deserialise(t & val) { val.deserialise(*this); }
    void deserialise(std::string & val) {
        size_t sz;
        deserialise(sz);
        auto buf(getbytes(sz));
        if (buf) val = std::string((char *)buf, sz); }
    void deserialise(int & x) { deserialisefundamental(x); }
    void deserialise(size_t & x) { deserialisefundamental(x); }
};

const char deserialiser::_zeroes[128] = {};

template <typename t> std::string mkjson(t const & what) {
    return what.json(); }

std::string mkjson(std::string const & what) {
    return "\"" + what + "\""; }

std::string mkjson(int val) {
    char buf[32];
    sprintf(buf, "%d", val);
    return buf; }

struct randomiser {
    void randombytes(void * ptr, size_t sz) {
        int r = syscall(SYS_getrandom, ptr, sz, 0);
        assert(r >= 0); }
    template <typename t> void randomise(t & val) { val.randomise(*this); }
    void randomise(std::string & x) {
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

    void randomise(int & x) { randombytes(&x, sizeof(x)); }
    void randomise(unsigned & x) { randombytes(&x, sizeof(x)); }
    void randomise(char & x) { randombytes(&x, sizeof(x)); }
};

template <typename what>
struct meta {
    what & inner() const {
        return *const_cast<what *>(static_cast<what const *>(this)); }
    std::string json() const {
        std::string acc;
        acc.append("{");
        inner().visit([&](const char * name, auto val) {
                acc.append("\"");
                acc.append(name);
                acc.append("\": ");
                auto v(mkjson(val));
                acc.append(v);
                acc.append(", "); });
        if (acc.size() > 1) acc.erase(acc.begin() + acc.size() - 2, acc.end());
        acc.append("}");
        return acc; }
    void serialise(serialiser & t) const {
        inner().visit([&](const char *, auto & val) { t.serialise(val); }); }
    void deserialise(deserialiser & t) {
        inner().visit([&](const char *, auto & val) { t.deserialise(val); }); }
    void randomise(randomiser & t) {
        inner().visit([&](const char *, auto & val) { t.randomise(val); }); }
};

struct testkey : meta<testkey> {
    std::string what;
    int number{0};

    explicit testkey(char const * what_, size_t len) : what{what_, len} {}

    explicit testkey(char const * what_) : what(what_) { }

    explicit testkey() : testkey("") {}

    template <typename visitor> void visit(visitor && v) {
        v("what", what);
        v("number", number);
    }

};

static std::string mkstr(std::string const & what) { return what; }
static std::string mkstr(int what) {
    char buf[128];
    sprintf(buf, "%d", what);
    return buf; }

int main() {
    testkey t{"FOOOO"};
    t.visit([](char const * name, auto val){
            printf("name %s, val %s\n", name, mkstr(val).c_str()); });
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

    return 0; }
