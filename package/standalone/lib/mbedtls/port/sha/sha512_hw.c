#include <drv_sec.h>
#include "mbedtls/sha512.h"
#include "mbedtls/platform_util.h"

#if defined(MBEDTLS_SHA512_C)

#define mbedtls_free       free
#define mbedtls_calloc     calloc

#define SHA512_VALIDATE_RET(cond)                           \
    MBEDTLS_INTERNAL_VALIDATE_RET( cond, MBEDTLS_ERR_SHA512_BAD_INPUT_DATA )
#define SHA512_VALIDATE(cond)  MBEDTLS_INTERNAL_VALIDATE( cond )

/*
 * 64-bit integer manipulation macros (big endian)
 */
#ifndef GET_UINT64_BE
#define GET_UINT64_BE(n,b,i)                            \
{                                                       \
    (n) = ( (uint64_t) (b)[(i)    ] << 56 )       \
        | ( (uint64_t) (b)[(i) + 1] << 48 )       \
        | ( (uint64_t) (b)[(i) + 2] << 40 )       \
        | ( (uint64_t) (b)[(i) + 3] << 32 )       \
        | ( (uint64_t) (b)[(i) + 4] << 24 )       \
        | ( (uint64_t) (b)[(i) + 5] << 16 )       \
        | ( (uint64_t) (b)[(i) + 6] <<  8 )       \
        | ( (uint64_t) (b)[(i) + 7]       );      \
}
#endif /* GET_UINT64_BE */

#ifndef PUT_UINT64_BE
#define PUT_UINT64_BE(n,b,i)                            \
{                                                       \
    (b)[(i)    ] = (unsigned char) ( (n) >> 56 );       \
    (b)[(i) + 1] = (unsigned char) ( (n) >> 48 );       \
    (b)[(i) + 2] = (unsigned char) ( (n) >> 40 );       \
    (b)[(i) + 3] = (unsigned char) ( (n) >> 32 );       \
    (b)[(i) + 4] = (unsigned char) ( (n) >> 24 );       \
    (b)[(i) + 5] = (unsigned char) ( (n) >> 16 );       \
    (b)[(i) + 6] = (unsigned char) ( (n) >>  8 );       \
    (b)[(i) + 7] = (unsigned char) ( (n)       );       \
}
#endif /* PUT_UINT64_BE */

int mbedtls_internal_sha512_process( mbedtls_sha512_context *ctx,
                                     const unsigned char data[128] )
{
    static unsigned char tmp[128];
    int ret = 0;
    int j;

    SHA512_VALIDATE_RET( ctx != NULL );
    SHA512_VALIDATE_RET( (const unsigned char *)data != NULL );

    memcpy(tmp, data, 128);

    for(j=0;j<128;j+=4) 
        convert_endian((tmp + j), 4);

    ret = sha_msg_update (tmp, 128);
    if( ret != 0 )
        goto fail_hw;
    
    sha_start();
    if( sha_is_done() ==0 ) {
        ret = 1;
        goto fail_hw;
    }

    // ret = sha_get_result_64 ((uint8_t*)ctx->state, 64);
    ret = sha_get_result ((uint8_t*)ctx->state, 64);

    if( ret != 0 )
        goto fail_hw;

    ret = sha_set_firstblk(0);
    if(  ret != 0 )
        goto fail_hw;

fail_hw:
    return ret;
}


/*
 * SHA-512 final digest
 */
int mbedtls_sha512_finish_ret( mbedtls_sha512_context *ctx,
                               unsigned char output[64] )
{
    int ret;
    unsigned used;
    uint64_t high, low;

    SHA512_VALIDATE_RET( ctx != NULL );
    SHA512_VALIDATE_RET( (unsigned char *)output != NULL );

    /*
     * Add padding: 0x80 then 0x00 until 16 bytes remain for the length
     */
    used = ctx->total[0] & 0x7F;

    ctx->buffer[used++] = 0x80;

    if( used <= 112 )
    {
        /* Enough room for padding + length in current block */
        memset( ctx->buffer + used, 0, 112 - used );
    }
    else
    {
        /* We'll need an extra block */
        memset( ctx->buffer + used, 0, 128 - used );

        if( ( ret = mbedtls_internal_sha512_process( ctx, ctx->buffer ) ) != 0 )
            return( ret );

        memset( ctx->buffer, 0, 112 );
    }

    /*
     * Add message length
     */
    high = ( ctx->total[0] >> 61 )
         | ( ctx->total[1] <<  3 );
    low  = ( ctx->total[0] <<  3 );

    PUT_UINT64_BE( high, ctx->buffer, 112 );
    PUT_UINT64_BE( low,  ctx->buffer, 120 );

    if( ( ret = mbedtls_internal_sha512_process( ctx, ctx->buffer ) ) != 0 )
        return( ret );

    /*
     * Output final state
     */
    PUT_UINT64_BE( ctx->state[0], output,  0 );
    PUT_UINT64_BE( ctx->state[1], output,  8 );
    PUT_UINT64_BE( ctx->state[2], output, 16 );
    PUT_UINT64_BE( ctx->state[3], output, 24 );
    PUT_UINT64_BE( ctx->state[4], output, 32 );
    PUT_UINT64_BE( ctx->state[5], output, 40 );

    if( ctx->is384 == 0 )
    {
        PUT_UINT64_BE( ctx->state[6], output, 48 );
        PUT_UINT64_BE( ctx->state[7], output, 56 );
        convert_endian_32((uint32_t *)output, 16);
    }
    else
    {
        convert_endian_32((uint32_t *)output, 12);
    }

    bignum_init(0);
    
    return( 0 );
}

