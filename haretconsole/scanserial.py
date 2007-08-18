#!/usr/bin/env python

# Script that can decode a raw haret log of traces into a list of
# read/written serial strings.
#
# (C) Copyright 2007 Kevin O'Connor <kevin@koconnor.net>
#
# This file may be distributed under the terms of the GNU GPL license.

import sys
import re
import os
import tempfile
import struct

import dis

HEXALL=0

redebug = dis.redebug
reirq = dis.reirq

# Codes to watch for
SCANTRACE = 1
WRITEINSN = 0xe5832000
READINSN = 0xe5933000
##SCANTRACE = 0
##WRITEINSN = 0x0935acb0
##READINSN = 0x0935a9f0

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
    """Show 'buf' in hex"""
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
        print "%s-%s %5s: %s" % (
            dis.getClock(BufferStart), dis.getClock(BufferEnd)
            , BufferType, encodeBuf(Buffer))
    Buffer = ""
    BufferStart = BufferEnd = BufferType = None

def procline(line):
    if SCANTRACE:
        # Look at memory trace events
        m = redebug.match(line)
        if m is None:
            return dis.procline(line)
        insn = int(m.group('insn'), 16)
        Rd = int(m.group('Rd'), 16)
    else:
        # Look at breakpoints
        m = reirq.match(line)
        if m is None:
            return dis.procline(line)
        parts = line.split()
        if parts[2] != 'insn':
            return dis.procline(line)
        insn = int(parts[3][:-1], 16)
        if insn not in (WRITEINSN, READINSN):
            return
        Rd = int(parts[4], 16)
    if insn == WRITEINSN:
        cmdtype = 'write'
    elif insn == READINSN:
        cmdtype = 'read'
    else:
        return dis.procline(line)

    if Rd > 0xff:
        return dis.procline(line)
    if cmdtype != BufferType and BufferType is not None:
        flushBuffer()
    appendBuffer(m, cmdtype, Rd)
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
