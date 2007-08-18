# Utility to parse out memory dumps from haret and translate them into
# register names.
#
# (C) Copyright 2007 Kevin O'Connor <kevin@koconnor.net>
#
# This file may be distributed under the terms of the GNU GPL license.

import re

# Regs_xxx = {addr: name, ...}
#   or
# Regs_xxx = {addr: (name, ((bits, name), (bits,name), ...)) }
#   or
# Regs_xxx = {addr: (name, func(bit)) }

RegsList = {}

import regs_pxa
import regs_s3c
import regs_omap


######################################################################
# Register list pre-processing
######################################################################

# Parse a bit name description.  It can accept a function that
# converts a number to a name or a list of bit/name 2-tuples that
# describes a bit.  The description can be an integer bit number or a
# string description of a set of bits (a comma separated list and/or
# hyphen separated range).
def parsebits(defs):
    if type(defs) != type(()):
        # lambda
        return tuple([((1<<i), defs(i)) for i in range(32)])
    out = ()
    for desc, val in defs:
        if type(desc) == type(1):
            out += (((1<<desc),val),)
            continue
        bits = ()
        for bit in desc.split(','):
            bitrange = bit.split('-', 1)
            if len(bitrange) > 1:
                bits += tuple(range(int(bitrange[0]), int(bitrange[1])-1, -1))
            else:
                bits += (int(bit),)
        mask = 0
        for bit in bits:
            mask |= (1<<bit)
        out += ((mask, val),)
    return out

# Internal function called at script startup time.  It processes the
# register maps into an internal uniform format.  The format is:
# RegsList = {"archname": {paddr: ("regname", ((bit_n, "bitname"),...))}}
def preproc():
    global RegsList
    rl = {}
    for name, regs in RegsList.items():
        archinfo = {}
        for addr, info in regs.items():
            if type(info) == type(""):
                info = (info,)
            else:
                info = (info[0], parsebits(info[1]))
            archinfo[addr] = info
        rl[name] = archinfo
    RegsList = rl
preproc()

######################################################################
# Memory display
######################################################################

# Last process "clock" val - used for determining clock offsets
LastClock = 0

# Given a matching TIMEPRE_S regex - return the string description of it.
def getClock(m):
    global LastClock
    clock = m.group('clock')
    t = "%07.3f" % (int(m.group('time')) / 1000.0,)
    if clock is None:
        return t
    clock = int(clock, 16)
    out = "%07d" % (clock - LastClock,)
    LastClock = clock
    return "%s(%s)" % (t, out)

# Internal class that stores an active "watch" region.  It is used
# when reformating the output string of a memory trace event.
class memDisplay:
    def __init__(self, reginfo, type, pos):
        self.type = ""
        if type:
            self.type = " " + type
        self.pos = pos
        self.name = reginfo[0]
        self.bits = reginfo[1]
    # Return a string representation of a set of changed bits.
    def getValue(self, desc, bits, mask, val):
        count = 0
        outval = 0
        pos = []
        bits &= mask
        for i in range(32):
            bit = 1<<i
            if bit & bits:
                if bit & val:
                    outval |= 1<<count
                pos.append(i)
                count += 1
        ignorelist = " ".join(["%d" % (self.pos*32+i) for i in pos])
        return " %s(%s)=%d" % (desc, ignorelist, outval)
    # Main function call into this class - display a mem trace line
    def displayFunc(self, m):
        val = int(m.group('val'), 16)
        mask = int(m.group('mask'), 16)
        if not self.bits or not mask:
            print "%s%s %8s=%08x (%08x)" % (
                getClock(m), self.type, self.name, val, mask)
        origmask = mask
        out = ""
        for bits, desc in self.bits:
            if bits & mask:
                out += self.getValue(desc, bits, mask, val)
                mask &= ~bits
        if mask:
            print "%s%s %8s=%08x (%08x)%s" % (
                getClock(m), self.type, self.name, val, origmask, out)
        else:
            print "%s%s %8s:%s" % (
                getClock(m), self.type, self.name, out)

# Default memory tracing line if no custom register specified.
def defaultFunc(m):
    e = m.end('clock')
    if e < 0:
        e = m.end('time')
    print getClock(m), m.string[e+6:]


######################################################################
# Parsing
######################################################################

# Regular expressions to search for.
TIMEPRE_S = r'^(?P<time>[0-9]+): ((?P<clock>[0-9a-f]+): )?'
re_detect = re.compile(r"^Detected machine (?P<name>.*)/(?P<arch>.*)"
                       r" \(Plat=.*\)$")
re_begin = re.compile(r"^Beginning memory tracing\.$")
re_watch = re.compile(r"^Watching (?P<type>.*)\((?P<pos>\d+)\):"
                      r" Addr (?P<vaddr>.*)\(@(?P<paddr>.*)\)$")
re_mem = re.compile(TIMEPRE_S + r"mem (?P<vaddr>.*)=(?P<val>.*)"
                    r" \((?P<mask>.*)\)$")

# Storage of known named registers that are being "watched"
VirtMap = {}
# The active architecture named registers
ArchRegs = {}

def handleWatch(m):
    vaddr = m.group('vaddr')
    paddr = int(m.group('paddr'), 16)
    reginfo = ArchRegs.get(paddr)
    if reginfo is not None:
        md = memDisplay(reginfo, m.group('type'), int(m.group('pos')))
        VirtMap[vaddr] = md.displayFunc
    print m.string

def handleBegin(m):
    global LastClock
    LastClock = 0
    VirtMap.clear()
    print m.string

def handleDetect(m):
    global ArchRegs
    ar = RegsList.get(m.group('name'))
    if ar is None:
        ar = RegsList.get("ARCH:"+m.group('arch'), {})
    ArchRegs = ar
    print m.string

def procline(line):
    m = re_mem.match(line)
    if m:
        func = VirtMap.get(m.group('vaddr'), defaultFunc)
        func(m)
        return
    m = re_watch.match(line)
    if m:
        return handleWatch(m)
    m = re_begin.match(line)
    if m:
        return handleBegin(m)
    m = re_detect.match(line)
    if m:
        return handleDetect(m)
    print line.rstrip()
