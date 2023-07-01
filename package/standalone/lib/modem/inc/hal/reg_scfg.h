#ifndef __REG_SCFG_H__
#define __REG_SCFG_H__

#ifdef __cplusplus
extern "C"{
#endif

// 0x4000F000 register ------------------------------------
typedef struct {
    uint32_t            MODE_PIN         :1 ;
    uint32_t            reserved0        :31;
}   SCFG_MODE;

typedef union {
    uint32_t            w;
    SCFG_MODE           b;
}   SCFG_MODE_U;


// 0x4000F004 register ------------------------------------
typedef struct {
    uint32_t            REMAP_CM0        :3 ;
	uint32_t            reserved0        :1 ;
	uint32_t            REMAP_CM3        :3 ;
	uint32_t            reserved1        :1 ;
	uint32_t            REMAP_GDMA       :3 ;
	uint32_t            reserved2        :1 ;
	uint32_t            REMAP_WIFI       :3 ;
	uint32_t            reserved3        :1 ;
    uint32_t            REMAP_HOST       :3 ;
	uint32_t            reserved4        :13;
}   SCFG_REMAP;

typedef union {
    uint32_t            w;
    SCFG_REMAP          b;
}   SCFG_REMAP_U;


// 0x4000F008 register ------------------------------------
typedef struct {
    uint32_t            R_WAIT	         :6 ;
    uint32_t            reserved0        :26;
}   SCFG_MEM_RWAIT;

typedef union {
    uint32_t            w;
    SCFG_MEM_RWAIT      b;
}   SCFG_MEM_RWAIT_U;


// 0x4000F00C register ------------------------------------
typedef struct {
	uint32_t            BUS_INC_EN       :10;
    uint32_t            reserved0        :22;
}   SCFG_BUS_INC_EN;

typedef union {
    uint32_t            w;
    SCFG_BUS_INC_EN     b;
}   SCFG_BUS_INC_EN_U;


// 0x4000F010 register ------------------------------------
typedef struct {
    uint32_t            RF_CON           :1 ;
    uint32_t            SEL_32K          :1 ;
    uint32_t            select_avia_adda :1	;
    uint32_t            TCLK_EN          :1 ;
    uint32_t            reserved0        :28;
}   SCFG_SEL_SPI_RF;

typedef union {
    uint32_t            	w;
    SCFG_SEL_SPI_RF         b;
}   SCFG_SEL_SPI_RF_U;


// 0x4000F014 register ------------------------------------
typedef struct {
    uint32_t            CAL              :13;
    uint32_t            reserved0        :19;
}   SCFG_CAL_LPO32K;

typedef union {
    uint32_t            w;
    SCFG_CAL_LPO32K     b;
}   SCFG_CAL_LPO32K_U;


// 0x4000F018 register ------------------------------------
typedef struct {
    uint32_t            POR              :1 ;
    uint32_t            WDOG             :1 ;
    uint32_t            PMC              :1 ;
    uint32_t            HOSTINF          :1 ;
    uint32_t            SYSRESETREQ_CM3  :1 ;
    uint32_t            SYSRESETREQ_CM0  :1 ;
	uint32_t            Reserved0		 :26;
}   SCFG_BR;

typedef union {
    uint32_t            w;
    SCFG_BR             b;
}   SCFG_BR_U;


// 0x4000F01C register ------------------------------------
typedef struct {
    uint32_t            PXO              :6 ;
    uint32_t            reserved0        :2 ;
    uint32_t            PXOE             :6 ;
    uint32_t            reserved1        :2 ;
    uint32_t            PXPE             :6 ;
    uint32_t            reserved2        :9 ;
    uint32_t            Enable           :1 ;
}   SCFG_XIP_DEEPSLEEP;

typedef union {
    uint32_t           			w;
    SCFG_XIP_DEEPSLEEP          b;
}   SCFG_XIP_DEEPSLEEP_U;


//---------------------------------------------------------
// SCFG Register Map
//---------------------------------------------------------
typedef struct {
    volatile SCFG_MODE_U                MODE              ;
    volatile SCFG_REMAP_U               REMAP             ;
    volatile SCFG_MEM_RWAIT_U           MEM_RWAIT         ;
    volatile SCFG_BUS_INC_EN_U          BUS_INC_EN        ;
    volatile SCFG_SEL_SPI_RF_U          SEL_SPI_RF        ;
    volatile SCFG_CAL_LPO32K_U          CAL_LPO32K        ;
    volatile SCFG_BR_U                  BR                ;
    volatile SCFG_XIP_DEEPSLEEP_U       XIP_DEEPSLEEP     ;
} SCFG_Type;



#ifdef __cplusplus
}
#endif

#endif

