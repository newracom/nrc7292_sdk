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

#include "nrc_sdk.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "coap2/coap.h"
#include "wifi_config_setup.h"
#include "wifi_connect_common.h"

#ifdef __GNUC__
#define UNUSED_PARAM __attribute__ ((unused))
#else /* not a GCC */
#define UNUSED_PARAM
#endif /* GCC */

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef NI_MAXSERV
#define NI_MAXSERV	32
#endif

#ifndef NI_MAXHOST
#define NI_MAXHOST  1025
#endif

#define MAX_KEY			64		/* Maximum length of a key in bytes. */

#define LIGHT_PIN		GPIO_02

static int coap_server_quit = 0;
static coap_time_t clock_offset;
static coap_time_t my_clock_base = 0;

struct coap_resource_t *time_resource = NULL;

static int resource_flags = COAP_RESOURCE_FLAGS_NOTIFY_CON; //0x02

static char *cert_file = NULL;			/* Combined certificate and private key in PEM */
static char *ca_file = NULL;			/* CA for cert_file - for cert checking in PEM */
static char *root_ca_file = NULL;		/* List of trusted Root CAs in PEM */
static int require_peer_cert = 1;		/* By default require peer cert */
static uint8_t key[MAX_KEY];
static ssize_t key_length = 0;
int key_defined = 0;
static const char *hint = "CoAP";
static int support_dynamic = 0;
static char *optarg;

typedef struct dynamic_resource_t {
	coap_string_t *uri_path;
	coap_string_t *value;
	coap_resource_t *resource;
	int created;
	uint16_t media_type;
} dynamic_resource_t;

static int dynamic_count = 0;
static dynamic_resource_t *dynamic_entry = NULL;

