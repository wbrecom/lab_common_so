/**
 * @file
 * @author  Wang Chuanpeng <chuanpeng@staff.sina.com.cn>
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * woo framework 
 */
#ifndef __single_uthash_mgr_H__
#define __single_uthash_mgr_H__

#include "uthash.h"
#include "woo/mutex_lock.h"
#include "woo/log.h"

namespace woo {
	const int DEFAULT_MAX_CACHE_LEN = 1024000;
	
	template <typename KeyT, typename ValueT>
	class LruUtCacheMgr{
		typedef struct _uthash_t {
			KeyT key_id;            /* we'll use this field as the key */
			ValueT* p_value;             
			UT_hash_handle hh; /* makes this structure hashable */
		} UthashT;

		public:
			LruUtCacheMgr<KeyT, ValueT>():cache_num_(DEFAULT_MAX_CACHE_LEN), block_h_(NULL){
			}

			LruUtCacheMgr<KeyT, ValueT>(uint64_t cache_num):cache_num_(cache_num), block_h_(NULL){
			}

			~LruUtCacheMgr<KeyT, ValueT>(){
				release();
			}

		private:
			uint64_t cache_num_;
			UthashT* block_h_;
			MutexLock mutex_lock_;

		public:
			void set_cache_num(uint64_t cache_num){
				cache_num_ = cache_num;
			}

			ValueT* find(KeyT key_id){
				UthashT* p_uthash_t = NULL;
				ValueT* p_value = NULL;
				
				mutex_lock_.lock();
				HASH_FIND(hh, block_h_, &key_id, sizeof(KeyT), p_uthash_t);
				if(p_uthash_t){
					HASH_DELETE(hh, block_h_, p_uthash_t);
					HASH_ADD(hh, block_h_, key_id, sizeof(KeyT), p_uthash_t);

					p_value = p_uthash_t->p_value;
				}
				mutex_lock_.unlock();

				return p_value;
			}

			bool add(KeyT key_id, ValueT* p_value){
				bool exe_flag = true;
				UthashT* p_temp_uthash_t = NULL;
				UthashT* p_new_uthash_t = (UthashT*)malloc(sizeof(UthashT));
				if(!p_new_uthash_t){
					LOG_ERROR("malloc block is failed!size:[%d]", sizeof(UthashT));
					return false;
				}
				p_new_uthash_t->key_id = key_id;
				p_new_uthash_t->p_value = p_value;
				
				mutex_lock_.lock();
				HASH_ADD(hh, block_h_, key_id, sizeof(KeyT), p_new_uthash_t);

				UthashT* p_null_uthash_t = NULL;
				if (HASH_COUNT(block_h_) >= cache_num_) {
					LOG_DEBUG("out of cache limit:[%l]", cache_num_);
					HASH_ITER(hh, block_h_, p_null_uthash_t, p_temp_uthash_t) {
						HASH_DELETE(hh, block_h_, p_null_uthash_t);
						free(p_null_uthash_t->p_value);
						free(p_null_uthash_t);
						break;
					}
				}
				mutex_lock_.unlock();
				
				return exe_flag;
			}

			bool del(KeyT key_id){
				bool exe_flag = true;
				UthashT* p_uthash_t = NULL;

				mutex_lock_.lock();
				HASH_FIND(hh, block_h_, &key_id, sizeof(KeyT), p_uthash_t);
				if(p_uthash_t){
					HASH_DELETE(hh, block_h_, p_uthash_t);
					free(p_uthash_t->p_value);
					free(p_uthash_t);
				}else{
					exe_flag = false;
				}
				mutex_lock_.unlock();

				return exe_flag;
			}

			bool release(){
				bool exe_flag = true;
				UthashT *p_crt_t = NULL;
				UthashT *p_tmp_t = NULL;

				mutex_lock_.lock();
				HASH_ITER(hh, block_h_, p_crt_t, p_tmp_t) {
					  HASH_DELETE(hh, block_h_, p_crt_t);
					  free(p_crt_t->p_value);
					  free(p_crt_t);
				}
				mutex_lock_.unlock();
				return exe_flag;
			}
	};
}

#endif 
