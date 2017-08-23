#include "serialise.H"

template <> void nonmetatypes::serialise(
    std::string const & str,
    serialiser & s) {
    size_t sz(str.size());
    s.serialise(sz);
    s.pushbytes(str.data(), sz); }

template <> void nonmetatypes::deserialise(
    std::string & str,
    deserialiser & ds) {
    size_t sz;
    ds.deserialise(sz);
    auto buf(ds.getbytes(sz));
    if (buf) str = std::string((char *)buf, sz); }
