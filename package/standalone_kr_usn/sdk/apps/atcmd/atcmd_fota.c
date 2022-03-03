/*
 * MIT License
 *
 * Copyright (c) 2020 Newracom, Inc.
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

#include "cJSON.h"

#include "atcmd.h"
#include "atcmd_fota.h"

#define _atcmd_fota_debug(fmt, ...)		//_atcmd_debug("FOTA: " fmt "\n",##__VA_ARGS__)
#define _atcmd_fota_log(fmt, ...)		_atcmd_info("FOTA: " fmt "\n", ##__VA_ARGS__)

/**********************************************************************************************/

static const char g_https_ssl_server_ca_crt[] =
"-----BEGIN CERTIFICATE-----\r\n"
"MIIDBjCCAe4CCQCWOp1NpvxjzDANBgkqhkiG9w0BAQsFADBFMQswCQYDVQQGEwJB\r\n"
"VTETMBEGA1UECAwKU29tZS1TdGF0ZTEhMB8GA1UECgwYSW50ZXJuZXQgV2lkZ2l0\r\n"
"cyBQdHkgTHRkMB4XDTE5MTEwMTA3MDI0MFoXDTIwMTAzMTA3MDI0MFowRTELMAkG\r\n"
"A1UEBhMCQVUxEzARBgNVBAgMClNvbWUtU3RhdGUxITAfBgNVBAoMGEludGVybmV0\r\n"
"IFdpZGdpdHMgUHR5IEx0ZDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB\r\n"
"AK3+RdJg0gdmjwXlGU8mpmjZFqs6etKPoqQifP+U6jbJ75Po7qwazdUsaW2aSyBo\r\n"
"zzA3kHYGwPTKL8m4ptcGmkcEDN2oLolcdtxpyBTA3Zg2qgaArdfwdWamN2NZp+cD\r\n"
"zBk0Hh6ygSxq8h16oq84eqN642ox+Kzh+WjhBFI58qGp+blffdZ89aMDwaFUBI9y\r\n"
"zW9/c5RoJQ4CjtFDvP8N4NXzRGdf5ydXPjbS0sFVPe/scP8u/KgynCTix8CRBGc5\r\n"
"hMow8Mxti2Yp4wl5fCKD3eEMuivEA16wexHrrOp5ZyUsT92flSjC96GfqY8XZw4H\r\n"
"KiVEi/O6ThaJ2GdpDe2adB8CAwEAATANBgkqhkiG9w0BAQsFAAOCAQEAKMHf6/Xb\r\n"
"XDKCippqiDI83xz3rb0EEt/VUkgEiVRHKBwztGT94djOPn5Yh5+Gdfhfp3WUItZ2\r\n"
"Djpii9GKHXXzIbW9qtjJHdhh9FDMbXxkjFBdEiVRkfTVKNy84N3DlVCUGcFC//lF\r\n"
"p7UC3vD6xrQtKyTQ8H4cqSrSkev2xmplfy82nZ/5O+6dew4FQ5xjr0FkyvVQqRrZ\r\n"
"NTr9ApyxCAxFkjM+dHs7Ei0aIOzHA3qXItlRtd8L9JD2jr2sGRi5KreyUyx9eZ/t\r\n"
"Ne0MdQ9sfCEfBQ07em5i4RBIfRuUjWj9qOJ0qbLVc01+aFe+Nj6OIV0lFblspRYw\r\n"
"Mjqoi7TUHGGfzA==\r\n"
"-----END CERTIFICATE-----\r\n";

static const char g_https_ssl_client_ca_crt[] =
"-----BEGIN CERTIFICATE-----\r\n"
"MIIDBjCCAe4CCQCSl2yS9eWWkzANBgkqhkiG9w0BAQsFADBFMQswCQYDVQQGEwJB\r\n"
"VTETMBEGA1UECAwKU29tZS1TdGF0ZTEhMB8GA1UECgwYSW50ZXJuZXQgV2lkZ2l0\r\n"
"cyBQdHkgTHRkMB4XDTE5MTEwMTExMDM1MloXDTIwMTAzMTExMDM1MlowRTELMAkG\r\n"
"A1UEBhMCQVUxEzARBgNVBAgMClNvbWUtU3RhdGUxITAfBgNVBAoMGEludGVybmV0\r\n"
"IFdpZGdpdHMgUHR5IEx0ZDCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB\r\n"
"AOCI9MV2dlppCfC135BqEpJzd1UPxbrE10pbGOHgALfbs0afd23sd/b0b04G7iGU\r\n"
"O2VZ5uSSCq5iNxS+97JeqUbIJP9ELIz9xEO3H5cdz8TRSNFoKeWlULA6ir53MGb+\r\n"
"8EnEGVCFF0AlvfZ/jkJ0XS/fL6ozW2wkBi05kJ84rXnIwwBBLNCLZ6NPtSZ9DltB\r\n"
"o/3eXpEpiNh26+aIldodKpekA5dUm7xThrh7YqE7kb7ZLeifuYtHQs+BUgF3hPtJ\r\n"
"Hg1ClftVBHg6N70lvvX5Z/bMmaOwFLJJvMEaGxt26xJ3YckqYioah1PBmuhhozXS\r\n"
"cWA51f8BatFbqquBeFP8StMCAwEAATANBgkqhkiG9w0BAQsFAAOCAQEAdwZ25Jfh\r\n"
"Al6fODpb61/ZnnpWdrsx9K14AgnmQ+zG4u9+Blj1KwSUZ82tsSL5+eZw1dqPd0D+\r\n"
"piP3mdnqXoxC6iJo0KhNtBk7TWAZ7NfwsjkUyZyY1UfZygEe2zPdQsNCz1leoxw0\r\n"
"buPoUPHmbhhB4/lXSVadP6oqOM1L0OEOOCakooTAVxW8y/0/ro95ijGGVoTKNTwA\r\n"
"3P6flw/+DhoDjTFcZXoIliucbWnZV7LwPdk8BzJPs3ClWf2UV+4prgdRFlCENqlO\r\n"
"0VLOlVyjuGtqKS+j/ETamGwZGVP/lJ9cI5omHmn4BHypd7xQSHrpBsWjInmxIfsG\r\n"
"vepVtWNFpNrEBg==\r\n"
"-----END CERTIFICATE-----\r\n";

