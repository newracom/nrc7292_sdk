/*
 *  Portable interface to the CPU cycle counter
 *
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of mbed TLS (https://tls.mbed.org)
 */

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#include "task.h"
#include "mbedtls/timing.h"

struct _hr_time
{
	uint64_t start;
};

void mbedtls_get_time(uint64_t *time) {
	TickType_t    xTicks = xTaskGetTickCount(  );
	*time = ( uint64_t )( ( TickType_t ) ( xTicks ) * portTICK_PERIOD_MS );
}

unsigned long mbedtls_timing_get_timer( struct mbedtls_timing_hr_time *val, int reset )
{
	struct _hr_time *t = (struct _hr_time *) val;

	if( reset )
	{
		mbedtls_get_time( &t->start);
		return( 0 );
	}
	else
	{
		unsigned long delta = 0;
		uint64_t now = 0;
		mbedtls_get_time( &now );
		delta = ( now -  t->start  );

		return( delta );
	}
}

/*
 * Set delays to watch
 */
void mbedtls_timing_set_delay( void *data, uint32_t int_ms, uint32_t fin_ms )
{
	mbedtls_timing_delay_context *ctx = (mbedtls_timing_delay_context *) data;

	ctx->int_ms = int_ms;
	ctx->fin_ms = fin_ms;

	if( fin_ms != 0 )
		(void) mbedtls_timing_get_timer( &ctx->timer, 1 );
}

/*
 * Get number of delays expired
 */
int mbedtls_timing_get_delay( void *data )
{
	mbedtls_timing_delay_context *ctx = (mbedtls_timing_delay_context *) data;
	unsigned long elapsed_ms;

	if( ctx->fin_ms == 0 )
		return( -1 );

	elapsed_ms = mbedtls_timing_get_timer( &ctx->timer, 0 );

	if( elapsed_ms >= ctx->fin_ms )
		return( 2 );

	if( elapsed_ms >= ctx->int_ms )
		return( 1 );

	return( 0 );
}
