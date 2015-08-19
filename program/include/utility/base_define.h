#ifndef _BASE_DEFINE_HEADER_
#define _BASE_DEFINE_HEADER_

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "limits.h"

#define PORT_LEN 64
#define IP_WORD_LEN 64
#define COMPRESS_NUM 1024
#define REDIS_COMPRESS_NUM 1024
#define COMPRESS_LEN 102400
#define REDIS_COMPRESS_LEN 102400
#define WORD_LEN 256
#define OPERATOR_LEN 32
#define MAX_BUFFER 128
#define BUF_SIZE 10240000
#define RATE_MAX 10000
#define MAX_WORD_LEN 5120
#define JSON_LEN 102400
#define MAX_FOLLOW_NUM 5000			// 最大关注数目
const char *HEX_CHARS = "0123456789abcdef";

#endif
