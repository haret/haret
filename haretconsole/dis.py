#!/usr/bin/env python

# Script that can process the output from haret's "wi" command and
# dissasemble the software trace events.

import sys
import re
import os
import tempfile
import struct

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

TIMEPRE_S = r'^(?P<time>[0-9]+): (?P<clock>[0-9a-f]+): '
redebug = re.compile(TIMEPRE_S + r'debug (?P<addr>.*):'
                     r' (?P<insn>.*)\(.*\) (?P<Rd>.*) (?P<Rn>.*)$')
reirq = re.compile(TIMEPRE_S + r'(?P<data>(irq |insn |mem |cpu resumed).*)$')
re_start = re.compile(r'^Replacing windows exception handlers')

def transRegVal(reg, val):
    return "r%d=%s" % (reg, val)

LastClock = 0

def getClock(m):
    global LastClock
    clock = int(m.group('clock'), 16)
    out = "%07d" % (clock - LastClock,)
    LastClock = clock
    return "%07.3f(%s)" % (int(m.group('time')) / 1000.0, out)

def procline(line):
    m = redebug.match(line)
    if m is None:
        m = reirq.match(line)
        if m is not None:
            print getClock(m), m.group('data')
        else:
            m = re_start.match(line)
            if m is not None:
                global LastClock
                LastClock = 0
            print line.rstrip()
        return
    insn = int(m.group('insn'), 16)
    iname = dis(insn)
    Rd = (insn >> 12) & 0xF
    Rn = (insn >> 16) & 0xF
    Rdval = transRegVal(Rd, m.group('Rd'))
    if Rd == Rn:
        regs = Rdval
    else:
        regs = Rdval + " " + transRegVal(Rn, m.group('Rn'))
    print "%s %s: %-30s # %s" % (
        getClock(m), m.group('addr'), iname, regs)

def main():
    lines = sys.stdin.readlines()
    for line in lines:
        procline(line)

if __name__ == '__main__':
    main()
