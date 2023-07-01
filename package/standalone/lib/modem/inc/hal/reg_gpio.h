/*********************************************************************/
/*    Newracom Library / IP Product                                  */
/*    Product Name :                                                 */
/*    Version      :                                                 */
/*    Author       :                                                 */
/*    Update       :                                                 */
/*********************************************************************/

#ifndef __REG_GPIO_H__
#define __REG_GPIO_H__

#ifdef __cplusplus
extern "C"{
#endif

// 0x000 register ------------------------------------
typedef struct {
    uint32_t             Pn0              :1 ;
    uint32_t             Pn1              :1 ;
    uint32_t             Pn2              :1 ;
    uint32_t             Pn3              :1 ;
    uint32_t             Pn4              :1 ;
    uint32_t             Pn5              :1 ;
    uint32_t             Pn6              :1 ;
    uint32_t             Pn7              :1 ;
    uint32_t             Pn8              :1 ;
    uint32_t             Pn9              :1 ;
    uint32_t             Pn10             :1 ;
    uint32_t             Pn11             :1 ;
    uint32_t             Pn12             :1 ;
    uint32_t             Pn13             :1 ;
    uint32_t             Pn14             :1 ;
    uint32_t             Pn15             :1 ;
    uint32_t             Pn16             :1 ;
    uint32_t             Pn17             :1 ;
    uint32_t             Pn18             :1 ;
    uint32_t             Pn19             :1 ;
    uint32_t             Pn20             :1 ;
    uint32_t             Pn21             :1 ;
    uint32_t             Pn22             :1 ;
    uint32_t             Pn23             :1 ;
    uint32_t             Pn24             :1 ;
    uint32_t             Pn25             :1 ;
    uint32_t             Pn26             :1 ;
    uint32_t             Pn27             :1 ;
    uint32_t             Pn28             :1 ;
    uint32_t             Pn29             :1 ;
    uint32_t             Pn30             :1 ;
    uint32_t             Pn31             :1 ;
}   GPIO_Pn_DATA;

typedef union {
    uint32_t             w;
    GPIO_Pn_DATA         b;
}   GPIO_Pn_DATA_U;


// 0x004 register ------------------------------------
typedef struct {
    uint32_t             PnDIR0           :1 ;
    uint32_t             PnDIR1           :1 ;
    uint32_t             PnDIR2           :1 ;
    uint32_t             PnDIR3           :1 ;
    uint32_t             PnDIR4           :1 ;
    uint32_t             PnDIR5           :1 ;
    uint32_t             PnDIR6           :1 ;
    uint32_t             PnDIR7           :1 ;
    uint32_t             PnDIR8           :1 ;
    uint32_t             PnDIR9           :1 ;
    uint32_t             PnDIR10          :1 ;
    uint32_t             PnDIR11          :1 ;
    uint32_t             PnDIR12          :1 ;
    uint32_t             PnDIR13          :1 ;
    uint32_t             PnDIR14          :1 ;
    uint32_t             PnDIR15          :1 ;
    uint32_t             PnDIR16          :1 ;
    uint32_t             PnDIR17          :1 ;
    uint32_t             PnDIR18          :1 ;
    uint32_t             PnDIR19          :1 ;
    uint32_t             PnDIR20          :1 ;
    uint32_t             PnDIR21          :1 ;
    uint32_t             PnDIR22          :1 ;
    uint32_t             PnDIR23          :1 ;
    uint32_t             PnDIR24          :1 ;
    uint32_t             PnDIR25          :1 ;
    uint32_t             PnDIR26          :1 ;
    uint32_t             PnDIR27          :1 ;
    uint32_t             PnDIR28          :1 ;
    uint32_t             PnDIR29          :1 ;
    uint32_t             PnDIR30          :1 ;
    uint32_t             PnDIR31          :1 ;
}   GPIO_Pn_DIR;

typedef union {
    uint32_t             w;
    GPIO_Pn_DIR          b;
}   GPIO_Pn_DIR_U;


// 0x008 register ------------------------------------
typedef struct {
    uint32_t             PnALT0_0         :1 ;
    uint32_t             PnALT0_1         :1 ;
    uint32_t             PnALT0_2         :1 ;
    uint32_t             PnALT0_3         :1 ;
    uint32_t             PnALT0_4         :1 ;
    uint32_t             PnALT0_5         :1 ;
    uint32_t             PnALT0_6         :1 ;
    uint32_t             PnALT0_7         :1 ;
    uint32_t             PnALT0_8         :1 ;
    uint32_t             PnALT0_9         :1 ;
    uint32_t             PnALT0_10        :1 ;
    uint32_t             PnALT0_11        :1 ;
    uint32_t             PnALT0_12        :1 ;
    uint32_t             PnALT0_13        :1 ;
    uint32_t             PnALT0_14        :1 ;
    uint32_t             PnALT0_15        :1 ;
    uint32_t             PnALT0_16        :1 ;
    uint32_t             PnALT0_17        :1 ;
    uint32_t             PnALT0_18        :1 ;
    uint32_t             PnALT0_19        :1 ;
    uint32_t             PnALT0_20        :1 ;
    uint32_t             PnALT0_21        :1 ;
    uint32_t             PnALT0_22        :1 ;
    uint32_t             PnALT0_23        :1 ;
    uint32_t             PnALT0_24        :1 ;
    uint32_t             PnALT0_25        :1 ;
    uint32_t             PnALT0_26        :1 ;
    uint32_t             PnALT0_27        :1 ;
    uint32_t             PnALT0_28        :1 ;
    uint32_t             PnALT0_29        :1 ;
    uint32_t             PnALT0_30        :1 ;
    uint32_t             PnALT0_31        :1 ;
}   GPIO_Pn_ALT0;

typedef union {
    uint32_t             w;
    GPIO_Pn_ALT0         b;
}   GPIO_Pn_ALT0_U;


// 0x00c register ------------------------------------
typedef struct {
    uint32_t             PnALT1_0         :1 ;
    uint32_t             PnALT1_1         :1 ;
    uint32_t             PnALT1_2         :1 ;
    uint32_t             PnALT1_3         :1 ;
    uint32_t             PnALT1_4         :1 ;
    uint32_t             PnALT1_5         :1 ;
    uint32_t             PnALT1_6         :1 ;
    uint32_t             PnALT1_7         :1 ;
    uint32_t             PnALT1_8         :1 ;
    uint32_t             PnALT1_9         :1 ;
    uint32_t             PnALT1_10        :1 ;
    uint32_t             PnALT1_11        :1 ;
    uint32_t             PnALT1_12        :1 ;
    uint32_t             PnALT1_13        :1 ;
    uint32_t             PnALT1_14        :1 ;
    uint32_t             PnALT1_15        :1 ;
    uint32_t             PnALT1_16        :1 ;
    uint32_t             PnALT1_17        :1 ;
    uint32_t             PnALT1_18        :1 ;
    uint32_t             PnALT1_19        :1 ;
    uint32_t             PnALT1_20        :1 ;
    uint32_t             PnALT1_21        :1 ;
    uint32_t             PnALT1_22        :1 ;
    uint32_t             PnALT1_23        :1 ;
    uint32_t             PnALT1_24        :1 ;
    uint32_t             PnALT1_25        :1 ;
    uint32_t             PnALT1_26        :1 ;
    uint32_t             PnALT1_27        :1 ;
    uint32_t             PnALT1_28        :1 ;
    uint32_t             PnALT1_29        :1 ;
    uint32_t             PnALT1_30        :1 ;
    uint32_t             PnALT1_31        :1 ;
}   GPIO_Pn_ALT1;

typedef union {
    uint32_t             w;
    GPIO_Pn_ALT1         b;
}   GPIO_Pn_ALT1_U;


// 0x030 register ------------------------------------
typedef struct {
    uint32_t             PnPU0            :1 ;
    uint32_t             PnPU1            :1 ;
    uint32_t             PnPU2            :1 ;
    uint32_t             PnPU3            :1 ;
    uint32_t             PnPU4            :1 ;
    uint32_t             PnPU5            :1 ;
    uint32_t             PnPU6            :1 ;
    uint32_t             PnPU7            :1 ;
    uint32_t             PnPU8            :1 ;
    uint32_t             PnPU9            :1 ;
    uint32_t             PnPU10           :1 ;
    uint32_t             PnPU11           :1 ;
    uint32_t             PnPU12           :1 ;
    uint32_t             PnPU13           :1 ;
    uint32_t             PnPU14           :1 ;
    uint32_t             PnPU15           :1 ;
    uint32_t             PnPU16           :1 ;
    uint32_t             PnPU17           :1 ;
    uint32_t             PnPU18           :1 ;
    uint32_t             PnPU19           :1 ;
    uint32_t             PnPU20           :1 ;
    uint32_t             PnPU21           :1 ;
    uint32_t             PnPU22           :1 ;
    uint32_t             PnPU23           :1 ;
    uint32_t             PnPU24           :1 ;
    uint32_t             PnPU25           :1 ;
    uint32_t             PnPU26           :1 ;
    uint32_t             PnPU27           :1 ;
    uint32_t             PnPU28           :1 ;
    uint32_t             PnPU29           :1 ;
    uint32_t             PnPU30           :1 ;
    uint32_t             PnPU31           :1 ;
}   GPIO_Pn_PULLUP;

typedef union {
    uint32_t             w;
    GPIO_Pn_PULLUP       b;
}   GPIO_Pn_PULLUP_U;

// 0x030 register ------------------------------------
typedef struct {
    uint32_t             PnPD0            :1 ;
    uint32_t             PnPD1            :1 ;
    uint32_t             PnPD2            :1 ;
    uint32_t             PnPD3            :1 ;
    uint32_t             PnPD4            :1 ;
    uint32_t             PnPD5            :1 ;
    uint32_t             PnPD6            :1 ;
    uint32_t             PnPD7            :1 ;
    uint32_t             PnPD8            :1 ;
    uint32_t             PnPD9            :1 ;
    uint32_t             PnPD10           :1 ;
    uint32_t             PnPD11           :1 ;
    uint32_t             PnPD12           :1 ;
    uint32_t             PnPD13           :1 ;
    uint32_t             PnPD14           :1 ;
    uint32_t             PnPD15           :1 ;
    uint32_t             PnPD16           :1 ;
    uint32_t             PnPD17           :1 ;
    uint32_t             PnPD18           :1 ;
    uint32_t             PnPD19           :1 ;
    uint32_t             PnPD20           :1 ;
    uint32_t             PnPD21           :1 ;
    uint32_t             PnPD22           :1 ;
    uint32_t             PnPD23           :1 ;
    uint32_t             PnPD24           :1 ;
    uint32_t             PnPD25           :1 ;
    uint32_t             PnPD26           :1 ;
    uint32_t             PnPD27           :1 ;
    uint32_t             PnPD28           :1 ;
    uint32_t             PnPD29           :1 ;
    uint32_t             PnPD30           :1 ;
    uint32_t             PnPD31           :1 ;
}   GPIO_Pn_PULLDOWN;

typedef union {
    uint32_t             	w;
    GPIO_Pn_PULLDOWN        b;
}   GPIO_Pn_PULLDOWN_U;


// 0x038 register ------------------------------------
typedef struct {
    uint32_t             PnDRV0           :16;
    uint32_t             reserved0        :16;
}   GPIO_Pn_DRV0;

typedef union {
    uint32_t             w;
    GPIO_Pn_DRV0         b;
}   GPIO_Pn_DRV0_U;


// 0x03c register ------------------------------------
typedef struct {
    uint32_t             PnDRV1           :16;
    uint32_t             reserved0        :16;
}   GPIO_Pn_DRV1;

typedef union {
    uint32_t             w;
    GPIO_Pn_DRV1         b;
}   GPIO_Pn_DRV1_U;


// 0x800 register ------------------------------------
typedef struct {
    uint32_t             I2C0_SCL         :8 ;
    uint32_t             I2C0_SDA         :8 ;
    uint32_t             I2C1_SCL         :8 ;
    uint32_t             I2C1_SDA         :8 ;
}   GPIO_UIO_SEL00;

typedef union {
    uint32_t             w;
    GPIO_UIO_SEL00       b;
}   GPIO_UIO_SEL00_U;

// 0x804 register ------------------------------------
typedef struct {
    uint32_t             I2C2_SCL         :8 ;
    uint32_t             I2C2_SDA         :8 ;
    uint32_t             I2C3_SCL         :8 ;
    uint32_t             I2C3_SDA         :8 ;
}   GPIO_UIO_SEL01;

typedef union {
    uint32_t             w;
    GPIO_UIO_SEL01       b;
}   GPIO_UIO_SEL01_U;

// 0x808 register ------------------------------------
typedef struct {
    uint32_t             I2CS0_SCL         :8 ;
    uint32_t             I2CS0_SDA         :8 ;
    uint32_t             I2CS1_SCL         :8 ;
    uint32_t             I2CS1_SDA         :8 ;
}   GPIO_UIO_SEL02;

typedef union {
    uint32_t             w;
    GPIO_UIO_SEL02       b;
}   GPIO_UIO_SEL02_U;

// 0x810 register ------------------------------------
typedef struct {
    uint32_t             SPI00_CLK        :8 ;
    uint32_t             SPI00_FSS        :8 ;
    uint32_t             SPI00_TXD        :8 ;
    uint32_t             SPI00_RXD        :8 ;
}   GPIO_UIO_SEL04;

typedef union {
    uint32_t             w;
    GPIO_UIO_SEL04       b;
}   GPIO_UIO_SEL04_U;


// 0x814 register ------------------------------------
typedef struct {
    uint32_t             SPI01_CLK        :8 ;
    uint32_t             SPI01_FSS        :8 ;
    uint32_t             SPI01_TXD        :8 ;
    uint32_t             SPI01_RXD        :8 ;
}   GPIO_UIO_SEL05;

typedef union {
    uint32_t             w;
    GPIO_UIO_SEL05       b;
}   GPIO_UIO_SEL05_U;

// 0x814 register ------------------------------------
typedef struct {
    uint32_t             SPI02_CLK        :8 ;
    uint32_t             SPI02_FSS        :8 ;
    uint32_t             SPI02_TXD        :8 ;
    uint32_t             SPI02_RXD        :8 ;
}   GPIO_UIO_SEL06;

typedef union {
    uint32_t             w;
    GPIO_UIO_SEL06       b;
}   GPIO_UIO_SEL06_U;

// 0x814 register ------------------------------------
typedef struct {
    uint32_t             SPI03_CLK        :8 ;
    uint32_t             SPI03_FSS        :8 ;
    uint32_t             SPI03_TXD        :8 ;
    uint32_t             SPI03_RXD        :8 ;
}   GPIO_UIO_SEL07;

typedef union {
    uint32_t             w;
    GPIO_UIO_SEL07       b;
}   GPIO_UIO_SEL07_U;

// 0x820 register ------------------------------------
typedef struct {
    uint32_t             UART00_TXD       :8 ;
    uint32_t             UART00_RXD       :8 ;
    uint32_t             UART00_RTS       :8 ;
    uint32_t             UART00_CTS       :8 ;
}   GPIO_UIO_SEL08;

typedef union {
    uint32_t             w;
    GPIO_UIO_SEL08       b;
}   GPIO_UIO_SEL08_U;


// 0x824 register ------------------------------------
typedef struct {
    uint32_t             UART01_TXD       :8 ;
    uint32_t             UART01_RXD       :8 ;
    uint32_t             UART01_RTS       :8 ;
    uint32_t             UART01_CTS       :8 ;
}   GPIO_UIO_SEL09;

typedef union {
    uint32_t             w;
    GPIO_UIO_SEL09       b;
}   GPIO_UIO_SEL09_U;


// 0x824 register ------------------------------------
typedef struct {
    uint32_t             UART02_TXD       :8 ;
    uint32_t             UART02_RXD       :8 ;
    uint32_t             UART02_RTS       :8 ;
    uint32_t             UART02_CTS       :8 ;
}   GPIO_UIO_SEL10;

typedef union {
    uint32_t             w;
    GPIO_UIO_SEL10       b;
}   GPIO_UIO_SEL10_U;

// 0x824 register ------------------------------------
typedef struct {
    uint32_t             UART03_TXD       :8 ;
    uint32_t             UART03_RXD       :8 ;
    uint32_t             UART03_RTS       :8 ;
    uint32_t             UART03_CTS       :8 ;
}   GPIO_UIO_SEL11;

typedef union {
    uint32_t             w;
    GPIO_UIO_SEL11       b;
}   GPIO_UIO_SEL11_U;

// 0x830 register ------------------------------------
typedef struct {
    uint32_t             PWM_A            :8 ;
    uint32_t             PWM_B            :8 ;
    uint32_t             PWM_C            :8 ;
    uint32_t             PWM_D            :8 ;
}   GPIO_UIO_SEL12;

typedef union {
    uint32_t             w;
    GPIO_UIO_SEL12       b;
}   GPIO_UIO_SEL12_U;

// 0x830 register ------------------------------------
typedef struct {
    uint32_t             PWM_E            :8 ;
    uint32_t             PWM_F            :8 ;
    uint32_t             PWM_G            :8 ;
    uint32_t             PWM_H            :8 ;
}   GPIO_UIO_SEL13;

typedef union {
    uint32_t             w;
    GPIO_UIO_SEL13       b;
}   GPIO_UIO_SEL13_U;

// 0x838 register ------------------------------------
typedef struct {
    uint32_t             EINT00           :8 ;
    uint32_t             EINT01           :8 ;
    uint32_t             EINT02        :8 ;
    uint32_t             EINT03        :8 ;
}   GPIO_UIO_SEL14;

typedef union {
    uint32_t             w;
    GPIO_UIO_SEL14       b;
}   GPIO_UIO_SEL14_U;


//---------------------------------------------------------
// GPIO Register Map
//---------------------------------------------------------
typedef struct {
    volatile GPIO_Pn_DATA_U              Pn_DATA            ; // 0x000
    volatile GPIO_Pn_DIR_U               Pn_DIR             ; // 0x004
    volatile GPIO_Pn_ALT0_U              Pn_ALT0            ; // 0x008
    volatile GPIO_Pn_ALT1_U              Pn_ALT1            ; // 0x00c
    volatile uint32_t                    RESERVED0[7]       ; // 
    volatile uint32_t          			 PD_INEN            ; // 0x010
    volatile GPIO_Pn_PULLUP_U            Pn_PULLUP          ; // 0x030
    volatile GPIO_Pn_PULLDOWN_U          Pn_PULLDOWN        ; // 
    volatile GPIO_Pn_DRV0_U              Pn_DRV0            ; // 0x038
    volatile GPIO_Pn_DRV1_U              Pn_DRV1            ; // 0x03c
    volatile uint32_t              		 PRF_DATA           ; // 0x03c
    volatile uint32_t              		 PRF_DIR            ; // 0x03c
    volatile uint32_t              		 PRF_ALT0           ; // 0x03c
    volatile uint32_t              		 PRF_ALT1           ; // 0x03c
    volatile uint32_t              		 PRF_ALT2           ; // 0x03c
    volatile uint32_t                    RESERVED2[6]       ; // 
    volatile uint32_t              		 PRF_IEN            ; // 0x03c
    volatile uint32_t              		 PRF_PULLUP         ; // 0x03c
    volatile uint32_t              		 PRF_PULLDOWN       ; // 0x03c
    volatile uint32_t              		 PRF_DRV0           ; // 0x03c
    volatile uint32_t              		 PRF_DRV1           ; // 0x03c
    volatile uint32_t                    RESERVED3[486]     ; // 
    volatile GPIO_UIO_SEL00_U            UIO_SEL00          ; // 0x800
    volatile GPIO_UIO_SEL01_U            UIO_SEL01          ; // 0x800
    volatile GPIO_UIO_SEL02_U            UIO_SEL02          ; // 0x800
    volatile uint32_t                    RESERVED12	    	; // 
    volatile GPIO_UIO_SEL04_U            UIO_SEL04          ; // 0x810
    volatile GPIO_UIO_SEL05_U            UIO_SEL05          ; // 0x814
    volatile GPIO_UIO_SEL06_U            UIO_SEL06          ; // 0x818
    volatile GPIO_UIO_SEL07_U            UIO_SEL07          ; // 0x81c
    volatile GPIO_UIO_SEL08_U            UIO_SEL08          ; // 0x820
    volatile GPIO_UIO_SEL09_U            UIO_SEL09          ; // 0x824
    volatile GPIO_UIO_SEL10_U            UIO_SEL10          ; // 0x828
    volatile GPIO_UIO_SEL11_U            UIO_SEL11          ; // 0x82C
    volatile GPIO_UIO_SEL12_U            UIO_SEL12          ; // 0x830
    volatile GPIO_UIO_SEL13_U            UIO_SEL13          ; // 
    volatile GPIO_UIO_SEL14_U            UIO_SEL14          ; // 0x838
} GPIO_Type;

#ifdef __cplusplus
}
#endif

#endif /* _REG_GPIO_H_ */
