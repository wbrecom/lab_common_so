#ifndef _MC_DB_INTERFACE_HEADER_
#define _MC_DB_INTERFACE_HEADER_
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <string>
#include "utility.h"
#include "base_define.h"
#include "json.h"
#include "db_interface.h"

#include <libmemcached/memcached.h>

using namespace std;

/*
 * 从memcache获取数据
 * */

typedef struct _Mc_Struct{
	char ip_[IP_WORD_LEN];	
	uint32_t port_;
	memcached_st *memc_;
} Mc_Struct;

class McDbInterface : public DbInterface{
	public:
		McDbInterface(const Db_Info_Struct& db_info_struct):
			DbInterface(db_info_struct){
				// load configure
			for(vector<string>::iterator port_it = vec_str_port_.begin();
				port_it != vec_str_port_.end(); port_it++){
				uint32_t n_port = strtoul((*port_it).c_str(), NULL, 10);

				for(vector<string>::iterator ip_it = vec_str_ip_.begin(); 
					ip_it != vec_str_ip_.end(); ip_it++){

					Mc_Struct mc_struct;
					strncpy(mc_struct.ip_, (*ip_it).c_str(), IP_WORD_LEN);
					mc_struct.port_ = n_port;
					mc_struct.memc_ = NULL;
					vec_mc_server_.push_back(mc_struct);
				}
			}
		}

		virtual ~McDbInterface(){
			for(vector<Mc_Struct>::iterator it = vec_mc_server_.begin(); 
			it != vec_mc_server_.end(); it ++){
				if((*it).memc_ != NULL)
				memcached_free((*it).memc_);
			}	
			vec_mc_server_.clear();
		}

