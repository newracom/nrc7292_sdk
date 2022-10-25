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
#include "wifi_config_setup.h"
#include "mxml.h"

#define BUFFER_SIZE	256

static void parsing_xml_data(char *data)
{
	mxml_node_t *tree;
	tree = mxmlLoadString(NULL, data, MXML_TEXT_CALLBACK);

	mxml_node_t *node;
	for (node = tree; node != NULL; node = mxmlWalkNext(node, tree, MXML_DESCEND)) {
		int type = mxmlGetType(node);
		switch(type) {
			case MXML_ELEMENT:
			nrc_usr_print("[ELEMENT] %s\n", mxmlGetElement(node));
			break;
			case MXML_INTEGER:
			nrc_usr_print("[INEGER] %d\n", mxmlGetInteger(node));
			break;
			case MXML_OPAQUE:
			nrc_usr_print("[OPAQUE] %s\n", mxmlGetOpaque(node));
			break;
			case MXML_TEXT:
			nrc_usr_print("[TEXT] %s\n", mxmlGetText(node, NULL));
			break;
			default:
			break;
		}
	}
}

static void create_xml_objects(char *buffer, int buffer_size)
{
	mxml_node_t *xml;
	mxml_node_t *data;
	mxml_node_t *node;
	mxml_node_t *group;

	xml = mxmlNewXML("1.0");
	if (xml == NULL){
		nrc_usr_print("[%s] mxmlNewXML() failed.\n", __func__);
	}

	data = mxmlNewElement(xml, "data");

	node = mxmlNewElement(data, "node");
	mxmlNewText(node, 0, "val1");
	node = mxmlNewElement(data, "node");
	mxmlNewText(node, 0, "val2");
	node = mxmlNewElement(data, "node");
	mxmlNewText(node, 0, "val3");

	group = mxmlNewElement(data, "group");

	node = mxmlNewElement(group, "node");
	mxmlNewText(node, 0, "val4");
	node = mxmlNewElement(group, "node");
	mxmlNewText(node, 0, "val5");
	node = mxmlNewElement(group, "node");
	mxmlNewText(node, 0, "val6");

	node = mxmlNewElement(data, "node");
	mxmlNewText(node, 0, "val7");
	node = mxmlNewElement(data, "node");
	mxmlNewText(node, 0, "val8");

	int len = mxmlSaveString(xml, buffer, buffer_size, MXML_NO_CALLBACK);
	nrc_usr_print("[%s] %s\nlen=%d\n", __func__, buffer, len);
}

/******************************************************************************
 * FunctionName : run_sample_xml
 * Description  : sample test for xml
 * Parameters   : void
 * Returns      : 0 or -1 (0: success, -1: fail)
 *******************************************************************************/
nrc_err_t run_sample_xml(void)
{
	char* data;
	data = nrc_mem_malloc(BUFFER_SIZE + 1);

	if(!data)
		return NRC_FAIL;

	memset(data, 0x0, BUFFER_SIZE);

	/* Create Object */
	create_xml_objects(data, BUFFER_SIZE);

	/* Create Parsing */
	parsing_xml_data(data);

	if(data)
		nrc_mem_free(data);

	nrc_usr_print("[%s] End of run_sample_xml!! \n",__func__);
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

	ret = run_sample_xml();
	nrc_usr_print("[%s] test result!! %s \n",__func__, (ret==0) ?  "Success" : "Fail");
}
