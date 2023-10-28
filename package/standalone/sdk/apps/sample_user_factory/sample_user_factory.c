/*
 * MIT License
 *
 * Copyright (c) 2023 Newracom, Inc.
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

#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"

#include "nrc_sdk.h"

#include "cJSON.h"

typedef struct {
	uint8_t model_name[16];
	uint8_t serial_number[16];
} user_factory_t;

user_factory_t user_factory_info;
#define USER_FACTORY_SIZE 512

int get_json_str_value(cJSON *cjson, char *key, char **value)
{
	cJSON *cjson_obj = NULL;

	cjson_obj = cJSON_GetObjectItem(cjson, key);
	if (cjson_obj && cjson_obj->valuestring) {
		*value = strdup(cjson_obj->valuestring);
		return 1;
	} else {
		nrc_usr_print("[%s] %s not found in json\n", __func__, key);
		return 0;
	}
}

void parse_user_factory(char *input, user_factory_t* data)
{
    cJSON *cjson = NULL;
    cjson = cJSON_Parse(input);

    if (cjson) {
        cJSON *model_json = cJSON_GetObjectItem(cjson, "model");
        if (model_json && model_json->valuestring) {
            strncpy((char*)data->model_name, model_json->valuestring, sizeof(data->model_name));
            data->model_name[sizeof(data->model_name) - 1] = '\0';
        } else {
            nrc_usr_print("[%s] Error: Missing or invalid 'model' field\n", __func__);
            goto exit;
        }

        cJSON *sn_json = cJSON_GetObjectItem(cjson, "sn");
        if (sn_json && sn_json->valuestring) {
            strncpy((char*)data->serial_number, sn_json->valuestring, sizeof(data->serial_number));
            data->serial_number[sizeof(data->serial_number) - 1] = '\0';
        } else {
            nrc_usr_print("[%s] Error: Missing or invalid 'sn' field\n", __func__);
            goto exit;
        }
    } else {
        nrc_usr_print("[%s] JSON parse error\n", __func__);
    }

exit:
    if (cjson) {
        cJSON_Delete(cjson);
    }
}

static nrc_err_t get_user_factory_data(user_factory_t* data)
{
	nrc_err_t ret = NRC_FAIL;

	char data_fcatory[USER_FACTORY_SIZE]={0,};
	ret = nrc_get_user_factory(data_fcatory, USER_FACTORY_SIZE);
	if(ret == NRC_SUCCESS){
		parse_user_factory(data_fcatory, data);
	}
	return ret;
}

void user_init(void)
{
	if(get_user_factory_data(&user_factory_info) == NRC_SUCCESS){
		A("[%s] model_name : %s\n", __func__, user_factory_info.model_name);
		A("[%s] serial_number : %s\n", __func__, user_factory_info.serial_number);	
	}
}
