/*********************************************************************/
/*    Newracom Library / IP Product                                  */
/*    Product Name :                                                 */
/*    Version      :                                                 */
/*    Author       :                                                 */
/*    Update       :                                                 */
/*********************************************************************/

#ifndef __REG_AUXADC_H__
#define __REG_AUXADC_H__

#ifdef __cplusplus
extern "C"{
#endif

// 0x000 register ------------------------------------
typedef struct {
    uint32_t          SWEN         :1 ;
    uint32_t          CHSEL        :2 ;
    uint32_t          reserved0    :29;
}   AUXADC_CTRL;

typedef union {
    uint32_t          w;
    AUXADC_CTRL       b;
}   AUXADC_CTRL_U;


// 0x004 register ------------------------------------
typedef struct {
    uint32_t          CH0_DATA     :10;
    uint32_t          CH0_VALID    :1 ;
    uint32_t          reserved0    :5 ;
    uint32_t          CH1_DATA     :10;
    uint32_t          CH1_VALID    :1 ;
    uint32_t          reserved1    :5 ;
}   AUXADC_DATA01;

typedef union {
    uint32_t          w;
    AUXADC_DATA01     b;
}   AUXADC_DATA01_U;


// 0x008 register ------------------------------------
typedef struct {
    uint32_t          CH2_DATA     :10;
    uint32_t          CH2_VALID    :1 ;
    uint32_t          reserved0    :5 ;
    uint32_t          CH3_DATA     :10;
    uint32_t          CH3_VALID    :1 ;
    uint32_t          reserved1    :5 ;
}   AUXADC_DATA23;

typedef union {
    uint32_t          w;
    AUXADC_DATA23     b;
}   AUXADC_DATA23_U;


//---------------------------------------------------------
// AUXADC Register Map
//---------------------------------------------------------
typedef struct {
    volatile AUXADC_CTRL_U            CTRL          ; // 0x000
    volatile AUXADC_DATA01_U          DATA01        ; // 0x004
    volatile AUXADC_DATA23_U          DATA23        ; // 0x008
} AUXADC_Type;

#ifdef __cplusplus
}
#endif

#endif /* __REG_AUXADC_H__ */
