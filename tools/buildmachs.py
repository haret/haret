#!/usr/bin/env python

# classname, archtype, oeminfos, machtype, memsize

import sys

def error(msg):
    sys.stderr.write(msg + "\n")
    sys.exit(1)

try:
    import csv
except:
    error("Sorry, this script needs Python v2.3 or later")

def main():
    platform = "PocketPC"
    # Read input and strip out comments
    lines = []
    for line in sys.stdin.readlines():
        line = line.lstrip()
        if not line:
            continue
        if line[0] == '#':
            continue
        lines.append(line)
    # Parse line using csv
    machs = []
    for line in csv.reader(lines):
        if len(line) < 3:
            if len(line) == 1 and line[0][:9] == 'PLATFORM=':
                platform = line[0][9:]
                continue
            error("Invalid line (less than 3 elements): %s" % line[0])
        data = {'classname': line[0].strip(),
                'platform': platform,
                'arch': line[1].strip(),
                'oems': line[2].split(';'),
                'machtype': None, 'memsize': None}
        if len(line) > 3 and line[3].strip():
            data['machtype'] = line[3].strip()
        if len(line) > 4 and line[4].strip():
            data['memsize'] = line[4].strip()
        if len(line) > 5:
            error("Too many items for mach %s" % line[0].strip())
        machs.append(data)
    # Build output file
    sys.stdout.write("""// !!! This file is auto generated !!!
// Please see tools/buildmachs.py to regenerate this file.

#include "arch-omap.h"
#include "arch-s3.h"
#include "arch-pxa.h"
#include "arch-sa.h"

#include "mach-types.h"
#include "memory.h" // memPhysSize
""")

    for mach in machs:
        # Optional init function
        initfunc = ""
        if mach['memsize'] is not None:
            initfunc = """
    void init() {
        Machine%s::init();
        memPhysSize = %s;
    }""" % (mach['arch'], mach['memsize'])

        # Build oemstrings
        oems = ""
        count = 0
        for oem in mach['oems']:
            oems += '\n        OEMInfo[%d] = L"%s";' % (count, oem)
            count += 1

        # Build optional platform
        platform = ""
        if mach['platform'] != 'PocketPC':
            platform = '\n        PlatformType = L"%s";' % mach['platform']

        # Build optional machtype
        machtype = ""
        if mach['machtype'] is not None:
            machtype = "machType = MACH_TYPE_%s;" % mach['machtype']

        # Build class output
        sys.stdout.write("""
class Mach%s : public Machine%s {
public:
    Mach%s() {
        name = "%s";%s%s
        %s
    }%s
};
REGMACHINE(Mach%s)
""" % (mach['classname'], mach['arch'], mach['classname'], mach['classname']
       , platform, oems, machtype, initfunc, mach['classname']))

if __name__ == '__main__':
    main()
