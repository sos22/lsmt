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

serialiser::~serialiser() { free(stage); }

deserialiser::deserialiser(void const * _buf, size_t _size)
    : buf_start(reinterpret_cast<uintptr_t>(_buf))
    , buf_end(buf_start + _size)
    , buf_next(buf_start) { }

bool deserialiser::failed() const { return buf_next > buf_end; }

void deserialiser::fail() { buf_next = buf_end + 1; }

void deserialiser::open(void const * start, size_t sz) {
	buf_start = reinterpret_cast<uint64_t>(start);
	buf_end = buf_start + sz;
	buf_next = buf_start; }

void deserialiser::close() {
	buf_start = 0;
	buf_end = 0;
	buf_next = 1; }

void const * deserialiser::getbytes(size_t sz) {
    if (buf_next + sz > buf_end) {
        fail();
        return nullptr; }
    else {
        auto res = reinterpret_cast<void const *>(buf_next);
        buf_next += sz;
        return res; } }

bool deserialiser::finished() const { return buf_next >= buf_end; }

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

extern "C" void dosomething(void *) {}