/*rsa key size is 1024 */
static const char server_cert_file[] =
"-----BEGIN CERTIFICATE-----\r\n"
"MIIDOjCCAqOgAwIBAgIJANTlt2wY/CcoMA0GCSqGSIb3DQEBCwUAMGoxFzAVBgNV\r\n"
"BAMMDkFuIENvQVAgYnJva2VyMRYwFAYDVQQKDA1Pd25UcmFja3Mub3JnMRQwEgYD\r\n"
"VQQLDAtnZW5lcmF0ZS1DQTEhMB8GCSqGSIb3DQEJARYSbm9ib2R5QGV4YW1wbGUu\r\n"
"bmV0MB4XDTE5MDcwNDAxMzMyMVoXDTMyMDYzMDAxMzMyMVowDjEMMAoGA1UEAwwD\r\n"
"c3J2MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDYZRPeNlPLC34KyoyAFTTy\r\n"
"+bbSvNoPB/VRe030FBwnzf/BSjzU6/0Jvr9attHQxG5DYg40Xu/B0o2u5iSwi8ow\r\n"
"zGjcuRqDFPJg2RE5u2+4ARfDMe6V7zPq18Y5Srjk/SfjD/7XOWNEfkEDf0z51zsz\r\n"
"xL9/6L8Mu4Yi0dmhtz3DdwIDAQABo4IBQjCCAT4wDAYDVR0TAQH/BAIwADAJBgNV\r\n"
"HREEAjAAMBEGCWCGSAGG+EIBAQQEAwIFoDAdBgNVHSUEFjAUBggrBgEFBQcDAgYI\r\n"
"KwYBBQUHAwQwCwYDVR0PBAQDAgOoMCYGCWCGSAGG+EIBDQQZFhdDb2FwIENsaWVu\r\n"
"dCBDZXJ0aWZpY2F0ZTAdBgNVHQ4EFgQUzGy3pAjgU30KA/Z3Fv5MTi+jZLgwgZwG\r\n"
"A1UdIwSBlDCBkYAUWKBNMv+7l/RzApqnudKvbF41RsuhbqRsMGoxFzAVBgNVBAMM\r\n"
"DkFuIENvQVAgYnJva2VyMRYwFAYDVQQKDA1Pd25UcmFja3Mub3JnMRQwEgYDVQQL\r\n"
"DAtnZW5lcmF0ZS1DQTEhMB8GCSqGSIb3DQEJARYSbm9ib2R5QGV4YW1wbGUubmV0\r\n"
"ggkA2pYsZ/BbUWkwDQYJKoZIhvcNAQELBQADgYEAi74NEk0d4kIjWKNfckf3Z189\r\n"
"b/pr5pOZCTSLZzmbI9XrcXKhQ9+v1cF0IJJ9KN5PI9b0JgMU+r/dsguFD934uCE2\r\n"
"5i6ZPONbZ6eec+wRV9rH3qxwbbzinrRy/OzJACIjuKUNlKI3foaFiW7AcjMeW4HM\r\n"
"DgeLBovzUsQUt0A6GjI=\r\n"
"-----END CERTIFICATE-----\r\n"
"-----BEGIN RSA PRIVATE KEY-----\r\n"
"MIICWwIBAAKBgQDYZRPeNlPLC34KyoyAFTTy+bbSvNoPB/VRe030FBwnzf/BSjzU\r\n"
"6/0Jvr9attHQxG5DYg40Xu/B0o2u5iSwi8owzGjcuRqDFPJg2RE5u2+4ARfDMe6V\r\n"
"7zPq18Y5Srjk/SfjD/7XOWNEfkEDf0z51zszxL9/6L8Mu4Yi0dmhtz3DdwIDAQAB\r\n"
"AoGAEzoP5NuQ4GaaAV1z1GGN/dTAMMNE4LcFTgMVrw0mX0cHZWYyN3zsU7RWDjpW\r\n"
"NCVv4p7Qwkh5JxCNNDAQrHsPMNLu2nAg+9q345tusvoDDD9ZOBU4hLMRMlxrQdkD\r\n"
"HNMslqqskJ7DCFk/PjFACU1bWE8AJlILFQoVVsw+RsPbYdECQQDzvBqXMOxhcG2s\r\n"
"0r0bxrryDy3QVLEYHhtVU0WQSsuy/bHI4ypZVTYCzhvkP8a0AwN7sjh+kkSRq8XR\r\n"
"2fJ5hMxpAkEA40jE0KYxd1UuPupHTvxgRGKDmcKbkjLlSdc4prxy9nviQ34vTQMP\r\n"
"kasdZTrlydAkOWF0g03tQvCGsiLPouuU3wJAce7NEz2oUYgHjJDaG2DTRJF53feo\r\n"
"7Ngt+L27N87u11WlxH0D78xYOgl0mkaBsOXzu9+8btYavWlpSEY0BT/heQJAfk1a\r\n"
"6ePhC0Jvr5C7Tb5btMTWAuUtVKIp3s3V8no4nJadVwpROMehqD5h2HZRacPbzXNF\r\n"
"rw3n8lH9WBKWNiAaGwJABRGp+cF3jmOAWdnkaUOqid5H7PRhB/GOai6ALL/C8BHJ\r\n"
"7KkgXusRCSWmXBcpLoPyB0emOvg5WXm97JRXKH5Xmg==\r\n"
"-----END RSA PRIVATE KEY-----\r\n";

static const char server_ca_file[] =
"-----BEGIN CERTIFICATE-----\r\n"
"MIICpTCCAg6gAwIBAgIJANqWLGfwW1FpMA0GCSqGSIb3DQEBCwUAMGoxFzAVBgNV\r\n"
"BAMMDkFuIENvQVAgYnJva2VyMRYwFAYDVQQKDA1Pd25UcmFja3Mub3JnMRQwEgYD\r\n"
"VQQLDAtnZW5lcmF0ZS1DQTEhMB8GCSqGSIb3DQEJARYSbm9ib2R5QGV4YW1wbGUu\r\n"
"bmV0MB4XDTE5MDcwNDAxMzMyMVoXDTMyMDYzMDAxMzMyMVowajEXMBUGA1UEAwwO\r\n"
"QW4gQ29BUCBicm9rZXIxFjAUBgNVBAoMDU93blRyYWNrcy5vcmcxFDASBgNVBAsM\r\n"
"C2dlbmVyYXRlLUNBMSEwHwYJKoZIhvcNAQkBFhJub2JvZHlAZXhhbXBsZS5uZXQw\r\n"
"gZ8wDQYJKoZIhvcNAQEBBQADgY0AMIGJAoGBAOjvFmeqlgGYNJopk571ZvIhwCfU\r\n"
"ujLAhiDkQ+1+Pi2EUf1P0XpvichKMg6uR/t1Nn5Ty2jfsCoxaHpw2qe7mJ2gYZNT\r\n"
"4aj5eKc1KuTGiGxEcThAMF5RqslYgyvLHsbvWA/RV4CVq3cMoKGsvz4mIRkTti42\r\n"
"g1cYlTEW/KYqCtlfAgMBAAGjUzBRMB0GA1UdDgQWBBRYoE0y/7uX9HMCmqe50q9s\r\n"
"XjVGyzAfBgNVHSMEGDAWgBRYoE0y/7uX9HMCmqe50q9sXjVGyzAPBgNVHRMBAf8E\r\n"
"BTADAQH/MA0GCSqGSIb3DQEBCwUAA4GBALHKT4iPBT+mmK+0QWTfHcV0fJhBOpUX\r\n"
"TIg9mWbJrDgO8jk7K9bzFaTYKNnI5a62H6kgl6Z77KocqBsNZYGzpbHaHR1ZJiA0\r\n"
"KCv7jbfTZu0rJBaViBt7CMWmQZr8s+Ap90tuqGta52xcH70KhgnrOpqAbIU6c6Rb\r\n"
"AJ1Z59qDGhfD\r\n"
"-----END CERTIFICATE-----\r\n";

