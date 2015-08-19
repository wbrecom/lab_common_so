#ifndef _GLOBAL_MAP_DB_INTERFACE_HEADER_
#define _GLOBAL_MAP_DB_INTERFACE_HEADER_
#include <stdlib.h> 
#include <vector>
#include <string>
#include "utility.h"
#include "base_define.h"
#include "global_db_interface.h"
#include "cmapdb.h"

using namespace std;

/*
 * 本文件用于支持自研数据库mapdb
 * */

class GlobalMapDbInterface : public GlobalDbInterface{
	public:
		GlobalMapDbInterface(const GlobalDbInfoStruct& global_db_info_struct):
		GlobalDbInterface(global_db_info_struct){
		}
		~GlobalMapDbInterface(){
		}
	private:
		MapDb map_db_;
	public:
		bool is_exist(uint64_t id){
			size_t length = 0;
			const char* result = NULL;
			int flag = map_db_.get(id, length, result);
			if(flag >= 1)
				return true;
			else
				return false;
		}

		bool get_value(uint64_t id, size_t& length, const char*& value){
			int flag = map_db_.get(id, length, value);
			if(flag >= 1)
				return true;
			else
				return false;

		}

		bool mget_value(uint64_t ids[], int ids_len, const char* value[], int& value_len){
			int flag = map_db_.mget(ids, ids_len, value, value_len);
			if(flag >= 1)
				return true;
			else
				return false;
		}

		int load_db_config(){
			char* db_file_name = global_db_info_struct_.db_file_name_;
			if(NULL == db_file_name)
				return -1;

			map_db_.initialize(db_file_name);
			return 1;
		}
};

#endif
