#ifndef __bson2json__H__
#define __bson2json__H__

#ifdef __cplusplus 
extern "C" {
#endif

enum protocol_type_t {
	PROTOCOL_TYPE_UNKNOWN = 0,
	PROTOCOL_TYPE_BSON = 1,
	PROTOCOL_TYPE_JSON = 2
};

int xbson_to_json(xbson *b, char *buf, size_t *len, size_t size);
int xbson_from_json(xbson *b, const char *buf);

int xbson_valid(const char *str);
int xbson_protocol(const char *str);
int xbson_input(xbson *b, const char *buf, int type);
int xbson_output(xbson *b, char *buf, size_t *len, size_t buf_size, int type);

#ifdef __cplusplus 
}
#endif

#endif
