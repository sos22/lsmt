/* Simple test where we serialise and deserialise a small structure a
 * million times using meta-generated serialisers, and then do it
 * again with a manually-written one, and compare the difference. */
#include <sys/time.h>
#include <math.h>
#include <assert.h>

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
    template <typename c_t, typename v_t> static void visit(c_t c, v_t && v) {
        v(c, "a", &smallstruct::a) &&
            v(c, "b", &smallstruct::b) &&
            v(c, "c", &smallstruct::c) &&
            v(c, "d", &smallstruct::d) &&
            v(c, "e", &smallstruct::e) &&
            v(c, "f", &smallstruct::f) &&
            v(c, "g", &smallstruct::g) &&
            v(c, "h", &smallstruct::h) &&
            v(c, "i", &smallstruct::i) &&
            v(c, "j", &smallstruct::j); }
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

struct stats {
    double sum{0};
    double sum2{0};
    unsigned nr{0};
    void sample(double what) {
        sum += what;
        sum2 += what * what;
        nr++; }
    double mean() const { return sum / nr; }
    double sd() const {
        return sqrt(sum2 / nr - mean() * mean()); } };

extern "C" void dosomething(smallstruct *);

int main() {
    stats manual_ser;
    stats manual_deser;
    stats meta_ser;
    stats meta_deser;
    for (unsigned x = 0; x < 5; x++) {
        // meta-generated serialisers;
        {   serialiser s;
            double start_ser = now();
            for (unsigned x = 0; x < NRITERS; x++) {
                s.serialise(smallstruct()); }
            double end_ser = now();
            deserialiser d(s.stage, s.cursor);
            for (unsigned x = 0; x < NRITERS; x++) {
                smallstruct s;
                d.deserialise(s);
                dosomething(&s); }
            double end_der = now();
            meta_ser.sample(end_ser - start_ser);
            meta_deser.sample(end_der - end_ser);
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
                s.manualdeserialise(d);
                dosomething(&s); }
            double end_der = now();
            manual_ser.sample(end_ser - start_ser);
            manual_deser.sample(end_der - end_ser);
            printf("Manual: %f %f\n", end_ser - start_ser, end_der - end_ser);}}
    printf("meta ser %f+-%f\n", meta_ser.mean(), meta_ser.sd());
    printf("meta deser %f+-%f\n", meta_deser.mean(), meta_deser.sd());
    printf("manual ser %f+-%f\n", manual_ser.mean(), manual_ser.sd());
    printf("manual deser %f+-%f\n", manual_deser.mean(), manual_deser.sd());
    return 0; }
