/*********************************************************************/
/*    Newracom Library / IP Product                                  */
/*    Product Name :                                                 */
/*    Version      :                                                 */
/*    Author       :                                                 */
/*    Update       :                                                 */
/*********************************************************************/

#ifndef __REG_SCU_H__
#define __REG_SCU_H__

#ifdef __cplusplus
extern "C"{
#endif

// 0x000 register ------------------------------------
typedef struct {
    uint32_t          CKSEL         :3 ;
    uint32_t          reserved0     :29;
}   SCU_CPUCKSEL;

typedef union {
    uint32_t          w;
    SCU_CPUCKSEL      b;
}   SCU_CPUCKSEL_U;


// 0x004 register ------------------------------------
typedef struct {
    uint32_t          SXINDIV       :6 ;
    uint32_t          SXINREQ       :1 ;
    uint32_t          SXINDIVEN     :1 ;
    uint32_t          XINDIV        :6 ;
    uint32_t          XINREQ        :1 ;
    uint32_t          XINDIVEN      :1 ;
    uint32_t          reserved0     :8 ;
    uint32_t          LPO16DIV      :6 ;
    uint32_t          LPO16REQ      :1 ;
    uint32_t          LPO16DIVEN    :1 ;
}   SCU_CKDIVCTRL;

typedef union {
    uint32_t          w;
    SCU_CKDIVCTRL     b;
}   SCU_CKDIVCTRL_U;


// 0x008 register ------------------------------------
typedef struct {
    uint32_t          MACCLKEN      :1 ;
    uint32_t          MACSECEN      :1 ;
    uint32_t          MACTMEN       :1 ;
    uint32_t          reserved0     :5 ;
    uint32_t          PHY1EN        :1 ;
    uint32_t          PHY2EN        :1 ;
    uint32_t          PHY4EN        :1 ;
    uint32_t          PHY8EN        :1 ;
    uint32_t          PHY16EN       :1 ;
    uint32_t          reserved1     :3 ;
    uint32_t          ADDA4EN       :1 ;
    uint32_t          ADDA8EN       :1 ;
    uint32_t          ADDA16EN      :1 ;
    uint32_t          ADDA32EN      :1 ;
    uint32_t          reserved2     :4 ;
    uint32_t          ADC_CLKEN     :1 ;
    uint32_t          DACCLKEN      :1 ;
    uint32_t          PHYRXCLKEN    :1 ;
    uint32_t          PHYTXCLKEN    :1 ;
    uint32_t          reserved3     :3 ;
    uint32_t          WIFIEN        :1 ;
}   SCU_CKWIFI;

typedef union {
    uint32_t          w;
    SCU_CKWIFI        b;
}   SCU_CKWIFI_U;


// 0x010 register ------------------------------------
typedef struct {
    uint32_t          TM00RST       :1 ;
    uint32_t          reserved0     :1 ;
    uint32_t          PWMRST        :1 ;
    uint32_t          I2CRST        :1 ;
    uint32_t          SPI00RST      :1 ;
    uint32_t          SPI01RST      :1 ;
    uint32_t          reserved1     :1 ;
    uint32_t          reserved2     :1 ;
    uint32_t          UART00RST     :1 ;
    uint32_t          UART01RST     :1 ;
    uint32_t          reserved3     :1 ;
    uint32_t          reserved4     :1 ;
    uint32_t          XIPRST        :1 ;
    uint32_t          DMARST        :1 ;
    uint32_t          WIFIRST       :1 ;
    uint32_t          HIFRST        :1 ;
    uint32_t          RTCRST        :1 ;
    uint32_t          SCFGRST       :1 ;
    uint32_t          GPIORST       :1 ;
    uint32_t          PMCRST        :1 ;
    uint32_t          SCURST        :1 ;
    uint32_t          WDTRST        :1 ;
    uint32_t          ROMRST        :1 ;
    uint32_t          reserved5     :1 ;
    uint32_t          SRAMRST       :1 ;
    uint32_t          ETCRST        :1 ;
    uint32_t          reserved6     :1 ;
    uint32_t          AHBRST        :1 ;
    uint32_t          CPUHRST       :1 ;
    uint32_t          CPUFRST       :1 ;
    uint32_t          reserved7     :2 ;
}   SCU_SWRST;

typedef union {
    uint32_t          w;
    SCU_SWRST         b;
}   SCU_SWRST_U;


// 0x014 register ------------------------------------
typedef struct {
    uint32_t          TM00RST       :1 ;
    uint32_t          reserved0     :1 ;
    uint32_t          PWMRST        :1 ;
    uint32_t          I2CRST        :1 ;
    uint32_t          SPI00RST      :1 ;
    uint32_t          SPI01RST      :1 ;
    uint32_t          reserved1     :1 ;
    uint32_t          reserved2     :1 ;
    uint32_t          UART00RST     :1 ;
    uint32_t          UART01RST     :1 ;
    uint32_t          reserved3     :1 ;
    uint32_t          reserved4     :1 ;
    uint32_t          XIPRST        :1 ;
    uint32_t          DMARST        :1 ;
    uint32_t          WIFIRST       :1 ;
    uint32_t          HIFRST        :1 ;
    uint32_t          RTCRST        :1 ;
    uint32_t          SCFGRST       :1 ;
    uint32_t          GPIORST       :1 ;
    uint32_t          PMCRST        :1 ;
    uint32_t          SCURST        :1 ;
    uint32_t          WDTRST        :1 ;
    uint32_t          ROMRST        :1 ;
    uint32_t          reserved5     :1 ;
    uint32_t          SRAMRST       :1 ;
    uint32_t          ETCRST        :1 ;
    uint32_t          reserved6     :1 ;
    uint32_t          AHBRST        :1 ;
    uint32_t          CPUHRST       :1 ;
    uint32_t          CPUFRST       :1 ;
    uint32_t          reserved7     :2 ;
}   SCU_RST_MASK;

typedef union {
    uint32_t          w;
    SCU_RST_MASK      b;
}   SCU_RST_MASK_U;


// 0x018 register ------------------------------------
typedef struct {
    uint32_t          TM00CLK       :1 ;
    uint32_t          reserved0     :1 ;
    uint32_t          PWMCLK        :1 ;
    uint32_t          I2CCLK        :1 ;
    uint32_t          SPI00CLK      :1 ;
    uint32_t          SPI01CLK      :1 ;
    uint32_t          UART03CLK     :1 ;
    uint32_t          UART02CLK     :1 ;
    uint32_t          UART00CLK     :1 ;
    uint32_t          UART01CLK     :1 ;
    uint32_t          reserved3     :1 ;
    uint32_t          reserved4     :1 ;
    uint32_t          XIPCLK        :1 ;
    uint32_t          DMACLK        :1 ;
    uint32_t          WIFICLK       :1 ;
    uint32_t          HIFCLK        :1 ;
    uint32_t          RTCCLK        :1 ;
    uint32_t          SCFGCLK       :1 ;
    uint32_t          GPIOCLK       :1 ;
    uint32_t          PMCCLK        :1 ;
    uint32_t          SCUCLK        :1 ;
    uint32_t          WDTCLK        :1 ;
    uint32_t          ROMCLK        :1 ;
    uint32_t          reserved5     :1 ;
    uint32_t          SRAMCLK       :1 ;
    uint32_t          ETCCLK        :1 ;
    uint32_t          reserved6     :1 ;
    uint32_t          AHBCLK        :1 ;
    uint32_t          CPUHCLK       :1 ;
    uint32_t          CPUFCLK       :1 ;
    uint32_t          reserved7     :2 ;
}   SCU_HCLKEN;

typedef union {
    uint32_t          w;
    SCU_HCLKEN        b;
}   SCU_HCLKEN_U;


// 0x020 register ------------------------------------
typedef struct {
    uint32_t          DIV           :4 ;
    uint32_t          reserved0     :20;
    uint32_t          SEL           :3 ;
    uint32_t          reserved1     :4 ;
    uint32_t          EN            :1 ;
}   SCU_PCLKCFG00;

typedef union {
    uint32_t          w;
    SCU_PCLKCFG00     b;
}   SCU_PCLKCFG00_U;


// 0x024 register ------------------------------------
typedef struct {
    uint32_t          DIV           :4 ;
    uint32_t          reserved0     :20;
    uint32_t          SEL           :3 ;
    uint32_t          reserved1     :4 ;
    uint32_t          EN            :1 ;
}   SCU_PCLKCFG01;

typedef union {
    uint32_t          w;
    SCU_PCLKCFG01     b;
}   SCU_PCLKCFG01_U;


// 0x028 register ------------------------------------
typedef struct {
    uint32_t          DIV           :4 ;
    uint32_t          reserved0     :20;
    uint32_t          SEL           :3 ;
    uint32_t          reserved1     :4 ;
    uint32_t          EN            :1 ;
}   SCU_PCLKCFG02;

typedef union {
    uint32_t          w;
    SCU_PCLKCFG02     b;
}   SCU_PCLKCFG02_U;


// 0x02c register ------------------------------------
typedef struct {
    uint32_t          DIV           :4 ;
    uint32_t          reserved0     :20;
    uint32_t          SEL           :3 ;
    uint32_t          reserved1     :4 ;
    uint32_t          EN            :1 ;
}   SCU_PCLKCFG03;

typedef union {
    uint32_t          w;
    SCU_PCLKCFG03     b;
}   SCU_PCLKCFG03_U;


// 0x040 register ------------------------------------
typedef struct {
    uint32_t          DIV           :4 ;
    uint32_t          reserved0     :20;
    uint32_t          SEL           :3 ;
    uint32_t          reserved1     :4 ;
    uint32_t          EN            :1 ;
}   SCU_PCLKCFG08;

typedef union {
    uint32_t          w;
    SCU_PCLKCFG08     b;
}   SCU_PCLKCFG08_U;


// 0x050 register ------------------------------------
typedef struct {
    uint32_t          DIV           :4 ;
    uint32_t          reserved0     :20;
    uint32_t          SEL           :3 ;
    uint32_t          reserved1     :4 ;
    uint32_t          EN            :1 ;
}   SCU_PCLKCFG12;

typedef union {
    uint32_t          w;
    SCU_PCLKCFG12     b;
}   SCU_PCLKCFG12_U;


// 0x054 register ------------------------------------
typedef struct {
    uint32_t          DIV           :4 ;
    uint32_t          reserved0     :20;
    uint32_t          SEL           :3 ;
    uint32_t          reserved1     :4 ;
    uint32_t          EN            :1 ;
}   SCU_PCLKCFG13;

typedef union {
    uint32_t          w;
    SCU_PCLKCFG13     b;
}   SCU_PCLKCFG13_U;


// 0x060 register ------------------------------------
typedef struct {
    uint32_t          DIV           :4 ;
    uint32_t          reserved0     :20;
    uint32_t          SEL           :3 ;
    uint32_t          reserved1     :4 ;
    uint32_t          EN            :1 ;
}   SCU_PCLKCFG16;

typedef union {
    uint32_t          w;
    SCU_PCLKCFG16     b;
}   SCU_PCLKCFG16_U;


// 0x064 register ------------------------------------
typedef struct {
    uint32_t          DIV           :4 ;
    uint32_t          reserved0     :20;
    uint32_t          SEL           :3 ;
    uint32_t          reserved1     :4 ;
    uint32_t          EN            :1 ;
}   SCU_PCLKCFG17;

typedef union {
    uint32_t          w;
    SCU_PCLKCFG17     b;
}   SCU_PCLKCFG17_U;


// 0x06c register ------------------------------------
typedef struct {
    uint32_t          DIV           :4 ;
    uint32_t          reserved0     :20;
    uint32_t          SEL           :3 ;
    uint32_t          reserved1     :4 ;
    uint32_t          EN            :1 ;
}   SCU_PCLKCFG19;

typedef union {
    uint32_t          w;
    SCU_PCLKCFG19     b;
}   SCU_PCLKCFG19_U;


// 0x070 register ------------------------------------
typedef struct {
    uint32_t          DIV           :4 ;
    uint32_t          reserved0     :20;
    uint32_t          SEL           :3 ;
    uint32_t          reserved1     :4 ;
    uint32_t          EN            :1 ;
}   SCU_PCLKCFG20;

typedef union {
    uint32_t          w;
    SCU_PCLKCFG20     b;
}   SCU_PCLKCFG20_U;


//---------------------------------------------------------
// SCU Register Map
//---------------------------------------------------------
typedef struct {
    volatile SCU_CPUCKSEL_U     CPUCKSEL   ; // 0x000
    volatile SCU_CKDIVCTRL_U    CKDIVCTRL  ; // 0x004
    volatile SCU_CKWIFI_U       CKWIFI     ; // 0x008
    volatile uint32_t           RESERVED0  ; // 
    volatile SCU_SWRST_U        SWRST      ; // 0x010
    volatile SCU_RST_MASK_U     RST_MASK   ; // 0x014
    volatile SCU_HCLKEN_U       HCLKEN     ; // 0x018
    volatile uint32_t           RESERVED1  ; // 
    volatile SCU_PCLKCFG00_U    PCLKCFG00  ; // 0x020
    volatile SCU_PCLKCFG01_U    PCLKCFG01  ; // 0x024
    volatile SCU_PCLKCFG02_U    PCLKCFG02  ; // 0x028
    volatile SCU_PCLKCFG03_U    PCLKCFG03  ; // 0x02c
    volatile uint32_t           RESERVED2[4]; // 
    volatile SCU_PCLKCFG08_U    PCLKCFG08  ; // 0x040
    volatile uint32_t           RESERVED3[3]; // 
    volatile SCU_PCLKCFG12_U    PCLKCFG12  ; // 0x050
    volatile SCU_PCLKCFG13_U    PCLKCFG13  ; // 0x054
    volatile uint32_t           RESERVED4[2]; // 
    volatile SCU_PCLKCFG16_U    PCLKCFG16  ; // 0x060
    volatile SCU_PCLKCFG17_U    PCLKCFG17  ; // 0x064
    volatile uint32_t           RESERVED5  ; // 
    volatile SCU_PCLKCFG19_U    PCLKCFG19  ; // 0x06c
    volatile SCU_PCLKCFG20_U    PCLKCFG20  ; // 0x070
} SCU_Type;

#ifdef __cplusplus
}
#endif

#endif /* _REG_SCU_H_ */
