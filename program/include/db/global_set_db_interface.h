#ifndef _GLOBAL_SET_DB_INTERFACE_HEADER_
#define _GLOBAL_SET_DB_INTERFACE_HEADER_
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
 * 本文件用于支持集合，目前集合元素仅支持uint64_t类型
 */

class GlobalSetDbInterface : public GlobalDbInterface{
	public:
		GlobalSetDbInterface(const GlobalDbInfoStruct& global_db_info_struct):
		GlobalDbInterface(global_db_info_struct){
		}
		~GlobalSetDbInterface(){
			set_global_data_.clear();
		}
	private:
		 __gnu_cxx::hash_set<uint64_t> set_global_data_;
	public:
		bool is_exist(uint64_t id){
			__gnu_cxx::hash_set<uint64_t>::iterator it = set_global_data_.find(id);
			if(it != set_global_data_.end())
				return true;
			else
				return false;
		}

		int load_db_config(){
			char* db_file_name = global_db_info_struct_.db_file_name_;
			if(NULL == db_file_name)
				return -1;

			FILE *fd = NULL;  
			char buf[64];  
			fd = fopen(db_file_name, "r");  
			if(NULL == fd){
				LOG_ERROR("%s read is error!", db_file_name);
				return 0;         
			}  
			while(!feof(fd)){  
				fgets(buf, 64, fd); 

				uint64_t id = strtoull(buf, NULL, 10);
				set_global_data_.insert(id);
			}  
			fclose(fd);  

			return 1;
		}
};

#endif
