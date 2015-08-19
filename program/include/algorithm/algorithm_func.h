#ifndef _ALGORITHM_FUNC_HEADER_
#define _ALGORITHM_FUNC_HEADER_

#include "algorithm_interface.h"
#include <dlfcn.h>

typedef std::map<int16_t, AlgorithmInfo*> MAP_ALG;
typedef std::pair<std::string, MAP_ALG> PAIR_MAP_ALG;
typedef std::vector<PAIR_MAP_ALG> VEC_PAIR_MAP_ALG;

const int16_t WHITELIST_FLAG = -2; 	//白名单
const int16_t DEFAULT_FLAG = -1; 	//默认
const int16_t LASTCODE_FLAG = 0; 	//尾号

int create_so_handle(MAP_ALG& map_alg, const char* so_folder, 
		const MAP_VEC_STR& map_user_plan, DbCompany*& p_db_company, 
		int interface_id){

	for(MAP_VEC_STR::const_iterator it = map_user_plan.begin();
			it != map_user_plan.end(); it++){
		int16_t id_mod = (*it).first;

		const VEC_STR& vec_str = (*it).second;
		if(vec_str.size() > 1){
			std::string so_name = vec_str[0];

			void *handle = NULL;
			char so_path_name[PATH_MAX];
			sprintf(so_path_name, "%s/%s.so", so_folder, so_name.c_str());
			handle = dlopen (so_path_name, RTLD_NOW);

			if(!handle){
				LOG_ERROR("failed to open so file! %s", so_path_name);
				continue;
			}else{
			}

			char func_name[PATH_MAX];
			sprintf(func_name, "create%s", so_name.c_str());
			CreateFactory_T create_factory = (CreateFactory_T)dlsym(handle, func_name); 

			char *error = NULL;
			if ((error = dlerror()) != NULL){
				LOG_ERROR("%s\n", dlerror());
				continue;
			}
			AlgorithmInterface* alg_inter = create_factory(p_db_company, interface_id);
			if(NULL == alg_inter){
				LOG_ERROR("new algorithm is fail!");
				continue;
			}

			AlgorithmInfo* algorithm_info = new AlgorithmInfo();
			if (NULL == algorithm_info){
				LOG_ERROR("new algorithm info failed!");
				continue;
			}

			algorithm_info->id_mod_ = id_mod;
			algorithm_info->handle_ = handle;
			strcpy(algorithm_info->global_db_name_, vec_str[1].c_str()); 
			algorithm_info->algorithm_interface_ = alg_inter;
			
			map_alg.insert(MAP_ALG::value_type(id_mod, algorithm_info));

		}
	}
	return 1;
}

/*
 * [ALG_INFO]
 * SO_SLIT_CHAR=44
 * SO_NAMES=INTER
 * SO_FOLDER=../LIB/
 *
 * [INTER]
 * UID_PLAN=0,inter_0,|1,inter_1,|-1,inter_0,|-2,inter_1,INTERSECTION_USER_DB
 * */

int initialize_algorithm(VEC_PAIR_MAP_ALG& vec_pair_map_alg, const char* p_alg_config, 
		DbCompany*& p_db_company, int interface_id){
		
	if(NULL == p_alg_config){
		LOG_ERROR("p_alg_config is NULL!");
		return -1;
	}

	if(NULL == p_db_company){
		LOG_ERROR("db company is NULL!");
		return -1;
	}

	char so_split_char = read_profile_int("ALG_INFO", "SO_SLIT_CHAR",
			44, p_alg_config);

	char so_names[PATH_MAX];
	if( !read_profile_string("ALG_INFO", "SO_NAMES", 
			so_names, PATH_MAX, "", p_alg_config))
	{
		return -1;
	}
	
	char so_folder[PATH_MAX];
	if( !read_profile_string("ALG_INFO", "SO_FOLDER", 
			so_folder, PATH_MAX, "", p_alg_config))
	{
		return -1;
	}

	std::vector<std::string> vec_so_names;
	split_string(vec_so_names, so_names, so_split_char);

	for(std::vector<std::string>::iterator it = vec_so_names.begin();
			it != vec_so_names.end(); it ++){

		MAP_VEC_STR map_user_plan;	
		char uid_plan[PATH_MAX];
		if( !read_profile_string((*it).c_str(), "UID_PLAN", 
			uid_plan, PATH_MAX, "", p_alg_config))
		{
			LOG_ERROR("load config uid plan!");
			continue;
		}

		split_map_vec_string(map_user_plan, uid_plan, '|', ',');

		MAP_ALG map_alg;
		create_so_handle(map_alg, so_folder, map_user_plan, 
				p_db_company, interface_id);
		vec_pair_map_alg.push_back(PAIR_MAP_ALG((*it), map_alg));
	}
	return 1;
};

