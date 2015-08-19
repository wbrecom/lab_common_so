#ifndef _DB_COMPANY_HEADER_
#define _DB_COMPANY_HEADER_

#include "db_interface_factory.h"
#include "global_db_company.h"
#include "ini_file.h"
#include "woo/log.h"
#include <map>
#include "time.h"
#include <curl/curl.h>

typedef struct _RequestPara{
	uint64_t n_key_; 		// 请求key
	int64_t n_log_id_;

	uint32_t n_item_key_; 	// 请求子key，一般不使用
	uint16_t type_id_; 		// 类型，一般不使用
	char* p_result_; 		// 结果返回
	char* p_str_key_; 		// 请求字符型 key

	DbInterface* p_db_interface_; 	// 当前数据库实体

	char split_char_; 				// 返回分割字符
	char second_split_char_; 		// 返回二级分割字符
	bool get_type_; 				// true 表示通过n_key_获取，false 表示通过p_str_key_获取
} RequestPara;

typedef std::map<uint16_t, RequestPara*> ReqResultMap; //db 标号，请求数据

int push_request_result_map(uint16_t db_id, uint64_t n_key, uint32_t n_item_key,
		uint16_t type_id, char* p_str_key, bool get_type, ReqResultMap& req_result_map){
	//暂时不支持 对同一个db使用多线程访问，里面会涉及到栈空间的共享空间的问题

	ReqResultMap::iterator it = req_result_map.find(db_id);
	if(it == req_result_map.end()){
		RequestPara* request_para = new RequestPara();

		if(NULL == request_para){
			LOG_ERROR("request para malloc mem is failed!");
			return -1;
		}else{
			request_para->n_key_ = n_key;
			request_para->n_log_id_ = woo::get_log_id();
			request_para->n_item_key_ = n_item_key;
			request_para->type_id_ = type_id;
			request_para->p_str_key_ = p_str_key;
			request_para->get_type_ = get_type;
			request_para->p_result_ = NULL;
			request_para->p_db_interface_ = NULL;
			request_para->split_char_ = 0;
			request_para->second_split_char_ = 0;

			req_result_map.insert(ReqResultMap::value_type(db_id, request_para));
		}
		return 1;
	}
	return 0;
}

//通过dbid获取数据内容
int get_data_from_req_result_map(uint16_t db_id, const ReqResultMap& req_result_map, 
		char*& p_result, char& split_char, char& second_split_char){
	ReqResultMap::const_iterator it = req_result_map.find(db_id);
	if(it == req_result_map.end()){
		return -1;
	}else{
		if((*it).second == NULL)
			return -1;
		else{
			p_result = (*it).second->p_result_;
			split_char = (*it).second->split_char_;
			second_split_char = (*it).second->second_split_char_;

			return 1;
		}
	}
}

//清空map数据
int release_request_result_map(ReqResultMap& req_result_map){

	for(ReqResultMap::iterator it = req_result_map.begin();
			it != req_result_map.end(); it++){
		if((*it).second == NULL)
			continue;
		else{
			delete (*it).second;
			(*it).second = NULL;
		}
	}
	req_result_map.clear();
	return 1;
}

class DbCompany{
	public :
		DbCompany(){}
		~DbCompany() {
			release();
		}

	private:
		GlobalDbCompany* global_db_company_;	

	public:
		GlobalDbCompany* get_global_db_company(){
			return global_db_company_;
		}

	private :
		int get_db_info(const char* file_name, std::vector<std::string>& vec_db_name){
			char split_char = (char)read_profile_int("DB_INFO", "DBS_SPLIT_CHAR",
					44, file_name);
			char dbs_name[PATH_MAX];
			if( !read_profile_string("DB_INFO", "DBS_NAME",
						 dbs_name, PATH_MAX, "", file_name))
			{
				LOG_ERROR("load dbs name error!");
				return -1;
			}

			split_string(vec_db_name, dbs_name, split_char);
			return 1;
		}

		void release(){
			for(std::map<uint16_t, DbInterface*>::iterator pos =
					map_db_interface_.begin(); pos != map_db_interface_.end(); pos++){
				if((*pos).second != NULL){
					delete (*pos).second;
					(*pos).second = NULL;
				}
			}
			map_db_interface_.clear();

			map_db_str_interface_.clear();

			curl_global_cleanup();
		}

