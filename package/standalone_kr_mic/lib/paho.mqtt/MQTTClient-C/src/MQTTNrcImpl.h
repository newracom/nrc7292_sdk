/*
 *
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

#ifndef __MQTT_NRC_IMPL_H__
#define __MQTT_NRC_IMPL_H__

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

typedef struct Timer
{
	TickType_t xTicksToWait;
	TimeOut_t xTimeOut;
} Timer;

typedef struct Network Network;

struct Network
{
	int my_socket;
	int (*mqttread) (Network*, unsigned char*, int, int);
	int (*mqttwrite) (Network*, unsigned char*, int, int);
	void (*disconnect) (Network*);
};

void TimerInit(Timer*);
char TimerIsExpired(Timer*);
void TimerCountdownMS(Timer*, unsigned int);
void TimerCountdown(Timer*, unsigned int);
int TimerLeftMS(Timer*);

typedef struct Mutex
{
	SemaphoreHandle_t sem;
} Mutex;

void MutexInit(Mutex*);
int MutexLock(Mutex*);
int MutexUnlock(Mutex*);

typedef struct Thread
{
	TaskHandle_t task;
} Thread;

int ThreadStart(Thread*, void (*fn)(void*), void* arg);

int nrc_sock_read(Network*, unsigned char*, int, int);
int nrc_sock_write(Network*, unsigned char*, int, int);

void NetworkInit(Network*);
int NetworkConnect(Network*, char*, int);
int NetworkDisconnect(Network* n);

#if defined( SUPPORT_MBEDTLS )
typedef struct Certs{
	const char *ca_cert;
	const char *client_cert;
	const char *client_pk;
	const char *client_pk_pwd;
	int ca_cert_length;
	int client_cert_length;
	int client_pk_length;
	int client_pk_pwd_length;
} Certs;

int NetworkConnectTLS(Network*, const char*, int, Certs*);
int NetworkDisconnectTLS(Network*);
#endif

#endif
