#include "mbedtls/sha1.h"
#include "mbedtls/platform_util.h"
#include "drv_sec.h"
#include <system_common.h>

#define SHA1_VALIDATE_RET(cond)                             \
    MBEDTLS_INTERNAL_VALIDATE_RET( cond, MBEDTLS_ERR_SHA1_BAD_INPUT_DATA )

#define SHA1_VALIDATE(cond)  MBEDTLS_INTERNAL_VALIDATE( cond )

#ifndef GET_UINT32_BE
#define GET_UINT32_BE(n,b,i)                            \
{                                                       \
    (n) = ( (uint32_t) (b)[(i)    ] << 24 )             \
        | ( (uint32_t) (b)[(i) + 1] << 16 )             \
        | ( (uint32_t) (b)[(i) + 2] <<  8 )             \
        | ( (uint32_t) (b)[(i) + 3]       );            \
}
#endif

#ifndef PUT_UINT32_BE
#define PUT_UINT32_BE(n,b,i)                            \
{                                                       \
    (b)[(i)    ] = (unsigned char) ( (n) >> 24 );       \
    (b)[(i) + 1] = (unsigned char) ( (n) >> 16 );       \
    (b)[(i) + 2] = (unsigned char) ( (n) >>  8 );       \
    (b)[(i) + 3] = (unsigned char) ( (n)       );       \
}
#endif

void mbedtls_sha1_init( mbedtls_sha1_context *ctx )
{
    SHA1_VALIDATE( ctx != NULL );

    memset( ctx, 0, sizeof( mbedtls_sha1_context ) );
}

void mbedtls_sha1_free( mbedtls_sha1_context *ctx )
{
    if( ctx == NULL )
        return;

    mbedtls_platform_zeroize( ctx, sizeof( mbedtls_sha1_context ) );
}

void mbedtls_sha1_clone( mbedtls_sha1_context *dst,
                         const mbedtls_sha1_context *src )
{
    SHA1_VALIDATE( dst != NULL );
    SHA1_VALIDATE( src != NULL );

    *dst = *src;
}

int mbedtls_internal_sha1_process( mbedtls_sha1_context *ctx,
                                      const unsigned char data[64] )
{
    // static unsigned char tmp[64];
    int ret = 0;
    int j;

    // memcpy(tmp, data, 64);

    // for(j=0;j<64;j+=4) 
    //     convert_endian((tmp + j), 4);

    ret = sha_msg_update (data, 64);
    if( ret != 0 )
        goto fail_hw;
    
    sha_start();
    if( sha_is_done() ==0 ) {
        ret = 1;
        goto fail_hw;
    }

    ret = sha_get_result ((uint8_t*)ctx->state, 20);
    if( ret != 0 )
        goto fail_hw;

    ret = sha_set_firstblk(0);
    if(  ret != 0 )
        goto fail_hw;

fail_hw:
    return ret;
}

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
void mbedtls_sha1_process( mbedtls_sha1_context *ctx,
                           const unsigned char data[64] )
{
    mbedtls_internal_sha1_process( ctx, data );
}
#endif

/*
 * SHA-1 context setup
 */
int mbedtls_sha1_starts_ret( mbedtls_sha1_context *ctx )
{
    int ret = 0;

    ctx->total[0] = 0;
    ctx->total[1] = 0;

    sha_init(ENDIAN_LE);

    ret = sha_configure(SHA_MODE_1, SHA_BIT_256, SHA_VERIANT_DIS, 1);
    if( ret != 0 ) 
    { 
        printf(" sha1_unit_update function is failed !!!\n");
        goto fail_hw;
    }

fail_hw :
    return ret;
}

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
void mbedtls_sha1_starts( mbedtls_sha1_context *ctx )
{
    mbedtls_sha1_starts_ret( ctx );
}
#endif


int mbedtls_sha1_update_ret(mbedtls_sha1_context *ctx, const unsigned char *input, size_t ilen)
{
    int     j;
    int     ret = 0;
    int     little_endian = 0;
    size_t fill;
    uint32_t left;

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

        if( ( ret = mbedtls_internal_sha1_process( ctx, ctx->buffer ) ) != 0 )
            return( ret );

        input += fill;
        ilen  -= fill;
        left = 0;
    }        
    

    while( ilen >= 64 )
    {
        if( ( ret = mbedtls_internal_sha1_process( ctx, input ) ) != 0 )
            return( ret );

        input += 64;
        ilen  -= 64;

        // print_ctx(ctx);
    }

    if( ilen > 0 )
        memcpy( (void *) (ctx->buffer + left), input, ilen );
        

    return( ret );
}

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
void mbedtls_sha1_update( mbedtls_sha1_context *ctx,
                          const unsigned char *input,
                          size_t ilen )
{
    mbedtls_sha1_update_ret( ctx, input, ilen );
}
#endif

int mbedtls_sha1_finish_ret( mbedtls_sha1_context *ctx,
                                unsigned char output[20] )
{
    int ret = 0;
    uint32_t used;
    uint32_t high, low, m;

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

        if( ( ret = mbedtls_internal_sha1_process( ctx, ctx->buffer ) ) != 0 )
            return( ret );

        memset( ctx->buffer, 0, 56 );
    }

    PUT_UINT32_BE(ctx->total[1]*8, ctx->buffer, 56);    
    PUT_UINT32_BE(ctx->total[0]*8, ctx->buffer, 60);    

    if( ( ret = mbedtls_internal_sha1_process( ctx, ctx->buffer ) ) != 0 )
        return( ret );

    // PUT_UINT32_BE( ctx->state[0], output,  0 );
    // PUT_UINT32_BE( ctx->state[1], output,  4 );
    // PUT_UINT32_BE( ctx->state[2], output,  8 );
    // PUT_UINT32_BE( ctx->state[3], output, 12 );
    // PUT_UINT32_BE( ctx->state[4], output, 16 );

    memcpy(output, ctx->state, 20);

    bignum_init(0);

    return 0;
}

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
void mbedtls_sha1_finish( mbedtls_sha1_context *ctx,
                          unsigned char output[20] )
{
    mbedtls_sha1_finish_ret( ctx, output );
}
#endif



