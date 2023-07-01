/*********************************************************************/
/*    Newracom Library / IP Product                                  */
/*    Product Name :                                                 */
/*    Version      :                                                 */
/*    Author       :                                                 */
/*    Update       :                                                 */
/*********************************************************************/

#ifndef __REG_EFUSE_H__
#define __REG_EFUSE_H__

#ifdef __cplusplus
extern "C"{
#endif

// 0x000 register ------------------------------------
typedef struct {
    uint32_t           START       :1 ;
    uint32_t           MODE        :1 ;
    uint32_t           reserved0   :5 ;
    uint32_t           INTEN       :1 ;
    uint32_t           SETUP_LD    :2 ;
    uint32_t           HOLD_LD     :2 ;
    uint32_t           PRD_LD      :9 ;
    uint32_t           reserved1   :3 ;
    uint32_t           ADDR        :5 ;
    uint32_t           reserved2   :3 ;
}   EFUSE_CTRL;

typedef union {
    uint32_t           w;
    EFUSE_CTRL         b;
}   EFUSE_CTRL_U;


// 0x004 register ------------------------------------
typedef struct {
    uint32_t           WDATA       :32;
}   EFUSE_WDATA;

typedef union {
    uint32_t           w;
    EFUSE_WDATA        b;
}   EFUSE_WDATA_U;


// 0x008 register ------------------------------------
typedef struct {
    uint32_t           RDATA       :32;
}   EFUSE_RDATA;

typedef union {
    uint32_t           w;
    EFUSE_RDATA        b;
}   EFUSE_RDATA_U;


// 0x00c register ------------------------------------
typedef struct {
    uint32_t           FLAG        :1 ;
    uint32_t           reserved0   :31;
}   EFUSE_STAT;

typedef union {
    uint32_t           w;
    EFUSE_STAT         b;
}   EFUSE_STAT_U;


//---------------------------------------------------------
// EFUSE Register Map
//---------------------------------------------------------
typedef struct {
    volatile EFUSE_CTRL_U              CTRL            ; // 0x000
    volatile EFUSE_WDATA_U             WDATA           ; // 0x004
    volatile EFUSE_RDATA_U             RDATA           ; // 0x008
    volatile EFUSE_STAT_U              STAT            ; // 0x00c
} EFUSE_Type;

#ifdef __cplusplus
}
#endif

#endif /* __REG_EFUSE_H__ */

