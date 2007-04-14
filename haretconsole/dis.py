#!/usr/bin/env python

# Script that can process the output from haret's "wi" command and
# dissasemble the software trace events.

import sys
import re
import os
import tempfile
import struct

import memalias

OBJDUMP='arm-linux-objdump'
OBJDUMPARGS='-D -b binary -m arm'
CLOCKRATE=416000000.0/64

def findObjdumpLoc():
    scriptloc = os.sep.join(sys.argv[0].split(os.sep)[:-1])
    tryloc = scriptloc + os.sep + OBJDUMP
    if os.access(tryloc, os.X_OK):
        loc = tryloc
    elif os.access(OBJDUMP, os.X_OK):
        # Check current directory
        loc = "." + os.sep + OBJDUMP
    else:
        # Try running flow from the execute path
        loc = OBJDUMP
    return loc

BINLOC=findObjdumpLoc()

InsnCache = {}
def dis(insn):
    if insn in InsnCache:
        return InsnCache[insn]
    f = tempfile.NamedTemporaryFile(bufsize=0)
    f.write(struct.pack("<I", insn))
    l = os.popen(BINLOC + " " + OBJDUMPARGS + " " + f.name)
    lines = l.readlines()
    f.close()
    out = "disassemble error"
    for line in lines:
        if line[:5] == '   0:':
            parts = line[5:].strip().split(None, 2)
            out = "%s %-6s %s" % tuple(parts)
            break
    InsnCache[insn] = out
    return out

TIMEPRE_S = memalias.TIMEPRE_S
re_debug = re.compile(TIMEPRE_S + r'debug (?P<addr>.*):'
                      r' (?P<insn>.*)\(.*\) (?P<Rd>.*) (?P<Rn>.*)$')
re_irq = re.compile(TIMEPRE_S + r'(?P<data>(irq |insn |cpu resumed).*)$')
getClock = memalias.getClock

def transRegVal(reg, val):
    return "r%d=%s" % (reg, val)

def procline(line):
    m = re_debug.match(line)
    if m is None:
        m = re_irq.match(line)
        if m is not None:
            print getClock(m), m.group('data')
        else:
            memalias.procline(line)
        return
    insn = int(m.group('insn'), 16)
    iname = dis(insn)
    Rd = (insn >> 12) & 0xF
    Rn = (insn >> 16) & 0xF
    Rdval = transRegVal(Rd, m.group('Rd'))
    Rnval = transRegVal(Rn, m.group('Rn'))
    if Rd == Rn and Rdval == Rnval:
        regs = Rdval
    else:
        regs = Rdval + " " + Rnval
    print "%s %s: %-30s # %s" % (
        getClock(m), m.group('addr'), iname, regs)

def main():
    lines = sys.stdin.readlines()
    for line in lines:
        procline(line.rstrip())

if __name__ == '__main__':
    main()