static int gpio_init(void)
{
	NRC_GPIO_CONFIG gpio_conf;

	gpio_conf.gpio_pin = LIGHT_PIN;
	gpio_conf.gpio_dir = GPIO_OUTPUT;
	gpio_conf.gpio_mode = GPIO_PULL_UP;
	gpio_conf.gpio_alt= GPIO_FUNC;
	nrc_gpio_config(&gpio_conf);

	return 0;
}

static int
verify_cn_callback(const char *cn,
				const uint8_t *asn1_public_cert UNUSED_PARAM,
				size_t asn1_length UNUSED_PARAM,
				coap_session_t *session UNUSED_PARAM,
				unsigned depth,
				int validated UNUSED_PARAM,
				void *arg UNUSED_PARAM)
{
	coap_log(LOG_INFO, "CN '%s' presented by client (%s)\n",
		cn, depth ? "CA" : "Certificate");
	return 1;
}

static void
hnd_get_time(coap_context_t  *ctx UNUSED_PARAM,
	 struct coap_resource_t *resource,
	 coap_session_t *session,
	 coap_pdu_t *request,
	 coap_binary_t *token,
	 coap_string_t *query,
	 coap_pdu_t *response)
{
	unsigned char buf[40];
	size_t len;
	time_t now;
	coap_tick_t t;
	(void)request;

	if (my_clock_base) {
		coap_ticks(&t);
		now = my_clock_base + (t / COAP_TICKS_PER_SECOND);

		if (query != NULL	&& coap_string_equal(query, coap_make_str_const("ticks"))) {
			len = snprintf((char *)buf, sizeof(buf), "%u", (unsigned int)now);
		} else {
			struct tm *tmp;
			tmp = gmtime(&now);
			if (!tmp) {
				response->code = COAP_RESPONSE_CODE(404);	/* If 'now' is not valid */
				return;
			}else {
				len = strftime((char *)buf, sizeof(buf), "%b %d %H:%M:%S", tmp);
			}
		}
		coap_add_data_blocked_response(resource, session, request, response,
			token, COAP_MEDIATYPE_TEXT_PLAIN, 1, len, buf);
	}else {
		/* if my_clock_base was deleted, we pretend to have no such resource */
		response->code = COAP_RESPONSE_CODE(404);
	}
}

