import serial, sys
import time

## CONFIGURATION ##
PORT             = "/dev/ttyAMA0"
DEBUG            = True


############# DEFINE ARGV ################
ssid = "halow_testing"
cntry = "US"
count = "10"
interval = "500"
tx_pw = "17"
remote_ip = "192.168.200.1"
##dhcp enable/disable
dhcp_enable = "0"  #if "1"(enable) , static_ip ""
static_ip ="192.168.200.12"
##open /wpa2
sec_mode = "1" #if "0"(open), passwd ""
passwd = "12345678"
##########################################


## FUNCTIONS ##

def usage_print():
    print("Usage: \n\tat_test_tool.py [sample_type] [sample_case]")
    print("Argument: ")
    print("\t-sample_type")
    print("\n\t\t[0:SAMPLE_APP_TEST   |   1:SAMPLE_APP_IOT_TEST   |   2:SAMPLE_TEST]")
    print("\n\t-sample_case")
    print("\n\t\t ---------------------------------")
    print("\n\t\t 0: Only for SAMPLE_APP_TEST")
    print("\n\t\t ---------------------------------")
    print("\n\t\t\t >softap [0:softap]")
    print("\n\t\t\t >fota [1:fota]")
    print("\n\t\t\t >server [2:tcp_server   |   3:udp_server]")
    print("\n\t\t\t >perry [4:uart   |   5:i2c   |   6:spi   |   7:adc   |   8.pwm   |   9.memory   |   10.gpio   |   11.timer]")
    print("\n\t\t\t >datatype [12:json   |   13:xml]")
    print("\n\t\t ---------------------------------")
    print("\n\t\t 1: Only for SAMPLE_APP_IOT_TEST")
    print("\n\t\t ---------------------------------")
    print("\n\t\t\t >server [0:coap_server]")
    print("\n\t\t ---------------------------------")
    print("\n\t\t 2: Only for SAMPLE_TEST")
    print("\n\t\t ---------------------------------")
    print("\n\t\t\t >aging [0:aging_tcp_client]")
    print("\n\t\t\t >iperf -c [1:iperf_udp_client]")
    print("\n\t\t\t >iperf -s [2:iperf_udp_server]")
    print("\n\t\t\t >socket [3:socket_client]")


    exit()

def strSAMPLE():
    if int(sys.argv[1]) == 0:
        return 'APP_TEST'
    elif int(sys.argv[1]) == 1:
        return 'IOT_TEST'
    elif int(sys.argv[1]) == 2:
        return 'TEST'
    else:
        usage_print()


def run_app():
    if int(sys.argv[2]) == 0: #softap
         test_result = ["AT+TSOFTAP={},{},{},{},{},1,192.168.200.1,{}".format(ssid,cntry,sec_mode,passwd,tx_pw,count)]
    elif int(sys.argv[2]) == 1: #fota
         test_result = ["AT+TFOTA={},{},{},{},{},{},{}".format(ssid,cntry,sec_mode,passwd,tx_pw,dhcp_enable,static_ip)]
    elif int(sys.argv[2]) == 2: #tcp_server
         test_result = ["AT+TTCP_SERVER={},{},{},{},{},{},{}".format(ssid,cntry,sec_mode,passwd,tx_pw,dhcp_enable,static_ip)]
    elif int(sys.argv[2]) == 3: #udp_server
         test_result = ["AT+TUDP_SERVER={},{},{},{},{},{},{}".format(ssid,cntry,sec_mode,passwd,tx_pw,dhcp_enable,static_ip)]
    elif int(sys.argv[2]) == 4: #uart
         test_result = ["AT+TUART={},{},{},{},{},{},{},{}".format(ssid,cntry,sec_mode,passwd,tx_pw,dhcp_enable,static_ip,remote_ip)]
    elif int(sys.argv[2]) == 5: #i2c
         test_result = ["AT+TI2C={},{}".format(count,interval)]
    elif int(sys.argv[2]) == 6: #spi
         test_result = ["AT+TSPI={},{}".format(count,interval)]
    elif int(sys.argv[2]) == 7: #adc
         test_result = ["AT+TADC={},{}".format(count,interval)]
    elif int(sys.argv[2]) == 8: #pwm
         test_result = ["AT+TPWM={},{}".format(count,interval)]
    elif int(sys.argv[2]) == 9: #memory
         test_result = ["AT+TMEMORY={},{}".format(count,interval)]
    elif int(sys.argv[2]) == 10: #gpio
         test_result = ["AT+TGPIO={},{}".format(count,interval)]
    elif int(sys.argv[2]) == 11: #timer
         test_result = ["AT+TTIMER"]
    elif int(sys.argv[2]) == 12: #json
         test_result = ["AT+TJSON"]
    elif int(sys.argv[2]) == 13: #xml
         test_result = ["AT+TXML"]
    else:
         usage_print()

    return test_result

def run_iot():
     if int(sys.argv[2]) == 0: #coap_server
         test_result = ["AT+TCOAP_SERVER={},{},{},{},{},{},{}".format(ssid,cntry,sec_mode,passwd,tx_pw,dhcp_enable,static_ip)]
     else:
        usage_print()

     return test_result