	public :
		int initialize(GlobalDbCompany* global_db_company) {
			curl_global_init(CURL_GLOBAL_ALL);

			for(std::map<uint16_t, DbInterface*>::iterator pos = map_db_interface_.begin();
					pos != map_db_interface_.end(); pos ++) {
				if((*pos).second != NULL) {
					(*pos).second->initialize();
				}
			}
			global_db_company_ = global_db_company;
			return 0;
		}
		int load_config(const char* file_name){
			std::vector<std::string> vec_db_name;
			int n_result = get_db_info(file_name, vec_db_name);

			if (n_result <= 0)
				return n_result;

			DbInterfaceFactory interface_factory;
			for(std::vector<std::string>::iterator pos = vec_db_name.begin();
					pos != vec_db_name.end(); pos ++){

				Db_Info_Struct db_info_struct;
				if(db_info_struct.load_config(file_name, (*pos).c_str()) <= 0)
					continue;

				DbInterface* p_db_interface = interface_factory.get_db_interface(db_info_struct);
				if(NULL == p_db_interface)
					continue;
				map_db_interface_.insert(std::make_pair(db_info_struct.db_id_, p_db_interface));

				map_db_str_interface_.insert(std::make_pair(db_info_struct.db_name_, p_db_interface));

			}
			return 1;
		}

		DbInterface* get_db_interface(uint16_t db_id){
			DbInterface* p_db_interface = NULL;
			std::map<uint16_t, DbInterface*>::iterator pos = map_db_interface_.find(db_id);
			if(pos == map_db_interface_.end())
				return p_db_interface;

			p_db_interface = (*pos).second;
			return p_db_interface;
		}
		
		DbInterface* get_db_interface(const char* db_name){
			DbInterface* p_db_interface = NULL;
			std::map<std::string, DbInterface*>::iterator pos = 
				map_db_str_interface_.find(std::string(db_name));
			if(pos == map_db_str_interface_.end()){
				LOG_ERROR("%s: can not found it!", db_name);
				return p_db_interface;
			}

			p_db_interface = (*pos).second;
			return p_db_interface;
		}


	private:
		std::map<uint16_t, DbInterface*> map_db_interface_;
		std::map<std::string, DbInterface*> map_db_str_interface_;

		static void* get_multi_db_pthread(void* thread_para){
			RequestPara* request_para = (RequestPara*)thread_para;

			if(NULL == request_para || NULL == request_para->p_db_interface_ ){
				LOG_ERROR("para is NULL!");
				return NULL;
			}

			woo::set_log_id(request_para->n_log_id_);

			if(!request_para->get_type_){ // 通过字符串获取
				if(request_para->p_str_key_ == NULL){
					LOG_ERROR("request is NULL!");
					return NULL;
				}
				request_para->p_db_interface_->s_get(request_para->type_id_, request_para->p_str_key_,
						request_para->p_result_, request_para->split_char_, 
						request_para->second_split_char_, request_para->n_key_);
			}
			else{ // 通过整型获取
				request_para->p_db_interface_->get(request_para->type_id_, request_para->n_key_,
						request_para->p_result_, request_para->split_char_,
						request_para->second_split_char_);
			}
			return request_para;
		}
	public:
		//添加不同数据库批量获取数据的内容
		int get_multi_db(ReqResultMap& req_result_map){
	
			int thread_num = 0;
			RequestPara* request_para_arr[req_result_map.size()];
			
			for(ReqResultMap::iterator it = req_result_map.begin();
					it != req_result_map.end(); it++){
				uint16_t db_id = (*it).first;
				RequestPara* request_para = (*it).second;

				if(NULL == request_para){
					LOG_ERROR("%"PRIu16" request is error!", db_id);
					continue;
				}

				DbInterface* p_db_interface = this->get_db_interface(db_id);
				if(NULL == p_db_interface){
					LOG_ERROR("%"PRIu16" db create is NULL!", db_id);
					continue;
				}

				request_para->p_db_interface_ = p_db_interface;
				request_para_arr[thread_num] = request_para;

				thread_num ++;
			}

			pthread_t thread_arr[thread_num];
			for(int i = 0 ; i < thread_num; i ++){
				pthread_create(&thread_arr[i], NULL, &DbCompany::get_multi_db_pthread, 
						request_para_arr[i]);
			}

			for(int i = 0 ; i < thread_num; i ++){
				pthread_join(thread_arr[i], NULL);
			}
			//获取数据返回
			return 1;
		}

		int mget_multi_db(){
			return 0;
		}
};

#endif


