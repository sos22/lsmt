#ifndef META_H__
#define META_H__

#include <assert.h>

#include "randomise.H"
#include "serialise.H"
#include "typestyles.H"


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

struct order {
    // Negative -> less than, 0 -> equal, positive -> greater than
private: int inner;
public:  explicit order(int _inner) : inner(_inner) { }
public:  const char * name() const {
    if (inner < 0) return "LT";
    else if (inner == 0) return "EQ";
    else return "GT"; }
public:  bool operator <(order o) const {
    if (o.inner < 0) return false;
    else if (inner < 0) return true;
    else if (o.inner == 0) return false;
    else if (inner == 0) return true;
    else return false; }
public: operator bool() const { return !!inner; } };

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
