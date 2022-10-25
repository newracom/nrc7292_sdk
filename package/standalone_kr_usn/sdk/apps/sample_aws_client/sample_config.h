/*
 * AWS IoT Device SDK for Embedded C 202103.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef SAMPLE_CONFIG_H_
#define SAMPLE_CONFIG_H_

/**
 * @brief Details of the MQTT broker to connect to.
 *
 * @note Your AWS IoT Core endpoint can be found in the AWS IoT console under
 * Settings/Custom Endpoint, or using the describe-endpoint API.
 */
#define AWS_IOT_ENDPOINT               "a15l156ratjegc-ats.iot.ap-northeast-2.amazonaws.com"

/**
 * @brief AWS IoT MQTT broker port number.
 *
 * In general, port 8883 is for secured MQTT connections.
 *
 * @note Port 443 requires use of the ALPN TLS extension with the ALPN protocol
 * name. When using port 8883, ALPN is not required.
 */
#ifndef AWS_MQTT_PORT
    #define AWS_MQTT_PORT    ( 8883 )
#endif

/**
 * @brief The server's root CA certificate.
 *
 * This certificate is used to identify the AWS IoT server and is publicly
 * available. Refer to the AWS documentation available in the link below
 * https://docs.aws.amazon.com/iot/latest/developerguide/server-authentication.html#server-authentication-certs
 *
 * Amazon's root CA certificate is automatically downloaded to the certificates
 * directory from @ref https://www.amazontrust.com/repository/AmazonRootCA1.pem
 * using the CMake build system.
 *
 * @note This certificate should be PEM-encoded.
 */
#ifndef ROOT_CA_CERT
    #define ROOT_CA_CERT    "-----BEGIN CERTIFICATE-----\r\n\
MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\r\n\
ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\r\n\
b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\r\n\
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\r\n\
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\r\n\
ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\r\n\
9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\r\n\
IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\r\n\
VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\r\n\
93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\r\n\
jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\r\n\
AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\r\n\
A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\r\n\
U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\r\n\
N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\r\n\
o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\r\n\
5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\r\n\
rqXRfboQnoZsG4q5WTP468SQvvG5\r\n\
-----END CERTIFICATE-----\r\n"

#endif

/**
 * @brief Path of the file containing the client certificate.
 *
 * Refer to the AWS documentation below for details regarding client
 * authentication.
 * https://docs.aws.amazon.com/iot/latest/developerguide/client-authentication.html
 *
 * @note This certificate should be PEM-encoded.
 */
#define CLIENT_CERT    "-----BEGIN CERTIFICATE-----\r\n\
MIIDWjCCAkKgAwIBAgIVAKUnaT5MwzwiUbX9zJqeKVWrctjXMA0GCSqGSIb3DQEB\r\n\
CwUAME0xSzBJBgNVBAsMQkFtYXpvbiBXZWIgU2VydmljZXMgTz1BbWF6b24uY29t\r\n\
IEluYy4gTD1TZWF0dGxlIFNUPVdhc2hpbmd0b24gQz1VUzAeFw0yMTA0MjAwODI1\r\n\
MjBaFw00OTEyMzEyMzU5NTlaMB4xHDAaBgNVBAMME0FXUyBJb1QgQ2VydGlmaWNh\r\n\
dGUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDYzA1pBr/fGKc1+uIv\r\n\
yRxoa4IzuHYletjW7jmybUUULzEnx7oHKx14z3Ko5oZrgOGoMzMzFZNYkdeGITJd\r\n\
wRk2uRZIr0NF873FdSMtlYo40ugRetRLXzudFNbZz/ZhH0iigH8bg6Zxz8B1LejU\r\n\
w7WWeFm5H9T3qQqg8nhgQJtKOPbnNakJHWUiPeV+Fodd1qNdHLmBPq51YriyX1XW\r\n\
vZBoZnHgjfsOA6OWvOiSaubS8ZfNduJ8LQHZjoQs8jbuMOjfAwd9tE64UpWbY8vz\r\n\
JcVXbrrxROMTSdBcTj0iadDPt/Qal/NhlK8QYWBuih6KCqJ+LrNSzE22eIiUwVyb\r\n\
TaljAgMBAAGjYDBeMB8GA1UdIwQYMBaAFFYReiCqJcbAdDIv6AWmghCd/VCQMB0G\r\n\
A1UdDgQWBBTgi+N6djtxSBgKdAWbHHPwK2XnHDAMBgNVHRMBAf8EAjAAMA4GA1Ud\r\n\
DwEB/wQEAwIHgDANBgkqhkiG9w0BAQsFAAOCAQEAK0M+RTYl8z7rcvuPwj/7P8Pn\r\n\
ugZ9G4V9pcA6Fx/Tkny1LcyqEDHS+UHCATAbVThfJMZbN2HArtDwnJOeK3GCunw1\r\n\
lLfUZmVL7mbXZT45pQIztEgu6+hdi22VD9oeKAqaQ5wT1/SBr+Zp3+wQfDyoIDD9\r\n\
N+ssES6dbVv97jyogxQQdUA9X2gf88I6FzAEZ7VITBzfDuTkX67T2dzNVYeoTMyU\r\n\
mi9dl4lNnfCO9eoK4Lyim9WmZ3o+ckodZSK8zAV1yRVTo7RuzaBm+wIoggWhO+VJ\r\n\
gJ1rTfpmquY51QO5YA+qLBQ95UywA79Cn6zv3YYNwBE6/s+Gj9tkO7ObIGB8ZA==\r\n\
-----END CERTIFICATE-----\r\n"


