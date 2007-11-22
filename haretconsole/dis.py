#!/usr/bin/env python

# Script that can process the output from haret's "wi" command and
# dissasemble the software trace events.
#
# (C) Copyright 2007 Kevin O'Connor <kevin@koconnor.net>
#
# This file may be distributed under the terms of the GNU GPL license.

import sys
import re
import os
import tempfile
import struct

import memalias

OBJDUMP=['arm-linux-objdump', 'arm-wince-mingw32ce-objdump']
OBJDUMPARGS='-D -b binary -m arm'
CLOCKRATE=416000000.0/64

def findObjdumpLoc():
    scriptloc = os.sep.join(sys.argv[0].split(os.sep)[:-1])
    trydirs = [scriptloc, '.', '/opt/mingw32ce/bin']
    for name in OBJDUMP:
        for loc in trydirs:
            tryloc = loc + os.sep + name
            if os.access(tryloc, os.X_OK):
                return tryloc
    # Try running from the path
    loc = OBJDUMP[0]
    return loc

BINLOC=findObjdumpLoc()

InsnCache = {}
def dis(insn, match):
    if insn in InsnCache:
        return InsnCache[insn]
    f = tempfile.NamedTemporaryFile(bufsize=0)
    f.write(struct.pack("<I", insn))
    l = os.popen(BINLOC + " " + OBJDUMPARGS + " " + f.name)
    lines = l.readlines()
    f.close()
    for line in lines:
        if line[:5] == '   0:':
            parts = line[5:].strip().split(None, 2)
            out = "%-6s %s" % tuple(parts[1:])
            break
    else:
        out = "%08x(%s)" % (insn, match.group('desc'))
    InsnCache[insn] = out
    return out

TIMEPRE_S = memalias.TIMEPRE_S
re_debug = re.compile(
    TIMEPRE_S + r'debug (?P<addr>.*):'
    r' (?P<insn>.*)\(.*\) (?P<Rd>.*) (?P<Rn>.*)$')
re_trace = re.compile(
    TIMEPRE_S + r'mmutrace (?P<addr>.*):'
    r' (?P<insn>.*)\((?P<desc>.*)\) (?P<vaddr>.*) (?P<val>.*)'
    r' \((?P<changed>.*)\)$')
re_irq = re.compile(
    TIMEPRE_S + r'(?P<data>(irq |cpu resumed).*)$')
getClock = memalias.getClock

def transRegVal(reg, val):
    return "r%d=%s" % (reg, val)

def procline(line):
    m = re_debug.match(line)
    if m is not None:
        insn = int(m.group('insn'), 16)
        iname = dis(insn, m)
        Rd = (insn >> 12) & 0xF
        Rn = (insn >> 16) & 0xF
        Rdval = transRegVal(Rd, m.group('Rd'))
        Rnval = transRegVal(Rn, m.group('Rn'))
        if Rd == Rn:
            regs = Rdval
        else:
            regs = Rdval + " " + Rnval
        print "%s %s: %-21s # %s" % (
            getClock(m), m.group('addr'), iname, regs)
        return

    m = re_trace.match(line)
    if m is not None:
        insn = int(m.group('insn'), 16)
        iname = dis(insn, m)
        changed = int(m.group('changed'), 16)
        if changed:
            changed = " (%08x)" % changed
        else:
            changed = ""
        addrname = m.group('vaddr')
        vaddr = int(addrname, 16)
        paddr = memalias.VirtTrace.get(vaddr & 0xfff00000)
        if paddr is not None:
            reginfo = memalias.ArchRegs.get(paddr | (vaddr & 0xfffff))
            if reginfo is not None:
                addrname = "%-8s" % reginfo[0]
        print "%s %s: %-21s # a=%s v=%s%s" % (
            getClock(m), m.group('addr'), iname
            , addrname, m.group('val'), changed)
        return

    m = re_irq.match(line)
    if m is not None:
        print getClock(m), m.group('data')
        return

    memalias.procline(line)

def main():
    lines = sys.stdin.readlines()
    for line in lines:
        procline(line.rstrip())

if __name__ == '__main__':
    main()
