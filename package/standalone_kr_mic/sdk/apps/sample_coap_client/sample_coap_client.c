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


#define COAP_SECURE_ENABLE	1 /* for 'coaps', default is get 'coap' */
#define COAP_GET_AND_PUT_LIGHT_TEST 1 /* Enable get & put 'light' test, default is get 'time' test */
#define COAP_CLIENT_PUT_REQUEST_PERIOD	4

#ifdef COAP_SECURE_ENABLE
#define COAP_DEFAULT_URI_SCHEME		"coaps"
#else
#define COAP_DEFAULT_URI_SCHEME		"coap"
#endif

#ifdef COAP_GET_AND_PUT_LIGHT_TEST
#define URI_RESOURCE		"light"
#else
#define URI_RESOURCE		"time"
#endif

#define COAP_METHOD_GET	"get"
#define COAP_METHOD_PUT	"put"

#define LIGHT_ON	"1"
#define LIGHT_OFF	"0"

#define BUFSIZE			256
#define MAX_USER			128		/* Maximum length of a user name in bytes. */
#define MAX_KEY			64		/* Maximum length of a key in bytes. */

int flags = 0;

typedef unsigned char method_t;
method_t method = 1;				/* the method we are using in our requests */
coap_block_t block = { .num = 0, .m = 0, .szx = 6 };
uint16_t last_block1_tid = 0;

static unsigned char _token_data[8];
coap_binary_t the_token = { 0, _token_data };
#define FLAGS_BLOCK 0x01
static coap_optlist_t *optlist = NULL;
static coap_uri_t uri;

static int ready = 0;				/* reading is done when this flag is set */
static coap_string_t proxy = { 0, NULL };
static uint16_t proxy_port = COAP_DEFAULT_PORT;
static unsigned int ping_seconds = 0;

static coap_string_t payload = { 0, NULL };		/* optional payload to send */
static int reliable = 0;
unsigned char msgtype = COAP_MESSAGE_CON;		/* usually, requests are sent confirmable */

static unsigned int wait_seconds = 30;			/* default timeout in seconds */
static unsigned int wait_ms = 0;
static int wait_ms_reset = 0;
static int obs_started = 0;
static unsigned int obs_seconds = 30;			/* default observe time */
static unsigned int obs_ms = 0;				/* timeout for current subscription */
static int obs_ms_reset = 0;
static int quit = 0;
static unsigned char RECV_DATA[BUFSIZE];

