#!/bin/sh
#

TARGET=nrc7292.standalone+ucode

for arg in $@
do
case $arg in
	uart|uart_hfc|hspi)
		HIF_TYPE=${1^^}
		;;

	19200|38400|57600|115200|230400|380400|460800|500000|576000|921600|1000000|1152000|1500000|2000000)
		UART_BAUDRATE="ATCMD_UART_BAUDRATE=$2"
		;;

	*)
		echo "Usage: $0 {hspi|uart|uart_hfc} [baudrate]"
		exit 0
esac
done

rm -f .build-target
make select target=$TARGET APP_NAME=ATCMD_$HIF_TYPE $UART_BAUDRATE
cat .build-target
make clean
