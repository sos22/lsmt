#include "meta.H"

#include "tests/basic.H"

std::string fundamentaltypes::mkjson(int val) {
    char buf[32];
    sprintf(buf, "%d", val);
    return buf; }

template <> std::string nonmetatypes::mkjson(std::string const & what) {
    return "\"" + what + "\""; }

int main() {
    basictest();
    return 0; }
