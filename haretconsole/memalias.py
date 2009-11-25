# Utility to parse out memory dumps from haret and translate them into
# register names.
#
# (C) Copyright 2007 Kevin O'Connor <kevin@koconnor.net>
#
# This file may be distributed under the terms of the GNU GPL license.

import re

# List of all available register names.
# RegsList["archname"] = Regs_xxx
#   Regs_xxx = {paddr: name, ...}
#     or
#   Regs_xxx = {paddr: (name, ((bits, name), (bits,name), ...)) }
RegsList = {}

# Helper - create description for registers that are composed of an
# incrementing list of one bit fields.
def regOneBits(name, start=0):
    return tuple([(i, "%s%d" % (name, i + start)) for i in range(32)])

# Helper - create description for registers that are composed of an
# incrementing list of two bit fields.
def regTwoBits(name, start=0):
    return tuple([("%d,%d" % (i, i+1), "%s%d" % (name, i/2 + start))
                  for i in range(0, 32, 2)])

# Helper - create description for registers that are composed of an
# incrementing list of four bit fields.
def regFourBits(name, start=0):
    return tuple([("%d-%d" % (i, i+3), "%s%d" % (name, i/4 + start))
                  for i in range(0, 32, 4)])

import regs_pxa
import regs_s3c
import regs_omap
import regs_msm
import regs_misc


######################################################################
# Register list pre-processing
######################################################################

# Parse a bit name description.  It accepts a list of bit/name
# 2-tuples that describes a bit.  The description can be an integer
# bit number or a string description of a set of bits (a comma
# separated list and/or hyphen separated range).
def parsebits(defs):
    out = ()
    for desc, val in defs:
        if type(desc) == type(1):
            out += (((1<<desc),val),)
            continue
        bits = ()
        for bit in desc.split(','):
            bitrange = bit.split('-', 1)
            if len(bitrange) > 1:
                start = int(bitrange[0])
                end = int(bitrange[1])
                if start > end:
                    bits += tuple(range(end, start+1))
                else:
                    bits += tuple(range(start, end+1))
            else:
                bits += (int(bit),)
        mask = 0
        for bit in bits:
            mask |= (1<<bit)
        out += ((mask, val),)
    return out

# Internal function called at script startup time.  It processes the
# register maps into an internal uniform format.  The format is:
# RegsList = {"archname": {paddr: ("regname", ((bitmask, "bitname"),...))}}
def preproc():
    global RegsList
    rl = {}
    for name, regs in RegsList.items():
        archinfo = {}
        for addr, info in regs.items():
            if type(info) == type(""):
                info = (info, ())
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

# Extract the specified bits from a value
def extractValue(val, bits, changed):
    count = 0
    outval = 0
    pos = []
    for i in range(32):
        bit = 1<<i
        if bit & bits:
            if bit & val:
                outval |= 1<<count
            if bit & changed:
                pos.append(i)
            count += 1
    return (outval, pos)

# Return a string representation of a set of changed bits.
def bitDecode(m, bits, desc, val, changed, add_il):
    outval, pos = extractValue(val, bits, changed)
    if add_il:
        varpos = int(m.group('varpos'))
        ignorelist = " ".join(["%d" % (varpos*32+i) for i in pos])
        return " %s(%s)=%x" % (desc, ignorelist, outval)
    return " %s=%x" % (desc, outval)

# Main function call into this class - display a mem trace line
def memDecode(m, reginfo):
    regname, bitdefs = reginfo
    var = m.group('var')
    pc = m.group('pc')
    val = int(m.group('val'), 16)
    changed = int(m.group('changed'), 16)
    notfirst = 1
    if not changed:
        # Start of trace - show all bit defs, but don't show the
        # list of bits to ignore.
        changed = val
        notfirst = 0
    if not bitdefs:
        # Only a register name exists (no bit definitions)
        out = bitDecode(m, ~0, "%08s" % regname, val, changed, notfirst)
        print "%s %8s %s%s" % (getClock(m), var, out, pc)
        return
    out = ""
    unnamedbits = changed
    for bits, desc in bitdefs:
        if bits & changed:
            out += bitDecode(m, bits, desc, val, changed, notfirst)
            unnamedbits &= ~bits
    if unnamedbits:
        # Not all changed bits are named - come up with a "dummy" name
        # for the remaining bits.
        out += bitDecode(m, ~0, "?", val&unnamedbits, unnamedbits, notfirst)
    if notfirst:
        print "%s %8s %8s:%s%s" % (getClock(m), var, regname, out, pc)
    else:
        # Show register value along with any additional info
        print "%s %8s %8s=%08x:%s%s" % (getClock(m), var, regname, val, out, pc)