static const char client_cert_file[] =
"-----BEGIN CERTIFICATE-----\r\n"
"MIIDOjCCAqOgAwIBAgIJANTlt2wY/CcpMA0GCSqGSIb3DQEBCwUAMGoxFzAVBgNV\r\n"
"BAMMDkFuIENvQVAgYnJva2VyMRYwFAYDVQQKDA1Pd25UcmFja3Mub3JnMRQwEgYD\r\n"
"VQQLDAtnZW5lcmF0ZS1DQTEhMB8GCSqGSIb3DQEJARYSbm9ib2R5QGV4YW1wbGUu\r\n"
"bmV0MB4XDTE5MDcwNDAxMzMzMloXDTMyMDYzMDAxMzMzMlowDjEMMAoGA1UEAwwD\r\n"
"Y2xpMIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQC/Kw1rRtjtH2uRPcvdqKQj\r\n"
"w8c+H3NIkmkSQ6UjpA45Vm6CC0jv3OD9k5136xkAvj4dJlFq0hP4rxmnJQcOd7xj\r\n"
"w5563244ADdJ4EKefZZ/kHyr9ZiK+wHR/TaJ10Ajo5g08RQMz/64FOoXJXIKcyMV\r\n"
"o7VJ4jPdCrVlht0NoebKNwIDAQABo4IBQjCCAT4wDAYDVR0TAQH/BAIwADAJBgNV\r\n"
"HREEAjAAMBEGCWCGSAGG+EIBAQQEAwIFoDAdBgNVHSUEFjAUBggrBgEFBQcDAgYI\r\n"
"KwYBBQUHAwQwCwYDVR0PBAQDAgOoMCYGCWCGSAGG+EIBDQQZFhdDb2FwIENsaWVu\r\n"
"dCBDZXJ0aWZpY2F0ZTAdBgNVHQ4EFgQUbdCbfqHFhuObPgiJPMQVTeXyI98wgZwG\r\n"
"A1UdIwSBlDCBkYAUWKBNMv+7l/RzApqnudKvbF41RsuhbqRsMGoxFzAVBgNVBAMM\r\n"
"DkFuIENvQVAgYnJva2VyMRYwFAYDVQQKDA1Pd25UcmFja3Mub3JnMRQwEgYDVQQL\r\n"
"DAtnZW5lcmF0ZS1DQTEhMB8GCSqGSIb3DQEJARYSbm9ib2R5QGV4YW1wbGUubmV0\r\n"
"ggkA2pYsZ/BbUWkwDQYJKoZIhvcNAQELBQADgYEAxYzOU0/C2OCGMvZieRR1/ljI\r\n"
"txH5LFVc1+K/Po9fWff0A+VPDM9HDQFdNV7Zwg8cPyqIyA373ACWmzy/LWai6Bcz\r\n"
"3hlcnuInvOxpdh1Bi0CxESLIY0/cLlidvlZzRoiBCCGFBoAomccrkUePLS0qCxO1\r\n"
"5x9XNs8iAwhmSBxsAcw=\r\n"
"-----END CERTIFICATE-----\r\n"
"-----BEGIN RSA PRIVATE KEY-----\r\n"
"MIICXgIBAAKBgQC/Kw1rRtjtH2uRPcvdqKQjw8c+H3NIkmkSQ6UjpA45Vm6CC0jv\r\n"
"3OD9k5136xkAvj4dJlFq0hP4rxmnJQcOd7xjw5563244ADdJ4EKefZZ/kHyr9ZiK\r\n"
"+wHR/TaJ10Ajo5g08RQMz/64FOoXJXIKcyMVo7VJ4jPdCrVlht0NoebKNwIDAQAB\r\n"
"AoGAaBalA9hbnSESjM69BkAgv2iGQAkX2Ff/5fX3IOTe6dFp8lz6pb/6sZeCkhzs\r\n"
"TD9Jys1mX2drgGi26w96PCJt7GKhaATT771FF+6UVBevP+4hmVV2mOnsMPEq0xkC\r\n"
"hOMNc6kBd+LPN5ZaNifHDGsw0DQDeWkpJyWjiKD35UoTQuECQQDvCxHAIm9fflpd\r\n"
"QJQ0u70KdA9UMB5HzPmMeXjflZht6SA5t2AY6fsZ18JQtt74R1de+a4klNI0ybrm\r\n"
"wEiyhfe9AkEAzLqXP/tB+djlCjkli8dzMm4kr8GujewFtqT896A6axItuMnqS6X7\r\n"
"WvY76IXhHJUC2MeQhBEUUGfvHreDLXkfAwJBAMyBZ9o8m5PTAXQuP50t0HkU+dhI\r\n"
"ol9DGWv4sFR4I0RE4fD+1RP7eXuOfwuWqAM45aK3cSiSuZO30lJ/xrIUsvECQQCX\r\n"
"/Em+4PvYzNE0Zrpd68K0hJpNtF6u07JmJlP6TYQw6rzwu01pvZ49qCFPfNxiyKjl\r\n"
"YDoEIV3QV2hoiFolDw8bAkEAisyD7ytMtKMe8tnzf2GZ9wd78bF/QTKAXoCkgPYe\r\n"
"cKnMYQJ7ToCuQgJYGdmJcRq6qJPp4qcEm4SmqefgupNLcA==\r\n"
"-----END RSA PRIVATE KEY-----\r\n";

static const char client_ca_file[] =
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

static char *cert_file = NULL; /* Combined certificate and private key in PEM */
static char *ca_file = NULL;   /* CA for cert_file - for cert checking in PEM */
static char *root_ca_file = NULL; /* List of trusted Root CAs in PEM */