static const char g_https_ssl_client_key[] =
"-----BEGIN RSA PRIVATE KEY-----\r\n"
"MIIEowIBAAKCAQEA4Ij0xXZ2WmkJ8LXfkGoSknN3VQ/FusTXSlsY4eAAt9uzRp93\r\n"
"bex39vRvTgbuIZQ7ZVnm5JIKrmI3FL73sl6pRsgk/0QsjP3EQ7cflx3PxNFI0Wgp\r\n"
"5aVQsDqKvncwZv7wScQZUIUXQCW99n+OQnRdL98vqjNbbCQGLTmQnzitecjDAEEs\r\n"
"0Itno0+1Jn0OW0Gj/d5ekSmI2Hbr5oiV2h0ql6QDl1SbvFOGuHtioTuRvtkt6J+5\r\n"
"i0dCz4FSAXeE+0keDUKV+1UEeDo3vSW+9fln9syZo7AUskm8wRobG3brEndhySpi\r\n"
"KhqHU8Ga6GGjNdJxYDnV/wFq0Vuqq4F4U/xK0wIDAQABAoIBAGseq7fw9jHX3tgp\r\n"
"zIi3MjkQQSQhrDGYayWcJFjOZ0lP1U2iEnYs1GbK4rcU81Ktx1Bo/ZCaY+IiFSke\r\n"
"mklMg/Gy1oO54I87GgE8QiP0IwVA2z6cNTDMF5ybsUmAz2Szx6tJlNInTJpb5y7M\r\n"
"V/A4V6TZE4JdkgYbgZ7d0bNEdO6eBg/z9BhqJ4Zl6VtEUXx4gMobOkdxrneqE3QY\r\n"
"wAMt9QkkTadsdQ1nPhouKbs3yqIhKWb7cd38D+dqGeYHBCDxoHxebUWkJNPE5h5Q\r\n"
"/OaVYlxhr1NMxP9a1p6A9xevpFYVJD7RK/GQluIdUrJqf6C6yClw2yP0x7LCGjq4\r\n"
"LPpYyYECgYEA+KbR5jGacSd3TZe/nIov+q/bwUaJaZEY5K4YyMZxJFJ1+ILUANY/\r\n"
"KLAPztDYq6wucriQBMYWzJ6iS3MRZFwfI6FAbebeX3TT8i9KPgHVhBr7QwOzVFtx\r\n"
"yTnUe5XrmRNl1QzZCEyriMZ7uCdbvkzBS16xajpsGetaYp1gm3C+rrMCgYEA5yuu\r\n"
"WrXya7ZX+PrSXdDKascTMyOm3qIZom1YPFVWo+jHFVEsUN2gmIPTHwMGDNnckOeb\r\n"
"00hf/BDbWMLCuvIkJzWRgvr83yW9zN9M+g4LSUImCRQOkMiOo0sUH3my2dFmt3W+\r\n"
"X6fp5/Ket4IU7Tgb1Z3DqcaU7QLPtFLzpvmQA2ECgYEAqzMYxBiVEKGut+Lqj9pp\r\n"
"TH42nS12wROhAxqHf/15uxt3lEJnu6fH1rjaOXh8Jj8nv98pcc/9tKbocXBpoiL3\r\n"
"Ya3N0Z2qsCidIVvED0tt+kYlh6+NkmBfyL+jd+/yRfQgIf91kwxO8p5OYq3esfjh\r\n"
"AYbSOqS891+fXNSkxoFrGJcCgYA/6zMNf+uk3slaXbgXGqktdxgW9s+oFXgzEjro\r\n"
"i8wmDDIn8cboIS/Lm/+fPo3IteCn7HKIrCVmJB8SXt/LIzLd6JDwf4e2B9CAOmol\r\n"
"Zga23eR4dCRG4j2WZycMQPE0CxN0vMjD2EDz0oESSpSQtwfzO+kjI3aARlu6B4m5\r\n"
"bJ3mYQKBgGxksP3SErSRNbnjCDk0ywio3BR2latOOcRoRJDx9tufMEx3ntQgdGKI\r\n"
"iBuGyHuy7FeJBK755T4xY3bDq59Q8gTcfrJjHldQPsU5J2jUHTB5vNWf+E3+nm6T\r\n"
"YjruhiS//W+Cn/bNrzTB0wEHck4OEFqXbBtqQZaWTUs3nCgAJRK4\r\n"
"-----END RSA PRIVATE KEY-----\r\n";

