/*
 * MIT License
 *
 * Copyright (c) 2023 Newracom, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

enum cmd_tags
{
	TLV_CMD_GET_SSID = 1,
	TLV_CMD_SET_SSID,
	TLV_CMD_GET_SECURITY,
	TLV_CMD_SET_SECURITY,
	TLV_CMD_GET_PASSWORD,
	TLV_CMD_SET_PASSWORD,
	TLV_CMD_GET_COUNTRY,
	TLV_CMD_SET_COUNTRY,
	TLV_CMD_GET_IPMODE,
	TLV_CMD_SET_IPMODE,
	TLV_CMD_GET_CONN_TIMEOUT,
	TLV_CMD_SET_CONN_TIMEOUT,
	TLV_CMD_GET_DISCONN_TIMEOUT,
	TLV_CMD_SET_DISCONN_TIMEOUT,
	TLV_CMD_GET_RATE_CONTROL,
	TLV_CMD_SET_RATE_CONTROL,
	TLV_CMD_GET_MCS,
	TLV_CMD_SET_MCS,
	TLV_CMD_GET_CCA_THRES,
	TLV_CMD_SET_CCA_THRES,

	TLV_CMD_GET_STATICIP,
	TLV_CMD_SET_STATICIP,
	TLV_CMD_GET_NETMASK,
	TLV_CMD_SET_NETMASK,
	TLV_CMD_GET_GATEWAY,
	TLV_CMD_SET_GATEWAY,

	TLV_CMD_SHOW_CONFIG,
	TLV_CMD_SHOW_SYSCONFIG,
	TLV_CMD_GET_APINFO,

	TLV_CMD_GET_TXPOWER,
	TLV_CMD_SET_TXPOWER,

	TLV_CMD_GET_SCAN_MAX_INTERVAL,
	TLV_CMD_SET_SCAN_MAX_INTERVAL,

	TLV_CMD_GET_BACKOFF_START_COUNT,
	TLV_CMD_SET_BACKOFF_START_COUNT,

	TLV_CMD_GET_APPLICATION_VERSION,
	TLV_CMD_GET_SDK_VERSION,
	TLV_CMD_GET_MACADDR,
	TLV_CMD_GET_RSSI,
	TLV_CMD_GET_SNR,
	TLV_CMD_FW_UPGRADE,
	TLV_CMD_REBOOT,
	TLV_CMD_NVS_RESET,
	TLV_CMD_MAX
};

char *tlv_encode(int tag, unsigned int length, char *value);
void tlv_decode(char *buffer, unsigned int *tag, unsigned int *length, char *value);
