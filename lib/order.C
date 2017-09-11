#include "meta.H"
#include "order.H"

const order order::lt{-1};
const order order::eq{0};
const order order::gt{1};

template <> order nonmetatypes::compare(order const & a, order const & b) {
    if (a.inner < 0) {
        if (b.inner < 0) return order::eq;
        else return order::lt;  }
    else if (a.inner == 0) {
        if (b.inner < 0) return order::gt;
        else if (b.inner == 0) return order::eq;
        else return order::gt; }
    else {
        if (b.inner <= 0) return order::gt;
        else return order::eq; } }

template <> order nonmetatypes::compare(
    std::string const & a,
    std::string const & b) {
    auto r = a.compare(b);
    if (r < 0) return order::lt;
    else if (r == 0) return order::eq;
    else return order::gt; }
