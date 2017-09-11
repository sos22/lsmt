#ifndef ORDER_H__
#define ORDER_H__

#include "typestyles.H"

struct order;

namespace nonmetatypes {
template <typename t> order compare(t const &, t const &); }

struct order {
    // -1 -> less than, 0 -> equal, +1 -> greater than
public:  int inner;
private: explicit constexpr order(int _inner) : inner(_inner) { }
public:  static const order lt;
public:  static const order eq;
public:  static const order gt;
public:  const char * name() const {
    if (inner < 0) return "LT";
    else if (inner == 0) return "EQ";
    else return "GT"; }
public: operator bool() const { return !!inner; } };
typestyles::nonmeta gettypestyle(order ***);

// Simple wrapper for compare operators. Has no state, so pretty
// pointless on the face of it, but makes it easier to get the right
// overload resolution and public/private.
struct comparer {
public: template <typename t> static order compare(const t & a, const t & b) {
    using sty = decltype(gettypestyle((t ***)nullptr));
    return compare(a, b, sty()); }
private: template <typename t> static order compare(const t & a,
                                                    const t & b,
                                                    typestyles::meta) {
    return a.compare(b); }
private: template <typename t> static order compare(t a,
                                                    t b,
                                                    typestyles::fundamental){
    if (a < b) return order::lt;
    else if (a == b) return order::eq;
    else return order::gt; }
private: template <typename t> static order compare(const t & a,
                                                    const t & b,
                                                    typestyles::nonmeta) {
    return nonmetatypes::compare(a, b); } };

// Assume we already have comparer::compare. Derive ordering
// operators.
template <typename a> bool operator< (const a & x, const a & y) {
    return comparer::compare(x, y) <  order::eq; }
template <typename a> bool operator<=(const a & x, const a & y) {
    return comparer::compare(x, y) <= order::eq; }
template <typename a> bool operator==(const a & x, const a & y) {
    return comparer::compare(x, y) == order::eq; }
template <typename a> bool operator>=(const a & x, const a & y) {
    return comparer::compare(x, y) >= order::eq; }
template <typename a> bool operator> (const a & x, const a & y) {
    return comparer::compare(x, y) >  order::eq; }

// Special case for unorderable types with operator== but no full compare.
template <typename a> bool operator!=(const a & x, const a & y) {
    return !(x == y); }

// Can't use template-generate operators for order itself, so do it by
// hand.
static inline bool operator< (const order & x, const order & y) {
    return x.inner <  y.inner; }
static inline bool operator<=(const order & x, const order & y) {
    return x.inner <= y.inner; }
static inline bool operator==(const order & x, const order & y) {
    return x.inner == y.inner; }
static inline bool operator>=(const order & x, const order & y) {
    return x.inner >= y.inner; }
static inline bool operator> (const order & x, const order & y) {
    return x.inner >  y.inner; }

#endif /* !ORDER_H__ */
