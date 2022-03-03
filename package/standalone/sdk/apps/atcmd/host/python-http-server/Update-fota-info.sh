#!/bin/sh
#

FOTA_INFO_FILE="fota.json"

SDK_VER="10.10.10"
CMD_VER="10.10.10"

HSPI_BIN="nrc7292_standalone_xip_ATCMD_HSPI.bin"
UART_BIN="nrc7292_standalone_xip_ATCMD_UART.bin"
UART_HFC_BIN="nrc7292_standalone_xip_ATCMD_UART_HFC.bin"

#############################################################

for arg in $@
do
case $arg in
	-h|--help)
		echo "Usage: $0 [options]"
		echo "Options:"
		echo "  --sdk=<version>        Change the version of User SDK. (default: $SDK_VER)"
		echo "  --cmd=<version>        Change the version of AT Command Set. (default: $CMD_VER)"
		echo "  --hspi=<name>          Change the firmware binary for HSPI. (default: $HSPI_BIN)"
		echo "  --uart=<name>          Change the firmware binary for UART. (default: $UART_BIN)"
		echo "  --uart-hfc=<name>      Change the firmware binary for UART-HFC. (default: $UART_HFC_BIN)"
		echo "                         *The UART-HFC supports hardware flow control using RTS and CTS."
		echo "  -h, --help             Print this messages"
		exit 0
		;;

	--sdk=*.*.*)
		SDK_VER=${arg:6}
		;;

	--cmd=*.*.*)
		CMD_VER=${arg:6}
		;;

	--hspi=*.bin)
		HSPI_BIN=${arg:7}
		;;

	--uart=*.bin)
		SDK_VER=${arg:7}
		;;

	--uart-hfc=*.bin)
		SDK_VER=${arg:11}
		;;
esac
done

for file in $HSPI_BIN $UART_BIN $UART_HFC_BIN
do
	if [ ! -f $file ]; then
		echo "No such file: $file"
		exit 1
	fi
done

HSPI_CRC=$(python/crc.py $HSPI_BIN)
UART_CRC=$(python/crc.py $UART_BIN)
UART_HFC_CRC=$(python/crc.py $UART_HFC_BIN)

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
