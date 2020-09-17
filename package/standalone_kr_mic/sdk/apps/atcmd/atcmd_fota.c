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

#define _atcmd_fota_debug(fmt, ...)		_atcmd_debug("fota: " fmt "\n",##__VA_ARGS__)
#define _atcmd_fota_log(fmt, ...)		_atcmd_info("fota: " fmt "\n", ##__VA_ARGS__)

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
		.check_done_cb = NULL,
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

static void _atcmd_fota_http_print_resp_info (httpc_resp_t *resp)
{
	_atcmd_fota_log("[ HTTP Resp Info ]");
	_atcmd_fota_log(" - Version: %.1f", resp->Version);
	_atcmd_fota_log(" - StatusCode: %d", resp->StatusCode);
	_atcmd_fota_log(" - ContentLength: %d", resp->ContentLength);
}

static void _atcmd_fota_http_parse_resp_header (char *header, httpc_resp_t *resp)
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

static int _atcmd_fota_http_get_file (const char *server_url, const char *file_path,
										int chunk_size, void (*cb)(char *, int, int))
{
	const int default_chunk_size = 512;
	ssl_certs_t *certs = NULL;
	con_handle_t handle;
	httpc_data_t data;
	httpc_resp_t resp;
	char *buf, *url;
	int buf_len, url_len;
	int ret;
	int i;

	if (!server_url || !strlen(server_url))
	{
		_atcmd_error("invalid server_url\n");
		return -1;
	}

	if (!file_path || !strlen(file_path))
	{
		_atcmd_error("invalid file_path\n");
		return -1;
	}

	if (chunk_size <= 0)
		chunk_size = default_chunk_size;

	buf_len = chunk_size;
	buf = _atcmd_malloc(buf_len);

	if (!buf && buf_len > default_chunk_size)
	{
		_atcmd_fota_log("http_get: chunk_size, %d->%d\n", buf_len, default_chunk_size);

		buf_len = default_chunk_size;
		buf = _atcmd_malloc(buf_len);
	}

	url_len = strlen(server_url) + strlen(file_path) + 2;
	url = _atcmd_malloc(url_len);

	if (!buf || !url)
	{
		_atcmd_error("malloc() failed, buf=%p url=%p\n", buf, url);

		ret = -1;
		goto http_get_file_exit;
	}

	data.data_in = buf;
	data.data_in_length = buf_len;

	if (server_url[strlen(server_url) - 1] == '/')
		sprintf(url, "%s%s", server_url, file_path);
	else
		sprintf(url, "%s/%s", server_url, file_path);

	if (memcmp(url, "https://", 8) == 0)
		certs = &g_https_ssl_certs;

	_atcmd_fota_log("http_get: %s", url);

	ret = nrc_httpc_get(&handle, url, NULL, &data, certs);
	if (ret != HTTPC_RET_OK)
	{
		_atcmd_error("nrc_httpc_get() failed, ret=%d\n", ret);

		ret = -1;
		goto http_get_file_exit;
	}

	memset(&resp, 0, sizeof(resp));

	for (i = data.recved_size ; i < buf_len ; )
	{
		if (resp.ContentLength == 0)
		{
			char *header = data.data_in;
			char *header_end = strstr(data.data_in, "\r\n\r\n");

			if (header_end)
			{
				int header_len = (header_end - header) + 4;

				memset(header_end, 0, 4);

				_atcmd_fota_http_parse_resp_header(header, &resp);
				_atcmd_fota_http_print_resp_info(&resp);

				if (resp.ContentLength > 0 && data.recved_size > header_len)
				{
					resp.ContentLengthDone = data.recved_size - header_len;

					if (cb)
						cb(data.data_in + header_len, data.recved_size - header_len, resp.ContentLength);
				}

				i = 0;
			}
		}

		data.data_in = buf + i;
		data.data_in_length = buf_len - i;
		data.recved_size = 0;

		ret = nrc_httpc_recv_response(&handle, &data);
		if (ret != HTTPC_RET_OK)
		{
			if (ret != HTTPC_RET_CON_CLOSED)
				_atcmd_error("nrc_httpc_recv_response() failed, ret=%d\n", ret);

			break;
		}

		if (resp.ContentLength == 0)
			i += data.recved_size;
		else
		{
			resp.ContentLengthDone += data.recved_size;

			if (cb)
				cb(data.data_in, data.recved_size, resp.ContentLength);
		}
	}

	nrc_httpc_close(&handle);

	ret = -1;

	if (i >= buf_len)
		_atcmd_error("no http resp header\n");
	else if (resp.StatusCode != 200)
		_atcmd_error("http resp error, status_code=%d\n", resp.StatusCode);
	else if (resp.ContentLengthDone != resp.ContentLength)
		_atcmd_error("http content length, %d/%d\n", resp.ContentLengthDone, resp.ContentLength);
	else
		ret = 0;

http_get_file_exit:

	if (buf)
		_atcmd_free(buf);

	if (url)
		_atcmd_free(url);

	return ret;
}

