#ifndef UTIL_HISTORY_BUFFER_H
#define UTIL_HISTORY_BUFFER_H

#include "system.h"

#if defined(INCLUDE_BUF_TRACKER) || defined(CUNIT)

struct h_buffer {
	int index;
	int max_item;
	int item_size;
	int item_count;
	uint32_t flag;
	void (*print)(uint8_t* data);
	bool (*copy)(uint8_t* dest, uint8_t* src, int src_size);
	const char* (*module_name)();
	uint8_t data[0];
};

#define H_BUFFER_FLAG_SHOW_ITEM (1UL<<0)
#define H_BUFFER_FLAG_SHOW_HEAD (1UL<<1)
#define H_BUFFER_FLAG_SHOW_INDEX (1UL<<2)
#define H_BUFFER_FLAG_DEFAULT (0xFFFFFFFF)
#define H_BUFFER_FLAG_NONE (0x0)

#define H_BUFFER_DECLARE(s, m)\
	static uint8_t arr[sizeof(struct h_buffer) + (s*m)] = {0,};

#define H_BUFFER_INIT(name, s, m, p, c, f)\
	util_h_buffer_init(arr, m, s, p, c, name, f);

void util_h_buffer_init(uint8_t* arr, int max_item, int item_size, void (*print)(uint8_t* data),
	bool (*copy)(uint8_t* dest, uint8_t* src, int src_size), const char* (*module_name_)(), uint32_t flag);
void util_h_buffer_coredump_register(uint8_t* arr);
void util_h_buffer_coredump_deregister(uint8_t* arr);
bool util_h_buffer_add_item(uint8_t* arr, uint8_t* item, int size);
void util_h_buffer_print(uint8_t* arr);
void util_h_buffer_set_flag(uint32_t flag);

#else //#if defined(INCLUDE_BUF_TRACKER) || defined(CUNIT)

#define H_BUFFER_FLAG_SHOW_ITEM
#define H_BUFFER_FLAG_SHOW_HEAD
#define H_BUFFER_FLAG_SHOW_INDEX
#define H_BUFFER_FLAG_DEFAULT
#define H_BUFFER_FLAG_NONE (0x0)

#define H_BUFFER_DECLARE(s, m)
#define H_BUFFER_INIT(name, s, m, p, c, f)

static inline void util_h_buffer_init(uint8_t* arr, int max_item, int item_size, void (*print)(uint8_t* data),
	bool (*copy)(uint8_t* dest, uint8_t* src, int src_size), const char* (*module_name_)(), uint32_t flag){};
static inline void util_h_buffer_coredump_register(uint8_t* arr){};
static inline void util_h_buffer_coredump_deregister(uint8_t* arr){};
static inline bool util_h_buffer_add_item(uint8_t* arr, uint8_t* item, int size){return true;};
static inline void util_h_buffer_print(uint8_t* arr){};
static inline void util_h_buffer_set_flag(uint32_t flag){};
#endif //#if defined(INCLUDE_BUF_TRACKER) || defined(CUNIT)

#endif /* UTIL_HISTORY_BUFFER_H */