static ssl_certs_t g_https_ssl_certs =
{
	.server_cert = g_https_ssl_server_ca_crt,
	.client_cert = g_https_ssl_client_ca_crt,
	.client_pk = g_https_ssl_client_key,

	.server_cert_length = sizeof(g_https_ssl_server_ca_crt),
	.client_cert_length = sizeof(g_https_ssl_client_ca_crt),
	.client_pk_length = sizeof(g_https_ssl_client_key),
};

static atcmd_fota_t g_atcmd_fota =
{
	.enable = false,
	.update = false,

	.params =
	{
		.fw_bin_type = FW_BIN_HSPI,
		.event_cb = NULL,
		.check_time = -1,
		.server_url = { 0, },
		.bin_name = { 0, },
		.bin_crc32 = 0
	},

	.task = NULL,

	.recv_buf =
	{
		.addr = NULL,
		.len = 0
	}
};

/**********************************************************************************************/

extern void atcmd_dump_hex_print (const void *buf, size_t len, bool ascii);
extern void atcmd_dump_hex_print_terminal (const void *buf, size_t len, bool ascii);

typedef struct
{
	float Version;
	int StatusCode;
	int ContentLength;
	int ContentLengthDone;
} httpc_resp_t;

static void _atcmd_fota_event_callback (enum FOTA_EVENT type, ...)
{
	if (g_atcmd_fota.params.event_cb)
	{
		va_list ap;
		atcmd_fota_event_t event;

		event.type = type;

		va_start(ap, type);

		switch (type)
		{
			case FOTA_EVT_VERSION:
				event.version.sdk = va_arg(ap, char *);
				event.version.atcmd = va_arg(ap, char *);
				break;

			case FOTA_EVT_BINARY:
				event.binary.name = va_arg(ap, char *);
				break;

			case FOTA_EVT_DOWNLOAD:
				event.download.total = va_arg(ap, uint32_t);
				event.download.len = va_arg(ap, uint32_t);
				break;

			case FOTA_EVT_UPDATE:
				event.update.bin_name = va_arg(ap, char *);
				event.update.bin_size = va_arg(ap, uint32_t);
				event.update.bin_crc32 = va_arg(ap, uint32_t);
				break;

			case FOTA_EVT_FAIL:
				break;

			default:
				_atcmd_fota_log("!!! invalid event type, %d !!!", type);
				return;
		}

		va_end(ap);

		g_atcmd_fota.params.event_cb(&event);
	}
}

static const char *_atcmd_fota_httpc_strerror (int ret)
{
	const char *str[] =
	{
		[HTTPC_RET_OK] = "Success",
		[-(HTTPC_RET_CON_CLOSED)] = "Connection closed by remote",
		[-(HTTPC_RET_ERROR_UNKNOWN)] = "Unknown error",
		[-(HTTPC_RET_ERROR_CONNECTION)] = "Connection fail",
		[-(HTTPC_RET_ERROR_RESOLVING_DNS)] = "Cannot resolve the hostname",
		[-(HTTPC_RET_ERROR_SOCKET_FAIL)] = "Socket creation fail",
		[-(HTTPC_RET_ERROR_SCHEME_NOT_FOUND)] = "Scheme(http:// or https://) not foun",
		[-(HTTPC_RET_ERROR_ALLOC_FAIL)] = "Memory allocation fail",
		[-(HTTPC_RET_ERROR_INVALID_HANDLE)] = "Invalid handle",
		[-(HTTPC_RET_ERROR_HEADER_SEND_FAIL)] = "Request Header send fail",
		[-(HTTPC_RET_ERROR_BODY_SEND_FAIL)] = "Request body send fail",
		[-(HTTPC_RET_ERROR_SEED_FAIL)] = "Seed creation fail",
		[-(HTTPC_RET_ERROR_CERT_LOADING_FAIL)] = "Certificate loading fail",
		[-(HTTPC_RET_ERROR_PK_LOADING_FAIL)] = "Private key loading fail",
		[-(HTTPC_RET_ERROR_TLS_CONNECTION)] = "TLS connection fail",
	};

	switch (ret)
	{
		case HTTPC_RET_OK:
		case HTTPC_RET_CON_CLOSED:
		case HTTPC_RET_ERROR_UNKNOWN:
		case HTTPC_RET_ERROR_CONNECTION:
		case HTTPC_RET_ERROR_RESOLVING_DNS:
		case HTTPC_RET_ERROR_SOCKET_FAIL:
		case HTTPC_RET_ERROR_SCHEME_NOT_FOUND:
		case HTTPC_RET_ERROR_ALLOC_FAIL:
		case HTTPC_RET_ERROR_INVALID_HANDLE:
		case HTTPC_RET_ERROR_HEADER_SEND_FAIL:
		case HTTPC_RET_ERROR_BODY_SEND_FAIL:
		case HTTPC_RET_ERROR_SEED_FAIL:
		case HTTPC_RET_ERROR_CERT_LOADING_FAIL:
		case HTTPC_RET_ERROR_PK_LOADING_FAIL:
		case HTTPC_RET_ERROR_TLS_CONNECTION:
			return str[-ret];

		default:
			return "Invalid Return";
	}
}


