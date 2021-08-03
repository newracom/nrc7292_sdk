#!/bin/sh
#

SDK_TOP=$(realpath ../../../../../)

PYTHON_HTTP_SERVER_ROOT=$SDK_TOP/sdk/apps/atcmd/host/python-http-server

OUT_ATCMD_HSPI_BIN=$SDK_TOP/out/nrc7292/standalone_xip/ATCMD_HSPI/nrc7292_standalone_xip_ATCMD_HSPI.bin
OUT_ATCMD_UART_BIN=$SDK_TOP/out/nrc7292/standalone_xip/ATCMD_UART/nrc7292_standalone_xip_ATCMD_UART.bin
OUT_ATCMD_UART_HFC_BIN=$SDK_TOP/out/nrc7292/standalone_xip/ATCMD_UART_HFC/nrc7292_standalone_xip_ATCMD_UART_HFC.bin


case $1 in
	clean)
		rm -vf fota.info
		rm -vf *.bin
		exit 0
		;;

	-h|--help)
		echo "Usage: $0 [clean]"
		exit 0
esac

cd $PYTHON_HTTP_SERVER_ROOT

for file in $(ls)
do
case $file in
	fota.info|*.bin)
		echo "Delete fota.info and binaries : $0 clean"
		exit 1
		;;
esac
done

cd $SDK_TOP

echo 

if [ -f .build-target ]; then
mv -v .build-target .build-target~
fi

echo

make select target=nrc7292.sdk.release APP_NAME=ATCMD_HSPI
make clean
make

echo

make select target=nrc7292.sdk.release APP_NAME=ATCMD_UART
make clean
make

echo

make select target=nrc7292.sdk.release APP_NAME=ATCMD_UART_HFC
make clean
make

echo

cp -vb $OUT_ATCMD_HSPI_BIN $PYTHON_HTTP_SERVER_ROOT
cp -vb $OUT_ATCMD_UART_BIN $PYTHON_HTTP_SERVER_ROOT
cp -vb $OUT_ATCMD_UART_HFC_BIN $PYTHON_HTTP_SERVER_ROOT

echo

if [ -f .build-target~ ]; then
mv -v .build-target~ .build-target
fi

echo

cd $PYTHON_HTTP_SERVER_ROOT
chmod -x *.bin
./Update-fota-info.sh

exit 0

