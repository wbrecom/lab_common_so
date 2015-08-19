#ifndef _GLOBAL_DB_COMPANY_HEADER_
#define _GLOBAL_DB_COMPANY_HEADER_
#include <stdlib.h> 
#include <vector>
#include <string>
#include <map>
#include "utility.h"
#include "base_define.h"
#include "global_db_interface_factory.h"
using namespace std;

class GlobalDbCompany{
	public:
		GlobalDbCompany(){
		}

		~GlobalDbCompany(){

			for(map<string, GlobalDbInterface*>::iterator it = map_global_db_interface_.begin();
					it != map_global_db_interface_.end(); it++){
				if((*it).second != NULL){
					delete (*it).second;
					(*it).second = NULL;
				}
			}
			map_global_db_interface_.clear();
		}
	private:
		map<string, GlobalDbInterface*> map_global_db_interface_;			

		int get_db_info(const char* file_name, std::vector<std::string>& vec_db_name){
			char split_char = (char)read_profile_int("GLOBAL_DB_INFO", "DBS_SPLIT_CHAR",
					44, file_name);
			char dbs_name[PATH_MAX];
			if( !read_profile_string("GLOBAL_DB_INFO", "DBS_NAME",
						 dbs_name, PATH_MAX, "", file_name))
			{
				LOG_ERROR("load dbs name error!");
				return -1;
			}

			split_string(vec_db_name, dbs_name, split_char);
			return 1;
		}

	public:
		GlobalDbInterface* get_global_db_interface(const char* db_name){
			map<string, GlobalDbInterface*>::iterator it = map_global_db_interface_.find(string(db_name));
			if(it != map_global_db_interface_.end()){
				return (*it).second;
			}
			else
				return NULL;
		}

		int update_global_db_interface(GlobalDbInterface* &p_new_db_interface,
				const char* db_name, const char* file_name){
		
			LOG_ERROR("%s;;;;;", db_name);
			GlobalDbInterfaceFactory global_db_interface_factory;

			map<string, GlobalDbInterface*>::iterator it = map_global_db_interface_.find(string(db_name));
			if(it != map_global_db_interface_.end()){
				if((*it).second != NULL){
					p_new_db_interface = global_db_interface_factory.
						get_global_db_interface((*it).second->global_db_info_struct_);

					if(NULL == p_new_db_interface){
						LOG_ERROR("update new global db is error!");
						return -1;
					}
					p_new_db_interface->load_db_config();

					std::swap((*it).second, p_new_db_interface);

					return 1;
				}
			}

			GlobalDbInfoStruct global_db_info_struct;
			if(global_db_info_struct.load_config(file_name, db_name) < 0){
				LOG_ERROR("update to load new global db is error!");
				return -1;
			}

			p_new_db_interface = global_db_interface_factory.
					get_global_db_interface(global_db_info_struct);
			p_new_db_interface->load_db_config();

			map_global_db_interface_.insert(map<string, GlobalDbInterface*>::value_type
					(string(db_name), p_new_db_interface));

			return 0;
		}

		int load_config(const char* file_name){

			std::vector<std::string> vec_db_name;
			int n_result = get_db_info(file_name, vec_db_name);

			if (n_result <= 0)
				return n_result;

			GlobalDbInterfaceFactory global_db_interface_factory;

			for(std::vector<std::string>::iterator pos = vec_db_name.begin();
					pos != vec_db_name.end(); pos ++){
				GlobalDbInfoStruct global_db_info_struct;

				if(global_db_info_struct.load_config(file_name, (*pos).c_str()) < 0)
					continue;

				GlobalDbInterface* p_db_interface = global_db_interface_factory.
					get_global_db_interface(global_db_info_struct);

				if(NULL == p_db_interface){
					LOG_ERROR("new db interface is error!");
					continue;
				}

				map_global_db_interface_.insert(map<string, GlobalDbInterface*>::value_type
						(string((*pos)), p_db_interface));
				p_db_interface->load_db_config();
			}
			return 1;
		}
};

#endif
