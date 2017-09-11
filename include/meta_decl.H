#ifndef META_DECL_H__
#define META_DECL_H__

#include <string>

#include "typestyles_decl.H"

struct serialiser;
struct deserialiser;
struct randomiser;
struct order;

/* std::string mkjson(t const &) -- turn an arbitrary type into json,
 * as a string.  Works for meta and fundamental types. Can be made to
 * work for nonmetas if they implement nonmetatypes::mkjson. */
namespace nonmetatypes {
template <typename t> std::string mkjson(t const &); }
namespace fundamentaltypes { std::string mkjson(int); }
template <typename t> std::string mkjson(t const & what,
                                         typestyles::nonmeta);
template <typename t> std::string mkjson(t const & what,
                                         typestyles::fundamental);
template <typename t> std::string mkjson(t const & what,
                                         typestyles::meta);
template <typename t> std::string mkjson(t const & what);

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
 *    template <typename state, typename visitor>
 *    static void visit(state c, visitor && v) {
 *       v(c, "field1", &foo::field1) &&
 *              v(c, "field2", &foo::field2) &&
 *              v(c, "field3", &foo::field3); } };
 *
 * The visitor should be called once for each field, giving it a
 * string description of the field (mostly useful for json) and a
 * pointer-to-member to the field. The visitor returns a bool, which
 * is either true to keep going or false to stop early. visit() itself
 * returns void. The state parameter to visit() is kind of redundant,
 * because a lambda capture could do the same thing, but making it
 * explicit seems to help the compiler do useful optimisations. */
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

private: what const & inner() const;
private: what & inner();
private: void serialise(serialiser & t) const;
private: void deserialise(deserialiser & t);
private: void randomise(randomiser & t);
private: order compare(const what & o) const;

    // mkjson() is a function template, rather than a class, so friend
    // declaration is tricky. Just make it public instead.
public:  std::string json() const;

    // Special case for operator==, in case one of the fields has ==
    // but not ordering. Still need an explicit operator== in @what,
    // so that it wins over the generic ones in order.H, but at least
    // this makes it a bit easier to define.
public:  bool operator==(const what & o) const; };

#endif /* !META_DECL_H__ */
