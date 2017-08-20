#ifndef META_H__
#define META_H__

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include <string>
#include <type_traits>

struct deserialiser;
struct randomiser;
struct serialiser;

namespace typestyles {
struct meta {};
struct nonmeta {};
struct fundamental {}; }

template <typename t> typename t::typestyle gettypestyle(t ***);
typestyles::fundamental gettypestyle(char ***);
typestyles::fundamental gettypestyle(unsigned char ***);
typestyles::fundamental gettypestyle(short ***);
typestyles::fundamental gettypestyle(unsigned short ***);
typestyles::fundamental gettypestyle(int ***);
typestyles::fundamental gettypestyle(unsigned int ***);
typestyles::fundamental gettypestyle(long ***);
typestyles::fundamental gettypestyle(unsigned long ***);
typestyles::nonmeta gettypestyle(std::string ***);

namespace nonmetatypes {
template <typename t> void serialise(t const &, serialiser &);
}

struct serialiser {
    void * stage {nullptr};
    size_t allocated {0};
    size_t cursor {0};

    // This is the API you want to call; the rest are implementation
    // details.
    template <typename t> void serialise(t const & what) {
        using sty = decltype(gettypestyle((t ***)nullptr));
        serialise(what, sty()); }

    template <typename t> void serialise(t const & what,
                                         typestyles::meta) {
        what.serialise(*this); }

    template <typename t> void serialise(t const & what,
                                         typestyles::nonmeta) {
        nonmetatypes::serialise(what, *this); }

    template <typename t> void serialise(t const & what,
                                         typestyles::fundamental) {
        pushbytes(&what, sizeof(what)); }

    ~serialiser() { free(stage); }

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
        cursor += sz; } };

namespace nonmetatypes {
template <typename t> void deserialise(t &, deserialiser &);
}
struct deserialiser {
    void const * const buf;
    size_t const size;
    size_t offset{0};

    // These are the API you want to call; the rest are implementation
    // details.
    bool failed() const { return offset > size; }
    void fail() { offset = size + 1; }
    template <typename t> void deserialise(t & val) {
        using sty = decltype(gettypestyle((t ***)nullptr));
        deserialise(val, sty()); }

    template <typename t> void deserialise(t & what, typestyles::meta) {
        what.deserialise(*this); }

    template <typename t> void deserialise(t & what, typestyles::nonmeta) {
        nonmetatypes::deserialise(what, *this); }

    template <typename t> void deserialise(t & what, typestyles::fundamental) {
        auto bytes(getbytes(sizeof(what)));
        if (bytes) *&what = *static_cast<t const *>(bytes);
        else what = 0; }

    deserialiser(void const * _buf, size_t _size) : buf(_buf), size(_size) {}

    void const * getbytes(size_t sz) {
        if (offset + sz > size) {
            fail();
            return nullptr; }
        else {
            auto res = reinterpret_cast<void const *>(
                reinterpret_cast<uintptr_t>(buf) + offset);
            offset += sz;
            return res; } } };

namespace nonmetatypes {
template <typename t> std::string mkjson(t const &); }
namespace fundamentaltypes {
std::string mkjson(int); }
template <typename t> std::string mkjson(t const & what,
                                         typestyles::nonmeta) {
    return nonmetatypes::mkjson(what); }
template <typename t> std::string mkjson(t const & what,
                                         typestyles::fundamental) {
    return fundamentaltypes::mkjson(what); }
template <typename t> std::string mkjson(t const & what,
                                         typestyles::meta) {
    return what.json(); }
template <typename t> std::string mkjson(t const & what) {
    using sty = decltype(gettypestyle((t ***)nullptr));
    return mkjson(what, sty()); }

namespace nonmetatypes {
template <typename t> void randomise(t &, randomiser &); }
struct randomiser {
    void randombytes(void * ptr, size_t sz);

    template <typename t> void randomise(t & val) {
        using sty = decltype(gettypestyle((t ***)nullptr));
        randomise(val, sty()); }

    template <typename t> void randomise(t & val, typestyles::meta) {
        val.randomise(*this); }
    template <typename t> void randomise(t & val, typestyles::nonmeta) {
        nonmetatypes::randomise(val, *this); }
    template <typename t> void randomise(t & val, typestyles::fundamental) {
        randombytes(&val, sizeof(val)); } };

template <typename what>
struct meta {
    // Sometimes useful for SFINAE
    using typestyle = typestyles::meta;

    what const & inner() const { return *static_cast<what const *>(this); }
    what & inner() { return *static_cast<what *>(this); }
    std::string json() const {
        std::string acc;
        acc.append("{");
        what::visit([&](const char * name, auto val) {
                acc.append("\"");
                acc.append(name);
                acc.append("\": ");
                auto v(mkjson(inner().*val));
                acc.append(v);
                acc.append(", ");
                return true; });
        if (acc.size() > 1) acc.erase(acc.begin() + acc.size() - 2, acc.end());
        acc.append("}");
        return acc; }
    struct serialisevisitor {
        what const & _inner;
        serialiser & _serialiser;
        explicit serialisevisitor(what const & inner_, serialiser & serialiser_)
            : _inner(inner_)
            , _serialiser(serialiser_) {}
        template <typename t> bool operator()(const char *, t val) {
            _serialiser.serialise(_inner.*val);
            return true; } };
    void serialise(serialiser & t) const {
        what::visit(serialisevisitor(inner(), t)); }
    void deserialise(deserialiser & t) {
        what::visit(
            [&](const char *, auto val) {
                t.deserialise(inner().*val);
                return true;});}
    void randomise(randomiser & t) {
        what::visit(
            [&](const char *, auto val) {
                t.randomise(inner().*val);
                return true; }); }
    bool operator<(const what & o) const {
        bool lt = false;
        what::visit(
            [&](const char *, auto val) {
                if (inner().*val < o.*val) {
                    lt = true;
                    return false; }
                else if (o.*val < inner().*val) {
                    lt = false;
                    return false; }
                else return true; });
        return lt; } };

// Assume we already have operator <. Derive some more relational
// operators.
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
