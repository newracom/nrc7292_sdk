#!/bin/sh
#

CHIP_NAME=nrc7292

SDK_VER="10.10.10"
CMD_VER="10.10.10"

FOTA_INFO_FILE="fota.json"

#############################################################

case $1 in
	7292|7394)
		CHIP_NAME=nrc$1
		;;

	clean)
		rm -vf $FOTA_INFO_FILE
		rm -vf *.bin
		exit 0
		;;

	*)
		echo "Usage: $0 {7292|7394|clean}"
		exit 0
		;;
esac

if [ -z $CHIP_NAME ]; then
	echo "no chip name"
	exit 1
fi

HSPI_BIN="$CHIP_NAME""_standalone_xip_ATCMD_HSPI.bin"
UART_BIN="$CHIP_NAME""_standalone_xip_ATCMD_UART.bin"
UART_HFC_BIN="$CHIP_NAME""_standalone_xip_ATCMD_UART_HFC.bin"

if [ -f $HSPI_BIN ]; then
	HSPI_CRC=$(python/crc.py $HSPI_BIN)
else
	echo "No such file: $HSPI_BIN"
fi

if [ -f $UART_BIN ]; then
	UART_CRC=$(python/crc.py $UART_BIN)
else
	echo "No such file: $UART_BIN"
fi

if [ -f $UART_HFC_BIN ]; then
	UART_HFC_CRC=$(python/crc.py $UART_HFC_BIN)
else
	echo "No such file: $UART_HFC_BIN"
fi

echo
echo "[ Before ]"
cat $FOTA_INFO_FILE

rm -f $FOTA_INFO_FILE

echo "{" >> $FOTA_INFO_FILE
echo "    \"AT_SDK_VER\" : \"$SDK_VER\"," >> $FOTA_INFO_FILE
echo "    \"AT_CMD_VER\" : \"$CMD_VER\"," >> $FOTA_INFO_FILE
echo "" >> $FOTA_INFO_FILE
echo "    \"AT_HSPI_BIN\" : \"$HSPI_BIN\"," >> $FOTA_INFO_FILE
echo "    \"AT_HSPI_CRC\" : \"$HSPI_CRC\"," >> $FOTA_INFO_FILE
echo "" >> $FOTA_INFO_FILE
echo "    \"AT_UART_BIN\" : \"$UART_BIN\"," >> $FOTA_INFO_FILE
echo "    \"AT_UART_CRC\" : \"$UART_CRC\"," >> $FOTA_INFO_FILE
echo "" >> $FOTA_INFO_FILE
echo "    \"AT_UART_HFC_BIN\" : \"$UART_HFC_BIN\"," >> $FOTA_INFO_FILE
echo "    \"AT_UART_HFC_CRC\" : \"$UART_HFC_CRC\"" >> $FOTA_INFO_FILE
echo "}" >> $FOTA_INFO_FILE

echo
echo "[ After ]"
cat $FOTA_INFO_FILE

echo
exit 0