	protected:
		memcached_st* get_mc_raw(SvrIpPort& svr_ip_port){

			uint16_t total_index = svr_ip_port.ip_index_ + svr_ip_port.port_index_ * ip_num_;
			if( NULL == vec_mc_server_[total_index].memc_){
				memcached_st *memc = NULL;
				memcached_return rc;
				memcached_server_st *servers;
				memc = memcached_create(NULL);
				if(NULL == memc){
					LOG_ERROR("mc allocate is error!");
					return NULL;
				}
				servers = memcached_server_list_append(NULL, (char*)vec_mc_server_[total_index].ip_, 
						vec_mc_server_[total_index].port_, &rc);
				rc = memcached_server_push(memc, servers);
				memcached_server_free(servers);
				memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_NO_BLOCK, true);	// 非阻塞模式
				memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT, db_info_struct_.timeout_);
				memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_POLL_TIMEOUT, db_info_struct_.timeout_); 
				memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_RETRY_TIMEOUT, 1);	// 单位是分钟

				vec_mc_server_[total_index].memc_ = memc;
				if(NULL == memc){
					LOG_ERROR("connect mc %s:%"PRIu32" error!", vec_mc_server_[total_index].ip_, 
							vec_mc_server_[total_index].port_);
					return NULL;
				}
			}
			return vec_mc_server_[total_index].memc_;
		}

	public :
		int s_get(uint16_t type_id, const char* p_str_key, char* &p_result, 
				char& split_char, char& second_split_char, uint64_t n_key){ 
			return get_i(type_id, n_key, 0, p_result, split_char, second_split_char, p_str_key, true);
		}

		// deal with item_key by char*
		int get_i(uint16_t type_id, const char* &p_str_key, uint32_t n_item_key, 
				char* &p_result, char& split_char, char& second_split_char){

			initialize();

			split_char = db_info_struct_.value_split_first_char_;
			second_split_char = db_info_struct_.value_split_second_char_;

			return 1;

		}

		int get(uint16_t type_id, uint64_t n_key, char* &p_result, 
				char& split_char, char& second_split_char){
			return get_i(type_id, n_key, 0, p_result, split_char, second_split_char);			
		}

		int get_i(uint16_t type_id, uint64_t n_key, uint32_t n_item_key, 
				char* &p_result, char& split_char, char& second_split_char, 
				const char* other_key = NULL, bool other_flag = false) {
			
			initialize();

			struct timeval tv_begin, tv_end;
			gettimeofday(&tv_begin, NULL);

			SvrIpPort svr_ip_port;
			get_ip_port(n_key, svr_ip_port);
			uint16_t total_index = svr_ip_port.ip_index_ + svr_ip_port.port_index_ * ip_num_;
			memcached_st* memc = get_mc_raw(svr_ip_port);
			if(NULL == memc){
				LOG_ERROR("MC %s:%"PRIu32" CLIENT IS NULL", vec_mc_server_[total_index].ip_, 
					vec_mc_server_[total_index].port_);
				return -1;
			}

			char str_key[WORD_LEN];
			size_t key_len = 0;
			if(!other_flag){
				key_len = snprintf(str_key, WORD_LEN, "%"PRIu64, n_key);
			}else{
				key_len = snprintf(str_key, WORD_LEN, "%s", other_key);
			}

			size_t result_str_length = 0;
			uint32_t flags = 0;
			memcached_return rc;

			char* result_str = memcached_get(memc, str_key, key_len, &result_str_length, &flags, &rc);
			
			gettimeofday(&tv_end, NULL);
			int time_get = (tv_end.tv_sec-tv_begin.tv_sec)*1000 + (tv_end.tv_usec-tv_begin.tv_usec)/1000;
			LOG_INFO("MC %s:%"PRIu32" GET %d ms", vec_mc_server_[total_index].ip_, 
					vec_mc_server_[total_index].port_, time_get);

			if (rc == MEMCACHED_SUCCESS){
				if(db_info_struct_.key_value_type_ == 0 && db_info_struct_.value_string_type_ == 0){
					if(db_info_struct_.compress_flag_){
						uLongf un_compr_len = COMPRESS_LEN - 1;
						int err = uncompress((Bytef*)un_compress_[un_compress_index_], &un_compr_len,
							(Bytef*)result_str, result_str_length);
						if (err != Z_OK){
							un_compress_[un_compress_index_][0] = '\0';
							LOG_ERROR("uncompress error!:%d:%d", err, un_compress_index_);
							free(result_str);
							result_str = NULL;
							return -1;
						}
						un_compress_[un_compress_index_][un_compr_len] = '\0';
						result_str_length = un_compr_len;
					}else{
						if(result_str_length > COMPRESS_LEN - 1){
							result_str_length = COMPRESS_LEN - 1;
						}
						memcpy(un_compress_[un_compress_index_], result_str, result_str_length);
						un_compress_[un_compress_index_][result_str_length] = '\0';
					}
					p_result = un_compress_[un_compress_index_];
					un_compress_index_ ++;
				}
				free(result_str);
				result_str = NULL;
				split_char = db_info_struct_.value_split_first_char_;
				second_split_char = db_info_struct_.value_split_second_char_;
				return (int)result_str_length;
			}else{
				LOG_ERROR("MC %s:%"PRIu32" GET %s fail! %d", vec_mc_server_[total_index].ip_, 
					vec_mc_server_[total_index].port_, str_key, rc);
				if(rc == MEMCACHED_SERVER_MARKED_DEAD){		// 服务进程死亡，则将长链接指针置空
					vec_mc_server_[total_index].memc_ = NULL;
				}
				return 0;
			}
		}

		int mget(uint16_t type_id, const char* str_keys[], uint32_t keys_num, char* results_array[],
				uint32_t& results_num, char& split_char, char& second_split_char){
			initialize();

			SvrIpPort svr_ip_port;
			get_ip_port(0, svr_ip_port);
			memcached_st* memc = get_mc_raw(svr_ip_port);
			if(NULL == memc){
				return 0;
			}
				
			size_t key_len_array[keys_num];

			for(uint32_t i = 0; i < keys_num; i++){
				key_len_array[i] = strlen(str_keys[i]);
			}
			
			struct timeval tv_begin, tv_end;
			gettimeofday(&tv_begin, NULL);

			memcached_return_t rc = memcached_mget(memc, str_keys, key_len_array, keys_num);

			gettimeofday(&tv_end, NULL);
			int time_mget = (tv_end.tv_sec-tv_begin.tv_sec)*1000 + (tv_end.tv_usec-tv_begin.tv_usec)/1000;
			uint16_t total_index = svr_ip_port.ip_index_ + svr_ip_port.port_index_ * ip_num_;
            LOG_INFO("MC %s:%"PRIu32" MGET %d ms", vec_mc_server_[total_index].ip_, 
					vec_mc_server_[total_index].port_, time_mget);

			char return_key[MEMCACHED_MAX_KEY];
			char *return_value = NULL; 
			size_t return_key_length = 0;
			size_t return_value_length = 0;  
			uint32_t flags = 0;
			results_num = 0;
			while((return_value = memcached_fetch(memc, return_key, 
							&return_key_length, &return_value_length, &flags, &rc))){
				if(rc == MEMCACHED_SUCCESS){
					//strncpy(un_compress_[un_compress_index_], return_value, return_value_length);
					memcpy(un_compress_[un_compress_index_], return_value, return_value_length);
					results_array[results_num] = un_compress_[un_compress_index_];
					if(return_value != NULL){
						free(return_value);
					}
					un_compress_index_ ++;
					results_num ++;
				}
			}
			return 0;
		}

		int mget(uint16_t type_id, uint64_t n_keys[], uint32_t key_num, MapMResult& map_result,
				char& split_char, char& second_split_char){

			uint32_t n_item_keys[1];
			return mget_i(type_id, n_keys, key_num, n_item_keys, 0, map_result, 
					split_char, second_split_char);
		}

		int mget_i(uint16_t type_id, uint64_t n_keys[], uint32_t key_num, 
				uint32_t n_item_keys[], uint32_t item_key_num,
				MapMResult& map_result, char& split_char, char& second_split_char){

			initialize();
			split_char = db_info_struct_.value_split_first_char_;
			second_split_char = db_info_struct_.value_split_second_char_;
			return 1;
		}

    protected:
		vector<Mc_Struct> vec_mc_server_;
};

#endif


