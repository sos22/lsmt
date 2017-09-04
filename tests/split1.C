/* Test which uses a meta serialiser without being able to instantiate
 * it. */
#include "split.H"
#include "serialise.H"

#include <assert.h>

int main() {
    split spl;
    spl.x = 12345;
    serialiser ser;
    ser.serialise(spl);
    deserialiser deser(ser.stage, ser.cursor);
    split spl2;
    deser.deserialise(spl2);
    assert(spl.x == spl2.x);
    return 0; }