static void _atcmd_fota_httpc_print_resp_info (httpc_resp_t *resp)
{
	_atcmd_fota_log("[ HTTP RESP INFO ]");
	_atcmd_fota_log(" - Version: %.1f", resp->Version);
	_atcmd_fota_log(" - StatusCode: %d", resp->StatusCode);
	_atcmd_fota_log(" - ContentLength: %d", resp->ContentLength);
}

static void _atcmd_fota_httpc_parse_resp_header (char *header, httpc_resp_t *resp)
{
	char *saveptr;
	char *token;

	token = strtok_r(header, "\r\n", &saveptr);

	while (token)
	{
//		_atcmd_fota_debug("http_resp_hdr: %s", token);

		if (strncmp(token, "HTTP/1.", 7) == 0)
			sscanf(token, "HTTP/%f %d %*s", &resp->Version, &resp->StatusCode);
		else if (strncmp(token, "Content-Length: ", 16) == 0)
			sscanf(token, "Content-Length: %d", &resp->ContentLength);

		token = strtok_r(NULL, "\r\n", &saveptr);
	}
}

static int _atcmd_fota_httpc_get (const char *server_url, const char *file_path,
									char *buf, int buf_len,
									void (*cb)(char *, int, int))
{
	static char url[ATCMD_FOTA_SERVER_URL_LEN_MAX + ATCMD_FOTA_BIN_NAME_LEN_MAX];
	ssl_certs_t *certs;
	con_handle_t handle;
	httpc_data_t data;
	httpc_resp_t resp;
	int url_len;
	char *header;
	char *header_end;
	int header_len;
	int ret;

	if (!server_url || !strlen(server_url))
	{
		_atcmd_fota_log("httpc_get: !!! invalid server_url !!!");
		return -1;
	}

	if (!file_path || !strlen(file_path))
	{
		_atcmd_fota_log("httpc_get: !!! invalid file_path !!!");
		return -1;
	}

	url_len = strlen(server_url) + strlen(file_path) + 2;
	if (url_len > sizeof(url))
	{
		_atcmd_fota_log("httpc_get: !!! URL length is greater than %d !!!", sizeof(url));
		return -1;
	}

	_atcmd_fota_debug("httpc_get: url_len=%d buf_len=%d", url_len, buf_len);

	memset(&handle, 0, sizeof(con_handle_t));
	memset(&data, 0, sizeof(httpc_data_t));
	memset(&resp, 0, sizeof(httpc_resp_t));

	data.data_in = buf;
	data.data_in_length = buf_len;

	if (server_url[strlen(server_url) - 1] == '/')
		sprintf(url, "%s%s", server_url, file_path);
	else
		sprintf(url, "%s/%s", server_url, file_path);

	if (memcmp(url, "https://", 8) == 0)
		certs = &g_https_ssl_certs;
	else
		certs = NULL;

	_atcmd_fota_log("httpc_get: %s", url);

	ret = nrc_httpc_get(&handle, url, NULL, &data, certs);
	if (ret != HTTPC_RET_OK)
	{
		_atcmd_fota_log("httpc_get: %s (%d)", _atcmd_fota_httpc_strerror(ret), ret);
		return -1;
	}

	memset(&resp, 0, sizeof(resp));

	for (ret = 1 ; ret > 0 ; )
	{
		if (resp.Version == 0)
		{
			header_end = strstr(buf, "\r\n\r\n");

			if (header_end)
			{
				header = buf;
				header_len = (header_end - header) + 4;

				_atcmd_fota_debug("httpc_get: header=%d", header_len);

				memset(header_end, 0, 4);

				_atcmd_fota_httpc_parse_resp_header(header, &resp);
				_atcmd_fota_httpc_print_resp_info(&resp);

				if (resp.StatusCode == 200)
				{
					data.data_in += data.recved_size;
					data.recved_size = data.data_in - (buf + header_len);
					data.data_in = buf + header_len;
					data.data_in_length = buf_len - header_len;
				}
				else
				{
					_atcmd_fota_log("httpc_get: !!! status_code != 200 (%d) !!!", resp.StatusCode);
					ret = -1;
				}

				continue;
			}
			else
			{
				data.data_in += data.recved_size;
				data.data_in_length -= data.recved_size;

				if (data.data_in >= (buf + buf_len))
				{
					_atcmd_fota_log("httpc_get: !!! buffer overflow (%d/%d) !!!",
										data.data_in - buf, buf_len);
					ret = -1;
					continue;
				}
			}
		}
		else if (resp.ContentLength > 0 && data.recved_size > 0)
		{
			if (cb)
				cb(data.data_in, data.recved_size, resp.ContentLength);

			resp.ContentLengthDone += data.recved_size;

			_atcmd_fota_debug("httpc_get: body=%d (%d/%d)",
						data.recved_size, resp.ContentLengthDone, resp.ContentLength);

			if (resp.ContentLengthDone == resp.ContentLength)
			{
				_atcmd_fota_log("httpc_get: done");
				ret = 0;
				continue;
			}

			data.data_in = buf;
			data.data_in_length = buf_len;
		}

		data.recved_size = 0;

		ret = nrc_httpc_recv_response(&handle, &data);
		if (ret == HTTPC_RET_OK)
			ret = 1;
		else
		{
			_atcmd_fota_log("httpc_get: %s (%d)", _atcmd_fota_httpc_strerror(ret), ret);

			if (ret == HTTPC_RET_CON_CLOSED && resp.ContentLengthDone == resp.ContentLength)
				ret = 0;
			else
			{
				_atcmd_fota_log("httpc_get: !!! content_length=%d/%d !!!",
									resp.ContentLengthDone, resp.ContentLength);
				ret = -1;
			}
		}
	}

	nrc_httpc_close(&handle);

	if (ret < 0 && cb)
		cb(NULL, 0, 0);

	return ret;
}

