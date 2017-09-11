#ifndef SERIALISE_H__
#define SERIALISE_H__

#include "typestyles.H"

#include "serialise_decl.H"

template <typename t> void serialiser::serialise(t const & what) {
    using sty = decltype(gettypestyle((t ***)nullptr));
    serialise(what, sty()); }

template <typename t> void serialiser::serialise(t const & what,
                                                 typestyles::meta) {
    what.serialise(*this); }
template <typename t> void serialiser::serialise(t const & what,
                                                 typestyles::nonmeta) {
    nonmetatypes::serialise(what, *this); }
template <typename t> void serialiser::serialise(t const & what,
                                                 typestyles::fundamental) {
    pushbytes(&what, sizeof(what)); }

    /* Deserialise one thing, putting the result into @val and
     * advancing the cursor. */
template <typename t> void deserialiser::deserialise(t & val) {
    using sty = decltype(gettypestyle((t ***)nullptr));
    deserialise(val, sty()); }

    /* Indirection layer to find the right deserialiser. */
template <typename t> void deserialiser::deserialise(t & what,
                                                     typestyles::meta) {
    what.deserialise(*this); }
template <typename t> void deserialiser::deserialise(t & what,
                                                     typestyles::nonmeta) {
    nonmetatypes::deserialise(what, *this); }
template <typename t> void deserialiser::deserialise(
    t & what, typestyles::fundamental) {
    if (buf_next + sizeof(t) > buf_end) {
        what = 0;
        fail(); }
    else {
        what = *reinterpret_cast<t const *>(buf_next);
        buf_next += sizeof(t); } }

#endif /* !SERIALISE_H__ */
