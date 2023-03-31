#include <drv_sec.h>
#include "mbedtls/sha256.h"
#include "mbedtls/platform_util.h"

#define mbedtls_free       free
#define mbedtls_calloc     calloc

#define SHA256_VALIDATE_RET(cond)                           \
    MBEDTLS_INTERNAL_VALIDATE_RET( cond, MBEDTLS_ERR_SHA256_BAD_INPUT_DATA )
#define SHA256_VALIDATE(cond)  MBEDTLS_INTERNAL_VALIDATE( cond )

/*
 * 32-bit integer manipulation macros (big endian)
 */
#ifndef GET_UINT32_BE
#define GET_UINT32_BE(n,b,i)                            \
do {                                                    \
    (n) = ( (uint32_t) (b)[(i)    ] << 24 )             \
        | ( (uint32_t) (b)[(i) + 1] << 16 )             \
        | ( (uint32_t) (b)[(i) + 2] <<  8 )             \
        | ( (uint32_t) (b)[(i) + 3]       );            \
} while( 0 )
#endif

#ifndef PUT_UINT32_BE
#define PUT_UINT32_BE(n,b,i)                            \
do {                                                    \
    (b)[(i)    ] = (unsigned char) ( (n) >> 24 );       \
    (b)[(i) + 1] = (unsigned char) ( (n) >> 16 );       \
    (b)[(i) + 2] = (unsigned char) ( (n) >>  8 );       \
    (b)[(i) + 3] = (unsigned char) ( (n)       );       \
} while( 0 )
#endif

int mbedtls_internal_sha256_process( mbedtls_sha256_context *ctx,
                                const unsigned char data[64] )
{
    static unsigned char tmp[64];
    int ret = 0;
    int j;

    SHA256_VALIDATE_RET( ctx != NULL );
    SHA256_VALIDATE_RET( (const unsigned char *)data != NULL );

    memcpy(tmp, data, 64);

    for(j=0;j<64;j+=4) 
        convert_endian((tmp + j), 4);

    ret = sha_msg_update (tmp, 64);
    if( ret != 0 )
        goto fail_hw;
    
    sha_start();
    if( sha_is_done() ==0 ) {
        ret = 1;
        goto fail_hw;
    }

    ret = sha_get_result ((uint8_t*)ctx->state, 32);
    if( ret != 0 )
        goto fail_hw;

    ret = sha_set_firstblk(0);
    if(  ret != 0 )
        goto fail_hw;

fail_hw:
    return ret;

}

/*
 * SHA-256 final digest
 */
int mbedtls_sha256_finish_ret( mbedtls_sha256_context *ctx,
                               unsigned char output[32] )
{
    int ret;
    uint32_t used;
    uint32_t high, low;

    SHA256_VALIDATE_RET( ctx != NULL );
    SHA256_VALIDATE_RET( (unsigned char *)output != NULL );

    /*
     * Add padding: 0x80 then 0x00 until 8 bytes remain for the length
     */
    used = ctx->total[0] & 0x3F;

    ctx->buffer[used++] = 0x80;

    if( used <= 56 )
    {
        /* Enough room for padding + length in current block */
        memset( ctx->buffer + used, 0, 56 - used );
    }
    else
    {
        /* We'll need an extra block */
        memset( ctx->buffer + used, 0, 64 - used );

        if( ( ret = mbedtls_internal_sha256_process( ctx, ctx->buffer ) ) != 0 )
            return( ret );

        memset( ctx->buffer, 0, 56 );
    }

    /*
     * Add message length
     */
    high = ( ctx->total[0] >> 29 )
         | ( ctx->total[1] <<  3 );
    low  = ( ctx->total[0] <<  3 );

    PUT_UINT32_BE(ctx->total[1]*8, ctx->buffer, 56);    
    PUT_UINT32_BE(ctx->total[0]*8, ctx->buffer, 60);    

    if( ( ret = mbedtls_internal_sha256_process( ctx, ctx->buffer ) ) != 0 )
        return( ret );

    /*
     * Output final state
     */
    PUT_UINT32_BE( ctx->state[0], output,  0 );
    PUT_UINT32_BE( ctx->state[1], output,  4 );
    PUT_UINT32_BE( ctx->state[2], output,  8 );
    PUT_UINT32_BE( ctx->state[3], output, 12 );
    PUT_UINT32_BE( ctx->state[4], output, 16 );
    PUT_UINT32_BE( ctx->state[5], output, 20 );
    PUT_UINT32_BE( ctx->state[6], output, 24 );

    if( ctx->is224 == 0 )
        PUT_UINT32_BE( ctx->state[7], output, 28 );

    bignum_init(0);

    return( 0 );
}

int mbedtls_sha256_starts_ret( mbedtls_sha256_context *ctx, int is224 )
{
    int ret = 0;

    SHA256_VALIDATE_RET( ctx != NULL );
    SHA256_VALIDATE_RET( is224 == 0 || is224 == 1 );

    ctx->total[0] = 0;
    ctx->total[1] = 0;
    ctx->is224 = is224;

    sha_init(ENDIAN_BI);

    ret = sha_configure(SHA_MODE_2, SHA_BIT_256, is224, 1);
    if( ret != 0 ) 
    { 
        printf(" sha1_unit_update function is failed !!!\n");
        goto fail_hw;
    }

fail_hw :

    return( ret );
}