static int _atcmd_fota_info_parse (char *fota_info, atcmd_fota_info_t *info)
{
	cJSON *cjson = NULL;
	cJSON *obj = NULL;
	const char *keys[] =
	{
		"AT_SDK_VER", "AT_CMD_VER",
		"AT_HSPI_BIN", "AT_HSPI_CRC",
		"AT_UART_BIN", "AT_UART_CRC",
		"AT_UART_HFC_BIN", "AT_UART_HFC_CRC",

		NULL
	};
	char *val = NULL;
	int i;

	_atcmd_fota_log("[ INFO FILE ]");

	cjson = cJSON_Parse(fota_info);
	if (!cjson)
		return -1;

	for (i = 0 ; keys[i] ; i++)
	{
		obj = cJSON_GetObjectItem(cjson, keys[i]);
		if (!obj)
		{
			_atcmd_fota_log("%s: no key", keys[i]);
			continue;
		}

		if (!obj->valuestring)
		{
			_atcmd_fota_log("%s: no value", keys[i]);
			continue;
		}

		val = strdup(obj->valuestring);

		switch (i)
		{
			case 0: /* AT_SDK_VER */
			case 1: /* AT_CMD_VER */
			{
				fw_ver_t *ver = &info->fw_ver[FW_VER_SDK + i];

				sscanf(val, "%d.%d.%d", &ver->major, &ver->minor, &ver->revision);

				_atcmd_fota_log("%s: %d.%d.%d", keys[i], ver->major, ver->minor, ver->revision);
				break;
			}

			case 2: /* AT_HSPI_BIN */
			case 4: /* AT_UART_BIN */
			case 6: /* AT_UART_HFC_BIN */
			{
				char *name = info->fw_bin[FW_BIN_HSPI + (i / 2) - 1].name;

				sscanf(val, "%s", name);

				_atcmd_fota_log("%s: %s", keys[i], name);
				break;
			}

			case 3: /* AT_HSPI_CRC */
			case 5: /* AT_UART_CRC */
			case 7: /* AT_UART_HFC_CRC */
			{
				uint32_t *crc32 = &info->fw_bin[FW_BIN_HSPI + ((i + 1) / 2) - 2].crc32;

				sscanf(val, "%lx", crc32);

				_atcmd_fota_log("%s: %lx", keys[i], *crc32);
				break;
			}
		}

		free(val);
	}

	cJSON_Delete(cjson);

	return 0;
}

