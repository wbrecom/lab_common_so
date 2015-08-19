/**
 * @file
 * @author  Li Jiong <lijiong@staff.sina.com.cn>
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * woo framework 
 */
#ifndef __woo_log_H__
#define __woo_log_H__

#include <sys/time.h>
#include <stdint.h>

#include <limits.h>
#include <stdio.h>
#include <string.h>

const int LOG_LEVEL_ERROR = 0;
const int LOG_LEVEL_WARN = 1;
const int LOG_LEVEL_INFO = 2;
const int LOG_LEVEL_TRACE = 3;
const int LOG_LEVEL_DEBUG = 4;

namespace woo {

int open_log(const char *path, size_t buf_size, const char *write_level, const char *flush_level);
int open_thread_log();
void set_log_id(int64_t id);
int64_t get_log_id();
int log(int level, int flush, const char *fmt, ...);
void close_log();

};

#define LOG_ERROR(fmt, arg...) \
	do { \
		woo::log(LOG_LEVEL_ERROR, 0, "[%s:%u] " fmt "\n", __FILE__, __LINE__, ##arg); \
	} while(0)

#define LOG_DEBUG(fmt, arg...) \
	do { \
		woo::log(LOG_LEVEL_DEBUG, 0, "[%s:%u] " fmt "\n", __FILE__, __LINE__, ##arg); \
	} while(0)

#define LOG_WARN(fmt, arg...) \
	do { \
		woo::log(LOG_LEVEL_WARN, 0, "[%s:%u] " fmt "\n", __FILE__, __LINE__, ##arg); \
	} while(0) 

#define LOG_INFO(fmt, arg...) \
	do { \
		woo::log(LOG_LEVEL_INFO, 0, "[%s:%u] " fmt "\n", __FILE__, __LINE__, ##arg); \
	} while(0) 

#define LOG_TRACE(fmt, arg...) \
	do { \
		woo::log(LOG_LEVEL_TRACE, 0, "[%s:%u] " fmt "\n", __FILE__, __LINE__, ##arg); \
	} while(0) 

#endif