/**
 * @brief Path of the file containing the client's private key.
 *
 * Refer to the AWS documentation below for details regarding client
 * authentication.
 * https://docs.aws.amazon.com/iot/latest/developerguide/client-authentication.html
 *
 * @note This private key should be PEM-encoded.
 */
 #define CLIENT_PRIVATE_KEY    "-----BEGIN RSA PRIVATE KEY-----\r\n\
MIIEowIBAAKCAQEA2MwNaQa/3xinNfriL8kcaGuCM7h2JXrY1u45sm1FFC8xJ8e6\r\n\
BysdeM9yqOaGa4DhqDMzMxWTWJHXhiEyXcEZNrkWSK9DRfO9xXUjLZWKONLoEXrU\r\n\
S187nRTW2c/2YR9IooB/G4Omcc/AdS3o1MO1lnhZuR/U96kKoPJ4YECbSjj25zWp\r\n\
CR1lIj3lfhaHXdajXRy5gT6udWK4sl9V1r2QaGZx4I37DgOjlrzokmrm0vGXzXbi\r\n\
fC0B2Y6ELPI27jDo3wMHfbROuFKVm2PL8yXFV2668UTjE0nQXE49ImnQz7f0Gpfz\r\n\
YZSvEGFgbooeigqifi6zUsxNtniIlMFcm02pYwIDAQABAoIBADSxe/zwCAi4nEOx\r\n\
g/JrZj03DEF37zdy4Wt0IY/toSGjysXJdLyzTQmFGnBU/4z3mWFheGtNLjAT2pee\r\n\
T/ibq34yhGkMOESzVoe8zd0RgLQJByDgguMC9aFbZFEyi8bVbLkalEpiuDznavbQ\r\n\
Pswf+W3mOKCXTRMZAh1Wcc2rRlNGjFt+sib9nbb79255Ralmgprlel4u5Mu3g18u\r\n\
UaDNyoDhbQgZTuV6+vUc55wiCbxp8dIcWYIqWWoj1Z4fH2QFaO7hliXJCg7TtXtk\r\n\
KWCeuF2k4ae0e6ZCcPKRJvn8PEspYTKp+BIX+zQEUFOJsRmsCc00lp35MnEYiA0J\r\n\
cVIeJMECgYEA/g2otjf2qNuIjZw4np3xIbbKymXqcOyomAeI/6eoD+ySUXiG8GrW\r\n\
oBjb8b1+0HhvGkJQAVw9qHpHp2hc7CLxKUUW14K+ZVpYdteEgfXVfLjPnXq4q6ix\r\n\
uVf2DQOJ6lqcRYt9dUQ1j5oUASLSRFfDlRFaee9gKuLTCFReM/Jm1UMCgYEA2nVQ\r\n\
G7D/D2MtAO3s0jkOLs/EQtgAipH2jeizYnXfFFhYI0ZSC+T1LMLoQ9hJamIZu9nm\r\n\
CaoWFnWcp6tCSqm+V8q9KYu6XrCaeYzl4clemnsl7rv9WlPR0JOFsXXZYwkxykwh\r\n\
smTKqtlC1wuoWW1afbpBHDW8EW7QwzpFtd6SiWECgYEAgtjSHhewyMNZD2Z4mulJ\r\n\
+k1FIP1gxAx/wN2Qp+vX2WvE6IUlKso8BHUJQvaQTBrbTjD9N9YkQ5PHN+e+hDhy\r\n\
8QiOfSzvlw/zJDJYIv9Ul0owVDGSz4axEn7FUWqTK8rDVeZEFZOt6mo3B2bB2UVC\r\n\
jJJFgSvS4gO1IUMiyap9YDUCgYBz4X7Y8Y6azvT0ZUEWUoWfdz6YZpQC7WJn7GQb\r\n\
eoMW+7XlIEwtWwpi8BDczEJFWDxmVTOdNMf6qfCrUEEd8eSyFS06eJAn4m5Ow9q9\r\n\
Zl/0ehapCLdfzs6hYV+tvuHNkvcCZFgLlUOcVRsLj3eB2GwD/WTEXgyhArNim+qW\r\n\
irYYgQKBgDWt1nS6vZweObCg6JBtObCcLIi78hMKPsf91jDPRdO2CQZnVzYBmxo2\r\n\
wbhVceaBEFXk68al74rPofsGL2gEj7VG/sCQZRwV6oQkoVpEoWLpHrgwsqkfbD0V\r\n\
qm2aPYKTm8SzK0IRvwELoYHnqyYjMsQTvxr369IezOE653KKObGb\r\n\
-----END RSA PRIVATE KEY-----\r\n"