static void _atcmd_fota_fw_check_callback (char *data, int len, int total)
{
	static char *fota_info = NULL;
	static uint32_t cnt = 0;

	if (!data || !len || !total)
	{
		if (fota_info)
		{
			_atcmd_free(fota_info);
			fota_info = NULL;
		}

		cnt = 0;
		return;
	}

	if (!fota_info)
	{
		fota_info = _atcmd_malloc(total);
		if (!fota_info)
		{
			_atcmd_error("malloc() failed\n");
			return;
		}
	}

	memcpy(fota_info + cnt, data, len);
	cnt += len;

	if (cnt == total)
	{
		fota_info[cnt] = '\0';

		_atcmd_fota_info_parse(fota_info, &g_atcmd_fota.info);

		_atcmd_free(fota_info);

		fota_info = NULL;
		cnt = 0;
	}
}

static int _atcmd_fota_fw_check (const char *server_url, bool *new_fw)
{
	const fw_ver_t fw_ver[FW_VER_NUM] =
	{
		[FW_VER_SDK] =
		{
			.major = SDK_VER_MAJOR,
			.minor = SDK_VER_MINOR,
			.revision = SDK_VER_REVISION,
		},

		[FW_VER_ATCMD] =
		{
			.major = ATCMD_VER_MAJOR,
			.minor = ATCMD_VER_MINOR,
			.revision = ATCMD_VER_REVISION,
		}
	};
	const fw_ver_t *cur_ver, *new_ver;
	int i;

	_atcmd_fota_log("Checking firmware ...");

	if (_atcmd_fota_httpc_get(server_url, ATCMD_FOTA_INFO_FILE,
							g_atcmd_fota.recv_buf.addr, g_atcmd_fota.recv_buf.len,
							_atcmd_fota_fw_check_callback) != 0)
	{
		memset(&g_atcmd_fota.info, 0, sizeof(atcmd_fota_info_t));
		return -1;
	}

	for (i = 0 ; i < FW_VER_NUM ; i++)
	{
		cur_ver = fw_ver + i;
		new_ver = g_atcmd_fota.info.fw_ver + i;

		if (new_ver->major > cur_ver->major)
			break;
		else if (new_ver->major == cur_ver->major)
		{
			if (new_ver->minor > cur_ver->minor)
				break;
			else if (new_ver->minor == cur_ver->minor)
			{
				if (new_ver->revision > cur_ver->revision)
					break;
			}
		}
	}

	if (i < FW_VER_NUM)
	{
		char sdk_ver[12];
		char atcmd_ver[12];

		*new_fw = true;

		snprintf(sdk_ver, sizeof(sdk_ver), "%d.%d.%d",
						g_atcmd_fota.info.fw_ver[FW_VER_SDK].major,
						g_atcmd_fota.info.fw_ver[FW_VER_SDK].minor,
						g_atcmd_fota.info.fw_ver[FW_VER_SDK].revision);

		snprintf(atcmd_ver, sizeof(atcmd_ver), "%d.%d.%d",
						g_atcmd_fota.info.fw_ver[FW_VER_ATCMD].major,
						g_atcmd_fota.info.fw_ver[FW_VER_ATCMD].minor,
						g_atcmd_fota.info.fw_ver[FW_VER_ATCMD].revision);

		_atcmd_fota_log("version: sdk=%s atcmd=%s", sdk_ver, atcmd_ver);

		_atcmd_fota_event_callback(FOTA_EVT_VERSION, sdk_ver, atcmd_ver);
	}
	else
	{
		memset(&g_atcmd_fota.info, 0, sizeof(atcmd_fota_info_t));
		*new_fw = false;
	}

	return 0;
}

static void _atcmd_fota_fw_update_callback (char *data, int len, int total)
{
	static uint32_t cnt = 0;
	static uint32_t event = 0;

	if (!data || !len || !total)
	{
		cnt = 0;
		event = 0;
		return;
	}

	if (cnt == 0)
	{
		_atcmd_fota_debug("erase: %d", total);

		util_fota_erase(0, total);

		event = total / 10;
	}

	util_fota_write(cnt, (uint8_t *)data, len);

	cnt += len;

	if (cnt >= event)
	{
		_atcmd_fota_log("download: %d/%d", cnt, total);
		_atcmd_fota_event_callback(FOTA_EVT_DOWNLOAD, total, cnt);

		event += total / 10;
	}

	if (cnt == total)
	{
		enum FW_BIN fw_bin_type = g_atcmd_fota.params.fw_bin_type;

		g_atcmd_fota.info.fw_bin[fw_bin_type].size = total;

		cnt = 0;
	}
}

