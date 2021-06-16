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
		.server_url = { 0, },
		.check_time = 0,
		.event_cb = NULL,
	},

	.task = NULL
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
				event.update.bin_crc = va_arg(ap, uint32_t);
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
	static char url[ATCMD_FOTA_URL_LEN_MAX];
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

	return ret;
}

static int _atcmd_fota_info_parse (char *fota_info, atcmd_fota_info_t *info)
{
	const char *fw_ver_type[FW_VER_NUM] =
	{
		"sdk", "cmd"
	};
	const char *fw_bin_type[FW_BIN_NUM] =
	{
		"hspi", "uart", "uart_hfc"
	};
	const char *token_name[] =
	{
		"AT_SDK_VER", "AT_CMD_VER",
		"AT_HSPI_BIN", "AT_HSPI_CRC",
		"AT_UART_BIN", "AT_UART_CRC",
		"AT_UART_HFC_BIN", "AT_UART_HFC_CRC",

		NULL
	};
	char *token;
	char *saveptr;
	int i;

	_atcmd_fota_log("[ INFO FILE ]");

	token = strtok_r(fota_info, "\n", &saveptr);

	for (i = 0 ; token && token_name[i] ; i++)
	{
/*		_atcmd_fota_debug("fota_info: token=%s", token); */

		if (strncmp(token, token_name[i], strlen(token_name[i])) != 0)
		{
			_atcmd_fota_log("!!! invalid info file !!!");
			return -1;
		}

		token += strlen(token_name[i]);

		switch (i)
		{
			case 0: /* AT_SDK_VER */
			case 1: /* AT_CMD_VER */
			{
				fw_ver_t *ver = &info->fw_ver[FW_VER_SDK + i];

				sscanf(token, ": %d.%d.%d", &ver->major, &ver->minor, &ver->revision);

				_atcmd_fota_log("%s: %d.%d.%d", token_name[i], ver->major, ver->minor, ver->revision);
				break;
			}

			case 2: /* AT_HSPI_BIN */
			case 4: /* AT_UART_BIN */
			case 6: /* AT_UART_HFC_BIN */
			{
				char *name = info->fw_bin[FW_BIN_HSPI + (i / 2) - 1].name;

				sscanf(token, ": %s", name);

				_atcmd_fota_log("%s: %s", token_name[i], name);
				break;
			}

			case 3: /* AT_HSPI_CRC */
			case 5: /* AT_UART_CRC */
			case 7: /* AT_UART_HFC_CRC */
			{
				uint32_t *crc = &info->fw_bin[FW_BIN_HSPI + ((i + 1) / 2) - 2].crc;

				sscanf(token, ": %lx", crc);

				_atcmd_fota_log("%s: %lx", token_name[i], *crc);
				break;
			}
		}

		token = strtok_r(NULL, "\n", &saveptr);
	}

	return 0;
}

static void _atcmd_fota_fw_check_callback (char *data, int len, int total)
{
	static char *fota_info = NULL;
	static uint32_t cnt = 0;

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
		_atcmd_fota_log("download: %d/%d", cnt + len, total);
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
			_atcmd_fota_log("update: %s size=%u crc=%X", fw_bin->name, fw_bin->size, fw_bin->crc);

			if (fw_bin->size > max_bin_size)
				_atcmd_fota_log("update: !!! binary size is greater than %u byte !!!", max_bin_size);
			else
			{
				fota_info_t fota_info;

				fota_info.crc = fw_bin->crc;
				fota_info.fw_length = fw_bin->size;

				_atcmd_fota_event_callback(FOTA_EVT_UPDATE, fw_bin->name, fw_bin->size, fw_bin->crc);

				util_fota_set_info(fota_info.fw_length, fota_info.crc);
				util_fota_update_done(&fota_info); /* no return if success */
			}
		}
	}

	return -1;
}

static void _atcmd_fota_task (void *pvParameters)
{
	bool new_fw = false;

	_atcmd_fota_log("task: run");

	memset(&g_atcmd_fota.info, 0, sizeof(atcmd_fota_info_t));

	while (g_atcmd_fota.enable)
	{
		if (g_atcmd_fota.params.check_time > 0)
		{
			_atcmd_fota_debug("task: delay=%d", g_atcmd_fota.params.check_time);

			vTaskDelay(pdMS_TO_TICKS(g_atcmd_fota.params.check_time * 1000));

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
			if (_atcmd_fota_fw_check(g_atcmd_fota.params.server_url, &new_fw) == 0)
			{
				if (!g_atcmd_fota.update || !new_fw)
					continue;

				_atcmd_fota_fw_update(); /* no return if success */
			}

			_atcmd_fota_log("failed");
			_atcmd_fota_event_callback(FOTA_EVT_FAIL);

			g_atcmd_fota.enable = false;
			g_atcmd_fota.update = false;
		}
	}

	if (g_atcmd_fota.recv_buf.addr)
	{
		_atcmd_free(g_atcmd_fota.recv_buf.addr);

		g_atcmd_fota.recv_buf.addr = NULL;
		g_atcmd_fota.recv_buf.len = 0;
	}

	g_atcmd_fota.task = NULL;

	_atcmd_fota_log("task: delete");

	vTaskDelete(NULL);
}

static bool _atcmd_fota_valid_params (atcmd_fota_params_t *params)
{
	if (params)
	{
		switch (params->fw_bin_type)
		{
			case FW_BIN_HSPI:
			case FW_BIN_UART:
			case FW_BIN_UART_HFC:
				if (params->check_time >= 0 && params->event_cb)
				{
					if (strncmp(params->server_url, "http://", 7) == 0)
						return true;
					else if (strncmp(params->server_url, "https://", 8) == 0)
						return true;
					else if (strlen(params->server_url) == 0)
						return true;
				}

			default:
				break;
		}
	}

	return false;
}

static int _atcmd_fota_enable (atcmd_fota_params_t *params)
{
	if (!_atcmd_fota_valid_params(params))
		return -1;

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

	if (strlen(params->server_url) > 0)
		memcpy(&g_atcmd_fota.params, params, sizeof(atcmd_fota_params_t));
	else
	{
		g_atcmd_fota.params.fw_bin_type = params->fw_bin_type;
		g_atcmd_fota.params.check_time = params->check_time;
		g_atcmd_fota.params.event_cb = params->event_cb;
	}

	if (g_atcmd_fota.enable)
	{
		_atcmd_fota_log("change: check_time=%d server_url=%s",
					g_atcmd_fota.params.check_time, g_atcmd_fota.params.server_url);

		vTaskResume(g_atcmd_fota.task);
	}
	else
	{
		_atcmd_fota_log("enable: check_time=%d server_url=%s",
					g_atcmd_fota.params.check_time, g_atcmd_fota.params.server_url);

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

bool atcmd_fota_valid_params (atcmd_fota_params_t *params)
{
	if (params)
	{
		if (params->check_time == -1)
			return true;
		else if (_atcmd_fota_valid_params(params))
			return true;
	}

	return false;
}

int atcmd_fota_set_params (atcmd_fota_params_t *params)
{
	if (!params)
		return -1;

	if (params->check_time == -1)
		return _atcmd_fota_disable();

	return _atcmd_fota_enable(params);
}

int atcmd_fota_get_params (atcmd_fota_params_t *params)
{
	if (!params)
		return -1;

	memcpy(params, &g_atcmd_fota.params, sizeof(atcmd_fota_params_t));

	return 0;
}

int atcmd_fota_update_firmware (void)
{
	return _atcmd_fota_update();
}

