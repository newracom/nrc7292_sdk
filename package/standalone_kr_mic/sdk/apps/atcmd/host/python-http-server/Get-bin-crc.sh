#!/bin/sh
#

BIN_FILES=$(ls *.bin)

for bin in $BIN_FILES
do
	echo "$bin: $(./python/crc.py $bin)"
done
