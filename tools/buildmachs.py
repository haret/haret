#!/usr/bin/env python

# classname, archtype, oeminfos, machtype, memsize

import sys
import string

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
    reader = csv.reader(lines, escapechar="\\")
    for line in reader:
        while line and line[-1][-1] == '\n':
            # Python 2.5 bug?
            line.pop()
            line += reader.next()
        if len(line) < 3:
            if len(line) == 1 and line[0][:9] == 'PLATFORM=':
                platform = line[0][9:]
                continue
            error("Invalid line (less than 3 elements): %s" % line[0])
        data = {'classname': line[0].strip(),
                'platform': platform,
                'arch': line[1].strip(),
                'oems': line[2].split(';'),
                'machtype': None, 'cmds': None}
        if len(line) > 3 and line[3].strip():
            data['machtype'] = line[3].strip()
        if len(line) > 4:
            data['cmds'] = line[4:]
        machs.append(data)
    # Build output file
    sys.stdout.write("""// !!! This file is auto generated !!!
// Please see tools/buildmachs.py to regenerate this file.

#include "arch-omap.h"
#include "arch-s3.h"
#include "arch-pxa.h"
#include "arch-imx.h"
#include "arch-sa.h"
#include "arch-msm.h"

#include "mach-types.h"
#include "script.h" // runMemScript
""")

    for mach in machs:
        # Optional init function
        initfunc = ""
        if mach['cmds'] is not None:
            cmds = [cmd.replace('\\\n', '').strip() for cmd in mach['cmds']]
            cmds = '"' + '\\n"\n                     "'.join(cmds) + '\\n"'
            initfunc = """
    void init() {
        Machine%s::init();
        runMemScript(%s);
    }""" % (mach['arch'], cmds)

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
