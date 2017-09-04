#ifndef META_DEFN_H__
#define META_DEFN_H__

#include "randomise.H"
#include "order.H"

template <typename t> std::string mkjson(t const & what,
                                         typestyles::nonmeta) {
    return nonmetatypes::mkjson(what); }

template <typename t> std::string mkjson(t const & what,
                                         typestyles::fundamental) {
    return fundamentaltypes::mkjson(what); }

template <typename t> std::string mkjson(t const & what,
                                         typestyles::meta) {
    return what.json(); }

template <typename t> std::string mkjson(t const & what) {
    using sty = decltype(gettypestyle((t ***)nullptr));
    return mkjson(what, sty()); }

template <typename what> what const & meta<what>::inner() const {
    return *static_cast<what const *>(this); }

template <typename what> what & meta<what>::inner() {
    return *static_cast<what *>(this); }

template <typename what> void meta<what>::serialise(serialiser & t) const {
    struct _state {
        what const * i;
        serialiser * j;
    } state;
    state.i = &inner();
    state.j = &t;
    what::visit(state, [](_state const & s,const char *, auto val) {
            s.j->serialise(s.i->*val);
            return true; }); }

template <typename what> void meta<what>::deserialise(deserialiser & t) {
    struct _state {
        what * i;
        deserialiser * j;
    } state;
    state.i = &inner();
    state.j = &t;
    what::visit(state,
        [&](_state const & s, const char *, auto val) {
            s.j->deserialise(s.i->*val);
            return true; });}

template <typename what> void meta<what>::randomise(randomiser & t) {
    what::visit(nullptr,
        [&](void *, const char *, auto val) {
            t.randomise(inner().*val);
            return true; }); }

template <typename what> order meta<what>::compare(what const & o) const {
    order res((order::eq));
    what::visit(nullptr, [&](void *, const char *, auto val) {
            res = comparer::compare(inner().*val, o.*val);
            return !res; });
    return res; }

template <typename what> std::string meta<what>::json() const {
    std::string acc;
    acc.append("{");
    what::visit(nullptr, [&](void *, const char * name, auto val) {
            acc.append("\"");
            acc.append(name);
            acc.append("\": ");
            auto v(mkjson(inner().*val));
            acc.append(v);
            acc.append(", ");
            return true; });
    if (acc.size() > 1) acc.erase(acc.begin() + acc.size() - 2, acc.end());
    acc.append("}");
    return acc; }

template <typename what> bool meta<what>::operator==(const what & o) const {
    bool res{true};
    what::visit(nullptr, [&](void *, const char *, auto val) {
            res = inner().*val == o.*val;
            return res; });
    return res; }


#endif /* !META_DEFN_H__ */
