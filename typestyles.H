#ifndef TYPESTYLES_H__
#define TYPESTYLES_H__

#include <string>

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

#endif /* !TYPESTYLES_H__ */
