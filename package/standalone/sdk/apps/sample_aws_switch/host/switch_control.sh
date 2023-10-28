#!/bin/bash
# Initialize variables with default values
green="undefined"
red="undefined"

# Process command-line arguments
while [[ $# -gt 0 ]]; do
  case "$1" in
    "green")
      shift
      if [ "$1" == "on" ] || [ "$1" == "off" ]; then
        green="$1"
        shift
      else
        echo "Invalid argument for green: $1"
        exit 1
      fi
      ;;
    "red")
      shift
      if [ "$1" == "on" ] || [ "$1" == "off" ]; then
        red="$1"
        shift
      else
        echo "Invalid argument for red: $1"
        exit 1
      fi
      ;;
    "off")
      green="off"
      red="off"
      shift
      ;;
    "on")
      green="on"
      red="on"
      shift
      ;;
    *)
      echo "Invalid argument: $1"
      exit 1
      ;;
  esac
done

# Prepare JSON output based on input

message=""

if [ "$green" == "on" ]; then
   message="{\"green\":\"on\""
elif [ "$green" == "off" ]; then
  message="{\"green\":\"off\""
fi

if [ "$red" == "on" ]; then
	if [ -n "$message" ]; then
		message="${message}, "
	else
		message="{"
	fi
	message="${message}\"red\":\"on\""
elif [ "$red" == "off" ]; then
	if [ -n "$message" ]; then
		message="${message}, "
	else
		message="{"
	fi
	message="${message}\"red\":\"off\""
fi

message="${message}}"

if [ -z "$message" ]; then
	echo "Usage: specify control commands"
	echo "i.e.,"
	echo "$0 red on/off"
	echo "$0 green on/off"
	echo "$0 green on/off red on/off"
	echo "$0 on : turn both on"
	echo "$0 off : turn botth off"
	exit 1;
else
	echo "Sending \"$message\""
	mosquitto_pub --cert aws-client.pem --key aws-client.key --cafile aws-ca.pem -h a15l156ratjegc-ats.iot.ap-northeast-2.amazonaws.com -p 8883 -t 'nrc_switch/control' -m "$message"	
fi


