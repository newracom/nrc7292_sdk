#ifndef	__SYSTEM_MACRO_H__
#define __SYSTEM_MACRO_H__

#ifdef XIP_FLASH
#define FLASH __attribute__((used,section(".xip")))
#else
#define FLASH
#endif /* XIP_FLASH */

#ifdef CACHE_XIP
#define ATTR_NC		__attribute__((section(".sfc_code_nc")))
#define ATTR_C		__attribute__((section(".sfc_code_c")))
#else
#define ATTR_NC
#define ATTR_C
#endif /* CACHE_XIP */

#define SET_FIELD(reg, field, variable, value)                                 \
    do {                                                                       \
        (variable) |= (uint32_t)(reg##_##field##_MASK & ((value) << reg##_##field##_SHIFT));    \
    } while (0)

#define MOD_FIELD(reg, field, variable, value)                                 \
    do {                                                                       \
        (variable) &= ~reg##_##field##_MASK;                                   \
        (variable) |= (uint32_t)(reg##_##field##_MASK & ((value) << reg##_##field##_SHIFT));    \
    } while (0)

#define CLEAR_FIELD(reg, field, variable)                                      \
    (variable) &= ~reg##_##field##_MASK;                                   \

#define GET_FIELD(reg, field, variable) \
    (((variable) & reg##_##field##_MASK) >> reg##_##field##_SHIFT)

#define BITSET(var, num)    (var) |= ((uint32_t)0x1 << (num))
#define BITCLR(var, num)    (var) &= ~((uint32_t)0x1 << (num))
#define BITTST(var, num)    ((var) & ((uint32_t)0x1 << (num))) ? 1 : 0
#define BITTST_64(var, num)    ((var) & ((uint64_t)0x1 << (num))) ? 1 : 0

#undef BIT
#define BIT(n) ((uint32_t)(0x00000001L << n))

#define ABS(x)      (((x) > 0) ? (x) : 0 - (x))
/// Macro to perform division by using bit shift
#define MOD(x,n) ((x) & ((n)-1))
/// substract y from x with non-negative result
#define SUB_NON_NEG(x, y)   (((x) > (y)) ? ((x) - (y)) : 0)

#define WORD_ALIGNED(x) (((uint32_t)(x) & 0x3) == 0)
#define WORD_ALIGN(x)   ((((uint32_t)(x) + 3) >> 2) << 2)

#define OFFSET_OF(t,m) ((unsigned long) &((t*)0)->m)

#define PRINT_PTR(name) A(#name " : 0x%x\n",name);
#define PRINT_VAL(name) A(#name " : %d\n",name);

/**
 *
 * * README: Weak Attribute Function MACRO - yj.kim - May 2, 2019 - https://192.168.1.237:8443/cb/wiki/112822
 *
 * * This technique is to reduce #ifdef directive and make our code more simple.
 * * You need to use it when a function is not declared in one of our platforms.
 *
 * **/
#define WEAK_ATTRIBUTE_FUNC(f, t, param, act)				\
	t f(param) __attribute__((weak, alias("weak_" #f))); 	\
	static t weak_##f(param) {								\
		act;												\
	};

#define STATIC_INLINE_FUNC(f, t, act, args...)				\
	static inline t f(args) {								\
		act;												\
	};														\

#define container_of(ptr, type, member) ({					\
	void *__mptr = (void *)(ptr);							\
	((type *)(__mptr - offsetof(type, member))); })
/////////////////////////////////////////////////////////////////////////////////////////////////////
// GNUC Specific
/////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef __GNUC__
#define WARN_IF_UNUSED __attribute__ ((warn_unused_result))
#define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
#define UNUSED_FUNCTION(x) __attribute__((__unused__)) UNUSED_ ## x
#else
#define WARN_IF_UNUSED
#define UNUSED(x) UNUSED_ ## x
#define UNUSED_FUNCTION(x) UNUSED_ ## x
#endif /* __GNUC__ */
/////////////////////////////////////////////////////////////////////////////////////////////////////

#define REG32(reg)			(*((volatile uint32_t *)(reg)))
#define IN32(reg) 			(*((volatile uint32_t *)(reg)))
#define OUT32(reg, data)	((*((volatile uint32_t *)(reg)))=(uint32_t)(data))

#define MIN(x, y)               (((x) < (y))? (x) : (y))
#define MAX(x, y)               (((x) > (y))? (x) : (y))
#define min(x, y)               (((x) < (y))? (x) : (y))
#define max(x, y)               (((x) > (y))? (x) : (y))

#define BITAND(A, B)            (((uint32_t)A)&((uint32_t)B))
#define BITOR(A, B)             (((uint32_t)A)|((uint32_t)B))
#define BITNOT(D)               (~D)
#define BITXOR(A, B)            (A^B)
#define BITS(D, M)              (D = BITOR(D,M))
#define BITC(D, M)              (D = BITAND(D,BITNOT(M)))
#define BITSC(D, M, C)          (D = BITOR(M,BITAND(D,BITNOT(C))))

#define PROFILE_INIT			uint32_t time_a, time_b;
#define PROFILE_REF(func)													\
	do {																	\
		A(#func " : before : %u us\n", TSF);	 							\
		time_a = TSF;														\
		func;																\
		time_b = TSF;														\
		A(#func " : after  : %u us (diff:%u us)\n", time_b, time_b-time_a);	\
	} while(0);					

#define READ_REG(offset)        (*(volatile uint32_t*)(offset))

#define WRITE_REG(offset,value) \
(*(volatile uint32_t*)(offset) = (uint32_t)(value));


#define WRITE_REG2(value,offset) (*(volatile uint32_t*)(offset) = (uint32_t)(value));

#define WRITE_REG_DBG(offset,value) 							\
 	do															\
 	{															\
 		A(#offset " 0x%08X", READ_REG(offset));					\
 		(*(volatile uint32_t*)(offset) = (uint32_t)(value));	\
 		A("-> 0x%08X\n", READ_REG(offset));						\
 	} while (0);

#define READ_FIELD(reg, field) \
	((*(volatile uint32_t*)reg & (reg##_##field##_MASK)) >> (reg##_##field##_SHIFT))

// Use this macro for registers where 0 has no effect, e.g., control registers
#define WRITE_FIELD(reg, field, value)                                       \
	*(volatile uint32_t*)reg |= (uint32_t)(value << reg##_##field##_SHIFT);  \

// Use this macro for registers where 0 does have an effect (for multi-bit field)
#define RMW_FIELD(reg, field, value)                                         \
	do {                                                                     \
		uint32_t temp = *(uint32_t*)reg;                                     \
		temp &= (uint32_t)(~reg##_##field##_MASK);                           \
		temp |= (uint32_t)(value << reg##_##field##_SHIFT);                  \
		*(uint32_t*)reg = temp;                                              \
	} while (0)

#define INDEXOF(base, item) ((((uint8_t*)item)-((uint8_t*)base))/sizeof(*item))

#if defined(INCLUDE_TRACE_ASSERT) //if-INCLUDE_TRACE_ASSERT
#define ASSERT(x)       if ((x)) {} else {show_assert(#x, __FILE__, __FUNCTION__, __LINE__); }
#define ASSERT_V(x, v)  if ((x)) {} else {show_assert_v(#x, __FILE__, __FUNCTION__, __LINE__, v); }
#define ASSERT_VALUE_RANGE(s,e,v)    if (((uint32_t)v>=(uint32_t)&s)&&((uint32_t)v<(uint32_t)&e)) {} else {show_assert("Range"#s"<="#v"<"#e, __FILE__, __FUNCTION__, __LINE__);}
#define ASSERT_VALUE_RANGE_V(s,e,v,l)    if (((uint32_t)v>=(uint32_t)&s)&&((uint32_t)v<(uint32_t)&e)) {} else {show_assert_v("Range"#s"<="#v"<"#e, __FILE__, __FUNCTION__, __LINE__, l);}
#else //else-INCLUDE_TRACE_ASSERT
#define ASSERT(x)
#define ASSERT_V(x, v)
#define ASSERT_VALUE_RANGE(s,e,v)
#define ASSERT_VALUE_RANGE_V(s,e,v,l)
#endif //end-INCLUDE_TRACE_ASSERT

#define	CHECK_RTC_TIME(x) ({x();});
#define	CHECK_RTC_TIME_V(x,v) ({x(v);});
#define	CHECK_RTC_TIME_VV(x,v,vv) ({x(v,vv);});

#define CHECK_TSF_INIT 					\
	uint32_t ck_point_start = 0;		\
	uint32_t ck_point_end = 0;			
	
#define CHECK_TSF_TIME(x)											\
	do																\
	{																\
		ck_point_start = TSF;										\
		x;															\
		ck_point_end = TSF;											\
		A(#x " - Runnig Time : %u\n",ck_point_end - ck_point_start);	\
	} while (0);
	

#define CASE_STRING_DECLARE(n) switch(n)
#define CASE_STRING_ITEM(n) case n: return #n;
#define CASE_STRING_DEFAULT(n) default: return #n;


// -- Timer --
#define system_delay

#define SYS_HDR(buf)    (buf)->sys_hdr
#define SYS_HDR_SIZE    sizeof(SYS_HDR)

#define LMAC_TXHDR_SIZE sizeof(LMAC_TXHDR)

#define HIF_HDR(buf)    (buf)->hif_hdr
#define HIF_HDR_SIZE    sizeof(HIF_HDR)

#define FRAME_HDR(buf)  (buf)->frame_hdr
#define FRAME_HDR_SIZE  sizeof(FRAME_HDR_SIZE)

#define TX_MAC_HDR(buf) (buf)->tx_mac_header
#define RX_MAC_HDR(buf) (buf)->rx_mac_header

#define LMAC_RXHDR_SIZE sizeof(LMAC_RXHDR)

#define LMAC_BUF_TO_SYS_BUF(buf) ((SYS_BUF*)(((uint32_t)(buf)) - sizeof(SYS_HDR)))
#define LMAC_BUF_TO_SYS_HDR(buf) SYS_HDR(LMAC_BUF_TO_SYS_BUF(buf))
#define LMAC_BUF_TO_FRAME_HDR(buf) FRAME_HDR(LMAC_BUF_TO_SYS_BUF(buf))
#define SYS_BUF_TO_LMAC_TXBUF(buf) ((LMAC_TXBUF*)(&(buf)->lmac_txhdr))
#define SYS_BUF_TO_LMAC_RXBUF(buf) ((LMAC_RXBUF*)(&(buf)->lmac_rxhdr))

#define SYS_BUF_NEXT(buf)           (buf)->sys_hdr.m_next
#define SYS_BUF_LINK(buf)           (buf)->sys_hdr.m_link
#define SYS_BUF_OFFSET(buf)         (buf)->sys_hdr.m_payload_offset
#define SYS_BUF_LENGTH(buf)         (buf)->sys_hdr.m_payload_length
#define SYS_BUF_DATA_LENGTH(buf)    (buf)->sys_hdr.m_payload_length - sizeof(LMAC_TXHDR)

#define LMAC_TXBUF_NEXT(buf)        (buf)->lmac_txhdr.m_next
#define LMAC_TXBUF_LINK(buf)        (buf)->lmac_txhdr.m_link
#define LMAC_TXBUF_MACHDR(buf)      ((GenericMacHeader*)( (buf)->machdr ))


#endif /* SYSTEM_COMMON_H */
