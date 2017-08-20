#ifndef META_H__
#define META_H__

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include <string>

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

    ~deserialiser() { free((void *)buf); }

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
            "non-arithmetic types can't use getfundamental");
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
    void deserialise(size_t & x) { deserialisefundamental(x); } };

template <typename t> std::string mkjson(t const & what) {
    return what.json(); }

std::string mkjson(std::string const & what);
std::string mkjson(int val);

struct randomiser {
    void randombytes(void * ptr, size_t sz);

    template <typename t> void randomise(t & val) { val.randomise(*this); }
    void randomise(std::string & x);
    void randomise(int & x) { randombytes(&x, sizeof(x)); }
    void randomise(unsigned & x) { randombytes(&x, sizeof(x)); }
    void randomise(char & x) { randombytes(&x, sizeof(x)); } };

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
                acc.append(", ");
                return true; });
        if (acc.size() > 1) acc.erase(acc.begin() + acc.size() - 2, acc.end());
        acc.append("}");
        return acc; }
    void serialise(serialiser & t) const {
        inner().visit(
            [&](const char *, auto & val) { t.serialise(val); return true; }); }
    void deserialise(deserialiser & t) {
        inner().visit(
            [&](const char *, auto & val) { t.deserialise(val); return true;});}
    void randomise(randomiser & t) {
        inner().visit(
            [&](const char *, auto & val) { t.randomise(val); return true; }); }
    bool operator<(const what & o) const {
        bool lt = false;
        inner().visit(
            [&](const char *, auto & val) {
                intptr_t delta =
                    reinterpret_cast<intptr_t>(&val) -
                    reinterpret_cast<intptr_t>(this);
                typedef decltype(&val) val_t;
                auto other = reinterpret_cast<val_t>(
                    reinterpret_cast<intptr_t>(&o) + delta);
                if (val < *other) {
                    lt = true;
                    return false; }
                else if (*other < val) {
                    lt = false;
                    return false; }
                else return true; });
        return lt; } };

// Assume we already have operator <. Derive some more.
template <typename a> bool operator>=(const a & x, const a & y) {
    return !(x < y); }
template <typename a> bool operator>(const a & x, const a & y) {
    return y < x; }
template <typename a> bool operator<=(const a & x, const a & y) {
    return !(y < x); }

template <typename a> bool operator==(const a & x, const a & y) {
    return !(x < y) && !(y < x); }
template <typename a> bool operator!=(const a & x, const a & y) {
    return !(x == y); }

#endif /* !META_H__ */
