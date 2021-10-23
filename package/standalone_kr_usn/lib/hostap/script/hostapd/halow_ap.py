#!/usr/bin/python

import os
import time
import commands

print "NRC AP setting for HaLow..."

print "[0] rmmod"
os.system("sudo killall -9 hostapd")
os.system("sudo rmmod nrc")
time.sleep(2)

print "[1] insmod"
os.system("sudo insmod ./nrc.ko power_save=1 fw_name=uni_s1g.bin")
time.sleep(5)

print "[2] hostpad"
os.system("sudo hostapd ./US/ap_halow_open.conf -ddd &")
time.sleep(3)

print "[3] set trx gain"
os.system('python /home/pi/nrc_pkg/python/shell.py run --cmd="phy rxgain 85"')
time.sleep(1)
os.system('python /home/pi/nrc_pkg/python/shell.py run --cmd="phy txgain 1"')
time.sleep(1)

print "[4] NAT"
os.system('sudo sh -c "echo 1 > /proc/sys/net/ipv4/ip_forward"')
os.system("sudo iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE")
os.system("sudo iptables -A FORWARD -i eth0 -o wlan0 -m state --state RELATED,ESTABLISHED -j ACCEPT")
os.system("sudo iptables -A FORWARD -i wlan0 -o eth0 -j ACCEPT")

print "[5] show version"
os.system('python /home/pi/nrc_pkg/python/shell.py run --cmd="show version"')

print "\n[6] ifconfig"
os.system('sudo ifconfig')

print "HaLow AP ready"
