#include "simple_frame_work_interface.h"

DYN_WORK(SimpleFrameWorkInterface);

//从远程数据库中获取信息
int SimpleFrameWorkInterface::get_data_from_remoteDB(const uint64_t uid, 
		__gnu_cxx::hash_set<uint64_t>& hashset_result_uids){
	
	DbInterface* p_remote_db_interface = p_db_company_->get_db_interface("EXAMPLE_REDIS_DB");
	if (NULL == p_remote_db_interface){
		LOG_ERROR("remote db_interface is NULL!");
		return -3;
	}
	
	char* p_result = NULL;
	char split_char, second_split_char;
	int result = p_remote_db_interface->get(0, uid, p_result, split_char, second_split_char);
	if (result <= 0 ){
		LOG_ERROR("get from remote db error!");
		return -2;
	}
	if (NULL == p_result){
		LOG_ERROR("get from remote db fail!");
		return -1;
	}

	uint32_t uid_num = 0;
	split_string_set_ids(hashset_result_uids, uid_num, (const char* &)p_result, split_char, 0);

	return 1;
}


// 从本地查询用户是否在指定集合中
bool SimpleFrameWorkInterface::uid_is_in_globalDB(const uint64_t uid){	

	GlobalDbCompany* p_global_db_company = p_db_company_->get_global_db_company();
	GlobalDbInterface* global_db_interface = NULL;

	if(p_global_db_company != NULL){
		global_db_interface = p_global_db_company->get_global_db_interface("EXAMPLE_SET_DB");
	}else{
		global_db_interface = NULL;
	}

	if(global_db_interface == NULL){
		LOG_ERROR("global_db_interface is NULL!");
		return false;
	}

	return global_db_interface->is_exist(uid);
}

// 从本地查询用户类型
int SimpleFrameWorkInterface::get_type_from_globalDB(const uint64_t uid, uint32_t& type){	

	GlobalDbCompany* p_global_db_company = p_db_company_->get_global_db_company();
	GlobalDbInterface* global_db_interface = NULL;

	if(p_global_db_company != NULL){
		global_db_interface = p_global_db_company->get_global_db_interface("EXAMPLE_KV_DB");
	}else{
		global_db_interface = NULL;
	}

	if(global_db_interface == NULL){
		LOG_ERROR("global_db_interface is NULL!");
		return -3;
	}

	// 注：由于get_value不是公有函数，必须进行显式指针转换
	GlobalKVDbInterface* p_kv_interface = (GlobalKVDbInterface*)global_db_interface;
	if(NULL == p_kv_interface){
		LOG_ERROR("conversion from GlobalDbInterface* to GlobalKVDbInterface* fail!");
		return -3;
	}

	if (!(p_kv_interface->get_value(uid, type))){ 
		type = 0;
	}
	
	return 1;
}

/*
 * 并行获取数据的一个例子
 * */
int SimpleFrameWorkInterface::get_multi_data(const uint64_t uid){
	ReqResultMap req_result_map;

	// 仅支持使用db_id
	const int redis_db_id = 0;
	int result = push_request_result_map(redis_db_id, uid, 0, 0, NULL, true, req_result_map);
	if(result <= 0){
		return -3;
	}

	const int mc_db_id = 3;
	char uid_str[20];
	snprintf(uid_str, 20, "%"PRIu64, uid);
	result = push_request_result_map(mc_db_id, uid, 0, 0, uid_str, false, req_result_map);
	if(result <= 0){
		return -3;
	}

	const int woo_db_id = 1;
	char woo_req_str[30];
	snprintf(woo_req_str, 30, "{\"id\":%"PRIu64, uid);
	result = push_request_result_map(woo_db_id, uid, 0, 0, woo_req_str, false, req_result_map);
	if(result <= 0){
		return -3;
	}

	// 并行获取数据
	if(p_db_company_->get_multi_db(req_result_map) <= 0){
		LOG_ERROR("get multi db fail! %"PRIu64, uid);
		return -2;
	}

	// 提取获取到的数据
	char split_char, second_split_char;
	__gnu_cxx::hash_set<uint64_t> uids_set;
	char* p_redis_result = NULL;
	uint32_t uid_num = 0;
	get_data_from_req_result_map(redis_db_id, req_result_map,
			p_redis_result, split_char, second_split_char);
	split_string_set_ids(uids_set, uid_num, (const char* &)p_redis_result, split_char, 0);

	// SKIPPED...
	
	return 1;
}

