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

/**********************************************************************************************/

static int _atcmd_param_to_int (const char *param, long int *val)
{
	char *end = NULL;
	char buf[16];
	long int ret;

	if (!param || !val)
		return -1;

	ret = strtol(param, &end, 10);

	if (end != (param + strlen(param)))
		return -1;

	if (errno == ERANGE && (ret == LONG_MIN || ret == LONG_MAX))
		return -1;

	snprintf(buf, sizeof(buf), "%ld", ret);

	if (strcmp(param, buf) != 0)
		return -1;

	*val = ret;

/*	_atcmd_debug("%s: %ld\n", __func__, *val); */

	return 0;
}

int atcmd_param_to_int8 (const char *param, int8_t *val)
{
	long int _val;

	if (_atcmd_param_to_int(param, &_val) != 0)
		return -1;

	if (_val < CHAR_MIN || _val > CHAR_MAX)
		return -1;

	*val = (int8_t)_val;

/*	_atcmd_debug("%s: %d\n", __func__, *val); */

	return 0;
}

int atcmd_param_to_int16 (const char *param, int16_t *val)
{
	long int _val;

	if (_atcmd_param_to_int(param, &_val) != 0)
		return -1;

	if (_val < SHRT_MIN || _val > SHRT_MAX)
		return -1;

	*val = (int16_t)_val;

/*	_atcmd_debug("%s: %d\n", __func__, *val); */

	return 0;
}

int atcmd_param_to_int32 (const char *param, int32_t *val)
{
	long int _val;

	if (_atcmd_param_to_int(param, &_val) != 0)
		return -1;

/*	if (_val < INT_MIN || _val > INT_MAX)
		return -1; */

	*val = (int32_t)_val;

/*	_atcmd_debug("%s: %ld\n", __func__, *val); */

	return 0;
}

/**********************************************************************************************/

static int _atcmd_param_to_uint (const char *param, unsigned long int *val)
{
	char *end = NULL;
	char buf[16];
	unsigned long int ret;

	if (!param || !val)
		return -1;

	if (param[0] == '-')
		return -1;

	ret = strtol(param, &end, 10);

	if (end != (param + strlen(param)))
		return -1;

	if (errno == ERANGE && ret == ULONG_MAX)
		return -1;

	snprintf(buf, sizeof(buf), "%lu", ret);

	if (strcmp(param, buf) != 0)
		return -1;

	*val = ret;

/*	_atcmd_debug("%s: %lu\n", __func__, *val); */

	return 0;
}

int atcmd_param_to_uint8 (const char *param, uint8_t *val)
{
	unsigned long int _val;

	if (_atcmd_param_to_uint(param, &_val) != 0)
		return -1;

	if (_val > UCHAR_MAX)
		return -1;

	*val = (uint8_t)_val;

/* 	_atcmd_debug("%s: %u\n", __func__, *val); */

	return 0;
}

int atcmd_param_to_uint16 (const char *param, uint16_t *val)
{
	unsigned long int _val;

	if (_atcmd_param_to_uint(param, &_val) != 0)
		return -1;

	if (_val > USHRT_MAX)
		return -1;

	*val = (uint16_t)_val;

/*	_atcmd_debug("%s: %u\n", __func__, *val); */

	return 0;
}

int atcmd_param_to_uint32 (const char *param, uint32_t *val)
{
	unsigned long int _val;

	if (_atcmd_param_to_uint(param, &_val) != 0)
		return -1;

/*	if (_val > UINT_MAX)
		return -1; */

	*val = (uint32_t)_val;

/*	_atcmd_debug("%s: %lu\n", __func__, *val); */

	return 0;
}

/**********************************************************************************************/

float atcmd_param_to_float (const char *param, float *val)
{
	char *end = NULL;
	char buf[16];
	float ret;

	if (!param || !val)
		return -1;

	ret = strtof(param, &end);

	if (end != (param + strlen(param)))
		return -1;

	if (errno == ERANGE)
		return -1;

	snprintf(buf, sizeof(buf), "%.6f", ret);

	if (strncmp(param, buf, strlen(param)) != 0)
		return -1;

	*val = ret;

/*	_atcmd_debug("%s: %f\n", __func__, *val); */

	return 0;

}

/**********************************************************************************************/

int atcmd_param_to_hex (const char *param, uint32_t *val)
{
	int len = strlen(param);
	int i;

	if (len < 2 || len > 10 || memcmp(param, "0x", 2) != 0)
		return -1;

	for (*val = 0, i = 2 ; i < len ; i++)
	{
		if (param[i] >= '0' && param[i] <= '9')
			*val = (*val << 4) + (param[i] - '0');
		else if (param[i] >= 'a' && param[i] <= 'f')
			*val = (*val << 4) + (param[i] - 'a' + 10);
		else if (param[i] >= 'A' && param[i] <= 'F')
			*val = (*val << 4) + (param[i] - 'a' + 10);
		else
			return -1;
	}

	return 0;
}

/**********************************************************************************************/

char *atcmd_param_to_str (const char *param, char *str, int len)
{
	int param_len;

	if (!param || !str || !len)
		return NULL;

	param_len = strlen(param);

	if (param_len <= 2 || (param_len - 2) > (len - 1))
		return NULL;

	if (param[0] != '"' || param[param_len - 1] != '"')
		return NULL;

	memcpy(str, &param[1], param_len - 2);
	str[param_len - 2] = '\0';

	return str;
}

char *atcmd_str_to_param (const char *str, char *param, int len)
{
	int str_len;

	if (!str || !param || !len)
		return NULL;

	str_len = strlen(str);
	if (len < (str_len + 2 + 1))
		return NULL;

	snprintf(param, len, "\"%s\"", str);
	param[str_len + 2] = '\0';

	return param;
}

