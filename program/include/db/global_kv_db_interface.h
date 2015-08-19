#ifndef _GLOBAL_KV_DB_INTERFACE_HEADER_
#define _GLOBAL_KV_DB_INTERFACE_HEADER_
#include <stdlib.h> 
#include <vector>
#include <string>
#include "utility.h"
#include "base_define.h"
#include "global_db_interface.h"
#ifdef __GNUC__
#include <ext/hash_map>
#else
#include <hash_map>
#endif

using namespace std;

/*
 * 本文件用于支持键值对类型，目前仅支持<uint64_t, uint32_t>类型
 * */

class GlobalKVDbInterface : public GlobalDbInterface{
	public:
		GlobalKVDbInterface(const GlobalDbInfoStruct& global_db_info_struct):
		GlobalDbInterface(global_db_info_struct){
		}
		~GlobalKVDbInterface(){
			hashmap_global_data_.clear();
		}
	private:
		 __gnu_cxx::hash_map<uint64_t, uint32_t> hashmap_global_data_;
	public:
		bool is_exist(uint64_t id){
			__gnu_cxx::hash_map<uint64_t, uint32_t>::iterator it = 
				hashmap_global_data_.find(id);
			if(it != hashmap_global_data_.end())
				return true;
			else
				return false;
		}

		bool get_value(uint64_t id, uint32_t& value){
			__gnu_cxx::hash_map<uint64_t, uint32_t>::iterator it = 
				hashmap_global_data_.find(id);
			if(it != hashmap_global_data_.end()){
				value = (*it).second;
				return true;
			}
			else
				return false;
		}

		int load_db_config(){
			char* db_file_name = global_db_info_struct_.db_file_name_;
			if(NULL == db_file_name)
				return -1;

			FILE *fd = NULL;  
			char buf[512];  
			fd = fopen(db_file_name, "r");  
			if(NULL == fd){
				LOG_ERROR("%s read is error!", db_file_name);
				return 0;         
			}  
			while(!feof(fd)){  
				fgets(buf, 512, fd); 

				uint64_t ids[4];
				uint32_t id_num = 0;
				split_string_ids(ids, id_num, (const char*)buf, global_db_info_struct_.kv_split_char_, 0);

				if(id_num < 2)
					continue;

				hashmap_global_data_.insert(__gnu_cxx::hash_map<uint64_t, uint32_t>::value_type(ids[0] , (uint32_t)ids[1]));
			}  
			fclose(fd);  

			return 1;
		}
};

#endif
