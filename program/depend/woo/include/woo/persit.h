/**
 * @file
 * @author  Li Jiong <lijiong@staff.sina.com.cn>
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * woo framework 
 */
#ifndef __woo_persit_H__
#define __woo_persit_H__

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "woo/crc.h"

namespace woo {

inline uint32_t sign_data(const char *value, size_t len) {
	return chksum_crc32((unsigned char *)value, len);
}

template<typename T>
struct PersitLess {
	bool operator() (const T& x, const T&y) {
		return x < y;
	}
};

template<typename T, typename CompT = PersitLess<T> >
class Persit {
public:	
	static const int RET_OK = 0;
	static const int RET_OPEN = -1;
	static const int RET_READ = -2;
	static const int RET_WRITE = -3;
	static const int RET_FILESIGN = -4;

public:
	Persit(uint32_t pid = 0, const CompT& comp = CompT()) : fd_(-1), pid_(pid), comp_(comp) {}

	int open(const char *path) {
		fd_ = ::open(path, O_RDWR);
		if (fd_ < 0) {
			printf("open %s error[%d][%m]\n", path, errno);
			return -1;
		}

		ssize_t nread;
		nread = pread(fd_, &cur_, sizeof(cur_), 0);
		if (nread != sizeof(cur_)) {
			printf("pread ret[%zd] ne [%zd] error[%d][%m]\n", nread, sizeof(cur_), errno);
			return -1;
		}

		uint32_t sign = sign_data((const char *)&cur_.value, sizeof(cur_.value));
		if (cur_.sign != sign){
			printf("sign[%08x] ne read [%08x]\n", sign, cur_.sign);
			return -1;
		}

		if (wrlock()) {
			printf("write lock file error[%d][%m]\n", errno);
			return -1;
		}

		return 0;
	}

	int wrlock() {
		static struct flock lock;
		lock.l_type = F_WRLCK;
		lock.l_start = 0;
		lock.l_whence = SEEK_SET;
		lock.l_len = 0;
		lock.l_pid = getpid();

		int ret = fcntl(fd_, F_SETLK, &lock);
		return ret;
	}

	void unlock() {
		static struct flock lock;
		lock.l_type = F_UNLCK;
		lock.l_start = 0;
		lock.l_whence = SEEK_SET;
		lock.l_len = 0;
		lock.l_pid = getpid();

		fcntl(fd_, F_SETLK, &lock);
	}

	int reload(T &v) {
		ssize_t nread;
		store_t new_cur;

		nread = pread(fd_, &new_cur, sizeof(new_cur), 0);
		if (nread != sizeof(new_cur)) {
			return -1;
		}

		if (new_cur.sign != sign_data((const char *)&new_cur.value, sizeof(new_cur.value))){
			return -1;
		}

		if (comp_(new_cur.value, cur_.value)) {
			return -1;
		}

		if (new_cur.pid == pid_) {
			return -1;
		}

		v = new_cur.value;

		return 0;
	}

	int update(T v) {
		int nwrite;
		store_t new_cur;

		new_cur.value = v;
		new_cur.sign = sign_data((const char *)&v, sizeof(v));
		new_cur.pid = pid_;

		nwrite = pwrite(fd_, &cur_, sizeof(cur_), sizeof(cur_));
		if (nwrite != sizeof(cur_)) {
			return -1;
		}

		nwrite = pwrite(fd_, &new_cur, sizeof(new_cur), 0);
		if (nwrite != sizeof(new_cur)) {
			return -1;
		}

		cur_ = new_cur;

		return 0;
	}

	const T* operator->() {
		return &cur_.value;
	}

	const T& get() {
		return cur_.value;
	}

	void close() {
		unlock();
		::close(fd_);
		fd_ = -1;
	}

private:
	typedef struct _store_t {
		T value;
		uint32_t sign;
		uint32_t pid;
	} store_t;

	int fd_;
	store_t cur_;
	uint32_t pid_;
	CompT comp_;
};

template<typename T>
class PersitReader {
public:	
	static const int RET_OK = 0;
	static const int RET_OPEN = -1;
	static const int RET_READ = -2;
	static const int RET_WRITE = -3;
	static const int RET_FILESIGN = -4;

public:
	PersitReader() : fd_(-1) {}

	int open(const char *path) {
		fd_ = ::open(path, O_RDWR);
		if (fd_ < 0) {
			return -1;
		}

		ssize_t nread;
		nread = pread(fd_, &cur_, sizeof(cur_), 0);
		if (nread != sizeof(cur_)) {
			return -1;
		}

		if (cur_.sign != sign_data((const char *)&cur_.value, sizeof(cur_.value))){
			return -1;
		}

		return 0;
	}

	int reload() {
		ssize_t nread;
		store_t new_cur;

		nread = pread(fd_, &new_cur, sizeof(new_cur), 0);
		if (nread != sizeof(new_cur)) {
			return -1;
		}

		if (new_cur.sign != sign_data((const char *)&new_cur.value, sizeof(new_cur.value))){
			return -1;
		}

		cur_ = new_cur;

		return 0;
	}	

	const T* operator->() {
		return &cur_.value;
	}

	const T& get() {
		return cur_.value;
	}

	void close() {
		::close(fd_);
		fd_ = -1;
	}

private:
	typedef struct _store_t {
		T value;
		uint32_t sign;
	} store_t;

	int fd_;
	store_t cur_;
};

}

#endif
