#include "encode_convert.h"
#include "stdio.h"
#include "stdlib.h"

// gb2312, gbk, utf-8, …
bool _isUTF8_(const char* str) 
{
	if (!str) return false;
	short nBytes = 0;
	unsigned char chr;
	//bool bAllAscii = true;

	int i = 0;
	while (str[i] != '\0') {
			chr = *(str + i);
	//      if (bAllAscii && (chr & 0x80))
	//              bAllAscii = false;
			if (!nBytes && (chr >= 0x80)) {
							if (chr >= 0xFC && chr <= 0xFD)
									nBytes = 6;
							else if (chr >= 0xF8)
									nBytes = 5;
							else if (chr >= 0xF0)
									nBytes = 4;
							else if (chr >= 0xE0)
									nBytes = 3;
							else if (chr >= 0xC0)
									nBytes = 2;
							else
									return false;
							--nBytes;
			}
			else if (nBytes) {
					if ( (chr & 0xC0) != 0x80)      //前2bit :10
							return false;
					--nBytes;
			}
			++i;
	}

	if (nBytes > 0)
			return false;
	//if (bAllAscii)
	//      return true;
	return true;
}

bool _isGBK_(const char* str)
{
	if (!str) return false;

	int len = (int)strlen(str);		
	unsigned char chr;
	int i = 0;
	while (i < len) {
		while (i < len && str[i] >= 0) ++i; //en
		if (i == len - 1) break;			//half gbk
		while (i < len - 1 && str[i] < 0) {
			chr = str[i];
			if ( (chr >= 129) && (chr <= 254) &&
				(chr >= 64) && (chr <= 254) && (chr != 127) ) { //gbk
				i += 2;
			}
			else return false;
		}
	}
	return true;
}

bool ConvertCore(
	const string& inCodeType, const string& outCodeType,
	const string& input, string& output)
{
	iconv_t cd = iconv_open(outCodeType.c_str(), inCodeType.c_str());

	if (cd == 0 || cd==(iconv_t)(-1))
	{
		printf("iconv_open fails\n"); 
		return false; 
	}

	char *convBuff = NULL;
	char *inBuff = NULL;

	size_t inBuffLen = 1024;
	inBuff = new char[inBuffLen];
	size_t convBuffLen = 1024;
	convBuff = new char[convBuffLen];

	if (inBuff==NULL || convBuff==NULL)
	{
		printf("\n");
		return false;
	}

	inBuff[0] = '\0';
	convBuff[0] = '\0';

	if (inBuffLen < input.size()+1)
	{
		inBuffLen = input.size() + 1;
		delete []inBuff;
		inBuff = new char[inBuffLen];
	}

	//int tmp = input.size();

	memcpy(inBuff, input.c_str(), input.size());
	inBuff[input.size()] = '\0';

	bool hasError = false;

	if (convBuffLen < input.size()*2)
	{
		convBuffLen = input.size() * 2;

		delete []convBuff;
		convBuff = new char[convBuffLen];

		if (NULL == convBuff)
		{
			char info[128];
			sprintf(info, "memalloc error, convBuffLen: %zu\n", convBuffLen); 
			printf(info);
			hasError = true;
		}
	}

	if (true == hasError)
	{
		delete []inBuff;
		inBuff = NULL;
	}
	else
	{
	#ifdef __CYGWIN__
		const char *in = inBuff;
	#else
		char *in = NULL;
		in = inBuff;
	#endif
		char *out = convBuff;
		//size_t inLen = input.size() + 1;
		size_t inLen = input.size() + 1;
		size_t outLen = inLen * 2;

		size_t rt;
		//iconv_t rt;
		//rt = iconv(cd, &in, &inLen, &out, &outLen);
	#ifdef __CYGWIN__
		const char *p = (char*)input.c_str();
	#else
		char *p = (char*)input.c_str();
	#endif
		rt = iconv(cd, &p, &inLen, &out, &outLen);

		if ((size_t)-1 == rt)
		{
			hasError = true;
		} 
		else
		{
			output = convBuff;
		}		
	}

	iconv_close(cd);

	delete []inBuff;
	inBuff= NULL;
	delete []convBuff;
	convBuff = NULL;

	if (hasError == true)
	{
		return false;
	}

	// test
	if (0)
	{
		printf("encode_convert test:\n");
		printf("input: %s, input.size: %zu\n", input.c_str(), input.size());
		printf("output: %s, output.size: %zu\n", output.c_str(), output.size());
	}

	return true;
}

// in:utf8, out:gbk
bool _UTF8_to_GBK_(const string& in, string& out)
{
	return ConvertCore("utf-8", "GBK", in, out);
}

// in:gbk, out:utf8
bool _GBK_to_UTF8_(const string& in, string& out)
{
	return ConvertCore("GBK", "utf-8", in, out);
}

// in:gb2312, out:utf8
bool _GB2312_to_UTF8_(const string& in, string& out)
{
	return ConvertCore("gb2312", "utf-8", in, out);
}

// in:utf8, out:gb2312
bool _UTF8_to_GB2312_(const string& in, string& out)
{
	return ConvertCore("utf-8", "gb2312", in, out);
}

