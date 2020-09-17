#ifndef __UMAC_MBX_BUILDER_H__
#define __UMAC_MBX_BUILDER_H__

enum MBX_TYPE {
	MBX_TYPE_MEMORY_ALLOC_REQUEST,
	MBX_TYPE_MEMORY_ALLOC_RESPONSE,
	MBX_TYPE_MEMORY_FREE,
	MBX_TYPE_SEND_DATA,
	MBX_TYPE_DISCARD_DATA,
	MBX_TYPE_MAX,
};

SYS_BUF* mbx_builder_create(uint8_t type, uint16_t channel);
bool mbx_builder_append(MBX_HDR *mb, void* data, uint8_t size);
void mbx_builder_destroy(SYS_BUF *packet);
#endif //__UMAC_MBX_BUILDER_H__