static int resolve_address(const coap_str_const_t *server, struct sockaddr *dst)
{
	struct addrinfo *res, *ainfo;
	struct addrinfo hints;
	static char addrstr[256];
	int error, len=-1;

	memset(addrstr, 0, sizeof(addrstr));
	if (server->length)
		memcpy(addrstr, server->s, server->length);
	else
		memcpy(addrstr, "localhost", 9);

	memset ((char *)&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_family = AF_UNSPEC;

	error = getaddrinfo(addrstr, NULL, &hints, &res);

	if (error != 0) {
		return error;
	}

	for (ainfo = res; ainfo != NULL; ainfo = ainfo->ai_next) {
		switch (ainfo->ai_family) {
			#if CONFIG_IPV6
			case AF_INET6:
			#endif
			case AF_INET:
				len = ainfo->ai_addrlen;
				memcpy(dst, ainfo->ai_addr, len);
				goto finish;
			default:
				;
		}
	}

finish:
	freeaddrinfo(res);
	return len;
}

static int verify_cn_callback(const char *cn,
				const uint8_t *asn1_public_cert UNUSED_PARAM,
				size_t asn1_length UNUSED_PARAM,
				coap_session_t *session UNUSED_PARAM,
				unsigned depth,
				int validated UNUSED_PARAM,
				void *arg UNUSED_PARAM)
{
	coap_log(LOG_INFO, "CN '%s' presented by server (%s)\n",
		cn, depth ? "CA" : "Certificate");
	return 1;
}

static coap_dtls_pki_t * setup_pki(coap_context_t *ctx)
{
	static coap_dtls_pki_t dtls_pki;
	static char client_sni[256];

	memset (&dtls_pki, 0, sizeof(dtls_pki));
	dtls_pki.version = COAP_DTLS_PKI_SETUP_VERSION;
	if (ca_file || root_ca_file) {
		dtls_pki.verify_peer_cert        = 1;
		dtls_pki.require_peer_cert       = 1;
		dtls_pki.allow_self_signed       = 1;
		dtls_pki.allow_expired_certs     = 1;
		dtls_pki.cert_chain_validation   = 1;
		dtls_pki.cert_chain_verify_depth = 2;
		dtls_pki.check_cert_revocation   = 1;
		dtls_pki.allow_no_crl            = 1;
		dtls_pki.allow_expired_crl       = 1;
		dtls_pki.validate_cn_call_back   = verify_cn_callback;
		dtls_pki.cn_call_back_arg        = NULL;
		dtls_pki.validate_sni_call_back  = NULL;
		dtls_pki.sni_call_back_arg       = NULL;
		memset(client_sni, 0, sizeof(client_sni));
		if (uri.host.length)
			memcpy(client_sni, uri.host.s, min(uri.host.length, sizeof(client_sni)));
		else
			memcpy(client_sni, "localhost", 9);
		dtls_pki.client_sni = client_sni;
	}
	dtls_pki.pki_key.key_type = COAP_PKI_KEY_PEM;
	dtls_pki.pki_key.key.pem.public_cert = cert_file;
	dtls_pki.pki_key.key.pem.private_key = cert_file;
	dtls_pki.pki_key.key.pem.ca_file = ca_file;
	return &dtls_pki;
}

static coap_session_t * get_session(
	coap_context_t *ctx,
	const char *local_addr,
	const char *local_port,
	coap_proto_t proto,
	coap_address_t *dst,
	const char *identity,
	const uint8_t *key,
	unsigned key_len)
{
	coap_session_t *session = NULL;

	if ( local_addr ) {
		int s;
		struct addrinfo hints;
		struct addrinfo *result = NULL, *rp;

		memset( &hints, 0, sizeof( struct addrinfo ) );
		hints.ai_family = AF_INET;	/* Allow IPv4*/
		hints.ai_socktype = COAP_PROTO_RELIABLE(proto) ? SOCK_STREAM : SOCK_DGRAM; /* Coap uses UDP */
		hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST | AI_NUMERICSERV | AI_ALL;

		s = getaddrinfo( local_addr, local_port, &hints, &result );
		if ( s != 0 ) {
			return NULL;
		}

		/* iterate through results until success */
		for ( rp = result; rp != NULL; rp = rp->ai_next ) {
			coap_address_t bind_addr;
			if ( rp->ai_addrlen <= sizeof( bind_addr.addr ) ) {
				coap_address_init( &bind_addr );
				bind_addr.size = rp->ai_addrlen;
				memcpy( &bind_addr.addr, rp->ai_addr, rp->ai_addrlen );
				if ((root_ca_file || ca_file || cert_file) &&
					(proto == COAP_PROTO_DTLS || proto == COAP_PROTO_TLS)) {
					coap_dtls_pki_t *dtls_pki = setup_pki(ctx);
					session = coap_new_client_session_pki(ctx, &bind_addr, dst, proto, dtls_pki);
				}
				else if ((identity || key)&&(proto == COAP_PROTO_DTLS || proto == COAP_PROTO_TLS) ) {
					session = coap_new_client_session_psk( ctx, &bind_addr, dst, proto,
					identity, key, key_len );
				}
				else {
					session = coap_new_client_session( ctx, &bind_addr, dst, proto );
				}
				if ( session )
					break;
			}
		}
		freeaddrinfo( result );
	} else {
		if ((root_ca_file || ca_file || cert_file)&&(proto == COAP_PROTO_DTLS || proto == COAP_PROTO_TLS)) {
			coap_dtls_pki_t *dtls_pki = setup_pki(ctx);
			session = coap_new_client_session_pki(ctx, NULL, dst, proto, dtls_pki);
		}
		else if ((identity || key) && (proto == COAP_PROTO_DTLS || proto == COAP_PROTO_TLS) ) {
			session = coap_new_client_session_psk( ctx, NULL, dst, proto, identity, key, key_len );
		}else {
			session = coap_new_client_session( ctx, NULL, dst, proto );
		}
	}
	return session;
}

static coap_pdu_t *
coap_new_request(coap_context_t *ctx,
	coap_session_t *session,
	method_t m,
	coap_optlist_t **options,
	unsigned char *data,
	size_t length)
{
	coap_pdu_t *pdu;
	(void)ctx;

	if (!(pdu = coap_new_pdu(session)))
		return NULL;

	pdu->type = msgtype;
	pdu->tid = coap_new_message_id(session);
	pdu->code = m;

	if ( !coap_add_token(pdu, the_token.length, the_token.s)) {
		coap_log(LOG_DEBUG, "cannot add token to request\n");
	}

	if (options)
		coap_add_optlist_pdu(pdu, options);

	if (length) {
		if ((flags & FLAGS_BLOCK) == 0)
			coap_add_data(pdu, length, data);
		else
			coap_add_block(pdu, length, data, block.num, block.szx);
	}

	return pdu;
}


static coap_tid_t clear_obs(coap_context_t *ctx, coap_session_t *session)
{
	coap_pdu_t *pdu;
	coap_optlist_t *option;
	coap_tid_t tid = COAP_INVALID_TID;
	unsigned char buf[2];
	(void)ctx;

	/* create bare PDU w/o any option  */
	pdu = coap_pdu_init(msgtype,COAP_REQUEST_GET, coap_new_message_id(session),
			coap_session_max_pdu_size(session));

	if (!pdu) {
		return tid;
	}

	if (!coap_add_token(pdu, the_token.length, the_token.s)) {
		coap_log(LOG_CRIT, "cannot add token\n");
		goto error;
	}

	for (option = optlist; option; option = option->next ) {
		if (option->number == COAP_OPTION_URI_HOST) {
			if (!coap_add_option(pdu, option->number, option->length,	option->data)) {
				goto error;
			}
			break;
		}
	}

	if (!coap_add_option(pdu, COAP_OPTION_OBSERVE,
			coap_encode_var_safe(buf, sizeof(buf), COAP_OBSERVE_CANCEL),	buf)) {
		coap_log(LOG_CRIT, "cannot add option Observe: %u\n", COAP_OBSERVE_CANCEL);
		goto error;
	}

	for (option = optlist; option; option = option->next ) {
		switch (option->number) {
			case COAP_OPTION_URI_PORT :
			case COAP_OPTION_URI_PATH :
			case COAP_OPTION_URI_QUERY :
				if (!coap_add_option(pdu, option->number, option->length,	option->data)) {
					goto error;
				}
				break;
			default:
				;
		}
	}

	if (flags & FLAGS_BLOCK) {
		block.num = 0;
		block.m = 0;
		coap_add_option(pdu, COAP_OPTION_BLOCK2,
			coap_encode_var_safe(buf, sizeof(buf), (block.num << 4 | block.m << 3 | block.szx)),buf);
	}

	if (coap_get_log_level() < LOG_DEBUG)
		coap_show_pdu(LOG_INFO, pdu);

	tid = coap_send(session, pdu);

	if (tid == COAP_INVALID_TID)
		coap_log(LOG_DEBUG, "clear_obs: error sending new request\n");

	return tid;
error:

	coap_delete_pdu(pdu);
	return tid;
}


static int event_handler(coap_context_t *ctx UNUSED_PARAM,
			coap_event_t event,struct coap_session_t *session UNUSED_PARAM)
{
	switch(event) {
		case COAP_EVENT_DTLS_CLOSED:
		case COAP_EVENT_TCP_CLOSED:
		case COAP_EVENT_SESSION_CLOSED:
			quit = 1;
			break;
		default:
			break;
	}
	return 0;
}

static inline int check_token(coap_pdu_t *received)
{
	return received->token_length == the_token.length &&
		memcmp(received->token, the_token.s, the_token.length) == 0;
}


static void
message_handler(struct coap_context_t *ctx,
				coap_session_t *session,
				coap_pdu_t *sent,
				coap_pdu_t *received,
				const coap_tid_t id UNUSED_PARAM)
{
	coap_pdu_t *pdu = NULL;
	coap_opt_t *block_opt;
	coap_opt_iterator_t opt_iter;
	unsigned char buf[4];
	coap_optlist_t *option;
	size_t len;
	unsigned char *databuf;
	coap_tid_t tid;

#ifndef NDEBUG
	coap_log(LOG_DEBUG, "** process incoming %d.%02d response:\n",
		(received->code >> 5), received->code & 0x1F);
	if (coap_get_log_level() < LOG_DEBUG)
		coap_show_pdu(LOG_INFO, received);
#endif

	/* check if this is a response to our original request */
	if (!check_token(received)) {
		/* drop if this was just some message, or send RST in case of notification */
		if (!sent && (received->type == COAP_MESSAGE_CON ||	received->type == COAP_MESSAGE_NON))
			coap_send_rst(session, received);
		return;
	}

	if (received->type == COAP_MESSAGE_RST) {
		coap_log(LOG_INFO, "got RST\n");
		return;
	}
	/* output the received data, if any */
	coap_get_data(received, &len, &databuf);
	memcpy(RECV_DATA, databuf, len);

	if (COAP_RESPONSE_CLASS(received->code) == 2) {
		/* set obs timer if we have successfully subscribed a resource */
		if (!obs_started && coap_check_option(received, COAP_OPTION_OBSERVE, &opt_iter)) {
			coap_log(LOG_DEBUG, "observation relationship established, set timeout to %d\n", obs_seconds);
			obs_started = 1;
			obs_ms = obs_seconds * 1000;
			obs_ms_reset = 1;
		}
		/* Got some data, check if block option is set. Behavior is undefined if
		* both, Block1 and Block2 are present. */
		block_opt = coap_check_option(received, COAP_OPTION_BLOCK2, &opt_iter);
		if (block_opt) { /* handle Block2 */
			uint16_t blktype = opt_iter.type;

			memset(RECV_DATA, 0, sizeof(RECV_DATA));

			/* There is no block option set, just read the data and we are done. */
			if (coap_get_data(received, &len, &databuf)) {
				memcpy(RECV_DATA, databuf, len);
				nrc_usr_print("Recv_DATA: %s\n", RECV_DATA);
				ready = 1;
			}

			if (coap_opt_block_num(block_opt) == 0) {
				/* See if observe is set in first response */
				ready = coap_check_option(received,
				COAP_OPTION_OBSERVE, &opt_iter) == NULL;
			}
			if(COAP_OPT_BLOCK_MORE(block_opt)) {
				/* more bit is set */
				coap_log(LOG_DEBUG, "found the M bit, block size is %u, block nr. %u\n",
				COAP_OPT_BLOCK_SZX(block_opt),
				coap_opt_block_num(block_opt));

				/* create pdu with request for next block */
				pdu = coap_new_request(ctx, session, method, NULL, NULL, 0); /* first, create bare PDU w/o any option  */
				if ( pdu ) {
					/* add URI components from optlist */
					for (option = optlist; option; option = option->next ) {
						switch (option->number) {
							case COAP_OPTION_URI_HOST :
							case COAP_OPTION_URI_PORT :
							case COAP_OPTION_URI_PATH :
							case COAP_OPTION_URI_QUERY :
								coap_add_option(pdu, option->number, option->length,
								option->data);
								break;
							default:
								;	 /* skip other options */
						}
					}

					/* finally add updated block option from response, clear M bit */
					/* blocknr = (blocknr & 0xfffffff7) + 0x10; */
					coap_log(LOG_DEBUG, "query block %d\n", (coap_opt_block_num(block_opt) + 1));
					coap_add_option(pdu, blktype,coap_encode_var_safe(buf, sizeof(buf),
						((coap_opt_block_num(block_opt) + 1) << 4)|COAP_OPT_BLOCK_SZX(block_opt)), buf);

					tid = coap_send(session, pdu);

					if (tid == COAP_INVALID_TID) {
						coap_log(LOG_DEBUG, "message_handler: error sending new request\n");
					} else {
						wait_ms = wait_seconds * 1000;
						wait_ms_reset = 1;
					}

					return;
				}
			}
			return;
		} else { /* no Block2 option */
			block_opt = coap_check_option(received, COAP_OPTION_BLOCK1, &opt_iter);

			if (block_opt) { /* handle Block1 */
				unsigned int szx = COAP_OPT_BLOCK_SZX(block_opt);
				unsigned int num = coap_opt_block_num(block_opt);
				coap_log(LOG_DEBUG, "found Block1 option, block size is %u, block nr. %u\n", szx, num);

				if (szx != block.szx) {
					unsigned int bytes_sent = ((block.num + 1) << (block.szx + 4));
					if (bytes_sent % (1 << (szx + 4)) == 0) {
						/* Recompute the block number of the previous packet given the new block size */
						num = block.num = (bytes_sent >> (szx + 4)) - 1;
						block.szx = szx;
						coap_log(LOG_DEBUG,	"new Block1 size is %u, block number %u completed\n",
							(1 << (block.szx + 4)), block.num);
					} else {
						coap_log(LOG_DEBUG, "ignoring request to increase Block1 size, "
							"next block is not aligned on requested block size boundary. "
							"(%u x %u mod %u = %u != 0)\n", block.num + 1, (1 << (block.szx + 4)),
							(1 << (szx + 4)), 	bytes_sent % (1 << (szx + 4)));
					}
				}

				if (payload.length <= (block.num+1) * (1 << (block.szx + 4))) {
					coap_log(LOG_DEBUG, "upload ready\n");
					ready = 1;
					return;
				}

				if (last_block1_tid == received->tid) {
					/*
					* Duplicate BLOCK1 ACK
					*
					* RFCs not clear here, but on a lossy connection, there could
					* be multiple BLOCK1 ACKs, causing the client to retransmit the
					* same block multiple times.
					*
					* Once a block has been ACKd, there is no need to retransmit it.
					*/
					return;
				}
				last_block1_tid = received->tid;

				/* create pdu with request for next block */
				pdu = coap_new_request(ctx, session, method, NULL, NULL, 0); /* first, create bare PDU w/o any option  */
				if (pdu) {
					/* add URI components from optlist */
					for (option = optlist; option; option = option->next ) {
						switch (option->number) {
						case COAP_OPTION_URI_HOST :
						case COAP_OPTION_URI_PORT :
						case COAP_OPTION_URI_PATH :
						case COAP_OPTION_CONTENT_FORMAT :
						case COAP_OPTION_URI_QUERY :
							coap_add_option(pdu, option->number, option->length, option->data);
							break;
						default:
							;	 /* skip other options */
						}
					}

					/* finally add updated block option from response, clear M bit */
					/* blocknr = (blocknr & 0xfffffff7) + 0x10; */
					block.num = num + 1;
					block.m = ((block.num+1) * (1 << (block.szx + 4)) < payload.length);

					coap_log(LOG_DEBUG, "send block %d\n", block.num);
					coap_add_option(pdu, COAP_OPTION_BLOCK1, coap_encode_var_safe(buf, sizeof(buf),
						(block.num << 4) | (block.m << 3) | block.szx), buf);

					coap_add_block(pdu, payload.length, payload.s, block.num, block.szx);
					if (coap_get_log_level() < LOG_DEBUG)
						coap_show_pdu(LOG_INFO, pdu);

					tid = coap_send(session, pdu);

					if (tid == COAP_INVALID_TID) {
						coap_log(LOG_DEBUG, "message_handler: error sending new request\n");
					} else {
						wait_ms = wait_seconds * 1000;
						wait_ms_reset = 1;
					}

					return;
				}
			} else {
				/* There is no block option set, just read the data and we are done. */
				memset(RECV_DATA, 0, sizeof(RECV_DATA));

				/* There is no block option set, just read the data and we are done. */
				if (coap_get_data(received, &len, &databuf)) {
					memcpy(RECV_DATA, databuf, len);
				}
			}
		}
		ready = 1;
	} else {	  /* no 2.05 */
		/* check if an error was signaled and output payload if so */
		if (COAP_RESPONSE_CLASS(received->code) >= 4) {
			;
		}
	}

	/* any pdu that has been created in this function must be sent by now */
	if(pdu == NULL)
		return;

	/* our job is done, we can exit at any time */
	ready = coap_check_option(received, COAP_OPTION_OBSERVE, &opt_iter) == NULL;
}


static method_t
cmdline_method(char *arg) {
	static const char *methods[] =
		{ 0, "get", "post", "put", "delete", "fetch", "patch", "ipatch", 0};
	unsigned char i;

	for (i=1; methods[i] && strcasecmp(arg,methods[i]) != 0 ; ++i)
		;

	return i;	 /* note that we do not prevent illegal methods */
}

static uint16_t
get_default_port(const coap_uri_t *u)
{
	return coap_uri_scheme_is_secure(u) ? COAPS_DEFAULT_PORT : COAP_DEFAULT_PORT;
}


static int
cmdline_uri(char *arg, int create_uri_opts) {
	unsigned char portbuf[2];
	unsigned char _buf[BUFSIZE];
	unsigned char *buf = _buf;
	size_t buflen;
	int res;

	if (proxy.length) {   /* create Proxy-Uri from argument */
		size_t len = strlen(arg);
		while (len > 270) {
			coap_insert_optlist(&optlist, coap_new_optlist(COAP_OPTION_PROXY_URI,
				270, (unsigned char *)arg));

			len -= 270;
			arg += 270;
		}

		coap_insert_optlist(&optlist, coap_new_optlist(COAP_OPTION_PROXY_URI,
			len, (unsigned char *)arg));

	} else {	  /* split arg into Uri-* options */
		if (coap_split_uri((unsigned char *)arg, strlen(arg), &uri) < 0) {
			coap_log(LOG_ERR, "invalid CoAP URI\n");
			return -1;
		}

		if (uri.scheme==COAP_URI_SCHEME_COAPS && !reliable && !coap_dtls_is_supported()) {
			coap_log(LOG_EMERG, "coaps URI scheme not supported in this version of libcoap\n");
			return -1;
		}

		if ((uri.scheme==COAP_URI_SCHEME_COAPS_TCP ||
				(uri.scheme==COAP_URI_SCHEME_COAPS && reliable)) && !coap_tls_is_supported()) {
			coap_log(LOG_EMERG, "coaps+tcp URI scheme not supported in this version of libcoap\n");
			return -1;
		}

		if (uri.port != get_default_port(&uri) && create_uri_opts) {
			coap_insert_optlist(&optlist, coap_new_optlist(COAP_OPTION_URI_PORT,
			coap_encode_var_safe(portbuf, sizeof(portbuf),(uri.port & 0xffff)), portbuf));
		}

		if (uri.path.length) {
			buflen = BUFSIZE;
			res = coap_split_path(uri.path.s, uri.path.length, buf, &buflen);


			while (res--) {
				coap_insert_optlist(&optlist, coap_new_optlist(COAP_OPTION_URI_PATH,
					coap_opt_length(buf),coap_opt_value(buf)));

				buf += coap_opt_size(buf);
			}
		}

		if (uri.query.length) {
			buflen = BUFSIZE;
			buf = _buf;
			res = coap_split_query(uri.query.s, uri.query.length, buf, &buflen);

			while (res--) {
				coap_insert_optlist(&optlist, coap_new_optlist(COAP_OPTION_URI_QUERY,
					coap_opt_length(buf), coap_opt_value(buf)));

				buf += coap_opt_size(buf);
			}
		}
	}

	return 0;
}

#define hexchar_to_dec(c) ((c) & 0x40 ? ((c) & 0x0F) + 9 : ((c) & 0x0F))

static void
decode_segment(const uint8_t *seg, size_t length, unsigned char *buf)
{
	while (length--) {
		if (*seg == '%') {
			*buf = (hexchar_to_dec(seg[1]) << 4) + hexchar_to_dec(seg[2]);
			seg += 2; length -= 2;
		} else {
			*buf = *seg;
		}
		++buf; ++seg;
	}
}

static int
check_segment(const uint8_t *s, size_t length)
{
	size_t n = 0;

	while (length) {
		if (*s == '%') {
			if (length < 2 || !(isxdigit(s[1]) && isxdigit(s[2])))
				return -1;

			s += 2;
			length -= 2;
		}
		++s; ++n; --length;
	}
	return n;
}

static int
cmdline_input(char *text, coap_string_t *buf)
{
	int len;
	len = check_segment((unsigned char *)text, strlen(text));

	if (len < 0)
		return 0;

	buf->s = (unsigned char *)coap_malloc(len);
	if (!buf->s)
		return 0;

	buf->length = len;
	decode_segment((unsigned char *)text, strlen(text), buf->s);
	return 1;
}

/******************************************************************************
 * FunctionName : run_sample_coap_client
 * Description  : sample test for coap client
 * Parameters   : WIFI_CONFIG
 * Returns      : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
nrc_err_t run_sample_coap_client(WIFI_CONFIG *param)
{
	coap_context_t  *ctx = NULL;
	coap_session_t *session = NULL;
	coap_address_t dst;
#if CONFIG_IPV6
	static char addr[INET6_ADDRSTRLEN];
#else
	static char addr[INET_ADDRSTRLEN];
#endif
	void *addrptr = NULL;
	SCAN_RESULTS results;
	int ret = -1;
	coap_pdu_t  *pdu;
	static coap_str_const_t server;
	uint16_t port = COAP_DEFAULT_PORT;
	char port_str[NI_MAXSERV] = "0";
	char node_str[NI_MAXHOST] = "";
	int opt, res;
	coap_log_t log_level = LOG_WARNING;
	unsigned char user[MAX_USER + 1], key[MAX_KEY];
	ssize_t user_length = 0, key_length = 0;
	int create_uri_opts = 1;

	coap_tid_t tid = COAP_INVALID_TID;
	fd_set readfds;
	struct timeval tv;
	coap_tick_t now;
	coap_queue_t *nextpdu;
	char input[BUFSIZE];
#ifdef COAP_GET_AND_PUT_LIGHT_TEST
	int light_value = 0;
	int light_test_count=0;
#endif
	const char *methods_table[] =
		{ 0, "get", "post", "put", "delete", "fetch", "patch", "ipatch", 0};

	int count = 0;
	int interval = 0;
  	int network_index = 0;
	tWIFI_STATE_ID wifi_state = WIFI_STATE_INIT;

	nrc_usr_print("[%s] Sample App for libCoap_Client\n",__func__);
	nrc_usr_print("[%s] [CoAP Client]\n",__func__);

	count = param->count;
	interval = param->interval;

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

	cert_file = (char *)client_cert_file;
	ca_file = (char *)client_ca_file;

	memset(RECV_DATA, 0, sizeof(RECV_DATA));
	memset(&uri, 0, sizeof(uri));
	memset(user, 0, sizeof(user));
	memset(key, 0, sizeof(key));
	memset(input, 0, sizeof(input));

	for(i=0; i<count; i++) {
#ifdef COAP_GET_AND_PUT_LIGHT_TEST
		if(light_test_count == 0){
			method = cmdline_method(COAP_METHOD_PUT);
			light_value = (light_value + 1) %2;
			if(light_value){
				cmdline_input(LIGHT_ON, &payload);
			}else{
				cmdline_input(LIGHT_OFF, &payload);
			}
		}else{
			method = cmdline_method(COAP_METHOD_GET);
		}
		light_test_count = (light_test_count+1) % COAP_CLIENT_PUT_REQUEST_PERIOD;
#else
		method = cmdline_method(COAP_METHOD_GET);
#endif
		sprintf(input, "%s://%s/%s", COAP_DEFAULT_URI_SCHEME, (char *)param->remote_addr, URI_RESOURCE);
		nrc_usr_print("[%s] %s\n", methods_table[method], input);

		coap_startup();
		coap_dtls_set_log_level(log_level);
		coap_set_log_level(log_level);

		if(input[0] != '\0')
			cmdline_uri(input, 0);

		if ( ( user_length < 0 ) || ( key_length < 0 ) ) {
			coap_log( LOG_CRIT, "Invalid user name or key specified\n" );
			return NRC_FAIL;
		}

		if (proxy.length) {
			server.length = proxy.length;
			server.s = proxy.s;
			port = proxy_port;
		} else {
			server = uri.host;
			port =  uri.port;
		}

		//resolve destination address where server should be sent
		res = resolve_address(&server, &dst.addr.sa);

		if (res < 0) {
			coap_log( LOG_WARNING, "Resolv_address Error");
			return NRC_FAIL;
		}

		ctx = coap_new_context( NULL );
		if ( !ctx ) {
			coap_log( LOG_EMERG, "cannot create context\n" );
			return NRC_FAIL;
		}

		coap_context_set_keepalive(ctx, ping_seconds);
		dst.size = res;
		dst.addr.sin.sin_port = htons( port );

		session = get_session(
					ctx,
					node_str[0] ? node_str : NULL, port_str,
					uri.scheme==COAP_URI_SCHEME_COAP_TCP ? COAP_PROTO_TCP :
					uri.scheme==COAP_URI_SCHEME_COAPS_TCP ? COAP_PROTO_TLS :
					(reliable ?
						uri.scheme==COAP_URI_SCHEME_COAPS ? COAP_PROTO_TLS : COAP_PROTO_TCP
						: uri.scheme==COAP_URI_SCHEME_COAPS ? COAP_PROTO_DTLS : COAP_PROTO_UDP),
					&dst,
					user_length > 0 ? (const char *)user : NULL,
					key_length > 0  ? key : NULL, (unsigned)key_length
				);

		if ( !session ) {
			coap_log( LOG_EMERG, "cannot create client session\n" );
			return NRC_FAIL;
		}
		/* add Uri-Host if server address differs from uri.host */
		switch (dst.addr.sa.sa_family) {
			case AF_INET:
				addrptr = &dst.addr.sin.sin_addr;
				break;
		#if CONFIG_IPV6
			case AF_INET6:
				addrptr = &dst.addr.sin6.sin6_addr;
				break;
		#endif
			default:
				;
		}
		coap_register_option(ctx, COAP_OPTION_BLOCK2);
		coap_register_response_handler(ctx, message_handler);
		coap_register_event_handler(ctx, event_handler);

		if (! (pdu = coap_new_request(ctx, session, method, &optlist, payload.s, payload.length))) {
			coap_log( LOG_EMERG, "cannot create new request\n" );
			return NRC_FAIL;
		}

	#ifndef NDEBUG
		coap_log(LOG_DEBUG, "sending CoAP request:\n");
		if (coap_get_log_level() < LOG_DEBUG)
			coap_show_pdu(LOG_INFO, pdu);
	#endif

		coap_send(session, pdu);

		wait_ms = wait_seconds * 1000;
		coap_log(LOG_DEBUG, "timeout is set to %u seconds\n", wait_seconds);

		while(!(ready && coap_can_exit(ctx)))
		{
			ret = coap_run_once( ctx, wait_ms == 0 ?
								obs_ms : obs_ms == 0 ?
								min(wait_ms, 1000) : min( wait_ms, obs_ms ) );

			if ( ret >= 0 ) {
				if ( wait_ms > 0 && !wait_ms_reset ) {
					if ( (unsigned)ret >= wait_ms ) {
						coap_log(LOG_INFO, "timeout\n");
						return NRC_FAIL;
					} else {
						wait_ms -= ret;
					}
				}
				if ( obs_ms > 0 && !obs_ms_reset ) {
					if ( (unsigned)ret >= obs_ms ) {
						coap_log(LOG_DEBUG, "clear observation relationship\n" );
						clear_obs( ctx, session );
						obs_ms = 0;
						obs_seconds = 0;
					} else {
						obs_ms -= ret;
					}
				}
				wait_ms_reset = 0;
				obs_ms_reset = 0;
			}
		}


		if(RECV_DATA[0]!='\0'){
			nrc_usr_print("%s\n", RECV_DATA);
		}
		if(optlist != NULL){
			coap_delete_optlist(optlist);
			optlist = NULL;
		}
		if(session)
			coap_session_release( session );

		if(ctx)
			coap_free_context( ctx );

		if(payload.s){
			coap_free(payload.s);
			payload.s = NULL;
			payload.length = 0;
		}

		_delay_ms(interval);
	}

	nrc_wifi_get_state(&wifi_state);
	if (wifi_state == WIFI_STATE_GET_IP || wifi_state == WIFI_STATE_CONNECTED) {
		nrc_usr_print("[%s] Trying to DISCONNECT... for exit\n",__func__);
		if (nrc_wifi_disconnect(network_index) != WIFI_SUCCESS) {
			nrc_usr_print ("[%s] Fail for Wi-Fi disconnection (results:%d)\n", __func__);
			return NRC_FAIL;
		}
	}

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
	ret = run_sample_coap_client(param);
	nrc_usr_print("[%s] test result!! %s \n",__func__, (ret==0) ?  "Success" : "Fail");
	if(param){
		nrc_mem_free(param);
	}
}

