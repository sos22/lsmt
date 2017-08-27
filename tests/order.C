#include "meta.H"

#include <iostream>

using namespace std;

std::ostream & operator<<(std::ostream & a, order b) { return a << b.name(); }

int main() {
#define opers(iter)                             \
    iter(<)                                     \
    iter(<=)                                \
    iter(==)                                \
    iter(!=)                                \
    iter(>=)                                \
    iter(>)

    cout << "  \t  \t";
#define one(name) cout << #name << "\t";
    opers(one)
#undef one
    cout << "bool\n";
    for (auto a : {order::lt, order::eq, order::gt}) {
        for (auto b : {order::lt, order::eq, order::gt}) {
            cout << a << "\t" << b << "\t";
#define one(name) cout << (a name b) << "\t";
            opers(one)
#undef one
            if (b == order::lt) cout << (bool)(a);
            cout << "\n";
        }
    }
    assert((order::eq ?: order::gt) == order::gt);
    assert((order::lt ?: order::gt) == order::lt);
    return 0; }
