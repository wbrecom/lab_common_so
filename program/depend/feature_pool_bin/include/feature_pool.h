#ifndef _FEATRUE_POOL_HEADER_
#define _FEATRUE_POOL_HEADER_

#include <assert.h>
#include <ext/hash_map>
#include <map>
#include <algorithm>
//#include "skiplist.h"
#include "arena.h"
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <set>
#include <stdio.h>
#include <math.h>
#include "pthread_rw_locker.h"

#define SPE_ITEM "\t"
#define FILL_CHAR '\001'

/*
feature pool主类
*/
#define MAIN_KEY_TYPE uint64_t
#define ITEM_KEY_TYPE uint64_t
#define START_POS_TYPE uint64_t
#define NUM_TYPE uint16_t
#define ITEM_VALUE_TYPE uint64_t
#define HASH_LOCKER_NUM 1024
#define ADD_VALUE_TYPE int64_t

typedef std::vector<ITEM_KEY_TYPE> ItemVec;
typedef struct _KeyItem{
	ItemVec item_vec_;
	MAIN_KEY_TYPE key_;
} KeyItem;
typedef std::vector<KeyItem> KeyItemVec;
typedef std::vector<MAIN_KEY_TYPE> KeyVec;
typedef std::map<ITEM_KEY_TYPE, ITEM_VALUE_TYPE> CodeItemMap;
typedef std::map<MAIN_KEY_TYPE, CodeItemMap> KeyCodeItemMap;
typedef std::map<ITEM_KEY_TYPE, int> ItemResultMap;
typedef std::map<MAIN_KEY_TYPE, ItemResultMap> KeyItemResultMap;

// added by xiatuo
typedef std::map<ITEM_KEY_TYPE, ADD_VALUE_TYPE> CodeItemMapAdd;
typedef std::map<MAIN_KEY_TYPE, CodeItemMapAdd> KeyCodeItemMapAdd;
// added end

	typedef __gnu_cxx::hash_map<MAIN_KEY_TYPE, char*> KeyHashMap;
	//typedef std::map<MAIN_KEY_TYPE, char*> KeyHashMap;
	typedef KeyHashMap::iterator KeyHashMapIterator;
	typedef std::map<std::string, KeyHashMap> KeyHashMaps;
	typedef KeyHashMaps::iterator KeyHashMapsIterator;

class FeaturePool{
public:

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
		//PthreadRWLocker& rw_locker = get_hash_locker(0);

