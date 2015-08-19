/*
 * 本文件用于演示lab_common_so框架的使用，包含如下内容：
 * 1、如何获取本地数据
 * 2、如何获取远程数据
 * 3、如何调用算法库
 * 4、完整的业务处理流程
 * */

#ifndef _SIMPLE_FRAME_WORK_INTERFACE_HEADER_
#define _SIMPLE_FRAME_WORK_INTERFACE_HEADER_

#include "work_interface.h"
#include "ini_file.h"
#include "woo/log.h"
#include <map>
#include <set>
#include <vector>
#include <fstream>
#include <iostream>
#include "json.h"
#include <time.h>
#ifdef __GNUC__
#include <ext/hash_map>
#else
#include <hash_map>
#endif

class SimpleFrameWorkInterface : public WorkInterface{
	public:
		SimpleFrameWorkInterface(DbCompany*& p_db_company, int interface_id):
			WorkInterface(p_db_company, interface_id){
		}
		~SimpleFrameWorkInterface(){
		}

	private:

		// 从远程数据库获取数据
		int get_data_from_remoteDB(const uint64_t uid, __gnu_cxx::hash_set<uint64_t>& hashset_result_uids); 

		// 查询用户是否在本地数据库中
		bool uid_is_in_globalDB(const uint64_t uid);

		// 从本地数据库获取数据
		int get_type_from_globalDB(const uint64_t uid, uint32_t& type);	

		// 并行获取数据
		int get_multi_data(const uint64_t uid);

		// 将结果转换成json格式输出
		void generate_output_str(const VEC_CAND& result_vec, 
				char* &p_out_string, int& n_out_len);

		// 业务处理函数
		int get_uid_good_follow(const uint64_t uid, const int64_t req_id, 
				char* &p_out_string, int& n_out_len);

	public:

		int work_core(json_object *req_json, char* &p_out_string, int& n_out_len, int64_t req_id);
			
};
#endif