static void
hnd_put_time(coap_context_t *ctx UNUSED_PARAM,
			struct coap_resource_t *resource,
			coap_session_t *session UNUSED_PARAM,
			coap_pdu_t *request,
			coap_binary_t *token UNUSED_PARAM,
			coap_string_t *query UNUSED_PARAM,
			coap_pdu_t *response)
{
	coap_tick_t t;
	size_t size;
	unsigned char *data;
	unsigned char buf[40];
	time_t now;
	struct tm *tmp;

	/* if my_clock_base was deleted, we pretend to have no such resource */
	response->code = my_clock_base ? COAP_RESPONSE_CODE(204) : COAP_RESPONSE_CODE(201);

	coap_resource_notify_observers(resource, NULL);
	/* coap_get_data() sets size to 0 on error */
	(void)coap_get_data(request, &size, &data);
	if (size == 0){		/* re-init */
		my_clock_base = clock_offset;
	}else {
		coap_ticks(&t);
		while(size--){
			my_clock_base = my_clock_base * 10 + *data++;
		}
		now = my_clock_base+ (t / COAP_TICKS_PER_SECOND);
		tmp = gmtime(&now);
		if (!tmp) {
			response->code = COAP_RESPONSE_CODE(404);	/* If 'now' is not valid */
			return;
		}else {
			strftime((char *)buf, sizeof(buf), "%b %d %H:%M:%S", tmp);
			nrc_usr_print("time(PUT) : %s\n", buf);
		}
	}

	if (!gmtime( (time_t*) &my_clock_base)) {
		unsigned char buf[3];
		response->code = COAP_RESPONSE_CODE(400);
		coap_add_option(response,
				COAP_OPTION_CONTENT_FORMAT,
				coap_encode_var_safe(buf, sizeof(buf),
				COAP_MEDIATYPE_TEXT_PLAIN), buf);
		coap_add_data(response, 22, (const uint8_t*)"Invalid set time value");
		/* re-init as value is bad */
		my_clock_base = clock_offset;
	}
}

static void
hnd_get_light(coap_context_t  *ctx UNUSED_PARAM,
	 struct coap_resource_t *resource,
	 coap_session_t *session,
	 coap_pdu_t *request,
	 coap_binary_t *token,
	 coap_string_t *query UNUSED_PARAM,
	 coap_pdu_t *response)
{
	(void)request;
	unsigned char buf[100];
	size_t len;
	int light_value;

	if (nrc_gpio_inputb(LIGHT_PIN, &light_value) != NRC_SUCCESS)
		return;

	if(light_value == 0){
		len = sprintf((char *)buf, "off");
	}else
		len = sprintf((char *)buf, "on");

	coap_add_data_blocked_response(resource, session, request, response,
			token, COAP_MEDIATYPE_TEXT_PLAIN, 1, len, buf);

	nrc_usr_print("[get] Light %s\n", (light_value==1) ? "[on]":"[off]");
}

static void
hnd_put_light(coap_context_t *ctx UNUSED_PARAM,
			struct coap_resource_t *resource UNUSED_PARAM,
			coap_session_t *session UNUSED_PARAM,
			coap_pdu_t *request,
			coap_binary_t *token UNUSED_PARAM,
			coap_string_t *query UNUSED_PARAM,
			coap_pdu_t *response)
{
	size_t size;
	unsigned char *data;
	int data_i;
	int light_value;

	if (nrc_gpio_inputb(LIGHT_PIN, &light_value) != NRC_SUCCESS)
		return;

	(void)coap_get_data(request, &size, &data);
	data_i= (*data)-'0';

	if (size == 0){
		unsigned char buf[3];
		response->code = COAP_RESPONSE_CODE(400);
		coap_add_option(response,
					COAP_OPTION_CONTENT_FORMAT,
					coap_encode_var_safe(buf, sizeof(buf),
					COAP_MEDIATYPE_TEXT_PLAIN), buf);
		coap_add_data(response, 22, (const uint8_t*)"Invalid value");
		nrc_usr_print("hnd_put_light error, size:%d\n", size);
	}else {
		if(light_value != data_i){
			nrc_gpio_outputb(LIGHT_PIN, data_i);
		}
		response->code = COAP_RESPONSE_CODE(201);
		nrc_usr_print("[put] Light %s\n", (data_i==1) ? "[on]":"[off]");
	}
}

static void
hnd_delete(coap_context_t *ctx,
	coap_resource_t *resource,
	coap_session_t *session UNUSED_PARAM,
	coap_pdu_t *request UNUSED_PARAM,
	coap_binary_t *token UNUSED_PARAM,
	coap_string_t *query UNUSED_PARAM,
	coap_pdu_t *response UNUSED_PARAM)
{
	int i;
	coap_string_t *uri_path;

	/* get the uri_path */
	uri_path = coap_get_uri_path(request);
	if (!uri_path) {
		response->code = COAP_RESPONSE_CODE(404);
		return;
	}

	for (i = 0; i < dynamic_count; i++) {
		if (coap_string_equal(uri_path, dynamic_entry[i].uri_path)) {
			/* Dynamic entry no longer required - delete it */
			coap_delete_string(dynamic_entry[i].value);
			if (dynamic_count-i > 1) {
				memmove (&dynamic_entry[i], &dynamic_entry[i+1],
					(dynamic_count-i-1) * sizeof (dynamic_entry[0]));
			}
			dynamic_count--;
			break;
		}
	}

	/* Dynamic resource no longer required - delete it */
	coap_delete_resource(ctx, resource);
	response->code = COAP_RESPONSE_CODE(202);
	return;
}