int release_algorithm(VEC_PAIR_MAP_ALG& vec_pair_map_alg){
	
	for(VEC_PAIR_MAP_ALG::iterator it = vec_pair_map_alg.begin();
				it != vec_pair_map_alg.end(); it ++){

		const MAP_ALG& map_alg = (*it).second;
		for(MAP_ALG::const_iterator o_it = map_alg.begin();
				o_it != map_alg.end(); o_it++){
			AlgorithmInfo* alg_info = (*o_it).second;
			if(NULL == alg_info)
				continue;
			
			if(alg_info->algorithm_interface_ != NULL){
				delete alg_info->algorithm_interface_;
				alg_info->algorithm_interface_ = NULL;
			}

			if(alg_info->handle_ != NULL)
				dlclose(alg_info->handle_);

			delete alg_info;
			alg_info = NULL;
		}

		(*it).second.clear();
	}
	vec_pair_map_alg.clear();
	return 1;
};

/*
 * [ALG_INFO]
 * SO_SLIT_CHAR=44
 * SO_NAMES=INTER
 * SO_FOLDER=../LIB/
 *
 * [INTER]
 * UID_PLAN=0,inter_0,|1,inter_1,|-1,inter_0,|-2,inter_1,INTERSECTION_USER_DB
 * ;-2表示白名单,同时后面会有白名单列表,-1表示默认,其他表示用户尾号控制
 * */

int whitelist_is_in(AlgorithmInfo* alg_info, uint64_t access_uid){
	int result = 1;

	if(NULL == alg_info){
		return -1;
	}
	
	AlgorithmInterface* alg_inter = alg_info->algorithm_interface_;
	if(NULL == alg_inter){
		LOG_ERROR("alg interface is NULL!");
		return -1;
	}

	char* global_db_name = alg_info->global_db_name_;
	if(NULL == global_db_name){
		LOG_ERROR("global db name is null!");
		return -1;
	}
	if(alg_inter->p_db_company_ != NULL)
	{
		GlobalDbCompany* p_global_db_company = 
			alg_inter->p_db_company_->get_global_db_company();
		
		if(NULL == p_global_db_company){
			LOG_ERROR("global db company is NULL!");
			return -1;
		}

		GlobalDbInterface* global_g_db_interface = 
			p_global_db_company->get_global_db_interface(global_db_name);
		if(NULL == global_g_db_interface){
			LOG_ERROR("%s is failed!", global_db_name);
			return -1;
		}

		if(!global_g_db_interface->is_exist(access_uid)){
			LOG_INFO("%"PRIu64" is not in whitelist!", access_uid);
			result = 0;
		}
		else
			result = 1;
	}else{
		LOG_ERROR("db company is NULL!");
		return -1;
	}
	return result;
}

/*
 * 运行算法
 * */
int run_algorithm(int64_t req_id, 
		const AccessStr& access_str,
		const VEC_PAIR_MAP_ALG& vec_pair_map_alg,
		VEC_CAND& vec_cand){

	uint64_t access_uid = access_str.uid_;
	uint32_t last_code = (access_uid / 10) % 10;	// 倒数第二位尾号
	int result = 1;

	for(VEC_PAIR_MAP_ALG::const_iterator it = vec_pair_map_alg.begin();
			it != vec_pair_map_alg.end(); it ++){
		const MAP_ALG& map_alg = (*it).second;

		// 处理白名单用户
		MAP_ALG::const_iterator o_it = map_alg.find(WHITELIST_FLAG);
		if(o_it != map_alg.end()){
			LOG_INFO("whitelist is passed!");

			AlgorithmInfo* alg_info = (*o_it).second;
			if(NULL == alg_info){
				LOG_ERROR("alg info is NULL!");
			}else{
				if(whitelist_is_in(alg_info, access_uid) > 0){
					
					AlgorithmInterface* alg_inter = alg_info->algorithm_interface_;
					if(NULL == alg_inter){
						LOG_ERROR("alg interface is NULL!");
					}else{
						result = alg_inter->algorithm_core(req_id, access_str, vec_cand);
						if(result <= 0){
							LOG_ERROR("algorithm core is failed!");
						}
					}
					continue;
				}
			}
		}

		// 处理满足倒数第二位尾号条件的用户
		o_it = map_alg.find(last_code);
		if(o_it != map_alg.end()){
			LOG_INFO("lastcode is passed!");

			AlgorithmInfo* alg_info = (*o_it).second;
			if(NULL == alg_info){
				LOG_ERROR("alg info is NULL!");
			}else{
				AlgorithmInterface* alg_inter = alg_info->algorithm_interface_;
				if(NULL == alg_inter){
					LOG_ERROR("alg interface is NULL!");
				}else{
					result = alg_inter->algorithm_core(req_id, access_str, vec_cand);
					if(result <= 0){
						LOG_ERROR("algorithm core is failed!");
					}
				}
				continue;
			}
		}
		
		// 处理默认情况
		o_it = map_alg.find(DEFAULT_FLAG);
		if(o_it != map_alg.end()){
			LOG_INFO("default is passed!");

			AlgorithmInfo* alg_info = (*o_it).second;
			if(NULL == alg_info){
				LOG_ERROR("alg info is NULL!");
			}else{
				AlgorithmInterface* alg_inter = alg_info->algorithm_interface_;
				if(NULL == alg_inter){
					LOG_ERROR("alg interface is NULL!");
				}else{
					result = alg_inter->algorithm_core(req_id, access_str, vec_cand);
					if(result <= 0){
						LOG_ERROR("algorithm core is failed!");
					}
				}
				continue;
			}
		}
	}
	return 1;
};

