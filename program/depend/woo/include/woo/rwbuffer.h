/**
 * @file
 * @author  Li Jiong <lijiong@staff.sina.com.cn>
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * woo framework 
 */
#ifndef __woo_rwbuffer_H__
#define __woo_rwbuffer_H__

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

namespace woo {

class RWBuffer {
public:
	RWBuffer() : buf_size_(0) {
		memset(&buf_, 0, sizeof(buf_));
	}

	bool init(size_t buf_size) {
		buf_[0].buf = (char *)malloc(buf_size + sizeof(uint32_t));
		if (! buf_[0].buf) {
			return false;
		}
		buf_[1].buf = (char *)malloc(buf_size + sizeof(uint32_t));
		if (! buf_[1].buf) {
			return false;
		}
		buf_size_ = buf_size;
		return true;
	}

	void destroy() {
		free(buf_[0].buf);
		free(buf_[1].buf);
		memset(&buf_, 0, sizeof(buf_));
		buf_size_ = 0;
	}

	char *get_read() {
		int i = 0;
		if (buf_[0].rd_off < buf_[1].rd_off) {
			i = 1;
		}
		if (buf_[i].rd_off < buf_[i].wr_off) {
			return buf_[i].buf + buf_[i].rd_off;
		}
		i = 1 - i;
		if (buf_[i].rd_off < buf_[i].wr_off) {
			return buf_[i].buf + buf_[i].rd_off;
		}
		return NULL;
	}

	void set_read(char *ptr) {
		for (int i = 0; i < 2; ++ i) {
			if ((buf_[i].buf + buf_[i].rd_off) == ptr) {
				buf_[i].rd_off += *(uint32_t *)ptr + sizeof(uint32_t);
				break;
			}
		}
	}

	char *get_write(size_t size) {
		int i = 0;
		if (buf_[0].rd_off < buf_[1].rd_off) {
			i = 1;
		}

		if (size + sizeof(uint32_t) + buf_[i].wr_off <= buf_size_) {
			return buf_[i].buf + buf_[i].wr_off;
		}
		if (buf_[i].rd_off == buf_[i].wr_off) {
			buf_[i].wr_off = 0;
			buf_[i].rd_off = 0;
		}
		i = 1 - i;
		if (size + sizeof(uint32_t) + buf_[i].wr_off <= buf_size_) {
			return buf_[i].buf + buf_[i].wr_off;
		}
		if (buf_[i].rd_off == buf_[i].wr_off) {
			buf_[i].wr_off = 0;
			buf_[i].rd_off = 0;
		}

		i = 0;
		if (buf_[0].rd_off < buf_[1].rd_off) {
			i = 1;
		}
		if (size + sizeof(uint32_t) + buf_[i].wr_off <= buf_size_) {
			return buf_[i].buf + buf_[i].wr_off;
		}
		i = 1 - i;
		if (size + sizeof(uint32_t) + buf_[i].wr_off <= buf_size_) {
			return buf_[i].buf + buf_[i].wr_off;
		}

		return NULL;
	}

	void set_write(char *ptr) {
		for (int i = 0; i < 2; ++ i) {
			if ((buf_[i].buf + buf_[i].wr_off) == ptr) {
				buf_[i].wr_off += *(uint32_t *)ptr + sizeof(uint32_t);
				break;
			}
		}
	}

	bool empty() {
		if ( (buf_[0].rd_off >= buf_[0].wr_off) 
				&& (buf_[1].rd_off >= buf_[1].wr_off)) {
			return true;
		} else {
			return false;
		}
	}

private:
	typedef struct _buf_t {
		char *buf;
		volatile size_t rd_off;
		volatile size_t wr_off;
	} buf_t;
	size_t  buf_size_;
	buf_t buf_[2];
};

}

#endif
