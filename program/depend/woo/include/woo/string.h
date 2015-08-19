/**
 * @file
 * @author  Li Jiong <lijiong@staff.sina.com.cn>
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * woo framework 
 */
#ifndef __woo_string_H__
#define __woo_string_H__

#include <stdio.h>
#include <sys/types.h>

namespace woo {


size_t strlcpy(char *dst, const char *src, size_t siz);

size_t strlcat(char *dst, const char *src, size_t siz);

char *rtrim(char *s);

char *
str_replace(char *dststring, const char *string, const char *substr, const char *replacement );

}

#endif
