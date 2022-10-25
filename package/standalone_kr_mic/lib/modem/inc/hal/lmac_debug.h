#ifndef LMAC_DEBUG_H
#define LMAC_DEBUG_H

#if defined(INCLUDE_LMAC_DEBUG)
#define LMAC_STATUS( n , r , f)\
    static uint32_t n , n##_old;\
    reg = READ_REG( r );\
    reg = GET_FIELD( r , f , reg );\
    n = reg - n##_old;\
    n##_old = reg;

#define LMAC_STATUS_ONE( n , r , f)\
    static uint32_t n;\
    reg = READ_REG( r );\
    n   = GET_FIELD( r , f , reg );\

#define LMAC_STATUS_INDIR( n , set , i , get )\
    static uint32_t __attribute__((__unused__)) n , n##_old;\
    WRITE_REG( set , i);\
    reg = READ_REG( get );\
    n = reg - n##_old;\
    n##_old = reg;

void lmac_report();
void lmac_print_vector(LMAC_TXBUF* buf);
void lmac_print_macheader(LMAC_TXBUF *buf);
void lmac_print_txbd(LMAC_TXBUF* buf);
#if defined(INCLUDE_COMPACT_CLI)
static inline void lmac_print_sys_buf(struct _SYS_BUF *buffer) {}
void lmac_show_tx_stats_mcs_agg();
#else
void lmac_print_sys_buf(struct _SYS_BUF *buffer);
#endif /* defined(INCLUDE_COMPACT_CLI) */
#else
static inline void lmac_report() {};
static inline void lmac_print_vector(LMAC_TXBUF* buf) {}
static inline void lmac_print_macheader(LMAC_TXBUF *buf) {}
static inline void lmac_print_txbd(LMAC_TXBUF* buf) {}
static inline void lmac_print_sys_buf(struct _SYS_BUF *buffer) {}
#endif /* defined(INCLUDE_LMAC_DEBUG) */
#endif /* LMAC_DEBUG_H */
