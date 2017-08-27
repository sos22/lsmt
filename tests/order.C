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
    for (auto a : {order(-1), order(0), order(1)}) {
        for (auto b : {order(-1), order(0), order(1)}) {
            cout << a << "\t" << b << "\t";
#define one(name) cout << (a name b) << "\t";
            opers(one)
#undef one
            if (b == order(-1)) cout << (bool)(a);
            cout << "\n";
        }
    }
    assert((order(0) ?: order(1)) == order(1));
    assert((order(-1) ?: order(1)) == order(-1));
    return 0; }
