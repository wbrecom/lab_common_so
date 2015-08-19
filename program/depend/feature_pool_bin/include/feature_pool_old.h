#ifndef _FEATRUE_POOL_HEADER_
#define _FEATRUE_POOL_HEADER_

#include <assert.h>
#include <ext/hash_map>
#include <map>
#include <algorithm>
#include "skiplist.h"
#include "arena.h"
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <set>
#include "pthread_rw_locker.h"

#define SPE_ITEM "\t"

/*
feature pool主类
*/
#define MAIN_KEY_TYPE uint64_t
#define ITEM_KEY_TYPE uint32_t
//#define ITEM_KEY_TYPE unsigned short
#define ITEM_VALUE_TYPE uint32_t

class FeaturePool{
public:

	typedef __gnu_cxx::hash_map<MAIN_KEY_TYPE, char*> KeyHashMap;
	//typedef std::map<MAIN_KEY_TYPE, char*> KeyHashMap;
	typedef KeyHashMap::iterator KeyHashMapIterator;

	//上述定义结构
public:		
	FeaturePool(){
	}
	~FeaturePool(){//析构函数，只是做hash_map以及map的结构释放，主要的内存释放交给内存管理器
		//to release map;
		key_hash_map_.clear();
	}

public:
	/*返回feature pool大小*/
	int size(){
		int size = 0;
		rw_locker_.rdlock();
		size = key_hash_map_.size();
		rw_locker_.unlock();

		return size;
	}
	/*增加内部add函数，批量加入*/
	int _add(MAIN_KEY_TYPE key, const char* bit_item, bool b_lock){

		size_t str_len = strlen(bit_item) + 1;
		char* new_bit_item = new char[str_len];
		if(NULL == new_bit_item)
			return -1;
		memcpy(new_bit_item, bit_item, str_len - 1);
		new_bit_item[str_len - 1] = '\0';
		if(b_lock)
			rw_locker_.wrlock();
		std::pair< KeyHashMapIterator, bool > pr = 
			key_hash_map_.insert(std::make_pair(key, new_bit_item));	
		if(b_lock)
			rw_locker_.unlock();

		if(pr.second)
			return 1;
		else
			return 0;
	}

	/*
 	* code: uint32_t 16: start_pos; 16 : num 	
  	*/
	/*增加或者减少分数，同时支持extra为空或者非空*/
	int incrby(MAIN_KEY_TYPE key, uint32_t code, ITEM_VALUE_TYPE add_score){
		return generic_add(key, code, add_score, 2, true);		
	}

	/*修改分数，同时支持extra为空或者非空*/
	int update(MAIN_KEY_TYPE key, uint32_t code, ITEM_VALUE_TYPE new_score){
		return generic_add(key, code, new_score, 1, true);		
	}

	/*添加item*/
	int add(MAIN_KEY_TYPE key, uint32_t code, ITEM_VALUE_TYPE score, bool lock_flag = true){
		return generic_add(key, code, score, 0, lock_flag);
	}

	bool get_score(ITEM_VALUE_TYPE& score, uint16_t start_pos, uint16_t num, size_t str_len, char* &p_item_value){
		if(NULL == p_item_value)
			return false;
		score = 0;
		bool b_flag = false;
		if(start_pos + num <= str_len){
			for(int i = 0; i < num; i++){
				if(p_item_value[start_pos + i] != '\001'){
					b_flag = true;
					score = score * 10 + (p_item_value[start_pos + i] - '0');
					//printf("false:%d:%d\n", i, score);
				}
				//else
				//	printf("true:%d\n", i);
			}
			return b_flag;
		}else
			return false;
	}

	bool update_score(uint16_t start_pos, uint16_t num, size_t str_len, 
		ITEM_VALUE_TYPE new_score, char* &p_item_value){
		if(NULL == p_item_value)
			return false;
		if(start_pos + num <= str_len){	
			bool flag = false;
			for(int i = num - 1; i >= 0; i--){
				if(flag){
					p_item_value[start_pos + i] = '\001';
					//printf("true:%s\n", p_item_value);
				}
				else{
					p_item_value[start_pos + i] = new_score % 10 + '0'; 
					//printf("false:%s\n", p_item_value);
				}
				new_score = new_score / 10;
				//printf("%d:new score:%d\n", i, new_score);
				if(0 == new_score)
					flag = true;
					
			}
			return true;

		}else{
			return false;
		}
	}

