#ifndef _XBSON_H_
#define _XBSON_H_

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef XBSON_DEBUGLOG
#define XBSON_LOG(fmt, arg...) \
	do { \
		printf("XBSON [%s:%u] " fmt "\n", __FILE__, __LINE__, ##arg); \
	} while(0) 
#else
#define XBSON_LOG(fmt, arg...) 
#endif


#ifdef __cplusplus 
extern "C" {
#endif

#define XBSON_OK 0
#define XBSON_ERROR -1

enum xbson_error_t {
    XBSON_SIZE_OVERFLOW = 1 /**< Trying to create a BSON object larger than INT_MAX. */
};

enum xbson_validity_t {
    XBSON_VALID = 0,                 /**< BSON is valid and UTF-8 compliant. */
    XBSON_NOT_UTF8 = ( 1<<1 ),       /**< A key or a string is not valid UTF-8. */
    XBSON_FIELD_HAS_DOT = ( 1<<2 ),  /**< Warning: key contains '.' character. */
    XBSON_FIELD_INIT_DOLLAR = ( 1<<3 ), /**< Warning: key starts with '$' character. */
    XBSON_ALREADY_FINISHED = ( 1<<4 )  /**< Trying to modify a finished BSON object. */
};

enum xbson_binary_subtype_t {
    XBSON_BIN_BINARY = 0,
    XBSON_BIN_FUNC = 1,
    XBSON_BIN_BINARY_OLD = 2,
    XBSON_BIN_UUID = 3,
    XBSON_BIN_MD5 = 5,
    XBSON_BIN_USER = 128
};

typedef enum {
    XBSON_EOO = 0,
    XBSON_DOUBLE = 1,
    XBSON_STRING = 2,
    XBSON_OBJECT = 3,
    XBSON_ARRAY = 4,
    XBSON_BINDATA = 5,
    XBSON_UNDEFINED = 6,
    XBSON_OID = 7,
    XBSON_BOOL = 8,
    XBSON_DATE = 9,
    XBSON_NULL = 10,
    XBSON_REGEX = 11,
    XBSON_DBREF = 12, /**< Deprecated. */
    XBSON_CODE = 13,
    XBSON_SYMBOL = 14,
    XBSON_CODEWSCOPE = 15,
    XBSON_INT = 16,
    XBSON_TIMESTAMP = 17,
    XBSON_LONG = 18
} xbson_type;

typedef int xbson_bool_t;

typedef struct {
    const char *cur;
    xbson_bool_t first;
} xbson_iterator;

typedef struct {
    char *data;
    char *cur;
    int dataSize;
    xbson_bool_t finished;
    int stack[32];
    int stackPos;
    int err; /**< Bitfield representing errors or warnings on this buffer */
    char *errstr; /**< A string representation of the most recent error or warning. */
} xbson;

int xbson_size(const xbson *b);
const char *xbson_data(xbson *b);
xbson_type xbson_find(xbson_iterator *it, const xbson *obj, const char *name);
void xbson_iterator_init(xbson_iterator *i , const xbson *b);
void xbson_iterator_from_buffer(xbson_iterator *i, const char *buffer);
xbson_bool_t xbson_iterator_more(const xbson_iterator *i);
xbson_type xbson_iterator_next(xbson_iterator *i);
xbson_type xbson_iterator_type(const xbson_iterator *i);
const char *xbson_iterator_key(const xbson_iterator *i);
const char *xbson_iterator_value(const xbson_iterator *i);

int xbson_iterator_int_raw(const xbson_iterator *i);
int64_t xbson_iterator_long_raw(const xbson_iterator *i);
double xbson_iterator_double_raw(const xbson_iterator *i);

double xbson_iterator_double(const xbson_iterator *i);
int xbson_iterator_int(const xbson_iterator *i);
int64_t xbson_iterator_long(const xbson_iterator *i);
xbson_bool_t xbson_iterator_bool(const xbson_iterator *i);
const char *xbson_iterator_string(const xbson_iterator *i);
int xbson_iterator_string_len(const xbson_iterator *i);

int xbson_iterator_bin_len(const xbson_iterator *i);
char xbson_iterator_bin_type(const xbson_iterator *i);
const char *xbson_iterator_bin_data(const xbson_iterator *i);

void xbson_iterator_subobject(const xbson_iterator *i, xbson *sub);
void xbson_iterator_subiterator(const xbson_iterator *i, xbson_iterator *sub);

void xbson_reset(xbson *b);
int xbson_init_data(xbson *b , char *data,  int dataSize);
int xbson_init_finished_data(xbson *b, char *data);
int xbson_ensure_space(xbson *b, const int bytesNeeded);
int xbson_finish(xbson *b);
int xbson_copy(xbson *out, const xbson *in); 
int xbson_append_int(xbson *b, const char *name, const int i);
int xbson_append_long(xbson *b, const char *name, const int64_t i);
int xbson_append_double(xbson *b, const char *name, const double d);
int xbson_append_string(xbson *b, const char *name, const char *str);
int xbson_append_string_n(xbson *b, const char *name, const char *str, int len);
int xbson_append_binary(xbson *b, const char *name, char type, const char *str, int len);
int xbson_append_bool(xbson *b, const char *name, const xbson_bool_t v);
int xbson_append_null(xbson *b, const char *name);
int xbson_append_bson(xbson *b, const char *name, const xbson *xbson);
int xbson_append_array(xbson *b, const char *name, const xbson *xbson);
int xbson_append_element(xbson *b, const char *name_or_null, const xbson_iterator *elem);
int xbson_append_start_object(xbson *b, const char *name);
int xbson_append_start_array(xbson *b, const char *name);
int xbson_append_finish_object(xbson *b);
int xbson_append_finish_array(xbson *b);


/* ext by wcp */
int xbson_replace_int(xbson *main_b, xbson *sub_b, const char *name, const int i );
int xbson_replace_long(xbson *main_b, xbson *sub_b, const char *name, const int64_t i);
int xbson_replace_double(xbson *main_b, xbson *sub_b, const char *name, const double d);
int xbson_replace_string(xbson *main_b, xbson *sub_b, const char *name, const char *str);
int xbson_replace_string_n(xbson *main_b, xbson *sub_b, const char *name, const char *str, int len);
int xbson_replace_bool(xbson *main_b, xbson *sub_b, const char *name, const xbson_bool_t v);
/*int xbson_replace_binary(xbson *b, const char *name, char type, const char *str, int len);
int xbson_replace_bson(xbson *b, const char *name, const xbson *xbson);
int xbson_replace_array(xbson *b, const char *name, const xbson *xbson);
int xbson_replace_element(xbson *b, const char *name_or_null, const xbson_iterator *elem);
int xbson_replace_start_object(xbson *b, const char *name);
int xbson_replace_start_array(xbson *b, const char *name);
int xbson_replace_finish_object(xbson *b);
int xbson_replace_finish_array(xbson *b);*/


/* ext */
const char *xbson_find_string(xbson *b, const char *name, const char *def_value );
int xbson_find_int(xbson *bson, const char *name, int def_value);
int64_t xbson_find_long(xbson *b, const char *name, int64_t def_value);
double xbson_find_double(xbson *bson, const char *name, double def_value);
int xbson_find_subobject(xbson *b, const char *name, xbson *sub);

xbson *xbson_create(int dataSize);
void xbson_destroy(xbson *);


#ifdef __cplusplus 
}
#endif

#endif