static void
hnd_get(coap_context_t *ctx UNUSED_PARAM,
	coap_resource_t *resource,
	coap_session_t *session,
	coap_pdu_t *request,
	coap_binary_t *token,
	coap_string_t *query UNUSED_PARAM,
	coap_pdu_t *response)
{
	coap_str_const_t *uri_path;
	int i;
	dynamic_resource_t *resource_entry = NULL;

	uri_path = coap_resource_get_uri_path(resource);
	if (!uri_path) {
		response->code = COAP_RESPONSE_CODE(404);
		return;
	}

	for (i = 0; i < dynamic_count; i++) {
		if (coap_string_equal(uri_path, dynamic_entry[i].uri_path)) {
			break;
		}
	}
	if (i == dynamic_count) {
		response->code = COAP_RESPONSE_CODE(404);
		return;
	}

	resource_entry = &dynamic_entry[i];

	coap_add_data_blocked_response(resource, session, request, response, token,
		resource_entry->media_type, -1, resource_entry->value->length,
		resource_entry->value->s);
	return;
}

static coap_dtls_key_t *
verify_sni_callback(const char *sni, void *arg UNUSED_PARAM)
{
	static coap_dtls_key_t dtls_key;

	memset (&dtls_key, 0, sizeof(dtls_key));
	dtls_key.key_type = COAP_PKI_KEY_PEM;
	dtls_key.key.pem.public_cert = cert_file;
	dtls_key.key.pem.private_key = cert_file;
	dtls_key.key.pem.ca_file = ca_file;
	if (sni[0]) {
		coap_log(LOG_INFO, "SNI '%s' requested\n", sni);
	}else {
		coap_log(LOG_DEBUG, "SNI not requested\n");
	}
	return &dtls_key;
}

static void
fill_keystore(coap_context_t *ctx)
{
	if (cert_file) {
		coap_dtls_pki_t dtls_pki;
		memset (&dtls_pki, 0, sizeof(dtls_pki));
		dtls_pki.version = COAP_DTLS_PKI_SETUP_VERSION;
		if (ca_file) {
			dtls_pki.verify_peer_cert        = 1;
			dtls_pki.require_peer_cert       = require_peer_cert;
			dtls_pki.allow_self_signed       = 1;
			dtls_pki.allow_expired_certs     = 1;
			dtls_pki.cert_chain_validation   = 1;
			dtls_pki.cert_chain_verify_depth = 2;
			dtls_pki.check_cert_revocation   = 1;
			dtls_pki.allow_no_crl            = 1;
			dtls_pki.allow_expired_crl       = 1;
			dtls_pki.validate_cn_call_back   = verify_cn_callback;
			dtls_pki.cn_call_back_arg        = NULL;
			dtls_pki.validate_sni_call_back  = verify_sni_callback;
			dtls_pki.sni_call_back_arg       = NULL;
		}
		dtls_pki.pki_key.key_type = COAP_PKI_KEY_PEM;
		dtls_pki.pki_key.key.pem.public_cert = cert_file;
		dtls_pki.pki_key.key.pem.private_key = cert_file;
		dtls_pki.pki_key.key.pem.ca_file = ca_file;

		/* If general root CAs are defined */
		if (root_ca_file) {
			coap_context_set_pki_root_cas(ctx, root_ca_file, NULL);
		}
		if (key_defined){
			coap_context_set_psk(ctx, hint, key, key_length);
		}
		coap_context_set_pki(ctx, &dtls_pki);
	}else if (key_defined) {
		coap_context_set_psk(ctx, hint, key, key_length);
	}else if (coap_dtls_is_supported() || coap_tls_is_supported()) {
		coap_log(LOG_DEBUG,"(D)TLS not enabled\n");
	}
}