/**
 * @brief The username value for authenticating client to MQTT broker when
 * username/password based client authentication is used.
 *
 * Refer to the AWS IoT documentation below for details regarding client
 * authentication with a username and password.
 * https://docs.aws.amazon.com/iot/latest/developerguide/custom-authentication.html
 * As mentioned in the link above, an authorizer setup needs to be done to use
 * username/password based client authentication.
 *
 * @note AWS IoT message broker requires either a set of client certificate/private key
 * or username/password to authenticate the client. If this config is defined,
 * the username and password will be used instead of the client certificate and
 * private key for client authentication.
 *
 * #define CLIENT_USERNAME    "...insert here..."
 */

/**
 * @brief The password value for authenticating client to MQTT broker when
 * username/password based client authentication is used.
 *
 * Refer to the AWS IoT documentation below for details regarding client
 * authentication with a username and password.
 * https://docs.aws.amazon.com/iot/latest/developerguide/custom-authentication.html
 * As mentioned in the link above, an authorizer setup needs to be done to use
 * username/password based client authentication.
 *
 * @note AWS IoT message broker requires either a set of client certificate/private key
 * or username/password to authenticate the client.
 *
 * #define CLIENT_PASSWORD    "...insert here..."
 */

/**
 * @brief MQTT client identifier.
 *
 * No two clients may use the same client identifier simultaneously.
 */
#ifndef CLIENT_IDENTIFIER
    #define CLIENT_IDENTIFIER    "sdkTest"
#endif

/**
 * @brief Size of the network buffer for MQTT packets.
 */
#define NETWORK_BUFFER_SIZE       ( 1024U )

/**
 * @brief The name of the operating system that the application is running on.
 * The current value is given as an example. Please update for your specific
 * operating system.
 */
#define OS_NAME                   "FreeRTOS"

/**
 * @brief The version of the operating system that the application is running
 * on. The current value is given as an example. Please update for your specific
 * operating system version.
 */
#define OS_VERSION                "1.1.1"

/**
 * @brief The name of the hardware platform the application is running on. The
 * current value is given as an example. Please update for your specific
 * hardware platform.
 */
#define HARDWARE_PLATFORM_NAME    "NRC7292"

/**
 * @brief The name of the MQTT library used and its version, following an "@"
 * symbol.
 */
#define MQTT_LIB    "core-mqtt@" MQTT_LIBRARY_VERSION

#define LIBRARY_LOG_LEVEL LOG_DEBUG
#endif /* ifndef DEMO_CONFIG_H_ */
