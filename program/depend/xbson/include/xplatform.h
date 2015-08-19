#ifndef __XPLATFORM_H__
#define __XPLATFORM_H__

/* big endian is only used for OID generation. little is used everywhere else */
#if __BYTE_ORDER == __BIG_ENDIAN
#define xbson_little_endian64(out, in) ( xbson_swap_endian64(out, in) )
#define xbson_little_endian32(out, in) ( xbson_swap_endian32(out, in) )
#define xbson_big_endian64(out, in) ( memcpy(out, in, 8) )
#define xbson_big_endian32(out, in) ( memcpy(out, in, 4) )
#else
#define xbson_little_endian64(out, in) ( memcpy(out, in, 8) )
#define xbson_little_endian32(out, in) ( memcpy(out, in, 4) )
#define xbson_big_endian64(out, in) ( xbson_swap_endian64(out, in) )
#define xbson_big_endian32(out, in) ( xbson_swap_endian32(out, in) )
#endif

#ifdef __cplusplus 
extern "C" {
#endif

inline void xbson_swap_endian64(void *outp, const void *inp) {
    const char *in = ( const char * )inp;
    char *out = ( char * )outp;

    out[0] = in[7];
    out[1] = in[6];
    out[2] = in[5];
    out[3] = in[4];
    out[4] = in[3];
    out[5] = in[2];
    out[6] = in[1];
    out[7] = in[0];

}

inline void xbson_swap_endian32( void *outp, const void *inp ) {
    const char *in = ( const char * )inp;
    char *out = ( char * )outp;

    out[0] = in[3];
    out[1] = in[2];
    out[2] = in[1];
    out[3] = in[0];
}

#ifdef __cplusplus 
}
#endif

#endif