void mbedtls_sha512_init( mbedtls_sha512_context *ctx )
{
    SHA512_VALIDATE( ctx != NULL );

    memset( ctx, 0, sizeof( mbedtls_sha512_context ) );
}

void mbedtls_sha512_free( mbedtls_sha512_context *ctx )
{
    if( ctx == NULL )
        return;

    mbedtls_platform_zeroize( ctx, sizeof( mbedtls_sha512_context ) );
}

void mbedtls_sha512_clone( mbedtls_sha512_context *dst,
                           const mbedtls_sha512_context *src )
{
    SHA512_VALIDATE( dst != NULL );
    SHA512_VALIDATE( src != NULL );

    *dst = *src;
}

/*
 * SHA-512 process buffer
 */
int mbedtls_sha512_update_ret( mbedtls_sha512_context *ctx,
                               const unsigned char *input,
                               size_t ilen )
{
    int ret;
    size_t fill;
    unsigned int left;

    SHA512_VALIDATE_RET( ctx != NULL );
    SHA512_VALIDATE_RET( ilen == 0 || input != NULL );

    if( ilen == 0 )
        return( 0 );

    left = (unsigned int) (ctx->total[0] & 0x7F);
    fill = 128 - left;

    ctx->total[0] += (uint64_t) ilen;

    if( ctx->total[0] < (uint64_t) ilen )
        ctx->total[1]++;

    if( left && ilen >= fill )
    {
        memcpy( (void *) (ctx->buffer + left), input, fill );

        if( ( ret = mbedtls_internal_sha512_process( ctx, ctx->buffer ) ) != 0 )
            return( ret );

        input += fill;
        ilen  -= fill;
        left = 0;
    }

    while( ilen >= 128 )
    {
        if( ( ret = mbedtls_internal_sha512_process( ctx, input ) ) != 0 )
            return( ret );

        input += 128;
        ilen  -= 128;
    }

    if( ilen > 0 )
        memcpy( (void *) (ctx->buffer + left), input, ilen );

    return( 0 );
}

/*
 * SHA-512 context setup
 */
int mbedtls_sha512_starts_ret( mbedtls_sha512_context *ctx, int is384 )
{
    int ret = 0;

    SHA512_VALIDATE_RET( ctx != NULL );
    SHA512_VALIDATE_RET( is384 == 0 || is384 == 1 );

    ctx->total[0] = 0;
    ctx->total[1] = 0;
    ctx->is384 = is384;

    sha_init(ENDIAN_BI);

    ret = sha_configure(SHA_MODE_2, SHA_BIT_512, is384, 1);
    if( ret != 0 ) 
    { 
        printf(" sha1_unit_update function is failed !!!\n");
        goto fail_hw;
    }

fail_hw :

    return( ret );
}

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
void mbedtls_sha512_starts( mbedtls_sha512_context *ctx,
                            int is384 )
{
    mbedtls_sha512_starts_ret( ctx, is384 );
}
#endif

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
void mbedtls_sha512_update( mbedtls_sha512_context *ctx,
                            const unsigned char *input,
                            size_t ilen )
{
    mbedtls_sha512_update_ret( ctx, input, ilen );
}
#endif

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
void mbedtls_sha512_finish( mbedtls_sha512_context *ctx,
                            unsigned char output[64] )
{
    mbedtls_sha512_finish_ret( ctx, output );
}
#endif

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
void mbedtls_sha512_process( mbedtls_sha512_context *ctx,
                             const unsigned char data[128] )
{
    mbedtls_internal_sha512_process( ctx, data );
}
#endif

