/**
 * @file
 * @author  Li Jiong <lijiong@staff.sina.com.cn>
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * woo framework 
 */
#ifndef __woo_mmap_H__
#define __woo_mmap_H__

#include <algorithm>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include "woo/log.h"

namespace woo {

inline int
mmap_file(const char *path, int prot, void **addr, size_t *size) {
	int flags = 0;

	if ((prot & PROT_READ) && (prot & PROT_WRITE)) {
		flags = O_RDWR;
	} else if (prot & PROT_WRITE) {
		flags = O_WRONLY;
	} else {
		flags = O_RDONLY;
	}

    int fd = open(path, flags);
    if (fd < 0) {
		LOG_ERROR("open [%s] error", path);
		return -1;
    }
    struct stat fs;
    fstat(fd, &fs);
    void *ptr = mmap(NULL, fs.st_size, prot, MAP_SHARED/*|MAP_LOCKED*/, fd, 0);
	close(fd);
	if (ptr != MAP_FAILED) {
		*addr = ptr;
		*size = fs.st_size;
		return 0;
	} else {
		LOG_ERROR("mmap [%s] error[%d][%m]", path, errno);
		return -1;
	}
}

inline int
mmap_file(const char *path, int prot, int mmap_flags, void **addr, size_t *size) {
	int flags = 0;

	if ((prot & PROT_READ) && (prot & PROT_WRITE)) {
		flags = O_RDWR;
	} else if (prot & PROT_WRITE) {
		flags = O_WRONLY;
	} else {
		flags = O_RDONLY;
	}

    int fd = open(path, flags);
    if (fd < 0) {
		LOG_ERROR("open [%s] error", path);
		return -1;
    }
    struct stat fs;
    fstat(fd, &fs);
    void *ptr = mmap(NULL, fs.st_size, prot, mmap_flags, fd, 0);
	close(fd);
	if (ptr != MAP_FAILED) {
		*addr = ptr;
		*size = fs.st_size;
		return 0;
	} else {
		LOG_ERROR("mmap [%s] error[%d][%m]", path, errno);
		return -1;
	}
}

class FileMmap {
public:
	FileMmap() : arr_(NULL), filesize_(0) {}
	int mmap(const char *path, int prot, int flags) {
		int ret;
		ret = woo::mmap_file(path, prot, flags, (void **)&arr_, &filesize_);
		if (ret) {
			LOG_ERROR("mmap [%s] error", path);
			return -1;
		}
		return 0;
	}

	void unmap() {
		if (arr_) {
			munmap(arr_, filesize_);
			arr_ = NULL;
		}
		filesize_ = 0;
	}

	void* base() { return arr_; }
	size_t size() { return filesize_; }

private:
	void *arr_;
	size_t filesize_;
};

template <typename StructT, typename CompT>
class StructFileMmap {
public:
	StructFileMmap(CompT comp = CompT()) : comp_(comp) {}
	int mmap(const char *path, int prot = PROT_READ, int flags = MAP_SHARED|MAP_LOCKED) {
		int ret = file_.mmap(path, prot, flags);
		if (ret) {
			return ret;
		}
		num_ = file_.size() / sizeof(StructT);
		return 0;
	}

	void munmap() {
		unmap();
	}

	void unmap() {
		file_.unmap();
		num_ = 0;
	}

	StructT *get(size_t i) {
		StructT *arr = (StructT *)file_.base();
		return arr + i;
	}
	
	size_t num() { return num_; }

	StructT *find (StructT key) {
		StructT *arr = (StructT *)file_.base();
		StructT *p = std::lower_bound(arr, arr + num_, key, comp_);
		if ((p == (arr + num_)) || comp_(key, *p)) {
			return NULL;
		}
		return p;
	}

	void sort() {
		StructT *arr = (StructT *)file_.base();
		std::sort(arr, arr + num_, comp_);
	}

private:
	CompT comp_;
	size_t num_;
	FileMmap file_;
};

}

#endif
