#! /usr/bin/env python3

import subprocess
import time

def time_thing(name, thing, nrsamples=10):
    results = []
    for _ in range(nrsamples):
        start = time.time()
        thing()
        end = time.time()
        results.append(end - start)
    results.sort()
    mean = sum(results) / len(results)
    sd = (sum([(x-mean)**2 for x in results])/len(results))**.5
    def percentile(perc):
        idx = len(results) * perc
        if idx == int(idx):
            return results[int(idx)]
        else:
            return results[int(idx)] * (1 - idx + int(idx)) + \
                results[int(idx)+1] * (idx - int(idx))
    print("%40s: mean %f, sd %f(%f), median %f, 10%% %f, 90%% %f" % (
        name, mean, sd, sd/mean, percentile(.5), percentile(0.1), percentile(0.9)))

def compilefile(filename):
    subprocess.check_call("clang++ -I. -O0 -std=gnu++1y -g -Wall -c {}".format(filename),
                          shell=True)

def baseline_no_meta(name, nr_structs, nr_fields):
    with open("test.C", "w") as w:
        w.write('#include "meta.H"\n\n')
        for x in range(nr_structs):
            w.write("struct struct%d {\n" % x)
            for y in range(nr_fields):
                w.write("    int a%d;\n" % y)
            w.write("};\n")
    time_thing("%s %d %d" % (name, nr_structs, nr_fields), lambda : compilefile("test.C"))

def unused_meta(name, nr_structs, nr_fields):
    """Structs have meta<> but nobody uses it."""
    with open("test.C", "w") as w:
        w.write('#include "meta.H"\n\n')
        for x in range(nr_structs):
            w.write("struct struct%d : meta<struct%d> {\n" % (x, x))
            for y in range(nr_fields):
                w.write("    int a%d;\n" % y)
            w.write("     template <typename c, typename t> static void visit(c cc, t && v) {\n")
            for y in range(nr_fields):
                if y == 0:
                    w.write("        ")
                else:
                    w.write("        && ")
                w.write('v(cc, "a%d", &struct%d::a%d)\n' % (y, x, y))
            w.write("; } };\n")
    time_thing("%s %d %d" % (name, nr_structs, nr_fields), lambda : compilefile("test.C"))

def gen_serialise(name, nr_structs, nr_fields):
    """Force instantiation of serialise template."""
    with open("test.C", "w") as w:
        w.write('#include "meta.H"\n\n')
        for x in range(nr_structs):
            w.write("struct struct%d : meta<struct%d> {\n" % (x, x))
            for y in range(nr_fields):
                w.write("    int a%d;\n" % y)
            w.write("     template <typename c, typename t> static void visit(c cc, t && v) {\n")
            if nr_fields == 0:
                w.write("true;")
            else:
                for y in range(nr_fields):
                    if y == 0:
                        w.write("         ")
                    else:
                        w.write("        && ")
                    w.write('v(cc, "a%d", &struct%d::a%d)\n' % (y, x, y))
            w.write("; } };\n")
            w.write("void ser%d(struct%d const & str, serialiser & ser) {\n" % (x, x))
            w.write("    ser.serialise(str); }")
    time_thing("%s %d %d" % (name, nr_structs, nr_fields), lambda : compilefile("test.C"))

def manual_serialise(name, nr_structs, nr_fields):
    """Generate a serialise method manually and explicitly, rather than
    relying on the template."""
    with open("test.C", "w") as w:
        w.write('#include "meta.H"\n\n')
        for x in range(nr_structs):
            w.write("struct struct%d : meta<struct%d> {\n" % (x, x))
            for y in range(nr_fields):
                w.write("    int a%d;\n" % y)
            w.write("};\n")
            w.write("void ser%d(struct%d const & str, serialiser & ser) {\n" % (x, x))
            for y in range(nr_fields):
                w.write("ser.pushbytes(&str.a%d, sizeof(str.a%d));\n" % (y, y))
            w.write("}\n")
    time_thing("%s %d %d" % (name, nr_structs, nr_fields), lambda : compilefile("test.C"))

def semi_manual_serialise(name, nr_structs, nr_fields):
    """Manually unrole top level of serialise template."""
    with open("test.C", "w") as w:
        w.write("#define private public\n")
        w.write('#include "meta.H"\n\n')
        for x in range(nr_structs):
            w.write("struct struct%d : meta<struct%d> {\n" % (x, x))
            for y in range(nr_fields):
                w.write("    int a%d;\n" % y)
            w.write("};\n")
            w.write("void ser%d(struct%d const & str, serialiser & ser) {\n" % (x, x))
            for y in range(nr_fields):
                w.write("ser.serialise(str.a%d);" % y)
            w.write("}\n")
    time_thing("%s %d %d" % (name, nr_structs, nr_fields), lambda : compilefile("test.C"))

def runtest(name, test):
    for nr_fields in [15]:
        for nr_structs in [500]:
            test(name, nr_structs, nr_fields)

runtest("semi_manual", semi_manual_serialise)
runtest("manual", manual_serialise)
runtest("serialise", gen_serialise)
runtest("base", baseline_no_meta)
runtest("unused", unused_meta)