int mbedtls_sha256_update_ret( mbedtls_sha256_context *ctx,
                               const unsigned char *input,
                               size_t ilen )
{
    int ret;
    size_t fill;
    uint32_t left;

    SHA256_VALIDATE_RET( ctx != NULL );
    SHA256_VALIDATE_RET( ilen == 0 || input != NULL );

    if( ilen == 0 )
        return( 0 );

    left = ctx->total[0] & 0x3F;
    fill = 64 - left;

    ctx->total[0] += (uint32_t) ilen;
    ctx->total[0] &= 0xFFFFFFFF;

    if( ctx->total[0] < (uint32_t) ilen )
        ctx->total[1]++;

    if( left && ilen >= fill )
    {
        memcpy( (void *) (ctx->buffer + left), input, fill );

        if( ( ret = mbedtls_internal_sha256_process( ctx, ctx->buffer ) ) != 0 )
            return( ret );

        input += fill;
        ilen  -= fill;
        left = 0;
    }

    while( ilen >= 64 )
    {
        if( ( ret = mbedtls_internal_sha256_process( ctx, input ) ) != 0 )
            return( ret );

        input += 64;
        ilen  -= 64;
    }

    if( ilen > 0 )
        memcpy( (void *) (ctx->buffer + left), input, ilen );

    return( 0 );
}

void mbedtls_sha256_init( mbedtls_sha256_context *ctx )
{
    SHA256_VALIDATE_RET( ctx != NULL );

    memset( ctx, 0, sizeof( mbedtls_sha256_context ) );
}

void mbedtls_sha256_free( mbedtls_sha256_context *ctx )
{
    if( ctx == NULL )
        return;

    mbedtls_platform_zeroize( ctx, sizeof( mbedtls_sha256_context ) );
}


void mbedtls_sha256_clone( mbedtls_sha256_context *dst,
                           const mbedtls_sha256_context *src )
{
    SHA256_VALIDATE( dst != NULL );
    SHA256_VALIDATE( src != NULL );

    *dst = *src;
}

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
void mbedtls_sha256_starts( mbedtls_sha256_context *ctx,
                            int is224 )
{
    mbedtls_sha256_starts_ret( ctx, is224 );
}
#endif

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
void mbedtls_sha256_update( mbedtls_sha256_context *ctx,
                            const unsigned char *input,
                            size_t ilen )
{
    mbedtls_sha256_update_ret( ctx, input, ilen );
}
#endif


#if !defined(MBEDTLS_DEPRECATED_REMOVED)
void mbedtls_sha256_finish( mbedtls_sha256_context *ctx,
                            unsigned char output[32] )
{
    mbedtls_sha256_finish_ret( ctx, output );
}
#endif

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
void mbedtls_sha256_process( mbedtls_sha256_context *ctx,
                             const unsigned char data[64] )
{
    mbedtls_internal_sha256_process( ctx, data );
}
#endif


/*
 * output = SHA-256( input buffer )
 */
int mbedtls_sha256_ret( const unsigned char *input,
                        size_t ilen,
                        unsigned char output[32],
                        int is224 )
{
    int ret;
    mbedtls_sha256_context ctx;

    SHA256_VALIDATE_RET( is224 == 0 || is224 == 1 );
    SHA256_VALIDATE_RET( ilen == 0 || input != NULL );
    SHA256_VALIDATE_RET( (unsigned char *)output != NULL );

    mbedtls_sha256_init( &ctx );

    if( ( ret = mbedtls_sha256_starts_ret( &ctx, is224 ) ) != 0 )
        goto exit;

    if( ( ret = mbedtls_sha256_update_ret( &ctx, input, ilen ) ) != 0 )
        goto exit;

    if( ( ret = mbedtls_sha256_finish_ret( &ctx, output ) ) != 0 )
        goto exit;

exit:
    mbedtls_sha256_free( &ctx );

    return( ret );
}


#if defined(MBEDTLS_SELF_TEST)
/*
 * FIPS-180-2 test vectors
 */
static const unsigned char sha256_test_buf[3][57] =
{
    { "abc" },
    { "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq" },
    { "" }
};

static const size_t sha256_test_buflen[3] =
{
    3, 56, 1000
};