static void
init_resources(coap_context_t *ctx)
{
	coap_resource_t *r;

	my_clock_base = clock_offset;			/* store clock base to use in /time */

	r = coap_resource_init(coap_make_str_const("time"), resource_flags);
	coap_register_handler(r, COAP_REQUEST_GET, hnd_get_time);
	coap_register_handler(r, COAP_REQUEST_PUT, hnd_put_time);
	coap_resource_set_get_observable(r, 1);

	/* time */
	coap_add_attr(r, coap_make_str_const("ct"), coap_make_str_const("0"), 0);
	coap_add_attr(r, coap_make_str_const("title"), coap_make_str_const("\"Internal Clock\""), 0);
	coap_add_attr(r, coap_make_str_const("rt"), coap_make_str_const("\"ticks\""), 0);
	coap_add_attr(r, coap_make_str_const("if"), coap_make_str_const("\"clock\""), 0);
	coap_add_resource(ctx, r);
	time_resource = r;

	/* light */
	r = coap_resource_init(coap_make_str_const("light"), resource_flags);
	coap_register_handler(r, COAP_REQUEST_GET, hnd_get_light);
	coap_register_handler(r, COAP_REQUEST_PUT, hnd_put_light);
	coap_resource_set_get_observable(r, 1);
	coap_add_attr(r, coap_make_str_const("ct"), coap_make_str_const("0"), 0);
	coap_add_attr(r, coap_make_str_const("title"), coap_make_str_const("\"Light State\""), 0);
	coap_add_resource(ctx, r);
}

static coap_context_t *
get_context(const char *node, const char *port)
{
	coap_context_t *ctx = NULL;
	int s;
	struct addrinfo hints;
	struct addrinfo *result, *rp;

	ctx = coap_new_context(NULL);
	if (!ctx) {
		return NULL;
	}

	fill_keystore(ctx);

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;					/* Allow IPv4 */
	hints.ai_socktype = SOCK_DGRAM;				/* Coap uses UDP */
	hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST;

	s = getaddrinfo(node, port, &hints, &result);
	if ( s != 0 ) {
		coap_log(LOG_WARNING, "getaddrinfo: %s\n", gai_strerror(s));
		coap_free_context(ctx);
		return NULL;
	}

	/* iterate through results until success */
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		coap_address_t addr, addrs;
		coap_endpoint_t *ep_udp = NULL, *ep_dtls = NULL, *ep_tcp = NULL, *ep_tls = NULL;

		if (rp->ai_addrlen <= sizeof(addr.addr)) {
			coap_address_init(&addr);
			addr.size = rp->ai_addrlen;
			memcpy(&addr.addr, rp->ai_addr, rp->ai_addrlen);
			addrs = addr;
			if (addr.addr.sa.sa_family == AF_INET) {
				uint16_t temp = ntohs(addr.addr.sin.sin_port) + 1;
				addrs.addr.sin.sin_port = htons(temp);
			}else {
				goto finish;
			}

			ep_udp = coap_new_endpoint(ctx, &addr, COAP_PROTO_UDP);
			if (ep_udp) {
				if (coap_dtls_is_supported() && (key_defined || cert_file)) {
					ep_dtls = coap_new_endpoint(ctx, &addrs, COAP_PROTO_DTLS);
					if (!ep_dtls)
						coap_log(LOG_CRIT, "cannot create DTLS endpoint\n");
				}
			} else {
				coap_log(LOG_CRIT, "cannot create UDP endpoint\n");
				continue;
			}

			if (ep_udp)
				goto finish;
		}
	}

	coap_log(LOG_WARNING,  "no context available for interface '%s'\n", node);

finish:
	freeaddrinfo(result);
	return ctx;
}

