#ifndef _GLOBAL_DB_INTERFACE_HEADER_
#define _GLOBAL_DB_INTERFACE_HEADER_
#include <stdlib.h> 
#include <vector>
#include <string>
#include "utility.h"
#include "base_define.h"
#ifdef __GNUC__
#include <ext/hash_map>
#else
#include <hash_map>
#endif

using namespace std;

typedef enum{
    GLOBAL_NULL_DB_TYPE,	// 占位用
    GLOBAL_SET_DB_TYPE,
    GLOBAL_KV_DB_TYPE,
	GLOBAL_MAP_DB_TYPE,
} GLOBAL_DB_TYPE;

struct GlobalDbInfoStruct{
		GlobalDbInfoStruct(){}
		GlobalDbInfoStruct(const GlobalDbInfoStruct& global_db_info_struct){
			this->db_type_ = global_db_info_struct.db_type_;
			strncpy(this->db_name_,  global_db_info_struct.db_name_, WORD_LEN);
			strncpy(this->db_file_name_, global_db_info_struct.db_file_name_, WORD_LEN);
			this->kv_split_char_ = global_db_info_struct.kv_split_char_;
		}

		GLOBAL_DB_TYPE db_type_;
		char db_name_[WORD_LEN];
		char db_file_name_[WORD_LEN];
		char kv_split_char_;

	public:
		int load_config(const char* file_name, const char* db_name){
			if(NULL == file_name || NULL == db_name){
				return -1;
			}

			strcpy(db_name_, db_name);
			
			if( !read_profile_string(db_name, "DB_FILE_NAME", 
					db_file_name_, WORD_LEN, "", file_name))
    	    {
				LOG_ERROR("load config svr_ip!");
				return -1;
        	}

			db_type_ = (GLOBAL_DB_TYPE)read_profile_int(db_name, "DB_TYPE",
					0, file_name);
			
			kv_split_char_ = (char)(GLOBAL_DB_TYPE)read_profile_int(db_name, "KV_SPLIT_CHAR",
					0, file_name);

			return 1;
		} 
};

class GlobalDbInterface{
	public:
		GlobalDbInterface(const GlobalDbInfoStruct& global_db_info_struct):
			global_db_info_struct_(global_db_info_struct){
		}
		virtual ~GlobalDbInterface(){
		}
	//protected:
	public:
		GlobalDbInfoStruct global_db_info_struct_;
	public:
		virtual bool is_exist(uint64_t id) = 0;
		virtual int load_db_config() = 0;
};

#endif