/*
 * output = SHA-1( input buffer )
 */
int mbedtls_sha1_ret( const unsigned char *input,
                      size_t ilen,
                      unsigned char output[20] )
{
    int ret;
    mbedtls_sha1_context ctx;

    SHA1_VALIDATE_RET( ilen == 0 || input != NULL );
    SHA1_VALIDATE_RET( (unsigned char *)output != NULL );

    mbedtls_sha1_init( &ctx );

    if( ( ret = mbedtls_sha1_starts_ret( &ctx ) ) != 0 )
        goto exit;

    if( ( ret = mbedtls_sha1_update_ret( &ctx, input, ilen ) ) != 0 )
        goto exit;

    if( ( ret = mbedtls_sha1_finish_ret( &ctx, output ) ) != 0 )
        goto exit;

exit:
    mbedtls_sha1_free( &ctx );

    return( ret );
}

#if !defined(MBEDTLS_DEPRECATED_REMOVED)
void mbedtls_sha1( const unsigned char *input,
                   size_t ilen,
                   unsigned char output[20] )
{
    mbedtls_sha1_ret( input, ilen, output );
}
#endif

#if defined(MBEDTLS_SELF_TEST)

/*
 * FIPS-180-1 test vectors
 */
static const unsigned char sha1_test_buf[3][57] =
{
    { "abc" },
    { "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq" },
    { "" }
};

static const size_t sha1_test_buflen[3] =
{
    3, 56, 1000
};

static const unsigned char sha1_test_sum[3][20] =
{   
    { 0xA9, 0x99, 0x3E, 0x36, 0x47, 0x06, 0x81, 0x6A, 0xBA, 0x3E,
      0x25, 0x71, 0x78, 0x50, 0xC2, 0x6C, 0x9C, 0xD0, 0xD8, 0x9D },
    { 0x84, 0x98, 0x3E, 0x44, 0x1C, 0x3B, 0xD2, 0x6E, 0xBA, 0xAE,
      0x4A, 0xA1, 0xF9, 0x51, 0x29, 0xE5, 0xE5, 0x46, 0x70, 0xF1 },
    { 0x34, 0xAA, 0x97, 0x3C, 0xD4, 0xC4, 0xDA, 0xA4, 0xF6, 0x1E,
      0xEB, 0x2B, 0xDB, 0xAD, 0x27, 0x31, 0x65, 0x34, 0x01, 0x6F }
};

void print_arr_to_hex(char *h, const unsigned char arr[], char length)
{
    int i;

    A("%s : 0x", h);
    for(i = 0; i < length; i++)
    {
        A("%02X", arr[i]);
    }
    A("\n");
}

void print_ctx(const mbedtls_sha1_context *ctx)
{
    A("TOTAL : %d, %d\n", ctx->total[1], ctx->total[0]);
    A("STATE : %08X, %08X, %08X, %08X, %08X\n", ctx->state[4], ctx->state[3], ctx->state[2], ctx->state[1], ctx->state[0]);
    print_arr_to_hex("BUFFER", ctx->buffer, 64);
}

int sha1_test_hw (cmd_tbl_t *t, int argc, char *argv[])
{
    int i, j,  buflen, ret = 0;
    unsigned char buf[1088];
    unsigned char sha1sum[20];
    mbedtls_sha1_context ctx;

    uint32_t B_T, A_T;

    mbedtls_sha1_init( &ctx );
    memset( buf, 'a', buflen = (1000+64) );

    for( i = 0; i < 3; i++ )
    {

        printf( "  SHA-1 test #%d: ", i);
        

        mbedtls_sha1_starts_ret(&ctx);

        if(  ret != 0 )
            goto fail_hw;
            
        if(i == 2) {
            
            memset( buf, 'a', buflen = 1000 );
            B_T = TSF;
            
            for( j = 0; j < 1000; j++ ) 
            {
                ret = mbedtls_sha1_update_ret(&ctx, buf, buflen);
                if( ret != 0 ) { 
                    printf(" sha1_unit_update function is failed !!!\n");
                    goto fail_hw;
                }
            }
            // print_ctx(&ctx); //DEBUG
        }
        else {

            memcpy( buf, sha1_test_buf[i], sha1_test_buflen[i]);
            B_T = TSF;
            ret = mbedtls_sha1_update_ret( &ctx, buf,
                                           sha1_test_buflen[i] );
            if( ret != 0 )
                goto fail_hw;
            
            // print_ctx(&ctx); //DEBUG
        }
        
        if( ( ret = mbedtls_sha1_finish_ret( &ctx, sha1sum ) ) != 0 )
            goto fail_hw;

        A_T = TSF;

        print_ctx(&ctx);
        print_arr_to_hex("RES", sha1sum, 20);
        printf( "passed\n" );
        A( "[HW] op time %d\n", A_T - B_T );
    }

    printf( "\n" );

    goto exit_hw;

fail_hw:
    printf( "failed\n" );

exit_hw:
    //free( buf );

    return( ret );
}

SUBCMD(test,
	  sha1_hw,
	  sha1_test_hw,
	  "test sha 1 hw hash function",
	  "test sha hw");

#endif /* MBEDTLS_SELF_TEST */