######################################################################
# Parsing
######################################################################

# Regular expressions to search for.
TIMEPRE_S = r'^(?P<time>[0-9]+): ((?P<clock>[0-9a-f]+): )?'
re_begin = re.compile(r"^Beginning memory tracing\.$")
re_detect = re.compile(
    r"^Detected machine (?P<name>.*)/(?P<arch>.*) \(Plat=.*\)$")
re_watch = re.compile(
    r"^Watching (?P<var>.*)\((?P<varpos>\d+)\): ("
    r"Addr (?P<vaddr>.*)\(@(?P<paddr>.*)\)|"
    r"Insn (?P<insn>.*))$")
re_mmu = re.compile(
    r"^(?P<varpos>\d+): Mapping (?P<vaddr>.*)\(@(?P<paddr>.*)\) accesses to"
    r" (?P<newvaddr>.*) \(tbl (?P<tbldev>.*)\)$")
re_mem = re.compile(
    TIMEPRE_S + r"(?P<type>insn|mem) (?P<var>.*)\((?P<varpos>\d+)\)"
    r" (?P<vaddr>.*)=(?P<val>.*) \((?P<changed>.*)\)(?P<pc>( @~.*)?)$")

# Storage of known named registers that are being "watched"
# VirtMap[vaddr] = (paddr, (name, ((bits, name), (bits,name), ...)))
VirtMap = {}
# Storage of known mmutrace mappings
# VirtTrace[vaddr & 0xFFF00000] = paddr
VirtTrace = {}
# Most recent observed values from watch
# WatchValues[paddr] = val
WatchValues = {}
# The active architecture named registers
# ArchRegs[paddr] = (name, ((bits, name), (bits,name), ...))
ArchRegs = {}
# The address of a register name
# ArchRegNames[name] = (paddr, bits)
ArchRegNames ={}

def lookupVirt(vaddr):
    """Locate register info for a vaddr returned from mmutrace report."""
    paddr = VirtTrace.get(vaddr & 0xfff00000)
    if paddr is None:
        return None, None
    paddr = paddr | (vaddr & 0xfffff)
    return paddr, ArchRegs.get(paddr)

def handleWatch(m):
    """Process a setup message from 'watch'"""
    name = m.group('vaddr')
    if name is not None:
        key = int(m.group('paddr'), 16)
    else:
        name = key = "insn:"+m.group('insn')
    VirtMap[name] = (key, ArchRegs.get(key))
    print m.string

def handleMem(m):
    """Process an update from the 'watch' command"""
    name = m.group('vaddr')
    if m.group('type') == 'insn':
        name = "insn:"+name
    paddr, reginfo = VirtMap.get(name, (None, None))
    WatchValues[paddr] = int(m.group('val'), 16)
    if reginfo is None:
        # No named register - invent a "dummy" one
        reginfo = (name, ())
    memDecode(m, reginfo)

def handleWatchMMU(m):
    """Process an mmutrace setup message"""
    VirtTrace[int(m.group('vaddr'), 16)] = int(m.group('paddr'), 16)
    print m.string

def handleBegin(m):
    """Note the beginning of a new 'watch' type command"""
    global LastClock
    LastClock = 0
    VirtMap.clear()
    VirtTrace.clear()
    WatchValues.clear()
    print m.string

def handleDetect(m):
    """Process a setup message that shows what machine is in use"""
    global ArchRegs
    ar = RegsList.get(m.group('name'))
    if ar is None:
        ar = RegsList.get("ARCH:"+m.group('arch'), {})
    ArchRegs = ar
    ArchRegNames.clear()
    for paddr, reginfo in ar.items():
        regname, bitdefs = reginfo
        if not bitdefs:
            ArchRegNames[regname] = (paddr, ~0)
            continue
        for bits, desc in bitdefs:
            ArchRegNames["%s/%s" % (regname, desc)] = (paddr, bits)
            ArchRegNames[desc] = (paddr, bits)
    print m.string

def procline(line):
    m = re_mem.match(line)
    if m:
        return handleMem(m)
    m = re_watch.match(line)
    if m:
        return handleWatch(m)
    m = re_mmu.match(line)
    if m:
        return handleWatchMMU(m)
    m = re_begin.match(line)
    if m:
        return handleBegin(m)
    m = re_detect.match(line)
    if m:
        return handleDetect(m)
    print line.rstrip()
