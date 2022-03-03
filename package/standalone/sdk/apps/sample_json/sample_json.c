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
#include "cJSON.h"

#define BUFFER_SIZE	256
// #define VALUE_PARSING 1

static void parsing_json_data(char *data)
{
	char *out = NULL;
	cJSON *root = cJSON_Parse(data);

#if VALUE_PARSING
	out = cJSON_Print(cJSON_GetObjectItem(root, "pc"));
	nrc_usr_print("%s\n", out);
	nrc_mem_free(out);

	out = cJSON_Print(cJSON_GetObjectItem(root, "rqi"));
	nrc_usr_print("%s\n", out);
	nrc_mem_free(out);

	out = cJSON_Print(cJSON_GetObjectItem(root, "to"));
	nrc_usr_print("%s\n", out);
	nrc_mem_free(out);
#endif /* VALUE_PARSING */

	out = cJSON_Print(root);
	nrc_usr_print("%s\n", out);
	cJSON_Delete(root);
	nrc_mem_free(out);
}

static void create_json_objects(char *data)
{
	cJSON *root = NULL;
	cJSON *i1 = NULL;
	cJSON *i2 = NULL;
	cJSON *i3 = NULL;
	char *out = NULL;
	const char *url[1] = {"mqtt://sensor01.farm.com",};

	/*
	 * A json sample for oneM2M sensor registration of Smart Farm Example(TR-0037).

		{
		        "fr":   "S",
		        "op":   1,
		        "pc":   {
		                "m2m:ae":       {
		                        "api":  "A01.com.farm.sensor01",
		                        "rr":   true,
		                        "poa":  ["mqtt://sensor01.farm.com"],
		                        "rn":   "sensor_ae01"
		                }
		        },
		        "rqi":  "m_createAE142308",
		        "to":   "/CSE3409165/farm_gateway",
		        "ty":   2
		}

	 */
	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, "fr", "S");
	cJSON_AddNumberToObject(root, "op", 1);
	cJSON_AddItemToObject(root, "pc", i1 = cJSON_CreateObject());
	cJSON_AddItemToObject(i1, "m2m:ae", i2 = cJSON_CreateObject());
	cJSON_AddStringToObject(i2, "api", "A01.com.farm.sensor01");
	cJSON_AddTrueToObject(i2, "rr");
	cJSON_AddItemToObject(i2, "poa", i3 = cJSON_CreateStringArray(url, 1));
	cJSON_AddStringToObject(i2, "rn", "sensor_ae01");
	cJSON_AddStringToObject(root, "rqi", "m_createAE142308");
	cJSON_AddStringToObject(root, "to", "/CSE3409165/farm_gateway");
	cJSON_AddNumberToObject(root, "ty", 2);
	out = cJSON_PrintUnformatted(root);
	nrc_usr_print("%s\n", out);
	memcpy(data, out, strlen(out));

	cJSON_Delete(root);
}

/******************************************************************************
 * FunctionName : run_sample_json
 * Description  : sample test for json
 * Parameters   : void
 * Returns      : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
nrc_err_t run_sample_json(void)
{
	char* data;
	data = nrc_mem_malloc(BUFFER_SIZE + 1);

	if(!data)
		return NRC_FAIL;

	memset(data, 0x0, BUFFER_SIZE);

	/* Create Object */
	create_json_objects(data);

	/* Create Parsing */
	parsing_json_data(data);

	if(data)
		nrc_mem_free(data);

	nrc_usr_print("[%s] End of run_sample_json!! \n",__func__);
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

	nrc_uart_console_enable(true);

	ret = run_sample_json();
	nrc_usr_print("[%s] test result!! %s \n",__func__, (ret==0) ?  "Success" : "Fail");
}
