#ifndef _ENCODE_CONVERT_HEADER_
#define _ENCODE_CONVERT_HEADER_
#include <iconv.h>
#include <stdlib.h>
#include <string.h>
#include <string>
//#include "global.h"
using namespace std;

// gb2312, gbk, utf-8, бн
bool _isUTF8_(const char* str);

bool _isGBK_(const char* str);

bool ConvertCore(
	const string& inCodeType, const string& outCodeType,
	const string& input, string& output);

// in:utf8, out:gbk
bool _UTF8_to_GBK_(const string& in, string& out);

// in:gbk, out:utf8
bool _GBK_to_UTF8_(const string& in, string& out);

// in:gb2312, out:utf8
bool _GB2312_to_UTF8_(const string& in, string& out);

// in:utf8, out:gb2312
bool _UTF8_to_GB2312_(const string& in, string& out);

#endif
