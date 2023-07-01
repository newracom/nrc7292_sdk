/*********************************************************************/
/*    Newracom Library / IP Product                                  */
/*    Product Name :                                                 */
/*    Version      :                                                 */
/*    Author       :                                                 */
/*    Update       :                                                 */
/*********************************************************************/

#ifndef __REG_SSP_H__
#define __REG_SSP_H__

#ifdef __cplusplus
extern "C"{
#endif

// 0x00 register ------------------------------------
typedef struct {
    uint32_t      DSS         :4 ;
    uint32_t      FRF         :2 ;
    uint32_t      SPO         :1 ;
    uint32_t      SPH         :1 ;
    uint32_t      SCR         :8 ;
    uint32_t      reserved0   :16;
}   SSP_CR0;

typedef union {
    uint32_t      w;
    SSP_CR0       b;
}   SSP_CR0_U;


// 0x04 register ------------------------------------
typedef struct {
    uint32_t      LBM         :1 ;
    uint32_t      SSE         :1 ;
    uint32_t      MS          :1 ;
    uint32_t      SOD         :1 ;
    uint32_t      reserved0   :28;
}   SSP_CR1;

typedef union {
    uint32_t      w;
    SSP_CR1       b;
}   SSP_CR1_U;


// 0x08 register ------------------------------------
typedef struct {
    uint32_t      DATA        :16;
    uint32_t      reserved0   :16;
}   SSP_DR;

typedef union {
    uint32_t      w;
    SSP_DR        b;
}   SSP_DR_U;


// 0x0c register ------------------------------------
typedef struct {
    uint32_t      TFE         :1 ;
    uint32_t      TNF         :1 ;
    uint32_t      RNE         :1 ;
    uint32_t      RFF         :1 ;
    uint32_t      BSY         :1 ;
    uint32_t      reserved0   :27;
}   SSP_SR;

typedef union {
    uint32_t      w;
    SSP_SR        b;
}   SSP_SR_U;


// 0x10 register ------------------------------------
typedef struct {
    uint32_t      CPSDVSR     :8 ;
    uint32_t      reserved0   :24;
}   SSP_CPSR;

typedef union {
    uint32_t      w;
    SSP_CPSR      b;
}   SSP_CPSR_U;


// 0x14 register ------------------------------------
typedef struct {
    uint32_t      RORIM       :1 ;
    uint32_t      RTIM        :1 ;
    uint32_t      RXIM        :1 ;
    uint32_t      TXIM        :1 ;
    uint32_t      reserved0   :28;
}   SSP_IMSC;

typedef union {
    uint32_t      w;
    SSP_IMSC      b;
}   SSP_IMSC_U;


// 0x18 register ------------------------------------
typedef struct {
    uint32_t      RORRIS      :1 ;
    uint32_t      RTRIS       :1 ;
    uint32_t      RXRIS       :1 ;
    uint32_t      TXRIS       :1 ;
    uint32_t      reserved0   :28;
}   SSP_RIS;

typedef union {
    uint32_t      w;
    SSP_RIS       b;
}   SSP_RIS_U;


// 0x1c register ------------------------------------
typedef struct {
    uint32_t      RORMIS      :1 ;
    uint32_t      RTMIS       :1 ;
    uint32_t      RXMIS       :1 ;
    uint32_t      TXMIS       :1 ;
    uint32_t      reserved0   :28;
}   SSP_MIS;

typedef union {
    uint32_t      w;
    SSP_MIS       b;
}   SSP_MIS_U;


// 0x20 register ------------------------------------
typedef struct {
    uint32_t      RORIC       :1 ;
    uint32_t      RTIC        :1 ;
    uint32_t      reserved0   :30;
}   SSP_ICR;

typedef union {
    uint32_t      w;
    SSP_ICR       b;
}   SSP_ICR_U;


// 0x24 register ------------------------------------
typedef struct {
    uint32_t      RXDMAE      :1 ;
    uint32_t      TXDMAE      :1 ;
    uint32_t      reserved0   :30;
}   SSP_DMACR;

typedef union {
    uint32_t      w;
    SSP_DMACR     b;
}   SSP_DMACR_U;


//---------------------------------------------------------
// SSP Register Map
//---------------------------------------------------------
typedef struct {
    volatile SSP_CR0_U            CR0          ; // 0x00
    volatile SSP_CR1_U            CR1          ; // 0x04
    volatile SSP_DR_U             DR           ; // 0x08
    volatile SSP_SR_U             SR           ; // 0x0c
    volatile SSP_CPSR_U           CPSR         ; // 0x10
    volatile SSP_IMSC_U           IMSC         ; // 0x14
    volatile SSP_RIS_U            RIS          ; // 0x18
    volatile SSP_MIS_U            MIS          ; // 0x1c
    volatile SSP_ICR_U            ICR          ; // 0x20
    volatile SSP_DMACR_U          DMACR        ; // 0x24
} SSP_Type;

#ifdef __cplusplus
}
#endif

#endif /* __REG_SSP_H__ */
