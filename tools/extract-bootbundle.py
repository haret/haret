#!/usr/bin/env python

# Tool to extract the parts that went into a boot-bundle created with
# make-bootbundle.py

import sys
import struct

def printUsage():
    print "Usage:\n   %s <bootbundle.exe>" % (sys.argv[0],)
    sys.exit(1)

def main():
    if len(sys.argv) != 2:
        printUsage()
    filename = sys.argv[1]

    # Read in file
    data = open(filename, "rb").read()

    # Verify and extract boot-bundle header
    if len(data) < 32 or data[-32:-24] != "HARET1\0\0":
        print "Not a valid boot bundle"
        sys.exit(1)
    ksize = struct.unpack("i", data[-24:-20])[0]
    isize = struct.unpack("i", data[-20:-16])[0]
    ssize = struct.unpack("i", data[-16:-12])[0]
    print ksize, isize, ssize
    hsize = len(data) - (ksize + isize + ssize + 32)
    if hsize < 0:
        print "Boot bundle header invalid"
        sys.exit(1)

    # Write out boot bundle parts.
    print "Writing extractharet.exe"
    f = open('extractharet.exe', 'wb')
    f.write(data[:hsize])
    f.close()
    print "Writing extractzimage"
    f = open('extractzimage', 'wb')
    f.write(data[hsize:hsize+ksize])
    f.close()
    print "Writing extractinitrd"
    f = open('extractinitrd', 'wb')
    f.write(data[hsize+ksize:hsize+ksize+isize])
    f.close()
    print "Writing extractlinload.txt"
    f = open('extractlinload.txt', 'wb')
    f.write(data[hsize+ksize+isize:hsize+ksize+isize+ssize])
    f.close()

if __name__ == '__main__':
    main()