void SimpleFrameWorkInterface::generate_output_str(const VEC_CAND& result_vec,
		char* &p_out_string, int& n_out_len){
	
	n_out_len = 0;
	char* p_out_tmp = p_out_string;

	int len = sprintf(p_out_tmp, "%s", "{\"return_code\":\"ok\",\"result\":[");
	p_out_tmp += len;
	n_out_len += len;

	int index = 0;
	for(VEC_CAND::const_iterator cand_it = result_vec.begin();
			cand_it != result_vec.end(); ++ cand_it){
		uint64_t tid = (*cand_it)->uid;
		uint32_t user_type = (*cand_it)->utype;

		if(index == 0){
			len = sprintf(p_out_tmp, "{\"tid\":%"PRIu64",\"utype\":%"PRIu32"}", tid, user_type);
		}else{
			len = sprintf(p_out_tmp, ",{\"tid\":%"PRIu64",\"utype\":%"PRIu32"}", tid, user_type);
		}

		index ++;
		p_out_tmp += len;
		n_out_len += len;
	}

	len = sprintf(p_out_tmp, "]}");
	p_out_tmp += len;
	n_out_len += len;

}

// 业务处理函数
int SimpleFrameWorkInterface::get_uid_good_follow(const uint64_t uid, const int64_t req_id, 
		char* &p_out_string, int& n_out_len){

	// 获取用户的关注数据
	__gnu_cxx::hash_set<uint64_t> follow_uids;
	int result = get_data_from_remoteDB(uid, follow_uids);
	if(result <= 0){
		return result;
	}

	// 保留关注中在集合中的数据，并获取用户类型
	VEC_CAND result_vec;
	for(__gnu_cxx::hash_set<uint64_t>::const_iterator it = follow_uids.begin();
			it != follow_uids.end(); ++ it){
		uint64_t follow_uid = *it;
		if(uid_is_in_globalDB(follow_uid)){
			uint32_t type = 0;
			get_type_from_globalDB(follow_uid, type);
			
			candidate_item_t* cand_ptr = new candidate_item_t();
			cand_ptr->uid = follow_uid;
			cand_ptr->utype = type;
			cand_ptr->tscore = 0;
	
			result_vec.push_back(cand_ptr);
		}
	}

	// 调用算法模型
	AccessStr access_str;
	access_str.uid_ = uid;
	result = algorithm_core(req_id, access_str, result_vec);

	// 输出最终结果
	generate_output_str(result_vec, p_out_string, n_out_len);

	// 释放内存
	for(VEC_CAND::iterator it = result_vec.begin(); it != result_vec.end(); ++ it){
		delete *it;
		*it = NULL;
	}
	result_vec.clear();
	follow_uids.clear();

	return 1;
}

/*
 * 业务入口函数
 * 输入：
 * 	req_json：请求串，json格式
 * 	req_id：用于记录在日志中的id
 * 输出：
 * 	p_out_string：输出字符串，一般拼成json格式
 * 	n_out_len：输出字符串的长度
 * */
int SimpleFrameWorkInterface::work_core(json_object *req_json, 
		char* &p_out_string, int& n_out_len, int64_t req_id){
	
	int spend_msec = -1;
	struct timeval tv_temp;
	tv_temp = calc_spend_time(tv_temp, "example_work start", spend_msec, true);
	
	int result = 0;

	json_object* cmd_json = json_object_object_get(req_json, "cmd");
	char* cmd_str = NULL;
	if(NULL == cmd_json){
		cmd_str = "query";
	}else{
		cmd_str = (char*)json_object_get_string(cmd_json);
	}

	if(strcasecmp(cmd_str, "query") != 0){
		return return_fail(403, "command not support", p_out_string, n_out_len);
	}

	json_object* uid_json = json_object_object_get(req_json, "uid");
	if(NULL == uid_json){
		return return_fail(403, "uid is empty", p_out_string, n_out_len);
	}else{
		uint64_t uid = json_object_get_int64(uid_json);
		result = get_uid_good_follow(uid, req_id, p_out_string, n_out_len);
	}

	tv_temp = calc_spend_time(tv_temp, "example_work all finish", spend_msec);
	return result;
}
