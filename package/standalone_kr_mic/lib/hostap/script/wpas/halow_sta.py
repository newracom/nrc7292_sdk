#!/usr/bin/python

import os
import time
import commands

print "NRC STA setting for HaLow ..."

print "[0] rmmod"
os.system("sudo killall -9 wpa_supplicant")
os.system("sudo rmmod nrc")

print "[1] insmod"
os.system("sudo insmod ./nrc.ko power_save=1 fw_name=uni_s1g.bin")
time.sleep(5)

print "[2] set trx gain"
os.system('python /home/pi/nrc_pkg/python/shell.py run --cmd="phy rxgain 85"')
time.sleep(1)
os.system('python /home/pi/nrc_pkg/python/shell.py run --cmd="phy txgain 1"')
time.sleep(1)

print "[3] wpa_supplicant"
os.system("sudo wpa_supplicant -iwlan0 -c ./US/sta_halow_open.conf -dddd &")
time.sleep(3)

print "[4] show version"
os.system('python /home/pi/nrc_pkg/python/shell.py run --cmd="show version"')

print "HaLow STA ready"

time.sleep(1)