/*
 * output = SHA-512( input buffer )
 */
int mbedtls_sha512_ret( const unsigned char *input,
                    size_t ilen,
                    unsigned char output[64],
                    int is384 )
{
    int ret;
    mbedtls_sha512_context ctx;

    SHA512_VALIDATE_RET( is384 == 0 || is384 == 1 );
    SHA512_VALIDATE_RET( ilen == 0 || input != NULL );
    SHA512_VALIDATE_RET( (unsigned char *)output != NULL );

    mbedtls_sha512_init( &ctx );

    if( ( ret = mbedtls_sha512_starts_ret( &ctx, is384 ) ) != 0 )
        goto exit;

    if( ( ret = mbedtls_sha512_update_ret( &ctx, input, ilen ) ) != 0 )
        goto exit;

    if( ( ret = mbedtls_sha512_finish_ret( &ctx, output ) ) != 0 )
        goto exit;

exit:
    mbedtls_sha512_free( &ctx );

    return( ret );
}



#if defined(MBEDTLS_SELF_TEST)

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

static void print_ctx(const mbedtls_sha512_context *ctx)
{
    A("TOTAL : %d, %d\n", ctx->total[1], ctx->total[0]);
    A("STATE : %08X, %08X, %08X, %08X, %08X, %08X, %08X, %08X\n", ctx->state[7], ctx->state[6], ctx->state[5], ctx->state[4], ctx->state[3], ctx->state[2], ctx->state[1], ctx->state[0]);
    print_arr_to_hex("BUFFER", ctx->buffer, 128);
}

/*
 * FIPS-180-2 test vectors
 */
static const unsigned char sha512_test_buf[3][113] =
{
    { "abc" },
    { "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmn"
      "hijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu" },
    { "" }
};

static const size_t sha512_test_buflen[3] =
{
    3, 112, 1000
};

static const unsigned char sha512_test_sum[6][64] =
{
    /*
     * SHA-384 test vectors
     */
    { 0xCB, 0x00, 0x75, 0x3F, 0x45, 0xA3, 0x5E, 0x8B,
      0xB5, 0xA0, 0x3D, 0x69, 0x9A, 0xC6, 0x50, 0x07,
      0x27, 0x2C, 0x32, 0xAB, 0x0E, 0xDE, 0xD1, 0x63,
      0x1A, 0x8B, 0x60, 0x5A, 0x43, 0xFF, 0x5B, 0xED,
      0x80, 0x86, 0x07, 0x2B, 0xA1, 0xE7, 0xCC, 0x23,
      0x58, 0xBA, 0xEC, 0xA1, 0x34, 0xC8, 0x25, 0xA7 },
    { 0x09, 0x33, 0x0C, 0x33, 0xF7, 0x11, 0x47, 0xE8,
      0x3D, 0x19, 0x2F, 0xC7, 0x82, 0xCD, 0x1B, 0x47,
      0x53, 0x11, 0x1B, 0x17, 0x3B, 0x3B, 0x05, 0xD2,
      0x2F, 0xA0, 0x80, 0x86, 0xE3, 0xB0, 0xF7, 0x12,
      0xFC, 0xC7, 0xC7, 0x1A, 0x55, 0x7E, 0x2D, 0xB9,
      0x66, 0xC3, 0xE9, 0xFA, 0x91, 0x74, 0x60, 0x39 },
    { 0x9D, 0x0E, 0x18, 0x09, 0x71, 0x64, 0x74, 0xCB,
      0x08, 0x6E, 0x83, 0x4E, 0x31, 0x0A, 0x4A, 0x1C,
      0xED, 0x14, 0x9E, 0x9C, 0x00, 0xF2, 0x48, 0x52,
      0x79, 0x72, 0xCE, 0xC5, 0x70, 0x4C, 0x2A, 0x5B,
      0x07, 0xB8, 0xB3, 0xDC, 0x38, 0xEC, 0xC4, 0xEB,
      0xAE, 0x97, 0xDD, 0xD8, 0x7F, 0x3D, 0x89, 0x85 },

    /*
     * SHA-512 test vectors
     */
    { 0xDD, 0xAF, 0x35, 0xA1, 0x93, 0x61, 0x7A, 0xBA,
      0xCC, 0x41, 0x73, 0x49, 0xAE, 0x20, 0x41, 0x31,
      0x12, 0xE6, 0xFA, 0x4E, 0x89, 0xA9, 0x7E, 0xA2,
      0x0A, 0x9E, 0xEE, 0xE6, 0x4B, 0x55, 0xD3, 0x9A,
      0x21, 0x92, 0x99, 0x2A, 0x27, 0x4F, 0xC1, 0xA8,
      0x36, 0xBA, 0x3C, 0x23, 0xA3, 0xFE, 0xEB, 0xBD,
      0x45, 0x4D, 0x44, 0x23, 0x64, 0x3C, 0xE8, 0x0E,
      0x2A, 0x9A, 0xC9, 0x4F, 0xA5, 0x4C, 0xA4, 0x9F },
    { 0x8E, 0x95, 0x9B, 0x75, 0xDA, 0xE3, 0x13, 0xDA,
      0x8C, 0xF4, 0xF7, 0x28, 0x14, 0xFC, 0x14, 0x3F,
      0x8F, 0x77, 0x79, 0xC6, 0xEB, 0x9F, 0x7F, 0xA1,
      0x72, 0x99, 0xAE, 0xAD, 0xB6, 0x88, 0x90, 0x18,
      0x50, 0x1D, 0x28, 0x9E, 0x49, 0x00, 0xF7, 0xE4,
      0x33, 0x1B, 0x99, 0xDE, 0xC4, 0xB5, 0x43, 0x3A,
      0xC7, 0xD3, 0x29, 0xEE, 0xB6, 0xDD, 0x26, 0x54,
      0x5E, 0x96, 0xE5, 0x5B, 0x87, 0x4B, 0xE9, 0x09 },
    { 0xE7, 0x18, 0x48, 0x3D, 0x0C, 0xE7, 0x69, 0x64,
      0x4E, 0x2E, 0x42, 0xC7, 0xBC, 0x15, 0xB4, 0x63,
      0x8E, 0x1F, 0x98, 0xB1, 0x3B, 0x20, 0x44, 0x28,
      0x56, 0x32, 0xA8, 0x03, 0xAF, 0xA9, 0x73, 0xEB,
      0xDE, 0x0F, 0xF2, 0x44, 0x87, 0x7E, 0xA6, 0x0A,
      0x4C, 0xB0, 0x43, 0x2C, 0xE5, 0x77, 0xC3, 0x1B,
      0xEB, 0x00, 0x9C, 0x5C, 0x2C, 0x49, 0xAA, 0x2E,
      0x4E, 0xAD, 0xB2, 0x17, 0xAD, 0x8C, 0xC0, 0x9B }
};

