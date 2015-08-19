#ifndef ___woo_keyblockfilemmap2_H__
#define ___woo_keyblockfilemmap2_H__

#include "woo/mmap.h"
#include "uthash.h"
#include "woo/mutex_lock.h"
#include "woo/lru_utcache_mgr.h"

namespace woo {

	typedef LruUtCacheMgr<uint64_t, char> LruUtCacheMgrUc;

template <typename KeyT, typename KeyCompT>
class KeyBlockFileMmap2 {
public:
	KeyBlockFileMmap2(KeyCompT key_comp = KeyCompT()) : key_comp_(key_comp), key_filesize_(0), key_arr_(NULL), key_num_(0), 
	block_fd_(-1){}	
	
	int mmap(const char *key_path, const char *block_path) {
		LOG_DEBUG("open %s %s", key_path, block_path);
		int ret;
		ret = woo::mmap_file(key_path, PROT_READ, MAP_SHARED|MAP_LOCKED, (void **)&key_arr_, &key_filesize_);
		if (ret) {
			LOG_ERROR("mmap [%s] error", key_path);
			return -1;
		}
		key_num_ = key_filesize_ / sizeof(KeyT);
		
		int fd = open(block_path, O_RDONLY);
		if (fd < 0) {
			LOG_ERROR("open [%s] error", block_path);
			return -1;
		}
		block_fd_ = fd;

		return 0;
	}

	int mmap(const char *key_path, int key_prot, int key_flags,
			const char *block_path) {
		LOG_DEBUG("open %s %s", key_path, block_path);
		int ret;
		ret = woo::mmap_file(key_path, key_prot, key_flags, (void **)&key_arr_, &key_filesize_);
		if (ret) {
			LOG_ERROR("mmap [%s] error", key_path);
			return -1;
		}
		key_num_ = key_filesize_ / sizeof(KeyT);
		
		int fd = open(block_path, O_RDONLY);
		if (fd < 0) {
			LOG_ERROR("open [%s] error", block_path);
			return -1;
		}
		block_fd_ = fd;

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

		close(block_fd_);
		block_fd_ = -1;
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

	//添加了新的mfind_new函数处理，减少处理keys的数量，提升时间
	int mfind_new (KeyT keys[], int sign[], int num, KeyT* result[]) {
		KeyT* key_arr_start = key_arr_;
		KeyT* key_arr_end = key_arr_ + key_num_;
	
		for(int i = 0; i < num;  i ++){
			if (sign[i] == 0)
				continue;

			KeyT key = keys[i];
			KeyT *p = std::lower_bound(key_arr_start, key_arr_end, key, key_comp_);
			key_arr_start = p ;
			if ((p == (key_arr_ + key_num_)) || key_comp_(key, *p)) {
				result[i] = NULL;
			}
			else{
				result[i] = p;
			}
		}
		return 0;
	}

	int mfind (KeyT keys[], int num, KeyT* result[]) {
		KeyT* key_arr_start = key_arr_;
		KeyT* key_arr_end = key_arr_ + key_num_;
	
		for(int i = 0; i < num;  i ++){
			KeyT key = keys[i];
			KeyT *p = std::lower_bound(key_arr_start, key_arr_end, key, key_comp_);
			key_arr_start = p ;
			if ((p == (key_arr_ + key_num_)) || key_comp_(key, *p)) {
				result[i] = NULL;
			}
			else{
				result[i] = p;
			}
		}
		return 0;
	}

	char *get_block(size_t offset) {
		//return block_base_ + offset;
		return NULL;
	}

	char *get_block(uint64_t key_id, size_t offset, size_t count, LruUtCacheMgrUc* p_cache_mgr) {

		if(p_cache_mgr == NULL){
			LOG_ERROR("p_cache_mgr is NULL!");
			return NULL;
		}

		char* re_block_content = p_cache_mgr->find(key_id);
		if (re_block_content == NULL){
			char* block_content = (char*)malloc(count + 1);
			if (!block_content)
			{
				LOG_ERROR("malloc block is failed!size:[%d]", count + 1);
				return NULL;
			}

			memset(block_content, '\0', count + 1);
			ssize_t result = pread(block_fd_, block_content, count, offset);
			if(result > 0){
				bool b_flag = p_cache_mgr->add(key_id, block_content);
				if(b_flag){
					re_block_content = block_content;
				}else{
					free(block_content);
					LOG_ERROR("add lru cache is failed!");
				}
			}
			else if(result != (ssize_t)count){
				free(block_content);
				LOG_ERROR("pread file num is error!");
			}
			else{
				free(block_content);
				LOG_ERROR("pread file is failed!");
			}
		}
		return re_block_content;
	}

private:
	KeyCompT key_comp_;
	size_t key_filesize_;

public:
	KeyT *key_arr_;
	size_t key_num_;

	int block_fd_;
};

}

#endif