		//rw_locker.rdlock();
		size = key_hash_map_.size();
		//rw_locker.unlock();
		return size;
	}
	/*增加内部add函数，批量加入*/
	int _add(MAIN_KEY_TYPE key, const char* bit_item, bool b_lock){

		size_t str_len = strlen(bit_item) + 1;
		char* new_bit_item = NULL;
		if(b_lock)
			rw_mem_locker_.wrlock();
		/*对于arena_ AllocateAligned 必须添加内存 的锁*/
		new_bit_item = arena_.AllocateAligned(str_len);
		//memset(new_bit_item, FILL_CHAR, str_len);
		if(b_lock)
			rw_mem_locker_.unlock();
		if(NULL == new_bit_item)
			return -1;
		memcpy(new_bit_item, bit_item, str_len - 1);
		new_bit_item[str_len - 1] = '\0';

		PthreadRWLocker& rw_locker = get_hash_locker(key);
		if(b_lock)
			rw_locker.wrlock();
		std::pair< KeyHashMapIterator, bool > pr = 
			key_hash_map_.insert(std::make_pair(key, new_bit_item));	
		if(b_lock)
			rw_locker.unlock();

		if(pr.second)
			return 1;
		else
			return 0;
	}

	/*
 	* code: ITEM_KEY_TYPE 48: start_pos; 16 : num 	
  	*/
	void get_start_num(START_POS_TYPE& start_pos, NUM_TYPE& num, ITEM_KEY_TYPE code){
		start_pos = code >> 16;
		num = code & 0xFFFF;
	}

	/*增加或者减少分数，同时支持extra为空或者非空*/
	int incrby(MAIN_KEY_TYPE key, ITEM_KEY_TYPE code, ADD_VALUE_TYPE add_score){
		//return generic_add(key, code, add_score, 2, true);		
		return specific_add(key, code, add_score, true);		
	}

	/*批量增加或者减少分数，同时支持extra为空或者非空*/
	//int incrbys(MAIN_KEY_TYPE key, const CodeItemMap& code_item_map, ItemResultMap& result_map){
	int incrbys(MAIN_KEY_TYPE key, const CodeItemMapAdd& code_item_map, ItemResultMap& result_map){
		for(CodeItemMapAdd::const_iterator pos = code_item_map.begin(); pos != code_item_map.end(); pos++){
			ITEM_KEY_TYPE code = (*pos).first;
			//ITEM_VALUE_TYPE add_score = (*pos).second;
			ADD_VALUE_TYPE add_score = (*pos).second;

			result_map.insert(std::make_pair(code, incrby(key, code, add_score)));
		}
		return 1;	
	}

	/*批量keys 增加或者减少分数，同时支持extra为空或者非空*/
	//int mincrbys(const KeyCodeItemMap& key_code_item_map, KeyItemResultMap& key_item_result_map){
	int mincrbys(const KeyCodeItemMapAdd& key_code_item_map, KeyItemResultMap& key_item_result_map){
		for(KeyCodeItemMapAdd::const_iterator key_pos = key_code_item_map.begin();
			key_pos != key_code_item_map.end(); key_pos++){
			MAIN_KEY_TYPE key = (*key_pos).first;
			//const CodeItemMap& code_item_map = (*key_pos).second;
			const CodeItemMapAdd& code_item_map = (*key_pos).second;

			ItemResultMap result_map;
			incrbys(key, code_item_map, result_map);

			key_item_result_map.insert(std::make_pair(key, result_map));
		}
		return 1;
	}	
	
	/*修改分数，同时支持extra为空或者非空*/
	int update(MAIN_KEY_TYPE key, ITEM_KEY_TYPE code, ITEM_VALUE_TYPE new_score){
		return generic_add(key, code, new_score, 1, true);		
	}

	/*批量修改分数，同时支持extra为空或者非空*/
	int updates(MAIN_KEY_TYPE key, const CodeItemMap& code_item_map, ItemResultMap& result_map){
		for(CodeItemMap::const_iterator pos = code_item_map.begin(); pos != code_item_map.end(); pos++){
			ITEM_KEY_TYPE code = (*pos).first;
			ITEM_VALUE_TYPE add_score = (*pos).second;

			result_map.insert(std::make_pair(code, update(key, code, add_score)));
		}
		return 1;
	}
	
	/*批量keys 修改分数，同时支持extra为空或者非空*/
	int mupdates(const KeyCodeItemMap& key_code_item_map, KeyItemResultMap& key_item_result_map){
		for(KeyCodeItemMap::const_iterator key_pos = key_code_item_map.begin();
			key_pos != key_code_item_map.end(); key_pos++){
			MAIN_KEY_TYPE key = (*key_pos).first;
			const CodeItemMap& code_item_map = (*key_pos).second;

			ItemResultMap result_map;
			updates(key, code_item_map, result_map);

			key_item_result_map.insert(std::make_pair(key, result_map));
		}
		return 1;
	}	
	/*添加item*/
	int add(MAIN_KEY_TYPE key, ITEM_KEY_TYPE code, ITEM_VALUE_TYPE score, bool lock_flag = true){
		return generic_add(key, code, score, 0, lock_flag);
	}
	
	/*批量添加item*/
	int adds(MAIN_KEY_TYPE key, const CodeItemMap& code_item_map, ItemResultMap& result_map){
		for(CodeItemMap::const_iterator pos = code_item_map.begin(); pos != code_item_map.end(); pos++){
			ITEM_KEY_TYPE code = (*pos).first;
			ITEM_VALUE_TYPE add_score = (*pos).second;

			result_map.insert(std::make_pair(code, add(key, code, add_score)));
		}
		return 1;
	}
	/*  added by xiatuo*/
	bool incr_score(START_POS_TYPE start_pos, NUM_TYPE num, size_t str_len,
        ADD_VALUE_TYPE add_score, char* &p_item_value){
		int i = num - 1;
		if(NULL == p_item_value)
			return false;
		if(start_pos + num <= str_len){	
			ADD_VALUE_TYPE div_value = add_score;
			//printf("add_score::%lld\n",div_value);
			if ( div_value > 0 ){
				while(i >= 0){
					ITEM_VALUE_TYPE i_value = p_item_value[start_pos + i] - '0' + div_value;
					//printf("%d\t%lld\t%lld\n",i, i_value,div_value);
					if(p_item_value[start_pos + i] == FILL_CHAR)
						i_value = div_value;		
					
					div_value = i_value / 10;
					char odd_value = i_value % 10 + '0';
					p_item_value[start_pos + i] = odd_value;
					if (div_value == 0)
						break;
					i -- ;
				}
				return true;
			}else{
				bool borrow_flag = false;
				bool modify_flag = false;
				int j = num - 1;
				int i_value;
				int minus_value;
				ITEM_VALUE_TYPE div_value = 0 - add_score;
				while( i>=0 ){
					if ( div_value == 0 && borrow_flag == false )
						break;
					if(p_item_value[start_pos + i] == FILL_CHAR){
						if (modify_flag){
							for( j = num-1; j>i; j-- ){
								p_item_value[ start_pos + j ] = '0';
							}
						}
						return false;
					}
					modify_flag = true;
					if (borrow_flag){
						i_value = p_item_value[ start_pos + i ] - '0' - 1;
						if( i_value < 0 ){
							i_value = 9;
						}else{
							borrow_flag = false;
						}
					}else{
						i_value = p_item_value[ start_pos + i ] - '0';
					}
					minus_value = div_value % 10;
					div_value = div_value / 10;
					i_value = i_value - minus_value;
					if( i_value < 0 ){
						borrow_flag = true;
						i_value += 10;
					}
					p_item_value[ start_pos + i] = i_value + '0';
					i--;
				}
				if( borrow_flag ){
					for( j = num -1; j>=i; j-- ){
						p_item_value[ start_pos +j ] = '0';
					}
					return false;
				}
				return true;
			}

		}else{
			return false;
		}
		
	}

	bool get_score(ITEM_VALUE_TYPE& score, START_POS_TYPE start_pos, NUM_TYPE num, 
		size_t str_len, char* &p_item_value){
		if(NULL == p_item_value)
			return false;
		score = 0;
		bool b_flag = false;
		if(start_pos + num <= str_len){
			for(int i = 0; i < num; i++){
				if(p_item_value[start_pos + i] != FILL_CHAR){
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

	bool update_score(START_POS_TYPE start_pos, NUM_TYPE num, size_t str_len, 
		ITEM_VALUE_TYPE new_score, char* &p_item_value){
		if(NULL == p_item_value)
			return false;
		if(start_pos + num <= str_len){	
			bool flag = false;
			for(int i = num - 1; i >= 0; i--){
				if(flag){
					p_item_value[start_pos + i] = FILL_CHAR;
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

	bool add_score(START_POS_TYPE start_pos, NUM_TYPE num, size_t str_len,
        ITEM_VALUE_TYPE new_score, char* p_item_value, char* &p_new_item_value){
		if(start_pos >= str_len){
            rw_mem_locker_.wrlock();
           	/*对于arena_ AllocateAligned 必须添加内存 的锁*/
			p_new_item_value = arena_.AllocateAligned(start_pos + num + 1);
			//memset(p_new_item_value, FILL_CHAR, start_pos + num + 1);
            rw_mem_locker_.unlock();

			if(NULL == p_new_item_value)
				return -1;
			if(p_item_value != NULL && str_len != 0){
				memcpy(p_new_item_value, p_item_value, str_len);
			}else{
				str_len = 0;
			}

			bool flag = false;	
			for(int i = num -1; i >= 0; i--){
				if(flag)
					p_new_item_value[start_pos + i] = FILL_CHAR;
				else
					p_new_item_value[start_pos + i] = new_score % 10 + '0';
				new_score = new_score / 10;
				if(new_score == 0)
					flag = true;
			}
			for(int i = (int)str_len; i < (int)start_pos; i ++){
				p_new_item_value[i] = FILL_CHAR;
			}
			p_new_item_value[start_pos + num] = '\0';
			return true;
		}else
			return false;
		
	}

	int specific_add(MAIN_KEY_TYPE key, ITEM_KEY_TYPE code, ADD_VALUE_TYPE score ,
			bool lock_flag = true)	{
		START_POS_TYPE start_pos = 0;
		NUM_TYPE num = 0;
		get_start_num(start_pos, num, code);
	
		PthreadRWLocker& rw_locker = get_hash_locker(key);
		if(lock_flag)
			rw_locker.rdlock();
		
		KeyHashMapIterator it = key_hash_map_.find(key);		
		if (it != key_hash_map_.end()){
			char* p_item_value = (*it).second;
			if(lock_flag)
				rw_locker.unlock();

			if(NULL == p_item_value )
				return -1;
		
			size_t item_len = strlen(p_item_value);	
			bool b_result = false;
			if(item_len > start_pos){
				//printf("%d:%d:%d\n", start_pos, num, item_len);
				//找到已存在的key, item_key的情况
				if(start_pos + num > item_len)
					return -1;
				else{
					if(lock_flag)
						rw_locker.wrlock();
					b_result = incr_score(start_pos, num, item_len, score, p_item_value);
					if(lock_flag)
						rw_locker.unlock();
					}
					if( ! b_result ){
						return -1;
					}
					return 1;
			}else{
				char* p_new_item_value = NULL;
				if( score < 0 ){
					score = 0;
				}
				b_result = add_score(start_pos, num, item_len, score, p_item_value, p_new_item_value);
				if(!b_result || NULL == p_new_item_value)
					return -2;
				if(lock_flag)
					rw_locker.wrlock();
				
				(*it).second = p_new_item_value;
				if(lock_flag)
					rw_locker.unlock();

				return 1;
				//找到了key,但是必须添加新的item_key的情况
			}
		}else{
			//找不到key,添加新的key以及item_key
			if(lock_flag)
				rw_locker.unlock();
			if( score < 0 ){
				score = 0;
			}
			char* p_new_item_value = NULL;
            bool b_result = add_score(start_pos, num, 0, score, NULL, p_new_item_value);
            if(!b_result || NULL == p_new_item_value)
                return -2;

			if(lock_flag)
				rw_locker.wrlock();
			std::pair< KeyHashMapIterator, bool > pr = 
				key_hash_map_.insert(std::make_pair(key, p_new_item_value));	
			if(lock_flag)
				rw_locker.unlock();

			if(pr.second)
				return 1;
			else
				return 0;
		}
			
		return 1;
	}

	/*基础增加，修改函数*/
	int generic_add(MAIN_KEY_TYPE key, ITEM_KEY_TYPE code, ITEM_VALUE_TYPE score, 
		int n_type, bool lock_flag = true){

		START_POS_TYPE start_pos = 0;
		NUM_TYPE num = 0;
		get_start_num(start_pos, num, code);
	
		PthreadRWLocker& rw_locker = get_hash_locker(key);
		if(lock_flag)
			rw_locker.rdlock();
		
		KeyHashMapIterator it = key_hash_map_.find(key);		
		if (it != key_hash_map_.end()){
			char* p_item_value = (*it).second;
			if(lock_flag)
				rw_locker.unlock();

			if(NULL == p_item_value )
				return -1;
		
			size_t item_len = strlen(p_item_value);	
			bool b_result = false;
			if(item_len > start_pos){
				//printf("%d:%d:%d\n", start_pos, num, item_len);
				//找到已存在的key, item_key的情况
				if(start_pos + num > item_len)
					return -1;
				else{
					if(n_type == 0 || n_type == 1){
						if(lock_flag)
							rw_locker.wrlock();
						b_result = update_score(start_pos, num, item_len, score, p_item_value); 
						if(lock_flag)
							rw_locker.unlock();
						if(!b_result)
							return -2;
					}else if(n_type == 2){
						/*ITEM_VALUE_TYPE old_score = 0;
						if(lock_flag)
							rw_locker.wrlock();
						b_result = get_score(old_score, start_pos, num, item_len, p_item_value);				
						if(!b_result){
						}
						b_result = update_score(start_pos, num, item_len, old_score + score, p_item_value);
						if(!b_result){
							if(lock_flag)
								rw_locker.unlock();
							return -2;
						}
						if(lock_flag)
							rw_locker.unlock();*/
						if(lock_flag)
							rw_locker.wrlock();
						b_result = incr_score(start_pos, num, item_len, score, p_item_value);
						if(lock_flag)
							rw_locker.unlock();
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
					rw_locker.wrlock();
				
				(*it).second = p_new_item_value;
				if(lock_flag)
					rw_locker.unlock();

				return 1;
				//找到了key,但是必须添加新的item_key的情况
			}
			
		}else{
			//找不到key,添加新的key以及item_key
			if(lock_flag)
				rw_locker.unlock();

			char* p_new_item_value = NULL;
            bool b_result = add_score(start_pos, num, 0, score, NULL, p_new_item_value);
            if(!b_result || NULL == p_new_item_value)
                return -2;

			if(lock_flag)
				rw_locker.wrlock();
			std::pair< KeyHashMapIterator, bool > pr = 
				key_hash_map_.insert(std::make_pair(key, p_new_item_value));	
			if(lock_flag)
				rw_locker.unlock();

			if(pr.second)
				return 1;
			else
				return 0;
			
		}
		return 1;
	}

	/*判断 key是否存在*/	
	bool contains(MAIN_KEY_TYPE key){
	
		PthreadRWLocker& rw_locker = get_hash_locker(key);
		rw_locker.rdlock();
		KeyHashMapIterator it = key_hash_map_.find(key);		
		if (it != key_hash_map_.end()){
			rw_locker.unlock();
			return true;
		}else{
			rw_locker.unlock();
			return false;
		}
	}

	/*判断 key, item_key是否存在*/
	bool exists(MAIN_KEY_TYPE key, ITEM_KEY_TYPE code){

		PthreadRWLocker& rw_locker = get_hash_locker(key);
		rw_locker.rdlock();
		KeyHashMapIterator it = key_hash_map_.find(key);		
		if (it != key_hash_map_.end()){
			char* p_item_value = (*it).second;
			rw_locker.unlock();
			if(NULL == p_item_value)
				return false;

			START_POS_TYPE start_pos = 0;
			NUM_TYPE num = 0;
			get_start_num(start_pos, num, code);

            size_t item_len = strlen(p_item_value);

			ITEM_VALUE_TYPE old_score = 0;
			return get_score(old_score, start_pos, num, item_len, p_item_value);
		
		}else{
			rw_locker.unlock();
			return false;
		}
	}

	/*查找key, item_key，返回数据内容*/
	bool get(MAIN_KEY_TYPE key, ITEM_KEY_TYPE code, ITEM_VALUE_TYPE& return_value){
		
		PthreadRWLocker& rw_locker = get_hash_locker(key);
		rw_locker.rdlock();
		KeyHashMapIterator it = key_hash_map_.find(key);		
		if (it != key_hash_map_.end()){
			char* p_item_value = (*it).second;
			rw_locker.unlock();
		
			START_POS_TYPE start_pos = 0;
			NUM_TYPE num = 0;
			get_start_num(start_pos, num, code);

            size_t item_len = strlen(p_item_value);

            return get_score(return_value, start_pos, num, item_len, p_item_value);	
		}else{
			rw_locker.unlock();
			return false;
		}
	}

	/*查找key, item_keys，返回数据内容*/
	bool gets(MAIN_KEY_TYPE key, const ItemVec& vec_code, 
		CodeItemMap& map_return_value){
		
		PthreadRWLocker& rw_locker = get_hash_locker(key);
		rw_locker.rdlock();
		KeyHashMapIterator it = key_hash_map_.find(key);		
		if (it != key_hash_map_.end()){
			char* p_item_value = (*it).second;
			rw_locker.unlock();
	
            size_t item_len = strlen(p_item_value);

			bool b_flag = false;
			for(ItemVec::const_iterator it = vec_code.begin(); it != vec_code.end(); it++){
				ITEM_KEY_TYPE code = (*it);
				START_POS_TYPE start_pos = 0;
				NUM_TYPE num = 0;
				get_start_num(start_pos, num, code);

				ITEM_VALUE_TYPE return_value = -1;
				b_flag = get_score(return_value, start_pos, num, item_len, p_item_value);

				if(b_flag){
					map_return_value.insert(std::make_pair(code, return_value));
				}
			}
            return true;
		}else{
			rw_locker.unlock();
			return false;
		}
	}
	
	/*查找keys, item_keys，返回数据内容*/
	bool mgets(const KeyVec& vec_key, const ItemVec& vec_code, 
		KeyCodeItemMap& map_return_value){
	
		for(KeyVec::const_iterator it = vec_key.begin(); it != vec_key.end(); it++){
			MAIN_KEY_TYPE key = (*it);
			CodeItemMap map_code_map;
			bool b_result = gets(key, vec_code, map_code_map);
			if (b_result){
				map_return_value.insert(std::make_pair(key, map_code_map));
			}
		}
		return true;	
		
	}

	/*查找keys, item_keys，返回数据内容*/
	bool mgets_i(const KeyItemVec& key_item_vec, KeyCodeItemMap& map_return_value){
		for(KeyItemVec::const_iterator it = key_item_vec.begin(); 
				it != key_item_vec.end(); it++){
			MAIN_KEY_TYPE key = (*it).key_;
			const ItemVec& item_vec = (*it).item_vec_;
			
			CodeItemMap map_code_map;
			bool b_result = gets(key, item_vec, map_code_map);
			if (b_result){
				map_return_value.insert(std::make_pair(key, map_code_map));
			}
		}	
		return true;	
	}

	/*删除某个key*/
	bool rem(MAIN_KEY_TYPE key){
		bool b_result = true;
		
		PthreadRWLocker& rw_locker = get_hash_locker(key);
		rw_locker.wrlock();
		key_hash_map_.erase(key);
        rw_locker.unlock();

		return b_result;
	}

	/*序列化文件，将内存数据导出到外存中
	文件格式：key --> value[strings]
	其中的间隔为'\t'
	*/
	bool seralize_old(const char* p_seralize_file_name, bool lock_flag = false){
		if (NULL == p_seralize_file_name)
			return false;

		std::ofstream out_file(p_seralize_file_name);
		if (!out_file.is_open())		
			return false;	

		//PthreadRWLocker& rw_locker = get_hash_locker(0); //我们一般不需要这个锁
		//if(lock_flag){
		//	rw_locker.rdlock();
		//}
	
		std::string str_sep = SPE_ITEM;
		for(KeyHashMapIterator it = key_hash_map_.begin();
			it != key_hash_map_.end(); it++){

			MAIN_KEY_TYPE key = (*it).first;
			out_file << key << SPE_ITEM << std::string((*it).second) << std::endl;
		}	

		//if(lock_flag){
		//	rw_locker.unlock();
		//}

		out_file.close();
		return true;
	}
	
	/*序列化文件，将内存数据导出到外存中
	文件格式：key --> value[strings]
	其中的间隔为'\t'
	*/
	bool seralize(const char* p_seralize_file_name, bool lock_flag = false){
		if (NULL == p_seralize_file_name)
			return false;

		FILE * fp = NULL;

		fp = fopen(p_seralize_file_name, "wb");
		if(NULL == fp){
			return false;
		}

		for(KeyHashMapIterator it = key_hash_map_.begin();
			it != key_hash_map_.end(); it++){
			MAIN_KEY_TYPE key = (*it).first;
			const char* value = (*it).second;
			uint32_t value_len = strlen(value);

			fwrite(&key, sizeof(key), 1, fp);
			fwrite(&value_len, sizeof(value_len), 1, fp);	
			fwrite(value, 1, value_len, fp);
			
		}
		
		fclose(fp);
		fp = NULL;
		return true;
	}

	/*反序列化，将外存文件载入内存中*/
	bool unseralize_old(const char* p_seralize_file_name){
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
	
	/*反序列化，将外存文件载入内存中*/
	bool unseralize(const char* p_seralize_file_name){
		if (NULL == p_seralize_file_name)
			return false;

		FILE* fp = NULL;

		fp = fopen(p_seralize_file_name, "rb");
		if( NULL == fp )
			return false;
		while(true){
			MAIN_KEY_TYPE key = 0;
			fread(&key, sizeof(MAIN_KEY_TYPE), 1, fp);
			if(feof(fp)){
				break;
			}
			uint32_t value_len = 0;
			fread(&value_len, sizeof(uint32_t), 1, fp);
			if(feof(fp)){
				break;
			}
			char p_temp[value_len + 1];
			fread(p_temp, 1, value_len, fp);
			if(feof(fp)){
				break;
			}
			p_temp[value_len] = '\0';
			_add(key, p_temp, false);
		}

		fclose(fp);
		fp = NULL;	
	
		return true;
	}			


private:
	PthreadRWLocker& get_hash_locker(MAIN_KEY_TYPE key){
		//return rw_locker_array_[key % HASH_LOCKER_NUM];
		return rw_locker_;
	}

private:
	leveldb::Arena arena_; //内存分配器
	PthreadRWLocker rw_locker_; // key_hash_map_ 读写锁
	PthreadRWLocker rw_mem_locker_; // arena_ 内存分配读写锁
	KeyHashMap key_hash_map_; //数据存储hash_map：

	//PthreadRWLocker rw_locker_array_[HASH_LOCKER_NUM];
};

#endif
