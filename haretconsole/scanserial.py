#!/usr/bin/env python

# Script that can decode a raw haret log of traces into a list of
# read/written serial strings.

import sys
import re
import os
import tempfile
import struct

import dis

CLOCKRATE=dis.CLOCKRATE
HEXALL=0

redebug = dis.redebug
reirq = dis.reirq

# Buffer variables
Buffer = ""
BufferType = None
BufferStart = None
BufferEnd = None

def appendBuffer(mtime, cmdtype, val):
    global Buffer, BufferType, BufferStart, BufferEnd
    if BufferStart == None:
        BufferStart = mtime
    BufferEnd = mtime
    BufferType = cmdtype
    Buffer = Buffer + chr(val)

def encodeBuf(buf):
    """Show 'buf' in hex.  Note only used with -x option"""
    out = ""
    for c in buf:
        o = ord(c)
        if HEXALL or ((o < 32 or o > 127) and o not in [10, 13]):
            out += "<%02x>" % o
        else:
            out += c
    return repr(out)

def flushBuffer():
    global Buffer, BufferType, BufferStart, BufferEnd
    if Buffer:
        print "%010.6f-%010.6f %5s: %s" % (
            BufferStart, BufferEnd, BufferType, encodeBuf(Buffer))
    Buffer = ""
    BufferStart = BufferEnd = BufferType = None

def procline(line):
    m = redebug.match(line)
    if m is None:
        return dis.procline(line)
    t = int(m.group('time'), 16) / CLOCKRATE
    insn = int(m.group('insn'), 16)
    if insn == 0xe5832000:
        cmdtype = 'write'
    elif insn == 0xe5933000:
        cmdtype = 'read'
    else:
        return dis.procline(line)
    Rd = int(m.group('Rd'), 16)
    if Rd > 0xff:
        return dis.procline(line)
    if cmdtype != BufferType and BufferType is not None:
        flushBuffer()
    appendBuffer(t, cmdtype, Rd)
    if (cmdtype == 'read' and (Rd == 10 or (Rd == 13 and Buffer == '0\r'))
        or cmdtype == 'write' and Rd == 13):
        flushBuffer()

def main():
    lines = sys.stdin.readlines()
    for line in lines:
        procline(line)
    flushBuffer()

if __name__ == '__main__':
    main()
