#ifndef __MQTT_MUTUAL_AUTH_H__
#define __MQTT_MUTUAL_AUTH_H__
/* mbedTLS sockets transport implementation. */
#include "mbedtls_freertos.h"

/* MQTT API headers. */
#include "core_mqtt.h"
#include "core_mqtt_state.h"

extern MQTTContext_t mqttContext;
extern NetworkContext_t networkContext;
extern MbedTLSParams_t mbedTLSParams;

int init_mqtt();
void disconnect_mqtt();

int subscribe(MQTTContext_t *pMqttContext);
int unsubscribe(MQTTContext_t *pMqttContext);
int publish(MQTTContext_t *pMqttContext, char *message, int size);
#endif // __MQTT_MUTUAL_AUTH_H__