	bool add_score(uint16_t start_pos, uint16_t num, size_t str_len,
                ITEM_VALUE_TYPE new_score, char* p_item_value, char* &p_new_item_value){
		if(start_pos >= str_len){
			p_new_item_value = new char[start_pos + num + 1];
			if(NULL == p_new_item_value)
				return 0;
			if(p_item_value != NULL && str_len != 0){
				memcpy(p_new_item_value, p_item_value, str_len);
			}else{
				str_len = 0;
			}

			bool flag = false;	
			for(int i = num -1; i >= 0; i--){
				if(flag)
					p_new_item_value[start_pos + i] = '\001';
				else
					p_new_item_value[start_pos + i] = new_score % 10 + '0';
				new_score = new_score / 10;
				if(new_score == 0)
					flag = true;
			}
			for(int i = str_len; i < start_pos; i ++){
				p_new_item_value[i] = '\001';
			}
			return true;
		}else
			return false;
		
	}
	/*基础增加，修改函数*/
	int generic_add(MAIN_KEY_TYPE key, uint32_t code, ITEM_VALUE_TYPE score, 
		int n_type, bool lock_flag = true){
	
		uint16_t start_pos = code >> 16;
                uint16_t num = (code << 16) >> 16;
	
		if(lock_flag)
			rw_locker_.rdlock();
		
		KeyHashMapIterator it = key_hash_map_.find(key);		
		if (it != key_hash_map_.end()){
			char* p_item_value = (*it).second;
			if(lock_flag)
				rw_locker_.unlock();

			if(NULL == p_item_value )
				return -1;
		
			size_t item_len = strlen(p_item_value);	
			bool b_result = false;
			if(item_len > start_pos){
				//printf("%d:%d:%d\n", start_pos, num, item_len);
				//找到已存在的key, item_key的情况
				if(start_pos + num >= item_len)
					return -1;
				else{
					if(n_type == 0 || n_type == 1){
						if(lock_flag)
							rw_locker_.wrlock();
						b_result = update_score(start_pos, num, item_len, score, p_item_value); 
						if(lock_flag)
							rw_locker_.unlock();
						if(!b_result)
							return -2;
					}else if(n_type == 2){
						ITEM_VALUE_TYPE old_score = 0;
						if(lock_flag)
							rw_locker_.wrlock();
						b_result = get_score(old_score, start_pos, num, item_len, p_item_value);				
						if(!b_result){
							if(lock_flag)
								rw_locker_.unlock();
							return -2;
						}
						b_result = update_score(start_pos, num, item_len, old_score + score, p_item_value);
						if(!b_result){
							if(lock_flag)
								rw_locker_.unlock();
							return -2;
						}
						if(lock_flag)
							rw_locker_.unlock();
					}else{
						return -3;
					}
					return 1;
				}
			}else{
				char* p_new_item_value = NULL;
				b_result = add_score(start_pos, num, item_len, score, p_item_value, p_new_item_value);
				if(!b_result || NULL == p_new_item_value)
					return -2;
				if(lock_flag)
					rw_locker_.wrlock();
				
				(*it).second = p_new_item_value;
				if(lock_flag)
					rw_locker_.unlock();

				return 1;
				//找到了key,但是必须添加新的item_key的情况
			}
			
		}else{
			//找不到key,添加新的key以及item_key
			if(lock_flag)
				rw_locker_.unlock();

			char* p_new_item_value = NULL;
                        bool b_result = add_score(start_pos, num, 0, score, NULL, p_new_item_value);
                        if(!b_result || NULL == p_new_item_value)
                                return -2;

			if(lock_flag)
				rw_locker_.wrlock();
			std::pair< KeyHashMapIterator, bool > pr = 
				key_hash_map_.insert(std::make_pair(key, p_new_item_value));	
			if(lock_flag)
				rw_locker_.unlock();

			if(pr.second)
				return 1;
			else
				return 0;
			
		}
		return 1;
	}

