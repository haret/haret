#!/usr/bin/env python
#
# Licensed under GPL v3
# (c) 2007 Paul Sokolovsky
#
"""
Take cpio archive on stdin, and append to it a file, dumping result on stdout.
Usage:
cpio-append <fname> [<archive_fname>]
"""

import sys
import os
import stat

def copyData(fIn, fOut):
    while True:
        b = fIn.read(4096)
        if not b: break
        fOut.write(b)

class FileWithAlign:

    def __init__(self, f):
	self.cnt = 0
	self.f = f
	
    def write(self, data):
	self.f.write(data)
	self.cnt += len(data)

    def align(self, align = 4):
	a = self.cnt & (align - 1)
	if a > 0:
	    self.write("\0" * (align - a))

    def close(self):
	self.f.close()

class CpioReader:

    def __init__(self, f = sys.stdin):
	self.debug = False
	self.cnt = 0
	self.f = f

    def read(self, cnt):
	self.cnt += cnt
	return self.f.read(cnt)

    def alignRead(self):
	a = self.cnt & 3
	if a > 0:
	    self.f.read(4 - a)
	    self.cnt += 4 - a

    def readHeader(self):
	self.alignRead()
	self.header = self.read(110)
	#print self.header
	assert self.header[0:6] == "070701"
	namesize = int(self.header[94:102], 16)
	self.fname = self.read(namesize)
	assert self.fname[-1] == "\0"
	self.fname = self.fname[:-1]
	self.size = int(self.header[54:62], 16)
	self.alignRead()
	
    def copyHeader(self, fOut):
	fOut.align()
	fOut.write(self.header)
	fOut.write(self.fname + "\0")
	fOut.align()

    def skipData(self):
	s = self.size
	while s > 0:
	    rs = s
	    if rs > 4096: rs = 4096
	    self.read(rs)
	    s -= rs

    def copyData(self, fOut):
	s = self.size
	while s > 0:
	    rs = s
	    if rs > 4096: rs = 4096
	    d = self.read(rs)
	    fOut.write(d)
	    s -= rs
	
    def close(self):
	self.f.close()

class CpioWriter:

    def __init__(self, f = sys.stdout):
	self.debug = False
	self.cnt = 0
	self.f = f

    def format_num(self, val, width = 8):
	val = "%X" % val
        val = val.zfill(width)
	return val

    def dump_field(self, v):
	if self.debug:
	    self.f.write(repr(v) + "|")
	else:
	    self.f.write(v)
	self.cnt += len(v)

    def writeHeader(self, fname, size):
	self.f.align()
	self.cnt = 0
	self.dump_field("070701")
	self.dump_field(self.format_num(0))
	self.dump_field(self.format_num(0100644))
	self.dump_field(self.format_num(0))
	self.dump_field(self.format_num(0))
	self.dump_field(self.format_num(1))
	#mtime
	self.dump_field(self.format_num(0))
	# size
	self.dump_field(self.format_num(size))
	self.dump_field(self.format_num(0))
	self.dump_field(self.format_num(0))
	self.dump_field(self.format_num(0))
	self.dump_field(self.format_num(0))
	# c_namesize
	self.dump_field(self.format_num(len(fname) + 1))
	self.dump_field(self.format_num(0))
	assert self.cnt == 110
	self.dump_field(fname + "\0")
	self.f.align()

    def writeFile(self, fname, archiveFname = None):
	if not archiveFname: archiveFname = fname
	size = os.stat(fname)
	self.writeHeader(archiveFname, size[stat.ST_SIZE])
	f = open(fname, "rb")
	copyData(f, self.f)
	f.close()

    def writeTrailer(self):
	self.writeHeader("TRAILER!!!", 0)
	

fIn = CpioReader()
fOut = FileWithAlign(sys.stdout)
while True:
    fIn.readHeader()
#    sys.stderr.write(fIn.fname + "\n")
    if fIn.fname == "TRAILER!!!":
	break
    fIn.copyHeader(fOut)
    fIn.copyData(fOut)
#sys.exit()

#fIn = open(sys.argv[1], "ab")
#fIn.seek(0, 2)
c = CpioWriter(fOut)
arcFname = None
if len(sys.argv) > 2:
    arcFname = sys.argv[2]
c.writeFile(sys.argv[1], arcFname)
c.writeTrailer()
#fOut.align(512)
fOut.close()
fIn.close()
