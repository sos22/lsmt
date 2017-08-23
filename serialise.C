#include "serialise.H"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

void serialiser::pushbytes(void const * what, size_t sz) {
    if (sz + cursor > allocated) {
        allocated += 4096;
        stage = realloc(stage, allocated);
        assert(stage); }
    memcpy(
        reinterpret_cast<void *>(
            reinterpret_cast<uint64_t>(stage) + cursor),
        what,
        sz);
    cursor += sz; }

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
