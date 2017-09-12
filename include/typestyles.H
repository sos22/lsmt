#ifndef TYPESTYLES_H__
#define TYPESTYLES_H__

#include <string>

#include "typestyles_decl.H"

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
typestyles::nonmeta gettypestyle(std::type_info ***);                                                                

#endif /* !TYPESTYLES_H__ */