static int _atcmd_fota_fw_update (void)
{
	enum FW_BIN fw_bin_type = g_atcmd_fota.params.fw_bin_type;
	fw_bin_t *fw_bin = &g_atcmd_fota.info.fw_bin[fw_bin_type];

	_atcmd_fota_log("Updating firmware ...");

	if (!strstr(fw_bin->name, ".bin"))
	{
		_atcmd_fota_log("!!! invalid firmware name, %s !!!", fw_bin->name);
		return -1;
	}

	_atcmd_fota_event_callback(FOTA_EVT_BINARY, fw_bin->name);

	if (_atcmd_fota_httpc_get(g_atcmd_fota.params.server_url, fw_bin->name,
							g_atcmd_fota.recv_buf.addr, g_atcmd_fota.recv_buf.len,
							_atcmd_fota_fw_update_callback) == 0)
	{
		uint32_t max_bin_size = SF_FOTA_INFO - SF_FOTA; /* hal/nrc7292/hal_sflash.h */

		if (fw_bin->size == 0)
			_atcmd_fota_log("update: !!! no binary !!!");
		else
		{
			_atcmd_fota_log("update: %s size=%u crc32=%X", fw_bin->name, fw_bin->size, fw_bin->crc32);

			if (fw_bin->size > max_bin_size)
				_atcmd_fota_log("update: !!! binary size is greater than %u byte !!!", max_bin_size);
			else
			{
				util_fota_set_info(fw_bin->size, fw_bin->crc32);
				util_fota_set_ready(true);

				_atcmd_fota_event_callback(FOTA_EVT_UPDATE, fw_bin->name, fw_bin->size, fw_bin->crc32);
				return 0;
			}
		}
	}

	return -1;
}

static void _atcmd_fota_task (void *pvParameters)
{
	bool new_fw = false;
	bool failed = false;

	_atcmd_fota_log("task: run");

	memset(&g_atcmd_fota.info, 0, sizeof(atcmd_fota_info_t));

	while (g_atcmd_fota.enable && !failed)
	{
		if (g_atcmd_fota.params.check_time > 0)
		{
			_atcmd_fota_debug("task: delay=%d", g_atcmd_fota.params.check_time);

			_delay_ms(g_atcmd_fota.params.check_time * 1000);

			_atcmd_fota_debug("task: expire");

			if (g_atcmd_fota.params.check_time == 0)
				continue;
		}
		else
		{
			_atcmd_fota_debug("task: suspend");

			vTaskSuspend(NULL);

			_atcmd_fota_debug("task: resume");

			if (g_atcmd_fota.params.check_time > 0)
				continue;
		}

		if (g_atcmd_fota.enable)
		{
			if (strlen(g_atcmd_fota.params.bin_name) > 0)
			{
				fw_bin_t *fw_bin = &g_atcmd_fota.info.fw_bin[g_atcmd_fota.params.fw_bin_type];

				strcpy(fw_bin->name, g_atcmd_fota.params.bin_name);
				fw_bin->crc32 = g_atcmd_fota.params.bin_crc32;
				new_fw = true;
			}
			else if (_atcmd_fota_fw_check(g_atcmd_fota.params.server_url, &new_fw) != 0)
			{
				failed = true;
				continue;
			}

			if (g_atcmd_fota.update && new_fw)
			{
				if (_atcmd_fota_fw_update() == 0)
					g_atcmd_fota.enable = false;
				else
					failed = true;
			}
		}
	}

	if (failed)
	{
		_atcmd_fota_log("failed");

		g_atcmd_fota.enable = false;
		g_atcmd_fota.update = false;

		_atcmd_fota_event_callback(FOTA_EVT_FAIL);
	}

	if (g_atcmd_fota.recv_buf.addr)
	{
		_atcmd_free(g_atcmd_fota.recv_buf.addr);

		g_atcmd_fota.recv_buf.addr = NULL;
		g_atcmd_fota.recv_buf.len = 0;
	}

	g_atcmd_fota.task = NULL;
	memset(&g_atcmd_fota.info, 0, sizeof(atcmd_fota_info_t));
	memset(&g_atcmd_fota.params, 0, sizeof(atcmd_fota_params_t));

	_atcmd_fota_log("task: delete");

	vTaskDelete(NULL);
}

#if 0
static void _atcmd_fota_print_params (atcmd_fota_params_t *params)
{
	_atcmd_fota_log("[ SET Parameters ]");
	_atcmd_fota_log(" - fw_bin_type : %d", params->fw_bin_type);
	_atcmd_fota_log(" - event_cb : %p", params->event_cb);
	_atcmd_fota_log(" - check_time : %d", params->check_time);
	_atcmd_fota_log(" - server_uri : %s", params->server_url);
	_atcmd_fota_log(" - bin_name : %s", params->bin_name);
	_atcmd_fota_log(" - bin_crc32 : %X", params->bin_crc32);
}
#else
#define _atcmd_fota_print_params(params)
#endif

static bool _atcmd_fota_valid_params (atcmd_fota_params_t *params)
{
	int len;

	if (!params)
		return false;

	_atcmd_fota_print_params(params);

	switch (params->fw_bin_type)
	{
		case FW_BIN_HSPI:
		case FW_BIN_UART:
		case FW_BIN_UART_HFC:
			if (params->event_cb)
				break;

		default:
			return false;
	}

	if (params->check_time < -1)
		return false;

	len = strlen(params->server_url);
	if (len > 0)
	{
		if (len > ATCMD_FOTA_SERVER_URL_LEN_MAX)
			return false;

		if (strncmp(params->server_url, "http://", 7) != 0 &&
			strncmp(params->server_url, "https://", 8) != 0)
			return false;
	}

	len = strlen(params->bin_name);
	if (len > 0)
	{
		if (len > ATCMD_FOTA_BIN_NAME_LEN_MAX)
			return false;

		if (params->check_time != 0)
			return false;

		if (strncmp(&params->bin_name[len - 4], ".bin", 4) != 0)
			return false;
	}

	return true;
}

