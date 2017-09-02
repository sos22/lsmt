/* Simple test where we serialise and deserialise a small structure a
 * million times using meta-generated serialisers, and then do it
 * again with a manually-written one, and compare the difference. */
#include <sys/time.h>

#include "meta.H"

#define NRITERS 10000000

struct smallstruct : meta<smallstruct> {
    unsigned a{0};
    unsigned b{0};
    unsigned c{0};
    unsigned d{0};
    unsigned e{0};
    unsigned f{0};
    unsigned g{0};
    unsigned h{0};
    unsigned i{0};
    unsigned j{0};
    template <typename v_t> static void visit(v_t && v) {
        v("a", &smallstruct::a) &&
            v("b", &smallstruct::b) &&
            v("c", &smallstruct::c) &&
            v("d", &smallstruct::d) &&
            v("e", &smallstruct::e) &&
            v("f", &smallstruct::f) &&
            v("g", &smallstruct::g) &&
            v("h", &smallstruct::h) &&
            v("i", &smallstruct::i) &&
            v("j", &smallstruct::j); }
    void manualserialise(serialiser & s) {
        s.pushbytes(&a, sizeof(a));
        s.pushbytes(&b, sizeof(b));
        s.pushbytes(&c, sizeof(c));
        s.pushbytes(&d, sizeof(d));
        s.pushbytes(&e, sizeof(e));
        s.pushbytes(&f, sizeof(f));
        s.pushbytes(&g, sizeof(g));
        s.pushbytes(&h, sizeof(h));
        s.pushbytes(&i, sizeof(i));
        s.pushbytes(&j, sizeof(j)); }
    void manualdeserialise(deserialiser & dd) {
        dd.deserialise(a, typestyles::fundamental());
        dd.deserialise(b, typestyles::fundamental());
        dd.deserialise(c, typestyles::fundamental());
        dd.deserialise(d, typestyles::fundamental());
        dd.deserialise(e, typestyles::fundamental());
        dd.deserialise(f, typestyles::fundamental());
        dd.deserialise(g, typestyles::fundamental());
        dd.deserialise(h, typestyles::fundamental());
        dd.deserialise(i, typestyles::fundamental());
        dd.deserialise(j, typestyles::fundamental()); } };

static double now() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec * 1e-6; }

int main() {
    while (true) {
        // meta-generated serialisers;
        {   serialiser s;
            double start_ser = now();
            for (unsigned x = 0; x < NRITERS; x++) {
                s.serialise(smallstruct()); }
            double end_ser = now();
            deserialiser d(s.stage, s.cursor);
            for (unsigned x = 0; x < NRITERS; x++) {
                smallstruct s;
                d.deserialise(s); }
            double end_der = now();
            printf("Meta: %f %f\n", end_ser - start_ser, end_der - end_ser); }
        // Manually-generated ones.
        {   serialiser s;
            double start_ser = now();
            for (unsigned x = 0; x < NRITERS; x++) {
                smallstruct().manualserialise(s); }
            double end_ser = now();
            deserialiser d(s.stage, s.cursor);
            for (unsigned x = 0; x < NRITERS; x++) {
                smallstruct s;
                s.manualdeserialise(d); }
            double end_der = now();
            printf("Manual: %f %f\n", end_ser - start_ser, end_der - end_ser);}}
    return 0; }
