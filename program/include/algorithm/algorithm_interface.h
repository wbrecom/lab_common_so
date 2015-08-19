#ifndef _ALGORITHM_INTERFACE_HEADER_
#define _ALGORITHM_INTERFACE_HEADER_

#include "db_company.h"
#include "ini_file.h"
#include "woo/log.h"
#include "cand_user.h"
#include <map>
#include <set>
#include <vector>
#include <fstream>
#include <iostream>
#include "json.h"
#include <time.h>

typedef struct{
	uint64_t uid_;
} AccessStr;

class AccessInfo{
	public:
		uint64_t uid_;
};

class AlgorithmInterface;
typedef AlgorithmInterface * (*CreateFactory_T) (DbCompany*& p_db_company, int interface_id);

#define DYN_ALGORITHMS(algorithmsName) \
	extern "C" \
	AlgorithmInterface * create##algorithmsName(DbCompany*& p_db_company, int interface_id) \
	{ return new (std::nothrow) algorithmsName(p_db_company, interface_id); }

typedef struct{
	int16_t id_mod_;
	void* handle_;
	char global_db_name_[PORT_LEN];//这个主要为了兼容本地获取测试uid白名单用户
	AlgorithmInterface* algorithm_interface_;
} AlgorithmInfo;

class AlgorithmInterface{
	public:
		AlgorithmInterface(DbCompany*& p_db_company, int interface_id):
			p_db_company_(p_db_company), interface_id_(interface_id){
		}
		virtual ~AlgorithmInterface(){
		}

	public:
		DbCompany*& p_db_company_;	

	protected:
		int interface_id_;

	public:
		// 算法入口，输入即输出
		virtual int algorithm_core(int64_t req_id, const AccessStr& access_str,
				VEC_CAND& vec_cand) = 0;

		// 算法入口，输入输出分离
		virtual int algorithm_core_new(int64_t req_id, const AccessInfo* access_info, 
				const VEC_CAND& input_vec, VEC_CAND& output_vec){
			return 0;
		}
	public:
		int get_interface_id(){
			return interface_id_;
		}
};

#endif
