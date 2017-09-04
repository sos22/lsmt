#ifndef SPLIT_H__
#define SPLIT_H__

#include "meta_decl.H"

struct split : meta<split> {
public: int x;
public: template <typename c, typename v> static void visit(c cc, v && vv) {
    vv(cc, "x", &split::x); } };

#endif /* !SPLIT__ */
