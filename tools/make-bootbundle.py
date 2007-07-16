#!/usr/bin/env python

import sys
import os
import stat
import struct

if len(sys.argv) != 5:
    print "make-bootbundle - Make a standalone HaRET boot bundle with kernel and initrd"
    print "Usage: make-bootbundle.py <path to haret.exe> <zImage> <initrd> <script>"
    sys.exit(0)

os.system("cat %s %s %s %s> bootbundle.exe"  % (sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4]))

exe = open("bootbundle.exe", "r+b")
kernelSt = os.stat(sys.argv[2])
initrdSt = os.stat(sys.argv[3])
scriptSt = os.stat(sys.argv[4])

exe.seek(0, 2)
exe.write("HARET1\0\0")
exe.write(struct.pack("i", kernelSt[stat.ST_SIZE]))
exe.write(struct.pack("i", initrdSt[stat.ST_SIZE]))
exe.write(struct.pack("i", scriptSt[stat.ST_SIZE]))
exe.write(struct.pack("i", 0))
exe.write(struct.pack("i", 0))
exe.write(struct.pack("i", 0))
exe.close()
