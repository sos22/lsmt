#ifndef TYPESTYLES_DECL_H__
#define TYPESTYLES_DECL_H__

/* Style of type, for our meta framework. Either meta, for things
 * which inherit from meta<>, nonmeta, for classes which don't, or
 * fundamental for types like int or char which we can treat as a
 * fixed size block of bytes. Extract the style of type T with
 * gettypestyle(T***), which can't actually be called but the return
 * type is useful. */
namespace typestyles {
struct meta {};
struct nonmeta {};
struct fundamental {}; }

#endif /* !TYPESTYLES_DECL_H__ */