def run_sample():
     if int(sys.argv[2]) == 0: #aging_tcp_client
         test_result = ["AT+TAGING_TC={},{},{},{},{},{},{},{}".format(ssid,cntry,sec_mode,passwd,tx_pw,dhcp_enable,static_ip,remote_ip)]
     elif int(sys.argv[2]) == 1: #iperf_udp_client
         test_result = ["AT+TIPERF_UC={},{},{},{},{},{},{},{},{}".format(ssid,cntry,sec_mode,passwd,tx_pw,dhcp_enable,static_ip,remote_ip,count)]
     elif int(sys.argv[2]) == 2: #iperf_udp_server
         test_result = ["AT+TIPERF_US={},{},{},{},{},{},{}".format(ssid,cntry,sec_mode,passwd,tx_pw,dhcp_enable,static_ip)]
     elif int(sys.argv[2]) == 3: #cnc_tcp_client
         test_result = ["AT+TSOCKET={},{},{},{},{},{},{},{}".format(ssid,cntry,sec_mode,passwd,tx_pw,dhcp_enable,static_ip,remote_ip)]
     else:
         usage_print()

     return test_result

def isNumber(s):
    try:
        float(s)
        return True
    except ValueError:
        return False


def print_to_stdout(line):
    sys.stdout.write(line)
    sys.stdout.flush()

def connect_to_nrc7292_via_serial(PORT):
    return serial.Serial(port=PORT, baudrate=38400, timeout=0.5)

def write_at_cmd_to_nrc7292(nrc7292, at_cmd):
    at_cmd_with_CRLF = at_cmd + "\r\n"
    if DEBUG:
        print_to_stdout(at_cmd_with_CRLF)
    nrc7292.write(at_cmd_with_CRLF.encode())

def read_response_from_nrc7292(nrc7292):
    response = nrc7292.read().decode()
    if DEBUG:
        print_to_stdout(response)
    return response

# The following function returns 'True' if "OK" is returned. The function returns 'False' if "ERROR" is returned.
OK    = "OK\r\n"
ERROR = "ERROR\r\n"
def write_and_block_until_OK_or_ERROR(nrc7292, at_cmd):
    write_at_cmd_to_nrc7292(nrc7292, at_cmd)
    collected_response = ""
    while not (OK in collected_response or ERROR in collected_response):
        collected_response += read_response_from_nrc7292(nrc7292)
    return not ERROR in collected_response

## OPERATIONS ##

nrc7292 = connect_to_nrc7292_via_serial(PORT)

if len(sys.argv) < 2 or len(sys.argv) > 3 :
    usage_print()

if strSAMPLE() == 'APP_TEST':
    if len(sys.argv) == 3:
       at_cmd_sequence = run_app()
    elif len(sys.argv) == 2:
       at_cmd_sequence = ["AT+TWIFI_STATE={},{},{},{},{},{},{},{},{}".format(ssid,cntry,sec_mode,passwd,tx_pw,dhcp_enable,static_ip,count,interval),\
                          "AT+TTCP_CLIENT={},{},{},{},{},{},{},{},{}".format(ssid,cntry,sec_mode,passwd,tx_pw,dhcp_enable,static_ip,remote_ip,count),\
                          "AT+TUDP_CLIENT={},{},{},{},{},{},{},{},{}".format(ssid,cntry,sec_mode,passwd,tx_pw,dhcp_enable,static_ip,remote_ip,count),\
                          "AT+THTTP={},{},{},{},{},{},{}".format(ssid,cntry,sec_mode,passwd,tx_pw,dhcp_enable,static_ip)]
    else:
       usage_print()

elif strSAMPLE() == 'IOT_TEST':
     if len(sys.argv) == 3:
        at_cmd_sequence = run_iot()
     elif len(sys.argv) == 2:
        at_cmd_sequence = ["AT+TAWS={},{},{},{},{},{},{},{},{}".format(ssid,cntry,sec_mode,passwd,tx_pw,dhcp_enable,static_ip,count,interval),\
                            "AT+TONEM2M={},{},{},{},{},{},{},{},{}".format(ssid,cntry,sec_mode,passwd,tx_pw,dhcp_enable,static_ip,count,interval),\
                            "AT+TMQTT={},{},{},{},{},{},{},{},{}".format(ssid,cntry,sec_mode,passwd,tx_pw,dhcp_enable,static_ip,count,interval),\
                            "AT+TCOAP_CLIENT={},{},{},{},{},{},{},{},{},{}".format(ssid,cntry,sec_mode,passwd,tx_pw,dhcp_enable,static_ip,remote_ip,count,interval)]
     else:
        usage_print()

elif strSAMPLE() =='TEST':
      if len(sys.argv) == 3:
        at_cmd_sequence = run_sample()
      else:
        usage_print()

else:
    usage_print()

for at_cmd in at_cmd_sequence:
    ok_or_error = write_and_block_until_OK_or_ERROR(nrc7292, at_cmd)
    if DEBUG:
        print("RESPONSE OK: " + str(ok_or_error))
    if not ok_or_error:
		break
    time.sleep(1)


