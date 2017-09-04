#ifndef SERIALISE_DECL_H__
#define SERIALISE_DECL_H__

struct deserialiser;
struct serialiser;

namespace nonmetatypes {
/* Serialiser for types which don't fit either meta or fundamental
 * styles.  This template is never defined, so you need to provide a
 * specialisation for anything which needs it. */
template <typename t> void serialise(t const &, serialiser &); }

/* Simple serialiser. Types are always serialisable if the
 * gettypestyle() is either meta or fundamental. nonmeta types are
 * serialisable iff they have a specialisation() method in the
 * nonmetatypes namespace.
 */
struct serialiser {
public:  void * stage {nullptr};
private: size_t allocated {0};
public:  size_t cursor {0};

    // Main API for serialising arbitrary types.
public:  template <typename t> void serialise(t const & what);

    // Indirection functions to help find the right serialiser
    // implementation for different type styles.
private: template <typename t> void serialise(t const & what,
                                              typestyles::meta);
private: template <typename t> void serialise(t const & what,
                                              typestyles::nonmeta);
private: template <typename t> void serialise(t const & what,
                                              typestyles::fundamental);

    // Helper for implementing serialisers for things which aren't
    // meta. Just copy a run of bytes into the staging area, advancing
    // the cursor.
public: void pushbytes(void const * what, size_t sz);

public: ~serialiser(); };

namespace nonmetatypes {
// Basically, the opposite of nonmetatypes::serialise.
template <typename t> void deserialise(t &, deserialiser &); }
/* Simple deserialiser. Should work for anything serialiser works
 * for. Note that this is a failure-oblivious deserialiser, so the
 * actual deserialise op always gives you back an instance of the
 * type, even after a deserialisation failure.  If you care, you
 * should almost certainly call failed() when you're done. */
struct deserialiser {
private: uintptr_t const buf_start;
private: uintptr_t const buf_end;
    /* Note that buf_next might be past the buf_end if we're failed. */
private: uintptr_t buf_next{0};

public:  deserialiser(void const * _buf, size_t _size);

    /* Has the deserialiser failed? If it has, anything it returned
     * should be treated as suspect. */
public:  bool failed() const;
    /* Mark the deserialiser as failed. */
public:  void fail();
    /* Deserialise one thing, putting the result into @val and
     * advancing the cursor. */
public:  template <typename t> void deserialise(t & val);
    /* Helper for implementing non-meta deserialisers. */
public:  void const * getbytes(size_t sz);

    /* Indirection layer to find the right deserialiser. */
private: template <typename t> void deserialise(t & what, typestyles::meta);
private: template <typename t> void deserialise(t & what, typestyles::nonmeta);
public:  template <typename t> void deserialise(
    t & what, typestyles::fundamental); };


#endif /* !SERIALISE_DECL_H__ */