/*
 * 重载函数，用于运行制定的算法
 * 输入：
 * alg_name：配置文件中的SO_NAMES字段对应的每一个section
 * input_cand：输入参数，不允许修改
 * output_cand：输出参数，作为返回结果
*/
int run_algorithm(int64_t req_id, const AccessInfo* access_info, const string& alg_name, 
		const VEC_PAIR_MAP_ALG& vec_pair_map_alg, const VEC_CAND& input_cand,
		VEC_CAND& output_cand){

	uint64_t access_uid = access_info->uid_;
	uint32_t last_code = (access_uid / 10) % 10;
	int result = 1;

	// 根据alg_name查找对应的算法文件
	VEC_PAIR_MAP_ALG::const_iterator it;
	for( it = vec_pair_map_alg.begin(); it != vec_pair_map_alg.end(); it++){
		if(it->first == alg_name){
			// 获取该section下对应的算法调用策略
			const MAP_ALG& map_alg = it->second;
			
			// 处理白名单
			MAP_ALG::const_iterator o_it = map_alg.find(WHITELIST_FLAG);
			if(o_it != map_alg.end()){
				LOG_INFO("whitelist is passed!");
				AlgorithmInfo* alg_info = (*o_it).second;
				if(NULL == alg_info){
					LOG_ERROR("alg info is NULL!");
				}else{
					if(whitelist_is_in(alg_info, access_uid) > 0){
						
						AlgorithmInterface* alg_inter = alg_info->algorithm_interface_;
						if(NULL == alg_inter){
							LOG_ERROR("alg interface is NULL!");
						}else{
							result = alg_inter->algorithm_core_new(req_id, access_info, input_cand, output_cand);
							if(result <= 0){
								LOG_ERROR("algorithm core is failed!");
							}
						}
						break;	// 命中白名单，执行完退出
					}
				}
			}

			// 处理尾号
			o_it = map_alg.find(last_code);
			if(o_it != map_alg.end()){
				LOG_INFO("lastcode is passed!");

				AlgorithmInfo* alg_info = (*o_it).second;
				if(NULL == alg_info){
					LOG_ERROR("alg info is NULL!");
				}else{
					AlgorithmInterface* alg_inter = alg_info->algorithm_interface_;
					if(NULL == alg_inter){
						LOG_ERROR("alg interface is NULL!");
					}else{
						result = alg_inter->algorithm_core_new(req_id, access_info, input_cand, output_cand);
						if(result <= 0){
							LOG_ERROR("algorithm core is failed!");
						}
					}
					break;	// 命中尾号，执行完退出
				}
			}
			
			// 处理默认情况
			o_it = map_alg.find(DEFAULT_FLAG);
			if(o_it != map_alg.end()){
				LOG_INFO("default is passed!");

				AlgorithmInfo* alg_info = (*o_it).second;
				if(NULL == alg_info){
					LOG_ERROR("alg info is NULL!");
				}else{
					AlgorithmInterface* alg_inter = alg_info->algorithm_interface_;
					if(NULL == alg_inter){
						LOG_ERROR("alg interface is NULL!");
					}else{
						result = alg_inter->algorithm_core_new(req_id, access_info, input_cand, output_cand);
						if(result <= 0){
							LOG_ERROR("algorithm core is failed!");
						}
					}
					break;	// 命中默认情况
				}
			}
			break;			// 处理完成之后，跳出循环，避免继续遍历
		}
	}

	// 若没有找到算法文件
	if(it == vec_pair_map_alg.end()){
		LOG_ERROR("alg_name %s not exist!", alg_name.c_str());
		return -1;
	}
	return 1;
};

#endif
