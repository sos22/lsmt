#ifndef META_H__
#define META_H__

#include <assert.h>

#include "order.H"
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
 *    template <typename visitor> static void visit(visitor && v) {
 *       v("field1", &foo::field1) &&
 *              v("field2", &foo::field2) &&
 *              v("field3", &foo::field3); } };
 *
 * The visitor should be called once for each field, giving it a
 * string description of the field (mostly useful for json) and a
 * pointer-to-member to the field. The visitor returns a bool, which
 * is either true to keep going or false to stop early. visit() itself
 * returns void. */
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
    friend struct comparer;

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
private: order compare(const what & o) const {
    order res(order::eq);
    what::visit([&](const char *, auto val) {
            res = comparer::compare(inner().*val, o.*val);
            return !res; });
    return res; }

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

    // Special case for operator==, in case one of the fields has ==
    // but not ordering. Still need an explicit operator== in @what,
    // so that it wins over the generic ones in order.H, but at least
    // this makes it a bit easier to define.
public:  bool operator==(const what & o) const {
    bool res{true};
    what::visit([&](const char *, auto val) {
            res = inner().*val == o.*val;
            return res; });
    return res; } };

#endif /* !META_H__ */
