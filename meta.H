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

/* Style of type, for our meta framework. Either meta, for things
 * which inherit from meta<>, nonmeta, for classes which don't, or
 * fundamental for types like int or char which we can treat as a
 * fixed size block of bytes. */
namespace typestyles {
struct meta {};
struct nonmeta {};
struct fundamental {}; }
// Types with a typestyle member are easy. Pretty much means meta<>
// types.
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
/* Serialiser for types which don't fit either meta or fundamental
 * styles.  This template is never defined, so you need to provide a
 * specialisation for anything which needs it. */
template <typename t> void serialise(t const &, serialiser &); }

/* Simple serialiser. Types are always serialisable if the
 * gettypestyle() is either meta or fundamental. nonmeta types are
 * serialisable iff they have a specialisation() method in the
 * nonmetatypes namespace.
 */
struct serialiser {
public:  void * stage {nullptr};
private: size_t allocated {0};
public:  size_t cursor {0};

    // Main API for serialising arbitrary types.
public:  template <typename t> void serialise(t const & what) {
        using sty = decltype(gettypestyle((t ***)nullptr));
        serialise(what, sty()); }

    // Indirection functions to help find the right serialiser
    // implementation for different type styles.
private: template <typename t> void serialise(t const & what,
                                              typestyles::meta) {
    what.serialise(*this); }
private: template <typename t> void serialise(t const & what,
                                              typestyles::nonmeta) {
    nonmetatypes::serialise(what, *this); }
private: template <typename t> void serialise(t const & what,
                                              typestyles::fundamental) {
    pushbytes(&what, sizeof(what)); }

    // Helper for implementing serialisers for things which aren't
    // meta.
public: void pushbytes(void const * what, size_t sz) {
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

public: ~serialiser() { free(stage); } };

namespace nonmetatypes {
// Basically, the opposite of nonmetatypes::serialise.
template <typename t> void deserialise(t &, deserialiser &); }

/* Simple deserialiser. Should work for anything serialiser works
 * for. Note that this is a failure-oblivious deserialiser, so the
 * actual deserialise op always gives you back an instance of the
 * type, even after a deserialisation failure.  If you care, you
 * should almost certainly call failed() when you're done. */
struct deserialiser {
private: void const * const buf;
private: size_t const size;
    /* Note that offset might be past the limit of size if we're failed. */
private: size_t offset{0};

public:  deserialiser(void const * _buf, size_t _size)
    : buf(_buf), size(_size) {}

    /* Has the deserialiser failed? If it has, anything it returned
     * should be treated as suspect. */
public:  bool failed() const { return offset > size; }
    /* Mark the deserialiser as failed. */
public:  void fail() { offset = size + 1; }
    /* Deserialise one thing, putting the result into @val and
     * advancing the cursor. */
public:  template <typename t> void deserialise(t & val) {
    using sty = decltype(gettypestyle((t ***)nullptr));
    deserialise(val, sty()); }
    /* Helper for implementing non-meta deserialisers. */
public:  void const * getbytes(size_t sz) {
    if (offset + sz > size) {
        fail();
        return nullptr; }
    else {
        auto res = reinterpret_cast<void const *>(
            reinterpret_cast<uintptr_t>(buf) + offset);
        offset += sz;
        return res; } }

    /* Indirection layer to find the right deserialiser. */
private: template <typename t> void deserialise(t & what, typestyles::meta) {
    what.deserialise(*this); }
private: template <typename t> void deserialise(t & what, typestyles::nonmeta) {
    nonmetatypes::deserialise(what, *this); }
private:  template <typename t> void deserialise(
    t & what, typestyles::fundamental) {
    auto bytes(getbytes(sizeof(what)));
    if (bytes) *&what = *static_cast<t const *>(bytes);
    else what = 0; } };

/* std::string mkjson(t const &) -- turn an arbitrary type into json,
 * as a string.  Works for meta and fundamental types. Can be made to
 * work for nonmetas if they implement nonmetatypes::mkjson. */
namespace nonmetatypes {
template <typename t> std::string mkjson(t const &); }
namespace fundamentaltypes { std::string mkjson(int); }
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

/* Tools for turning a type into something completely random, which is
 * sometimes useful for fuzzing. */
namespace nonmetatypes {
template <typename t> void randomise(t &, randomiser &); }
struct randomiser {
    /* Primary API: randomise @val. Distribution aims for
     * uniformity. */
public:  template <typename t> void randomise(t & val) {
        using sty = decltype(gettypestyle((t ***)nullptr));
        randomise(val, sty()); }
    /* Generate a load of independently uniform bytes. Mostly useful
     * if you're implementing a nonmeta randomiser. */
public:  void randombytes(void * ptr, size_t sz);

    /* Indirection layer to cope with different type styles. */
private: template <typename t> void randomise(t & val, typestyles::meta) {
    val.randomise(*this); }
private: template <typename t> void randomise(t & val, typestyles::nonmeta) {
    nonmetatypes::randomise(val, *this); }
private: template <typename t> void randomise(
    t & val,
    typestyles::fundamental) {
    randombytes(&val, sizeof(val)); } };

/* Type t can inherit from meta<t> to get some useful operations for free:
 *
 * -- serialise
 * -- deserialise
 * -- mkjson
 * -- randomise
 * -- operator <
 *
 * If you do, you need to implement a visit() method, which needs to give
 * the meta framework all of your member pointers. e.g.:
 *
 * struct foo : meta<foo> {
 *    int field1;
 *    std::string field2;
 *    some_other_type field3;
 *    template <typename visitor> static bool visit(visitor && v) {
 *       return v("field1", &foo::field1) &&
 *              v("field2", &foo::field2) &&
 *              v("field3", &foo::field3); } };
 *
 * The visitor should be called once for each field, giving it a
 * string description of the field (mostly useful for json) and a
 * pointer-to-member to the field. The visitor returns a bool, which
 * is either true to keep going or false to stop early. */
template <typename what>
struct meta {
    // Tag so that gettypestyle can easily recognise meta<> classes,
    // doing the SFINAE trick.
public: using typestyle = typestyles::meta;

    // Most things in this class are private, intended only to help
    // implement APIs on the matching interface class, so they're
    // private here and we're friends with the interfaces.
    friend struct deserialiser;
    friend struct randomiser;
    friend struct serialiser;

private: what const & inner() const { return *static_cast<what const *>(this); }
private: what & inner() { return *static_cast<what *>(this); }
private: void serialise(serialiser & t) const {
    what::visit([&](const char *, auto val) {
            t.serialise(inner().*val);
            return true; }); }
private: void deserialise(deserialiser & t) {
    what::visit(
        [&](const char *, auto val) {
            t.deserialise(inner().*val);
            return true; });}
private: void randomise(randomiser & t) {
    what::visit(
        [&](const char *, auto val) {
            t.randomise(inner().*val);
            return true; }); }

    // mkjson() is a function template, rather than a class, so friend
    // declaration is tricky. Just make it public instead.
public:  std::string json() const {
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

    // operators pretty much have to be public, or else what's the
    // point?
public:  bool operator<(const what & o) const {
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
