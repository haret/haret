#!/usr/bin/env python

# This script is useful for taking haret memory dumps and converting
# them back into binary output.  This can be useful, for example, when
# one wants to push that data into other tools like objdump.
#
# (C) Copyright 2007 Kevin O'Connor <kevin@koconnor.net>
#
# This file may be distributed under the terms of the GNU GPL license.

import sys
import struct

def unhex(str):
    return int(str, 16)

def parseMem(filename, memstart, memend=None):
    f = open(filename, 'r')
    mem = []
    for line in f:
        parts = line.split('|')
        if len(parts) < 2:
            continue
        try:
            vaddr = unhex(parts[0])
        except ValueError:
            continue
        if vaddr != memstart:
            continue
        if vaddr >= memend:
            break
        parts = parts[1].split()
        mem.extend([unhex(v) for v in parts])
        memstart += len(parts) * 4
    return mem

def printUsage():
    sys.stderr.write("Usage:\n %s <mem-dump.txt> <start> <end>\n"
                     % (sys.argv[0],))
    sys.exit(1)

def main():
    if len(sys.argv) != 4:
        printUsage()
    filename = sys.argv[1]
    startaddr = int(sys.argv[2], 0)
    endaddr = int(sys.argv[3], 0)
    mem = parseMem(filename, startaddr, endaddr)
    for i in mem:
        sys.stdout.write(struct.pack("<I", i))

if __name__ == '__main__':
    main()
