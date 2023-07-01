/*********************************************************************/
/*    Newracom Library / IP Product                                  */
/*    Product Name :                                                 */
/*    Version      :                                                 */
/*    Author       :                                                 */
/*    Update       :                                                 */
/*********************************************************************/

#ifndef __REG_SFC_H__
#define __REG_SFC_H__

#ifdef __cplusplus
extern "C"{
#endif

// 0x00 register ------------------------------------
typedef struct {
    uint32_t        FLRMOD      :2 ;
    uint32_t        reserved0   :1 ;
    uint32_t        BUSRC       :1 ;
    uint32_t        PEM         :1 ;
    uint32_t        EQIOF       :1 ;
    uint32_t        reserved1   :1 ;
    uint32_t        BUSEEn      :1 ;
    uint32_t        CSC         :1 ;
    uint32_t        reserved2   :23;
}   SFC_FLMOD;

typedef union {
    uint32_t        w;
    SFC_FLMOD       b;
}   SFC_FLMOD_U;


// 0x04 register ------------------------------------
typedef struct {
    uint32_t        SCKL        :4 ;
    uint32_t        SCKH        :4 ;
    uint32_t        reserved0   :24;
}   SFC_FLBRT;

typedef union {
    uint32_t        w;
    SFC_FLBRT       b;
}   SFC_FLBRT_U;


// 0x08 register ------------------------------------
typedef struct {
    uint32_t        CSHPW       :4 ;
    uint32_t        reserved0   :28;
}   SFC_FLCSH;

typedef union {
    uint32_t        w;
    SFC_FLCSH       b;
}   SFC_FLCSH_U;


// 0x0c register ------------------------------------
typedef struct {
    uint32_t        EN          :1 ;
    uint32_t        reserved0   :31;
}   SFC_FLPEM;

typedef union {
    uint32_t        w;
    SFC_FLPEM       b;
}   SFC_FLPEM_U;


// 0x10 register ------------------------------------
typedef struct {
    uint32_t        CMD         :8 ;
    uint32_t        reserved0   :24;
}   SFC_FLCMD;

typedef union {
    uint32_t        w;
    SFC_FLCMD       b;
}   SFC_FLCMD_U;


// 0x14 register ------------------------------------
typedef struct {
    uint32_t        STS         :8 ;
    uint32_t        reserved0   :24;
}   SFC_FLSTS;

typedef union {
    uint32_t        w;
    SFC_FLSTS       b;
}   SFC_FLSTS_U;


// 0x18 register ------------------------------------
typedef struct {
    uint32_t        SEA         :24;
    uint32_t        reserved0   :8 ;
}   SFC_FLSEA;

typedef union {
    uint32_t        w;
    SFC_FLSEA       b;
}   SFC_FLSEA_U;


// 0x1c register ------------------------------------
typedef struct {
    uint32_t        BEA         :24;
    uint32_t        reserved0   :8 ;
}   SFC_FLBEA;

typedef union {
    uint32_t        w;
    SFC_FLBEA       b;
}   SFC_FLBEA_U;


// 0x20 register ------------------------------------
typedef struct {
    uint32_t        FLDAT       :32;
}   SFC_FLDAT;

typedef union {
    uint32_t        w;
    SFC_FLDAT       b;
}   SFC_FLDAT_U;


// 0x24 register ------------------------------------
typedef struct {
    uint32_t        FLWCP       :32;
}   SFC_FLWCP;

typedef union {
    uint32_t        w;
    SFC_FLWCP       b;
}   SFC_FLWCP_U;


// 0x28 register ------------------------------------
typedef struct {
    uint32_t        SFFCDV      :4 ;
    uint32_t        reserved0   :28;
}   SFC_FLCKDLY;

typedef union {
    uint32_t        w;
    SFC_FLCKDLY     b;
}   SFC_FLCKDLY_U;


// 0x2c register ------------------------------------
typedef struct {
    uint32_t        STS2        :8 ;
    uint32_t        reserved0   :24;
}   SFC_FLSTS2;

typedef union {
    uint32_t        w;
    SFC_FLSTS2      b;
}   SFC_FLSTS2_U;


// 0x30 register ------------------------------------
typedef struct {
    uint32_t        CEN         :1 ;
    uint32_t        CWRAP       :1 ;
    uint32_t        CINVAL      :1 ;
    uint32_t        reserved0   :29;
}   SFC_CCTRL;

typedef union {
    uint32_t        w;
    SFC_CCTRL       b;
}   SFC_CCTRL_U;


// 0x38 register ------------------------------------
typedef struct {
    uint32_t        WRCMD       :8 ;
    uint32_t        reserved0   :24;
}   SFC_WRCMD;

typedef union {
    uint32_t        w;
    SFC_WRCMD       b;
}   SFC_WRCMD_U;


//---------------------------------------------------------
// SFC Register Map
//---------------------------------------------------------
typedef struct {
    volatile SFC_FLMOD_U            FLMOD          ; // 0x00
    volatile SFC_FLBRT_U            FLBRT          ; // 0x04
    volatile SFC_FLCSH_U            FLCSH          ; // 0x08
    volatile SFC_FLPEM_U            FLPEM          ; // 0x0c
    volatile SFC_FLCMD_U            FLCMD          ; // 0x10
    volatile SFC_FLSTS_U            FLSTS          ; // 0x14
    volatile SFC_FLSEA_U            FLSEA          ; // 0x18
    volatile SFC_FLBEA_U            FLBEA          ; // 0x1c
    volatile SFC_FLDAT_U            FLDAT          ; // 0x20
    volatile SFC_FLWCP_U            FLWCP          ; // 0x24
    volatile SFC_FLCKDLY_U          FLCKDLY        ; // 0x28
    volatile SFC_FLSTS2_U           FLSTS2         ; // 0x2c
    volatile SFC_CCTRL_U            CCTRL          ; // 0x30
    volatile uint32_t               RESERVED0      ; // 
    volatile SFC_WRCMD_U            WRCMD          ; // 0x38
} SFC_Type;

#ifdef __cplusplus
}
#endif

#endif /* __REG_SFC_H__ */
