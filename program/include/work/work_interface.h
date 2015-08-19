#ifndef _WORK_INTERFACE_HEADER_
#define _WORK_INTERFACE_HEADER_

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
#include <algorithm_interface.h>
#include <algorithm_func.h>

class WorkInterface;
typedef WorkInterface * (*CreateFactory_W) (DbCompany*& p_db_company, int interface_id);

#define DYN_WORK(workName) \
	extern "C" \
	WorkInterface * create##workName(DbCompany*& p_db_company, int interface_id) \
	{ return new (std::nothrow) workName(p_db_company, interface_id); }

typedef struct{
	void* handle_;
	WorkInterface* work_interface_;
} WorkInfo;

int return_fail(int fail_code, const char* fail_string, char* &p_out_string, int& n_out_len){
	n_out_len = sprintf(p_out_string, "{\"return_code\":\"%d\", \"error_msg\":\"%s\"}", 
			fail_code, fail_string);

	return 1;
}

class WorkInterface{
	public:
		WorkInterface(DbCompany*& p_db_company, int interface_id):
			p_db_company_(p_db_company), interface_id_(interface_id){

		}
		virtual ~WorkInterface(){
			release_algorithm(vec_pair_map_alg_);
		}

	protected:
		DbCompany*& p_db_company_;	
		int interface_id_;
		char p_alg_config_[PATH_MAX];

		VEC_PAIR_MAP_ALG vec_pair_map_alg_;
	protected:
		int algorithm_core(int64_t req_id, const AccessStr& access_str, VEC_CAND& vec_cand){
			return run_algorithm(req_id, access_str, vec_pair_map_alg_, vec_cand);
		}

		/*
		 * 添加重载函数，用于支撑新的业务
		 * 输入：alg_name：配置文件中的SO_NAMES字段对应的每一个section
		 * 输出：output_vec
		 * 修改时间：2015/01/04
		 * 修改人：程天翔
		 */
		int algorithm_core(int64_t req_id, const AccessInfo* access_info, const string alg_name, 
				const VEC_CAND& input_vec, VEC_CAND& output_vec){
			return run_algorithm(req_id, access_info, alg_name, vec_pair_map_alg_, input_vec, output_vec);
		}

	public:
		int initialize(const char* p_alg_config){
			if(NULL == p_alg_config){
				LOG_ERROR("p_alg_config is NULL!");
				return -1;
			}

			strcpy(p_alg_config_, p_alg_config);

			return initialize_algorithm(vec_pair_map_alg_, p_alg_config,
					p_db_company_, interface_id_);
		}

		/*int return_fail(int fail_code, const char* fail_string, char* &p_out_string, int& n_out_len){
			n_out_len = sprintf(p_out_string, "{\"return_code\":\"%d\", \"error_msg\":\"%s\"}", 
					fail_code, fail_string);

			return 1;
		}*/

	protected:

	public:
		virtual int work_core(json_object *req_json, char* &p_out_string, 
				int& n_out_len, int64_t req_id) = 0;
	public:
		int get_interface_id(){
			return interface_id_;
		}
};
#endif