static int _atcmd_fota_fw_parse_info (char *fw_info, atcmd_fota_info_t *info)
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

	token = strtok_r(fw_info, "\n", &saveptr);

	for (i = 0 ; token && token_name[i] ; i++)
	{
//		_atcmd_fota_debug("fw_info: token=%s", token);

		if (strncmp(token, token_name[i], strlen(token_name[i])) != 0)
		{
			_atcmd_fota_log("invalid info file");
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

static void _atcmd_fota_fw_check_info (char *data, int len, int total)
{
	static char *fw_info = NULL;
	static uint32_t cnt = 0;

//	_atcmd_fota_debug("fw_check: len=%d total=%d", len, total);

	if (!fw_info)
	{
		fw_info = _atcmd_malloc(total);
		if (!fw_info)
		{
			_atcmd_error("malloc() failed\n");
			return;
		}
	}

	memcpy(fw_info + cnt, data, len);
	cnt += len;

	if (cnt == total)
	{
		fw_info[cnt] = '\0';

//		_atcmd_fota_debug("fw_check: %s", fw_info);

		_atcmd_fota_fw_parse_info(fw_info, &g_atcmd_fota.info);

		_atcmd_free(fw_info);

		fw_info = NULL;
		cnt = 0;
	}
}

static int _atcmd_fota_fw_check (const char *server_url)
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

	if (_atcmd_fota_http_get_file(server_url, "fota.info", 0, _atcmd_fota_fw_check_info) != 0)
		return -1;

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

		snprintf(sdk_ver, sizeof(sdk_ver), "%d.%d.%d",
						g_atcmd_fota.info.fw_ver[FW_VER_SDK].major,
						g_atcmd_fota.info.fw_ver[FW_VER_SDK].minor,
						g_atcmd_fota.info.fw_ver[FW_VER_SDK].revision);

		snprintf(atcmd_ver, sizeof(atcmd_ver), "%d.%d.%d",
						g_atcmd_fota.info.fw_ver[FW_VER_ATCMD].major,
						g_atcmd_fota.info.fw_ver[FW_VER_ATCMD].minor,
						g_atcmd_fota.info.fw_ver[FW_VER_ATCMD].revision);

		_atcmd_fota_log("fw_check: sdk=%s atcmd=%s", sdk_ver, atcmd_ver);

		if (g_atcmd_fota.params.check_done_cb)
			g_atcmd_fota.params.check_done_cb(sdk_ver, atcmd_ver);

		return 1;
	}

	return 0;
}

static void _atcmd_fota_fw_write (char *data, int len, int total)
{
	static uint32_t cnt = 0;

	if (cnt == 0)
	{
		_atcmd_fota_log("fw_erase: %d", total);

		util_fota_erase(0, total);
	}

	_atcmd_fota_log("fw_write: %d/%d", cnt + len, total);

	util_fota_write(cnt, (uint8_t *)data, len);

	cnt += len;

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

	if (!strstr(fw_bin->name, ".bin"))
		return -1;

	if (_atcmd_fota_http_get_file(g_atcmd_fota.params.server_url, fw_bin->name, 8192, _atcmd_fota_fw_write) == 0)
	{
		_atcmd_fota_log("fw_update: %s crc=%x size=%d", fw_bin->name, fw_bin->crc, fw_bin->size);

		if (fw_bin->size > 0)
		{
			fota_info_t fota_info;

			fota_info.crc = fw_bin->crc;
			fota_info.fw_length = fw_bin->size;

			/*
			 * If the update is successful, the system is reset to run the new firmware.
			 */
			util_fota_update_done(&fota_info);
		}
	}

	return -1;
}

static void _atcmd_fota_task (void *pvParameters)
{
	_atcmd_fota_log("task_run");

	while (g_atcmd_fota.enable)
	{
		if (g_atcmd_fota.params.check_time > 0)
		{
			_atcmd_fota_log("task_delay: %d", g_atcmd_fota.params.check_time);

			vTaskDelay(pdMS_TO_TICKS(g_atcmd_fota.params.check_time * 1000));

			_atcmd_fota_log("task_expire");

			if (g_atcmd_fota.params.check_time == 0)
				continue;
		}
		else
		{
			_atcmd_fota_log("task_suspend");

			vTaskSuspend(NULL);

			_atcmd_fota_log("task_resume");

			if (g_atcmd_fota.params.check_time > 0)
				continue;
		}

		if (g_atcmd_fota.enable)
		{
			_atcmd_fota_log("Checking firmware ...");

			_atcmd_fota_fw_check(g_atcmd_fota.params.server_url);

			if (g_atcmd_fota.update)
			{
				_atcmd_fota_log("Updating firmware ...");

				_atcmd_fota_fw_update();
			}
		}
	}

	_atcmd_fota_log("task exit");

	g_atcmd_fota.task = NULL;

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
				if (params->check_time >= 0 && params->check_done_cb)
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
#define ATCMD_FOTA_TASK_PRIORITY		2
#define ATCMD_FOTA_TASK_STACK_SIZE		1024

	if (!_atcmd_fota_valid_params(params))
		return -1;

	if (strlen(params->server_url) > 0)
		memcpy(&g_atcmd_fota.params, params, sizeof(atcmd_fota_params_t));
	else
	{
		g_atcmd_fota.params.fw_bin_type = params->fw_bin_type;
		g_atcmd_fota.params.check_time = params->check_time;
		g_atcmd_fota.params.check_done_cb = params->check_done_cb;
	}

	if (g_atcmd_fota.enable)
	{
		_atcmd_fota_log("change_params: check_time=%d server_url=%s",
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
			_atcmd_fota_log("xTaskCreate() failed");

			g_atcmd_fota.enable = false;

			return -1;
		}
	}

	return 0;
}

static int _atcmd_fota_disable (void)
{
	_atcmd_fota_log("disable");

	g_atcmd_fota.enable = false;

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