/*
 * Checkup routine
 */
int mbedtls_sha512_self_test_hw(cmd_tbl_t *t, int argc, char *argv[])
{
    int i, j, k, buflen, ret = 0;
    unsigned char *buf;
    unsigned char sha512sum[64];
    mbedtls_sha512_context ctx;

    uint32_t B_T, A_T;

    buf = mbedtls_calloc( 1024, sizeof(unsigned char) );
    if( NULL == buf )
    {
        A( "Buffer allocation failed\n" );

        return( 1 );
    }

    mbedtls_sha512_init( &ctx );

    for( i = 0; i < 6; i++ )
    {
        j = i % 3;
        k = i < 3;

        A( "  SHA-%d test #%d: ", 512 - k * 128, j + 1 );

        if( ( ret = mbedtls_sha512_starts_ret( &ctx, k ) ) != 0 )
            goto fail;

        if( j == 2 )
        {
            memset( buf, 'a', buflen = 1000 );
            B_T = TSF;

            for( j = 0; j < 1000; j++ )
            {
                ret = mbedtls_sha512_update_ret( &ctx, buf, buflen );
                if( ret != 0 )
                    goto fail;
            }
        }
        else
        {
            B_T = TSF;
            ret = mbedtls_sha512_update_ret( &ctx, sha512_test_buf[j],
                                             sha512_test_buflen[j] );
            if( ret != 0 )
                goto fail;
        }

        if( ( ret = mbedtls_sha512_finish_ret( &ctx, sha512sum ) ) != 0 )
            goto fail;

        if( memcmp( sha512sum, sha512_test_sum[i], 64 - k * 16 ) != 0 )
        {
            ret = 1;
            goto fail;
        }

        A_T = TSF;

        print_ctx(&ctx);
        print_arr_to_hex("RES", sha512sum, 64 - k * 16);
        A( "[HW] op time %d\n", A_T - B_T );
        A( "passed\n" );
    }

    A( "\n" );

    goto exit;

fail:
    A( "failed\n" );

exit:
    mbedtls_free( buf );

    return( ret );
}

SUBCMD(test,
	  sha512_hw,
	  mbedtls_sha512_self_test_hw,
	  "test sha 1 hw hash function",
	  "test sha hw");

#endif /* MBEDTLS_SELF_TEST */

#endif /* MBEDTLS_SHA512_C */