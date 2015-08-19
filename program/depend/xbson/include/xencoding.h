#ifndef _XBSON_ENCODING_H_
#define _XBSON_ENCODING_H_

#ifdef __cplusplus 
extern "C" {
#endif

int xbson_check_field_name(xbson *b, const char *string, const int length);

xbson_bool_t xbson_check_string(xbson *b, const char *string, const int length);

#ifdef __cplusplus 
}
#endif

#endif