/******************************************************************************
 * FunctionName : run_sample_coap_server
 * Description  : sample test for coap server
 * Parameters   : WIFI_CONFIG
 * Returns      : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
nrc_err_t run_sample_coap_server(WIFI_CONFIG *param)
{
	int interval = 0;
	char *pcmdStr_log = NULL;
	coap_context_t  *ctx;
	char *group = NULL;
	coap_tick_t now;
	char addr_str[NI_MAXHOST] = "0.0.0.0";
	char port_str[NI_MAXSERV] = "5683";
	int opt;
	coap_log_t log_level = LOG_DEBUG;
	unsigned wait_ms;
	time_t t_last = 0;
	tWIFI_STATE_ID wifi_state = WIFI_STATE_INIT;
  	int network_index = 0;
	SCAN_RESULTS results;

	nrc_usr_print("[%s] Sample App for libCoap_Server\n",__func__);
	nrc_usr_print("[%s] [CoAP Server]\n",__func__);

	int i = 0;
	int ssid_found =false;

	/* set initial wifi configuration */
	while(1){
		if (wifi_init(param)== WIFI_SUCCESS) {
			nrc_usr_print ("[%s] wifi_init Success !! \n", __func__);
			break;
		} else {
			nrc_usr_print ("[%s] wifi_init Failed !! \n", __func__);
			_delay_ms(1000);
		}
	}

	/* find AP */
	while(1){
		if (nrc_wifi_scan() == WIFI_SUCCESS){
			if (nrc_wifi_scan_results(&results)== WIFI_SUCCESS) {
				/* Find the ssid in scan results */
				for(i=0; i<results.n_result ; i++){
					if(strcmp((char*)param->ssid, (char*)results.result[i].ssid)== 0 ){
						ssid_found = true;
						break;
					}
				}

				if(ssid_found){
					nrc_usr_print ("[%s] %s is found \n", __func__, param->ssid);
					break;
				}
			}
		} else {
			nrc_usr_print ("[%s] Scan fail !! \n", __func__);
			_delay_ms(1000);
		}
	}

	/* connect to AP */
	while(1) {
		if (wifi_connect(param)== WIFI_SUCCESS) {
			nrc_usr_print ("[%s] connect to %s successfully !! \n", __func__, param->ssid);
			break;
		} else{
			nrc_usr_print ("[%s] Fail for connection %s\n", __func__, param->ssid);
			_delay_ms(1000);
		}
	}

	nrc_wifi_get_network_index(&network_index );

	/* check the IP is ready */
	while(1){
		nrc_wifi_get_state(&wifi_state);
		if (wifi_state == WIFI_STATE_GET_IP) {
			nrc_usr_print("[%s] IP ...\n",__func__);
			break;
		} else{
			nrc_usr_print("[%s] Current State : %d...\n",__func__, wifi_state);
		}
		_delay_ms(1000);
	}

	if(gpio_init() != 0)  {
		nrc_usr_print ("[%s] Fail to init GPIO\n", __func__);
		return NRC_FAIL;
	}
	clock_offset = sys_now();
	cert_file = (char *)server_cert_file;
	ca_file = (char *)server_ca_file;
	root_ca_file = NULL;
	coap_server_quit = 0;

	coap_startup();
	coap_dtls_set_log_level(log_level);
	coap_set_log_level(log_level);
	ctx = get_context(addr_str, port_str);

	if (!ctx){
		coap_log(LOG_CRIT, "[%s] cannot get context\n", __func__);
		return NRC_FAIL;
	}

	init_resources(ctx);
	wait_ms = COAP_RESOURCE_CHECK_TIME * 1000;

	while ( !coap_server_quit )
	{
		int ret = coap_run_once( ctx, wait_ms );

		if ( ret < 0 ) {
			return NRC_FAIL;
		} else if ( ret && (unsigned)ret < wait_ms ) {
			wait_ms -= ret;
		} else {
			time_t t_now = sys_now();
			if (t_last != t_now) {
				int i;
				t_last = t_now;
				if (time_resource) {
					coap_resource_notify_observers(time_resource, NULL);
				}
				for (i = 0; i < dynamic_count; i++) {
					coap_resource_notify_observers(dynamic_entry[i].resource, NULL);
				}
			}

			if (ret) {
				wait_ms = COAP_RESOURCE_CHECK_TIME * 1000;

			}
		}
	}
	coap_free_context(ctx);
	coap_cleanup();
	coap_server_quit = 0;

	nrc_usr_print("[%s] End of run_sample_coap_server!! \n",__func__);

	return NRC_SUCCESS;
}


/******************************************************************************
 * FunctionName : user_init
 * Description  : Start Code for User Application, Initialize User function
 * Parameters   : none
 * Returns      : none
 *******************************************************************************/
void user_init(void)
{
	nrc_err_t ret;
	WIFI_CONFIG* param;

	nrc_uart_console_enable(true);

	param = nrc_mem_malloc(WIFI_CONFIG_SIZE);
	memset(param, 0x0, WIFI_CONFIG_SIZE);

	set_wifi_config(param);
	ret = run_sample_coap_server(param);
	nrc_usr_print("[%s] test result!! %s \n",__func__, (ret==0) ?  "Success" : "Fail");
	if(param){
		nrc_mem_free(param);
	}
}

