#ifndef _WORK_INTERFACE_FACTORY_HEADER_
#define _WORK_INTERFACE_FACTORY_HEADER_

#include "work_interface.h"
#include "db_company.h"
#include "ini_file.h"
#include "woo/log.h"
#include <map>
#include <set>
#include <vector>
#include <fstream>
#include <iostream>
#include "json.h"
#include <time.h>

class WorkInterfaceFactory{
	public:
		WorkInterfaceFactory(DbCompany*& p_db_company):p_db_company_(p_db_company){
		}
		~WorkInterfaceFactory(){

			release_work_interface_so();
		}

	private:
		DbCompany*& p_db_company_;
		std::map<std::string, WorkInfo*> map_work_interface_so_;

		int release_work_interface_so(){

			for (std::map<std::string, WorkInfo*>::iterator it = map_work_interface_so_.begin();
					it != map_work_interface_so_.end(); it ++){
				if ((*it).second != NULL){
					WorkInterface* p_work_interface = (*it).second->work_interface_;
					if(p_work_interface != NULL){
						delete p_work_interface;
						p_work_interface = NULL;
					}

					if((*it).second->handle_ != NULL){
						dlclose((*it).second->handle_);
					}
					delete (*it).second;
				}
			}
			map_work_interface_so_.clear();

			return 1;

		}
	public:
		int initialize(const char* p_work_config){

			if(NULL == p_work_config){
				LOG_ERROR("work config is NULL!");

				return -1;
			}

			char work_split_char = read_profile_int("WORK_INFO", "WORK_SLIT_CHAR",
					44, p_work_config);

			char work_names[PATH_MAX];
			if( !read_profile_string("WORK_INFO", "WORK_NAMES", 
					work_names, PATH_MAX, "", p_work_config))
			{
				LOG_ERROR("load config work_names!");
				return -1;
			}
			
			char conf_folder[PATH_MAX];
			if( !read_profile_string("WORK_INFO", "CONF_FOLDER", 
					conf_folder, PATH_MAX, "", p_work_config))
			{
				LOG_ERROR("load config conf_folder!");
				return -1;
			}
			
			char lib_folder[PATH_MAX];
			if( !read_profile_string("WORK_INFO", "LIB_FOLDER", 
					lib_folder, PATH_MAX, "", p_work_config))
			{
				LOG_ERROR("load config lib_folder!");
				return -1;
			}
			
			std::vector<std::string> vec_work_names;
			split_string(vec_work_names, work_names, work_split_char);

			int interface_id = 0;
			for(std::vector<std::string>::iterator it = vec_work_names.begin();
					it != vec_work_names.end(); it ++){

				char api[PATH_MAX];
				if( !read_profile_string((*it).c_str(), "API", 
					api, PATH_MAX, "", p_work_config))
				{
					LOG_ERROR("load config api!");
					continue;
				}

				char so_name[PATH_MAX];
				if( !read_profile_string((*it).c_str(), "SO_NAME", 
					so_name, PATH_MAX, "", p_work_config))
				{
					LOG_ERROR("load config so name!");
					continue;
				}
				
				char alg_ini[PATH_MAX];
				if( !read_profile_string((*it).c_str(), "ALG_INI", 
					alg_ini, PATH_MAX, "", p_work_config))
				{
					LOG_ERROR("load config alg ini!");
					continue;
				}
				
				void *handle = NULL;
				char so_path_name[PATH_MAX];
				sprintf(so_path_name, "%s/%s.so", lib_folder, so_name);
				handle = dlopen (so_path_name, RTLD_NOW);

				if(!handle){
					LOG_ERROR("%s\n", dlerror());
					continue;
				}

				dlerror();
				char func_name[PATH_MAX];
				sprintf(func_name, "create%s", so_name);
				CreateFactory_W create_factory = (CreateFactory_W)dlsym(handle, func_name); 

				char *error = NULL;
				if ((error = dlerror()) != NULL){
					LOG_ERROR("%s\n", error);

					dlclose(handle);
					continue;
				}
				
				WorkInterface* work_inter = create_factory(p_db_company_, interface_id);
				if(NULL == work_inter){
					LOG_ERROR("new work is fail!");

					dlclose(handle);
					continue;
				}

				interface_id ++;
				
				char alg_config[PATH_MAX];
				sprintf(alg_config, "%s/%s", conf_folder, alg_ini);
				work_inter->initialize(alg_config);

				WorkInfo* work_info = new WorkInfo();
				if (NULL == work_info){
					LOG_ERROR("new algorithm info failed!");

					dlclose(handle);
					continue;
				}

				work_info->handle_ = handle;
				work_info->work_interface_ = work_inter;

				map_work_interface_so_.insert(std::map<std::string, WorkInfo*>::value_type(std::string(api), work_info));
			}

			return 1;
		}

	public:
		WorkInterface* get_interface(const char* api_command){
			
			std::map<std::string, WorkInfo*>::iterator it = map_work_interface_so_.find(api_command);
			if(it != map_work_interface_so_.end()){
				if ((*it).second != NULL){
					return (*it).second->work_interface_;
				}else
					return NULL;
			}
			return NULL;
	}
		
};
#endif
