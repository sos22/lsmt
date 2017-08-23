#ifndef RANDOMISE_H__
#define RANDOMISE_H__

#include "typestyles.H"

struct randomiser;

/* Tools for turning a type into something completely random, which is
 * sometimes useful for noddy fuzzing. */
namespace nonmetatypes {
template <typename t> void randomise(t &, randomiser &); }
struct randomiser {
    /* Primary API: randomise @val. Distribution aims for
     * uniformity. */
public:  template <typename t> void randomise(t & val) {
        using sty = decltype(gettypestyle((t ***)nullptr));
        randomise(val, sty()); }
    /* Generate a load of independently uniform bytes. Mostly useful
     * if you're implementing a nonmeta randomiser. */
public:  void randombytes(void * ptr, size_t sz);

    /* Indirection layer to cope with different type styles. */
private: template <typename t> void randomise(t & val, typestyles::meta) {
    val.randomise(*this); }
private: template <typename t> void randomise(t & val, typestyles::nonmeta) {
    nonmetatypes::randomise(val, *this); }
private: template <typename t> void randomise(
    t & val,
    typestyles::fundamental) {
    randombytes(&val, sizeof(val)); } };

#endif /* !RANDOMISE_H__ */
