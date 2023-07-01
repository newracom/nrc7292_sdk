/*********************************************************************/
/*    Newracom Library / IP Product                                  */
/*    Product Name :                                                 */
/*    Version      :                                                 */
/*    Author       :                                                 */
/*    Update       :                                                 */
/*********************************************************************/

#ifndef __REG_HSUART_H__
#define __REG_HSUART_H__

#ifdef __cplusplus
extern "C"{
#endif

// 0x000 register ------------------------------------
typedef struct {
    uint32_t             DATA            :8 ;
    uint32_t             FE              :1 ;
    uint32_t             PE              :1 ;
    uint32_t             BE              :1 ;
    uint32_t             OE              :1 ;
    uint32_t             reserved0       :20;
}   HSUART_DR;

typedef union {
    uint32_t             w;
    HSUART_DR            b;
}   HSUART_DR_U;


// 0x004 register ------------------------------------
typedef struct {
    uint32_t             FE              :1 ;
    uint32_t             PE              :1 ;
    uint32_t             BE              :1 ;
    uint32_t             OE              :1 ;
    uint32_t             reserved0       :28;
}   HSUART_RSR_ECR;

typedef union {
    uint32_t             w;
    HSUART_RSR_ECR       b;
}   HSUART_RSR_ECR_U;


// 0x018 register ------------------------------------
typedef struct {
    uint32_t             CTS             :1 ;
    uint32_t             DSR             :1 ;
    uint32_t             DCD             :1 ;
    uint32_t             BUSY            :1 ;
    uint32_t             RXFE            :1 ;
    uint32_t             TXFF            :1 ;
    uint32_t             RXFF            :1 ;
    uint32_t             TXFE            :1 ;
    uint32_t             RI              :1 ;
    uint32_t             reserved0       :23;
}   HSUART_FR;

typedef union {
    uint32_t             w;
    HSUART_FR            b;
}   HSUART_FR_U;


// 0x020 register ------------------------------------
typedef struct {
    uint32_t             ILPDVSR         :8 ;
    uint32_t             reserved0       :24;
}   HSUART_ILPR;

typedef union {
    uint32_t             w;
    HSUART_ILPR          b;
}   HSUART_ILPR_U;


// 0x024 register ------------------------------------
typedef struct {
    uint32_t             BAUD_DIVINT     :16;
    uint32_t             reserved0       :16;
}   HSUART_IBRD;

typedef union {
    uint32_t             w;
    HSUART_IBRD          b;
}   HSUART_IBRD_U;


// 0x028 register ------------------------------------
typedef struct {
    uint32_t             BAUD_DIVFRAC    :6 ;
    uint32_t             reserved0       :26;
}   HSUART_FBRD;

typedef union {
    uint32_t             w;
    HSUART_FBRD          b;
}   HSUART_FBRD_U;


// 0x02c register ------------------------------------
typedef struct {
    uint32_t             BRK             :1 ;
    uint32_t             PEN             :1 ;
    uint32_t             EPS             :1 ;
    uint32_t             STP2            :1 ;
    uint32_t             FEN             :1 ;
    uint32_t             WLEN            :2 ;
    uint32_t             SPS             :1 ;
    uint32_t             reserved0       :24;
}   HSUART_LCR_H;

typedef union {
    uint32_t             w;
    HSUART_LCR_H         b;
}   HSUART_LCR_H_U;


// 0x030 register ------------------------------------
typedef struct {
    uint32_t             UARTEN          :1 ;
    uint32_t             SIREN           :1 ;
    uint32_t             SIRLP           :1 ;
    uint32_t             reserved0       :4 ;
    uint32_t             LBE             :1 ;
    uint32_t             TXE             :1 ;
    uint32_t             RXE             :1 ;
    uint32_t             DTR             :1 ;
    uint32_t             RTS             :1 ;
    uint32_t             Out1            :1 ;
    uint32_t             Out2            :1 ;
    uint32_t             RTSEn           :1 ;
    uint32_t             CTSEn           :1 ;
    uint32_t             reserved1       :16;
}   HSUART_CR;

typedef union {
    uint32_t             w;
    HSUART_CR            b;
}   HSUART_CR_U;


// 0x034 register ------------------------------------
typedef struct {
    uint32_t             TXIFLSEL        :3 ;
    uint32_t             RXIFLSEL        :3 ;
    uint32_t             reserved0       :26;
}   HSUART_IFLS;

typedef union {
    uint32_t             w;
    HSUART_IFLS          b;
}   HSUART_IFLS_U;


// 0x038 register ------------------------------------
typedef struct {
    uint32_t             RIMIM           :1 ;
    uint32_t             CTSMIM          :1 ;
    uint32_t             DCDMIM          :1 ;
    uint32_t             DSRMIM          :1 ;
    uint32_t             RXIM            :1 ;
    uint32_t             TXIM            :1 ;
    uint32_t             RTIM            :1 ;
    uint32_t             FEIM            :1 ;
    uint32_t             PEIM            :1 ;
    uint32_t             BEIM            :1 ;
    uint32_t             OEIM            :1 ;
    uint32_t             reserved0       :21;
}   HSUART_IMSC;

typedef union {
    uint32_t             w;
    HSUART_IMSC          b;
}   HSUART_IMSC_U;


// 0x03c register ------------------------------------
typedef struct {
    uint32_t             RIRMIS          :1 ;
    uint32_t             CTSRMIS         :1 ;
    uint32_t             DCDRMIS         :1 ;
    uint32_t             DSRRMIS         :1 ;
    uint32_t             RXRIS           :1 ;
    uint32_t             TXRIS           :1 ;
    uint32_t             RTRIS           :1 ;
    uint32_t             FERIS           :1 ;
    uint32_t             PERIS           :1 ;
    uint32_t             BERIS           :1 ;
    uint32_t             OERIS           :1 ;
    uint32_t             reserved0       :21;
}   HSUART_RIS;

typedef union {
    uint32_t             w;
    HSUART_RIS           b;
}   HSUART_RIS_U;


// 0x040 register ------------------------------------
typedef struct {
    uint32_t             RIMMIS          :1 ;
    uint32_t             CTSMMIS         :1 ;
    uint32_t             DCDMMIS         :1 ;
    uint32_t             DSRMMIS         :1 ;
    uint32_t             RXMIS           :1 ;
    uint32_t             TXMIS           :1 ;
    uint32_t             RTMIS           :1 ;
    uint32_t             FEMIS           :1 ;
    uint32_t             PEMIS           :1 ;
    uint32_t             BEMIS           :1 ;
    uint32_t             OEMIS           :1 ;
    uint32_t             reserved0       :21;
}   HSUART_MIS;

typedef union {
    uint32_t             w;
    HSUART_MIS           b;
}   HSUART_MIS_U;


// 0x044 register ------------------------------------
typedef struct {
    uint32_t             RIMIC           :1 ;
    uint32_t             CTSMIC          :1 ;
    uint32_t             DCDMIC          :1 ;
    uint32_t             DSRMIC          :1 ;
    uint32_t             RXIC            :1 ;
    uint32_t             TXIC            :1 ;
    uint32_t             RTIC            :1 ;
    uint32_t             FEIC            :1 ;
    uint32_t             PEIC            :1 ;
    uint32_t             BEIC            :1 ;
    uint32_t             OEIC            :1 ;
    uint32_t             reserved0       :21;
}   HSUART_ICR;

typedef union {
    uint32_t             w;
    HSUART_ICR           b;
}   HSUART_ICR_U;


// 0x048 register ------------------------------------
typedef struct {
    uint32_t             RXDMAE          :1 ;
    uint32_t             TXDMAE          :1 ;
    uint32_t             DMAONERR        :1 ;
    uint32_t             reserved0       :29;
}   HSUART_DMACR;

typedef union {
    uint32_t             w;
    HSUART_DMACR         b;
}   HSUART_DMACR_U;


//---------------------------------------------------------
// HSUART Register Map
//---------------------------------------------------------
typedef struct {
    volatile HSUART_DR_U                 DR               ; // 0x000
    volatile HSUART_RSR_ECR_U            RSR_ECR          ; // 0x004
    volatile uint32_t                    RESERVED0[4]     ; // 
    volatile HSUART_FR_U                 FR               ; // 0x018
    volatile uint32_t                    RESERVED1        ; // 
    volatile HSUART_ILPR_U               ILPR             ; // 0x020
    volatile HSUART_IBRD_U               IBRD             ; // 0x024
    volatile HSUART_FBRD_U               FBRD             ; // 0x028
    volatile HSUART_LCR_H_U              LCR_H            ; // 0x02c
    volatile HSUART_CR_U                 CR               ; // 0x030
    volatile HSUART_IFLS_U               IFLS             ; // 0x034
    volatile HSUART_IMSC_U               IMSC             ; // 0x038
    volatile HSUART_RIS_U                RIS              ; // 0x03c
    volatile HSUART_MIS_U                MIS              ; // 0x040
    volatile HSUART_ICR_U                ICR              ; // 0x044
    volatile HSUART_DMACR_U              DMACR            ; // 0x048
} HSUART_Type;

#ifdef __cplusplus
}
#endif

#endif /* _REG_HSUART_H_ */
