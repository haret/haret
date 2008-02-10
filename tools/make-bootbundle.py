#!/usr/bin/env python

import sys
import os
import stat
import struct
import getopt
import re

optlist, args = getopt.gnu_getopt(sys.argv[1:], "o:vh?")
opts = {}
opts.update(optlist)

if len(args) != 4:
    print "make-bootbundle - Make a standalone HaRET boot bundle with kernel and initrd"
    print "Usage: make-bootbundle.py -o <outfile> <path to haret.exe> <zImage> <initrd> <script>"
    sys.exit(0)

outfile = opts["-o"]

os.system("cat %s %s %s %s> %s"  % (args[0], args[1], args[2], args[3], outfile))

exe = open(outfile, "r+b")
kernelSt = os.stat(args[1])
initrdSt = os.stat(args[2])
scriptSt = os.stat(args[3])

exe.seek(0, 2)
exe.write("HARET1\0\0")
exe.write(struct.pack("i", kernelSt[stat.ST_SIZE]))
exe.write(struct.pack("i", initrdSt[stat.ST_SIZE]))
exe.write(struct.pack("i", scriptSt[stat.ST_SIZE]))
exe.write(struct.pack("i", 0))
exe.write(struct.pack("i", 0))
exe.write(struct.pack("i", 0))
exe.close()

if opts.has_key("-v"):
    haretSt = os.stat(args[0])
    print "HaRET:\t", haretSt[stat.ST_SIZE]
    print "Kernel:\t", kernelSt[stat.ST_SIZE]
    print "Initrd:\t", initrdSt[stat.ST_SIZE]
    print "Script:\t", scriptSt[stat.ST_SIZE]
    print "Header:\t", 8 + 4*6
    print "-------------"
    print "Total:\t", \
	haretSt[stat.ST_SIZE] + kernelSt[stat.ST_SIZE] + \
	initrdSt[stat.ST_SIZE] + scriptSt[stat.ST_SIZE] + 8 + 4*6
