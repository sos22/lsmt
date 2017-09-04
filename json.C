#include "meta.H"

#include <string>

std::string fundamentaltypes::mkjson(int val) {
    char buf[32];
    sprintf(buf, "%d", val);
    return buf; }

template <> std::string nonmetatypes::mkjson(std::string const & what) {
    return "\"" + what + "\""; }