	/*判断 key是否存在*/	
	bool contains(MAIN_KEY_TYPE key){
	
		rw_locker_.rdlock();
		KeyHashMapIterator it = key_hash_map_.find(key);		
		if (it != key_hash_map_.end()){
			rw_locker_.unlock();
			return true;
		}else{
			rw_locker_.unlock();
			return false;
		}
	}

	/*判断 key, item_key是否存在*/
	bool exists(MAIN_KEY_TYPE key, uint32_t code){

		rw_locker_.rdlock();
		KeyHashMapIterator it = key_hash_map_.find(key);		
		if (it != key_hash_map_.end()){
			char* p_item_value = (*it).second;
			rw_locker_.unlock();
			if(NULL == p_item_value)
				return false;

			uint16_t start_pos = code >> 16;
                        uint16_t num = (code << 16) >> 16;
                        size_t item_len = strlen(p_item_value);

			ITEM_VALUE_TYPE old_score = 0;
			return get_score(old_score, start_pos, num, item_len, p_item_value);
		
		}else{
			rw_locker_.unlock();
			return false;
		}
	}

	/*查找key, item_key，返回数据内容*/
	bool get(MAIN_KEY_TYPE key, uint32_t code, ITEM_VALUE_TYPE& return_value){
		
		rw_locker_.rdlock();
		KeyHashMapIterator it = key_hash_map_.find(key);		
		if (it != key_hash_map_.end()){
			char* p_item_value = (*it).second;
			rw_locker_.unlock();
	
			uint16_t start_pos = code >> 16;
                        uint16_t num = (code << 16) >> 16;
                        size_t item_len = strlen(p_item_value);

                        return get_score(return_value, start_pos, num, item_len, p_item_value);	
		}else{
			rw_locker_.unlock();
			return false;
		}
	}
	
	/*删除某个key*/
	bool rem(MAIN_KEY_TYPE key){
		
		bool b_result = true;
		
		rw_locker_.wrlock();
		KeyHashMap::size_type it = key_hash_map_.erase(key);
            	rw_locker_.unlock();

		return b_result;
	}

	/*序列化文件，将内存数据导出到外存中
	文件格式：奇数行—— 文本长度，文本内容
		偶数行—— item_key score extra文本长度 extra文本内容【重复item，同时保证有序】
	其中的间隔为'\t'
	*/
	bool seralize(const char* p_seralize_file_name, bool lock_flag = false){
		if (NULL == p_seralize_file_name)
			return false;

		std::ofstream out_file(p_seralize_file_name);
		if (!out_file.is_open())		
			return false;	

		if(lock_flag){
			rw_locker_.rdlock();
		}
	
		std::string str_sep = SPE_ITEM;
		for(KeyHashMapIterator it = key_hash_map_.begin();
			it != key_hash_map_.end(); it++){

			MAIN_KEY_TYPE key = (*it).first;
			out_file << key << SPE_ITEM << std::string((*it).second) << std::endl;
		}	

		if(lock_flag){
			rw_locker_.unlock();
		}

		out_file.close();
		return true;
	}

	/*反序列化，将外存文件载入内存中*/
	bool unseralize(const char* p_seralize_file_name){
		if (NULL == p_seralize_file_name)
			return false;

		std::ifstream in_file(p_seralize_file_name);
		if(!in_file.is_open())
			return false;

		int i = 0;
		std::string line;
		std::stringstream str_stream;

		while(getline(in_file, line)){
			str_stream.clear();
			str_stream.str(line);

			MAIN_KEY_TYPE key;
			std::string str_value;
			str_stream >> key >> str_value;
			_add(key, str_value.c_str(), false);
			i ++;
		}
		in_file.close();

		return true;
	}			

private:
	//leveldb::Arena arena_; //内存分配器
	//Sorted_Set_Comparator item_comp_; //比较函数
	PthreadRWLocker rw_locker_; // key_hash_map_ 读写锁
	//PthreadRWLocker rw_mem_locker_; // arena_ 内存分配读写锁
	KeyHashMap key_hash_map_; //数据存储hash_map：
};

#endif
