#!/bin/bash
#

case $1 in
	http)
#		python -m SimpleHTTPServer 8080
		python3 -m http.server 8080
		;;

	https)
		python3 python/https-server.py
		;;

	*)
		echo "Usage: $0 <http|https>"
esac

