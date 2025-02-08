#!/bin/bash
#

fota_info_file="fota.json"

case $1 in
	clean)
		rm -vf $fota_info_file
		rm -vf *.bin
		exit 0
		;;

	-h|--help)
		echo "Usage: $0 [clean]"
		exit 0
		;;
esac

echo

PS3='Please select chip name : '
options=("nrc7292" "nrc7394" "Quit")
select chip_name in "${options[@]}"; 
do
    case $REPLY in
        [1-2])
			break 
			;;

        3) 
			exit 0
			;;
    esac
done

echo

if [ $chip_name = "nrc7292" ]; then
	flash_size="2M"
else
	PS3='Please select flash size : '
	options=("2M" "4M" "Quit")
	select flash_size in "${options[@]}"; 
	do
		case $REPLY in
			[1-2])
				break 
				;;

			3) 
				exit 0
				;;
		esac
	done
fi

echo

at_sdk_ver="10.10.10"
read -e -r -p "(Optional) Please enter SDK version ($at_sdk_ver) : " sdk_ver
if [ ! -z $sdk_ver ]; then
	at_sdk_ver=$sdk_ver
fi

echo

at_cmd_ver="10.10.10"
read -e -r -p "(Optional) Please enter ATCMD version ($at_cmd_ver) : " cmd_ver
if [ ! -z $cmd_ver ]; then
	at_cmd_ver=$cmd_ver
fi

echo

at_hspi_bin="$chip_name""_standalone_xip_ATCMD_HSPI_""$flash_size"".bin"
read -e -r -p "(Optional) Please enter ATCMD_HSPI binary name ($at_hspi_bin) : " hspi_bin
if [ ! -z $hspi_bin ]; then
	at_hspi_bin=$hspi_bin
fi

echo

at_uart_bin="$chip_name""_standalone_xip_ATCMD_UART_""$flash_size"".bin"
read -e -r -p "(Optional) Please enter ATCMD_UART binary name ($at_uart_bin) : " uart_bin
if [ ! -z $uart_bin ]; then
	at_uart_bin=$uart_bin
fi

echo

at_uart_hfc_bin="$chip_name""_standalone_xip_ATCMD_UART_HFC_""$flash_size"".bin"
read -e -r -p "(Optional) Please enter ATCMD_UART_HSPI binary name ($at_uart_hfc_bin) : " uart_hfc_bin
if [ ! -z $uart_hfc_bin ]; then
	at_uart_hfc_bin=$uart_hfc_bin
fi

echo

echo "============================================================================"
echo " - AT_SDK_VER      : $at_sdk_ver"
echo " - AT_CMD_VER      : $at_cmd_ver"
echo " - AT_HSPI_BIN     : $at_hspi_bin"
echo " - AT_UART_BIN     : $at_uart_bin"
echo " - AT_UART_HFC_BIN : $at_uart_hfc_bin"
echo "============================================================================"

read -p "Please enter any key to continue ..."

if [ -f $at_hspi_bin ]; then
	at_hspi_crc=$(python/crc.py $at_hspi_bin)
else
	echo "No such file: $at_hspi_bin"
fi

if [ -f $at_uart_bin ]; then
	at_uart_crc=$(python/crc.py $at_uart_bin)
else
	echo "No such file: $at_uart_bin"
fi

if [ -f $at_uart_hfc_bin ]; then
	at_uart_hfc_crc=$(python/crc.py $at_uart_hfc_bin)
else
	echo "No such file: $at_uart_hfc_bin"
fi

echo
echo "[ Before ]"
cat $fota_info_file

rm -f $fota_info_file

echo "{" >> $fota_info_file
echo "    \"AT_SDK_VER\" : \"$at_sdk_ver\"," >> $fota_info_file
echo "    \"AT_CMD_VER\" : \"$at_cmd_ver\"," >> $fota_info_file
echo "" >> $fota_info_file
echo "    \"AT_HSPI_BIN\" : \"$at_hspi_bin\"," >> $fota_info_file
echo "    \"AT_HSPI_CRC\" : \"$at_hspi_crc\"," >> $fota_info_file
echo "" >> $fota_info_file
echo "    \"AT_UART_BIN\" : \"$at_uart_bin\"," >> $fota_info_file
echo "    \"AT_UART_CRC\" : \"$at_uart_crc\"," >> $fota_info_file
echo "" >> $fota_info_file
echo "    \"AT_UART_HFC_BIN\" : \"$at_uart_hfc_bin\"," >> $fota_info_file
echo "    \"AT_UART_HFC_CRC\" : \"$at_uart_hfc_crc\"" >> $fota_info_file
echo "}" >> $fota_info_file

echo
echo "[ After ]"
cat $fota_info_file

echo
exit 0