static bool _atcmd_fota_support (void)
{
	return util_fota_is_support();
}

static int _atcmd_fota_enable (atcmd_fota_params_t *params)
{
	if (!g_atcmd_fota.recv_buf.addr)
	{
		char *buf;

		buf = _atcmd_malloc(ATCMD_FOTA_RECV_BUF_SIZE);
		if (!buf)
		{
			_atcmd_fota_log("enable: !!! recv_buf allocation failed !!!");
			return -1;
		}

		g_atcmd_fota.recv_buf.addr = buf;
		g_atcmd_fota.recv_buf.len = ATCMD_FOTA_RECV_BUF_SIZE;
	}

	g_atcmd_fota.params.fw_bin_type = params->fw_bin_type;
	g_atcmd_fota.params.event_cb = params->event_cb;

	g_atcmd_fota.params.check_time = params->check_time;

	if (strlen(params->server_url) > 0)
		strcpy(g_atcmd_fota.params.server_url, params->server_url);

	if (strlen(params->bin_name) > 0)
	{
		strcpy(g_atcmd_fota.params.bin_name, params->bin_name);
		g_atcmd_fota.params.bin_crc32 = params->bin_crc32;
	}

	if (g_atcmd_fota.enable)
	{
		_atcmd_fota_log("change: check_time=%d server_url=%s",
				g_atcmd_fota.params.check_time, g_atcmd_fota.params.server_url);

		if (strlen(g_atcmd_fota.params.bin_name) > 0)
		{
			_atcmd_fota_log("        bin_name=%s bin_crc32=0x%X",
				g_atcmd_fota.params.bin_name, g_atcmd_fota.params.bin_crc32);
		}

		vTaskResume(g_atcmd_fota.task);
	}
	else
	{
		_atcmd_fota_log("enable: check_time=%d server_url=%s",
				g_atcmd_fota.params.check_time, g_atcmd_fota.params.server_url);

		if (strlen(g_atcmd_fota.params.bin_name) > 0)
		{
			_atcmd_fota_log("        bin_name=%s bin_crc32=0x%X",
				g_atcmd_fota.params.bin_name, g_atcmd_fota.params.bin_crc32);
		}

		g_atcmd_fota.enable = true;

		if (xTaskCreate(_atcmd_fota_task, "atcmd_fota",
						ATCMD_FOTA_TASK_STACK_SIZE, NULL,
						ATCMD_FOTA_TASK_PRIORITY,
						&g_atcmd_fota.task) != pdPASS)
		{
			_atcmd_fota_log("enable: xTaskCreate() failed");

			g_atcmd_fota.enable = false;

			if (g_atcmd_fota.recv_buf.addr)
			{
				_atcmd_free(g_atcmd_fota.recv_buf.addr);

				g_atcmd_fota.recv_buf.addr = NULL;
				g_atcmd_fota.recv_buf.len = 0;
			}

			return -1;
		}
	}

	return 0;
}

static int _atcmd_fota_disable (void)
{
	_atcmd_fota_log("disable");

	g_atcmd_fota.enable = false;
	g_atcmd_fota.update = false;

	if (g_atcmd_fota.task)
	{
		if (g_atcmd_fota.params.check_time > 0)
			xTaskAbortDelay(g_atcmd_fota.task);
		else
			vTaskResume(g_atcmd_fota.task);
	}

	return 0;
}

static int _atcmd_fota_update (void)
{
	_atcmd_fota_log("update");

	if (!g_atcmd_fota.enable)
	   return -1;

	if (!g_atcmd_fota.update)
	{
		g_atcmd_fota.update = true;

		if (g_atcmd_fota.params.check_time > 0)
			xTaskAbortDelay(g_atcmd_fota.task);
		else
			vTaskResume(g_atcmd_fota.task);
	}

	return 0;
}
/**********************************************************************************************/

int atcmd_fota_set_params (atcmd_fota_params_t *params)
{
	if (!_atcmd_fota_valid_params(params))
		return -EINVAL;

	if (!_atcmd_fota_support())
		return -1;

	if (params->check_time == -1)
		return _atcmd_fota_disable();

	return _atcmd_fota_enable(params);
}

int atcmd_fota_get_params (atcmd_fota_params_t *params)
{
	if (!params)
		return -1;

	if (!_atcmd_fota_support())
		return -1;

	memcpy(params, &g_atcmd_fota.params, sizeof(atcmd_fota_params_t));

	return 0;
}

int atcmd_fota_update_firmware (void)
{
	if (!_atcmd_fota_support())
		return -1;

	return _atcmd_fota_update();
}