static const unsigned char sha256_test_sum[6][32] =
{
    /*
     * SHA-224 test vectors
     */
    { 0x23, 0x09, 0x7D, 0x22, 0x34, 0x05, 0xD8, 0x22,
      0x86, 0x42, 0xA4, 0x77, 0xBD, 0xA2, 0x55, 0xB3,
      0x2A, 0xAD, 0xBC, 0xE4, 0xBD, 0xA0, 0xB3, 0xF7,
      0xE3, 0x6C, 0x9D, 0xA7 },
    { 0x75, 0x38, 0x8B, 0x16, 0x51, 0x27, 0x76, 0xCC,
      0x5D, 0xBA, 0x5D, 0xA1, 0xFD, 0x89, 0x01, 0x50,
      0xB0, 0xC6, 0x45, 0x5C, 0xB4, 0xF5, 0x8B, 0x19,
      0x52, 0x52, 0x25, 0x25 },
    { 0x20, 0x79, 0x46, 0x55, 0x98, 0x0C, 0x91, 0xD8,
      0xBB, 0xB4, 0xC1, 0xEA, 0x97, 0x61, 0x8A, 0x4B,
      0xF0, 0x3F, 0x42, 0x58, 0x19, 0x48, 0xB2, 0xEE,
      0x4E, 0xE7, 0xAD, 0x67 },

    /*
     * SHA-256 test vectors
     */
    { 0xBA, 0x78, 0x16, 0xBF, 0x8F, 0x01, 0xCF, 0xEA,
      0x41, 0x41, 0x40, 0xDE, 0x5D, 0xAE, 0x22, 0x23,
      0xB0, 0x03, 0x61, 0xA3, 0x96, 0x17, 0x7A, 0x9C,
      0xB4, 0x10, 0xFF, 0x61, 0xF2, 0x00, 0x15, 0xAD },
    { 0x24, 0x8D, 0x6A, 0x61, 0xD2, 0x06, 0x38, 0xB8,
      0xE5, 0xC0, 0x26, 0x93, 0x0C, 0x3E, 0x60, 0x39,
      0xA3, 0x3C, 0xE4, 0x59, 0x64, 0xFF, 0x21, 0x67,
      0xF6, 0xEC, 0xED, 0xD4, 0x19, 0xDB, 0x06, 0xC1 },
    { 0xCD, 0xC7, 0x6E, 0x5C, 0x99, 0x14, 0xFB, 0x92,
      0x81, 0xA1, 0xC7, 0xE2, 0x84, 0xD7, 0x3E, 0x67,
      0xF1, 0x80, 0x9A, 0x48, 0xA4, 0x97, 0x20, 0x0E,
      0x04, 0x6D, 0x39, 0xCC, 0xC7, 0x11, 0x2C, 0xD0 }
};

static void print_arr_to_hex(char *h, const unsigned char arr[], char length)
{
    int i;

    A("%s : 0x", h);
    for(i = 0; i < length; i++)
    {
        A("%02X", arr[i]);
    }
    A("\n");
}

static void print_ctx(const mbedtls_sha256_context *ctx)
{
    A("TOTAL : %d, %d\n", ctx->total[1], ctx->total[0]);
    A("STATE : %08X, %08X, %08X, %08X, %08X, %08X, %08X, %08X\n", ctx->state[7], ctx->state[6], ctx->state[5], ctx->state[4], ctx->state[3], ctx->state[2], ctx->state[1], ctx->state[0]);
    print_arr_to_hex("BUFFER", ctx->buffer, 64);
}

/*
 * Checkup routine
 */
int mbedtls_sha256_self_test_hw(cmd_tbl_t *t, int argc, char *argv[])
{
    int i, j, k, buflen, ret = 0;
    unsigned char *buf;
    unsigned char sha256sum[32];
    mbedtls_sha256_context ctx;

    uint32_t B_T, A_T;

    buf = mbedtls_calloc( 1024, sizeof(unsigned char) );
    if( NULL == buf )
    {
        A( "Buffer allocation failed\n" );

        return( 1 );
    }

    mbedtls_sha256_init( &ctx );

    for( i = 0; i < 6; i++ )
    {
        j = i % 3;
        k = i < 3;

        A( "  SHA-%d test #%d: ", 256 - k * 32, j + 1 );

        if( ( ret = mbedtls_sha256_starts_ret( &ctx, k ) ) != 0 )
            goto fail;

        if( j == 2 )
        {
            memset( buf, 'a', buflen = 1000 );
            B_T = TSF;

            for( j = 0; j < 1000; j++ )
            {
                ret = mbedtls_sha256_update_ret( &ctx, buf, buflen );
                if( ret != 0 )
                    goto fail;
            }

        }
        else
        {
            B_T = TSF;
            ret = mbedtls_sha256_update_ret( &ctx, sha256_test_buf[j],
                                             sha256_test_buflen[j] );
            if( ret != 0 )
                 goto fail;
        }

        if( ( ret = mbedtls_sha256_finish_ret( &ctx, sha256sum ) ) != 0 )
            goto fail;


        if( memcmp( sha256sum, sha256_test_sum[i], 32 - k * 4 ) != 0 )
        {
            ret = 1;
            goto fail;
        }

        A_T = TSF;

        print_ctx(&ctx);
        print_arr_to_hex("RES", sha256sum,  32 - k * 4 );
        
        A( "[HW] op time %d\n", A_T - B_T );
        A( "passed\n" );
    }

    A( "\n" );

    goto exit;

fail:
    A( "failed\n" );

exit:
    mbedtls_sha256_free( &ctx );
    mbedtls_free( buf );

    return( ret );
}

SUBCMD(test,
	  sha256_hw,
	  mbedtls_sha256_self_test_hw,
	  "test sha 1 hw hash function",
	  "test sha hw");

#endif /* MBEDTLS_SELF_TEST */


