/**
 * @file
 * @author  Li Jiong <lijiong@staff.sina.com.cn>
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * woo framework 
 */
#ifndef ___woo_keyblockfilemmap_H__
#define ___woo_keyblockfilemmap_H__

#include "woo/mmap.h"

namespace woo {

template <typename KeyT, typename KeyCompT>
class KeyBlockFileMmap {
public:
	KeyBlockFileMmap(KeyCompT key_comp = KeyCompT()) : key_comp_(key_comp), key_filesize_(0), key_arr_(NULL), key_num_(0), 
	block_filesize_(0), block_base_(NULL){}
	int mmap(const char *key_path, const char *block_path) {
		LOG_DEBUG("open %s %s", key_path, block_path);
		int ret;
		ret = woo::mmap_file(key_path, PROT_READ, MAP_SHARED|MAP_LOCKED, (void **)&key_arr_, &key_filesize_);
		if (ret) {
			LOG_ERROR("mmap [%s] error", key_path);
			return -1;
		}
		key_num_ = key_filesize_ / sizeof(KeyT);

		ret = woo::mmap_file(block_path, PROT_READ, MAP_SHARED|MAP_LOCKED, (void **)&block_base_, &block_filesize_);
		if (ret) {
			LOG_ERROR("mmap [%s] error", block_path);
			return -1;
		}
		return 0;
	}

	int mmap(const char *key_path, int key_prot, int key_flags,
			const char *block_path, int block_prot, int block_flags) {
		LOG_DEBUG("open %s %s", key_path, block_path);
		int ret;
		ret = woo::mmap_file(key_path, key_prot, key_flags, (void **)&key_arr_, &key_filesize_);
		if (ret) {
			LOG_ERROR("mmap [%s] error", key_path);
			return -1;
		}
		key_num_ = key_filesize_ / sizeof(KeyT);

		ret = woo::mmap_file(block_path, block_prot, block_flags, (void **)&block_base_, &block_filesize_);
		if (ret) {
			LOG_ERROR("mmap [%s] error", block_path);
			return -1;
		}
		return 0;
	}

	void munmap() {
		unmap();
	}

	void unmap() {
		if (key_arr_) { 
			::munmap(key_arr_, key_filesize_);
			key_arr_ = NULL;
		}
		key_filesize_ = 0;
		key_num_ = 0;
		if (block_base_) {
			::munmap(block_base_, block_filesize_);
			block_base_ = NULL;
		}
		block_filesize_ = 0;
	}

	KeyT *get(size_t i) {
		return key_arr_ + i;
	}
	
	size_t key_num() { return key_num_; }

	KeyT *find (KeyT key) {
		KeyT *p = std::lower_bound(key_arr_, key_arr_+ key_num_, key, key_comp_);
		if ((p == (key_arr_ + key_num_)) || key_comp_(key, *p)) {
			return NULL;
		}
		return p;
	}

	char *get_block(size_t offset) {
		return block_base_ + offset;
	}

private:
	KeyCompT key_comp_;
	size_t key_filesize_;
public:
	KeyT *key_arr_;
	size_t key_num_;
	size_t block_filesize_;
	char* block_base_;
};

}

#endif
