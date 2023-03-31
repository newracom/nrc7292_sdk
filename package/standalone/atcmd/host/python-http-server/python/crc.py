#!/usr/bin/python
#

import sys
import zlib

def unsigned32(n):
    return n & 0xFFFFFFFF

def getCRC32(filename):
    try:
        f = open(filename, 'rb')
        crc = zlib.crc32(f.read())
        return unsigned32(crc)
    except IOError:
        print("Error: Cannot find such file.")
        exit(1)

if len(sys.argv) == 1:
    print("Error: There's no file name")
    print("Usage:")
    print("    $python crc.py <file name>")
    exit(1)

print("%x" %  getCRC32(sys.argv[1]))

