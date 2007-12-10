#!/usr/bin/env python

# Script that can decode a raw haret log of traces into a list of
# read/written serial strings.
#
# (C) Copyright 2007 Kevin O'Connor <kevin@koconnor.net>
#
# This file may be distributed under the terms of the GNU GPL license.

import sys
import getopt

import dis

class serBuf:
    """Class that accumulates characters into a buffer."""
    def __init__(self, hexall):
        self.hexall = hexall
        self.buffer = ""
        self.bufferStart = self.bufferEnd = self.bufferType = None

    def encodeBuf(self):
        """Show buffer in hex"""
        out = ""
        for c in self.buffer:
            o = ord(c)
            if self.hexall or ((o < 32 or o > 127) and o not in [10, 13]):
                out += "<%02x>" % o
            else:
                out += c
        return repr(out)

    def flush(self):
        """Print out internal buffer and reset it"""
        if self.buffer:
            print "%s-%s %5s: %s" % (
                dis.getClock(self.bufferStart), dis.getClock(self.bufferEnd)
                , self.bufferType, self.encodeBuf())
        self.buffer = ""
        self.bufferStart = self.bufferEnd = self.bufferType = None

    def append(self, mtime, cmdtype, val):
        """Add a character to the internal buffer"""
        if cmdtype != self.bufferType and self.bufferType is not None:
            self.flush()
        if self.bufferStart == None:
            self.bufferStart = mtime
        self.bufferEnd = mtime
        self.bufferType = cmdtype
        self.buffer = self.buffer + chr(val)
        if (cmdtype == 'read' and (val == 10 or (val == 13
                                                 and self.buffer == '0\r'))
            or cmdtype == 'write' and val == 13):
            self.flush()

def procline(sb, line, addr):
    m = dis.re_trace.match(line)
    if m is not None:
        vaddr = int(m.group('vaddr'), 16)
        val = int(m.group('val'), 16)
        paddr, reginfo = dis.memalias.lookupVirt(vaddr)
        if paddr == addr and val <= 0xff:
            insn = int(m.group('insn'), 16)
            if insn & (1<<20):
                cmdtype = 'read'
            else:
                cmdtype = 'write'
            sb.append(m, cmdtype, val)
            return
    sb.flush()
    dis.procline(line)

def printUsage():
    print "Usage:\n   %s [-h] <paddr>" % (sys.argv[0],)
    sys.exit(1)

def main():
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'h')
    except getopt.error:
        printUsage()
    try:
        addr = int(args[0], 16)
    except:
        printUsage()
    hexall = ('-h', '') in opts

    sb = serBuf(hexall)
    lines = sys.stdin.readlines()
    for line in lines:
        procline(sb, line.rstrip(), addr)
    sb.flush()

if __name__ == '__main__':
    main()
