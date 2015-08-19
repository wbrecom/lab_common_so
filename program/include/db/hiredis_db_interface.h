#ifndef _HIREDIS_DB_INTERFACE_HEADER_
#define _HIREDIS_DB_INTERFACE_HEADER_
#include "hiredis.h"
#include <stdlib.h> 
#include <vector>
#include <string>
#include "utility.h"
#include "base_define.h"
#include "db_interface.h"

using namespace std;

/*
 * 从redis获取数据
 * */

typedef struct _Redis_Struct{
	char ip_[IP_WORD_LEN];	
	uint32_t port_;
	redisContext *redis_;
} Redis_Struct;

class HiRedisDbInterface;

typedef struct _RedisThreadPara{
	HiRedisDbInterface* p_redis_db_interface_;
	ReqStruct* p_req_struct_;
	SvrIpPort* p_svr_ip_port_;	
    VecMResult* p_vec_result_;
	char (*p_uncompress_) [COMPRESS_LEN];
	int n_spend_time_[4];
} RedisThreadPara;

//template<typename Hash_Func>
class HiRedisDbInterface : public DbInterface{
	public:
		HiRedisDbInterface(const Db_Info_Struct& db_info_struct):
					DbInterface(db_info_struct){
			// load configure

			for(vector<string>::iterator port_it = vec_str_port_.begin();
                    port_it != vec_str_port_.end(); port_it++){
				uint32_t n_port = strtoul((*port_it).c_str(), NULL, 10);

				for(vector<string>::iterator ip_it = vec_str_ip_.begin(); 
					ip_it != vec_str_ip_.end(); ip_it++){

					Redis_Struct redis_struct;
					strncpy(redis_struct.ip_, (*ip_it).c_str(), IP_WORD_LEN);
					redis_struct.port_ = n_port;
					redis_struct.redis_ = NULL;
					vec_redis_server_.push_back(redis_struct);
				}
			}
		}	
		~HiRedisDbInterface(){
			for(vector<Redis_Struct>::iterator it = vec_redis_server_.begin(); 
				it != vec_redis_server_.end(); it ++){
				if((*it).redis_ != NULL)
					redisFree((*it).redis_);
			}
			vec_redis_server_.clear();
		}
	private:

		redisContext* get_redis_raw(SvrIpPort& svr_ip_port){
			uint16_t total_index = svr_ip_port.ip_index_ + svr_ip_port.port_index_ * ip_num_;
			struct timeval tv;
			tv.tv_sec = db_info_struct_.timeout_ / 1000; //seconds
			tv.tv_usec = db_info_struct_.timeout_ * 1000 - tv.tv_sec * 1000 * 1000; //useconds
			redisContext *redis = redisConnectWithTimeout(vec_redis_server_[total_index].ip_,
					vec_redis_server_[total_index].port_, tv);

			if(NULL == redis)
				return NULL;

			if(redis->err){
				LOG_ERROR("%s:%"PRIu32" err:%s", vec_redis_server_[total_index].ip_, vec_redis_server_[total_index].port_, redis->errstr);
				redisFree(redis);
				return NULL; 
			}

			redisSetTimeout(redis, tv);
			return redis;
		}

		redisContext* get_redis(uint64_t n_key, SvrIpPort& svr_ip_port){

			get_ip_port(n_key, svr_ip_port);
			return get_redis_raw(svr_ip_port);
		}

	public:
		int s_get(uint16_t type_id, const char* p_str_key, char* &p_result, 
				char& split_char, char& second_split_char, uint64_t n_key){ 
			initialize();

			return get_i(type_id, n_key, 0, p_result, split_char, second_split_char, p_str_key, true);
		}

		int s_set(uint16_t type_id, const char* p_str_key, const char* p_value, uint64_t n_key){
			initialize();
			
			return set_i(type_id, n_key, 0, p_value, p_str_key, true);
		}


		// deal with item_key by char*
		int get_i(uint16_t type_id, const char* &p_str_key, uint32_t n_item_key, char* &p_result,
				char& split_char, char& second_split_char){
			return 1;
		}

		int get(uint16_t type_id, uint64_t n_key, char* &p_result, 
				char& split_char, char& second_split_char){ 
			initialize();
			return get_i(type_id, n_key, 0, p_result, split_char, second_split_char);
		}
	
		int set_i(uint16_t type_id, uint64_t n_key, uint32_t item_key,
				const char* value, const char* other_key = NULL, bool other_flag = false){ 
		// -1 is NULL, -2 is error, 1 is success, 0 is not exist
		//
			if(value == NULL){
				LOG_ERROR("value is NULL!");
				return -1;
			}

			SvrIpPort svr_ip_port;
			redisContext* redis = get_redis(n_key, svr_ip_port);
			if(NULL == redis)
				return -1;
			
			redisReply *reply = NULL;
			if(db_info_struct_.db_index_ != 0){
				reply = (redisReply*)redisCommand(redis, "SELECT %d", db_info_struct_.db_index_);
				if(NULL == reply){
					if(redis->err){
						LOG_ERROR("SELECT redis error:%s", redis->errstr);
						redisFree(redis);
						return -3;
					}

					reply = (redisReply*)redisCommand(redis, "SELECT %d", db_info_struct_.db_index_);
					if(reply == NULL){
						LOG_ERROR("SELECT %d redis reply is NULL!", db_info_struct_.db_index_);
						redisFree(redis);
						return -2;
					}
				}
				freeReplyObject(reply);
			}

			if(other_flag){
				if(other_key == NULL){
					redisFree(redis);
					LOG_ERROR("other key is NULL!");
					return -1;
				}
				reply = (redisReply*)redisCommand(redis, "SET %s %s", other_key, value);
			}else{
				reply = (redisReply*)redisCommand(redis, "SET %"PRIu64" %s", n_key, value);
			}
			

			if(reply == NULL){
				LOG_ERROR("SET redis reply is NULL!");
				redisFree(redis);
				return -2;
			}

			if(REDIS_REPLY_ERROR == reply->type){
				LOG_ERROR("SET reply error:%s", reply->str);
				if(other_flag){
					if(other_key == NULL){
						freeReplyObject(reply);
						redisFree(redis);
						LOG_ERROR("other key is NULL!");
						return -1;
					}
					reply = (redisReply*)redisCommand(redis, "SET %s %s", other_key, value);
				}else{
					reply = (redisReply*)redisCommand(redis, "SET %"PRIu64" %s", n_key, value);
				}
			}
			
			if(reply == NULL){
				LOG_ERROR("SET redis reply is NULL!");
				redisFree(redis);
				return -2;
			}

			if(redis->err){
				LOG_ERROR("SET error:%s", redis->errstr);
				freeReplyObject(reply);
				redisFree(redis);
				return -3;
			}

			if(REDIS_REPLY_ERROR == reply->type){
				LOG_ERROR("SET reply error:%s", reply->str);
				freeReplyObject(reply);
				redisFree(redis);
				return -3;
			}

			freeReplyObject(reply);
			redisFree(redis);

			return 1;

		}

		int get_i(uint16_t type_id, uint64_t n_key, uint32_t item_key,
				char* &p_result, char& split_char, char& second_split_char, 
				const char* other_key = NULL, bool other_flag = false){ 
		    // -1 is NULL, -2 is error, 1 is success, 0 is not exist
			struct timeval tv_begin, tv_end;
			gettimeofday(&tv_begin, NULL);

			SvrIpPort svr_ip_port;
			redisContext* redis = get_redis(n_key, svr_ip_port);

			uint16_t total_index = svr_ip_port.ip_index_ + svr_ip_port.port_index_ * ip_num_;
			char* redis_ip = vec_redis_server_[total_index].ip_;
			uint32_t redis_port = vec_redis_server_[total_index].port_;
			if(NULL == redis){
				LOG_ERROR("CREATE redis %s:%"PRIu32" fail!", redis_ip, redis_port);
				return -1;
			}

			// 选定数据库
			redisReply *reply = NULL;
			if(db_info_struct_.db_index_ != 0){
				reply = (redisReply*)redisCommand(redis, "SELECT %d", db_info_struct_.db_index_);
				if(NULL == reply){
					if(redis->err){
						LOG_ERROR("SELECT redis error:%s", redis->errstr);
						redisFree(redis);
						return -3;
					}

					reply = (redisReply*)redisCommand(redis, "SELECT %d", db_info_struct_.db_index_);
					if(reply == NULL){
						LOG_ERROR("SELECT redis %s:%"PRIu32" db:%d fail!", redis_ip, redis_port, db_info_struct_.db_index_);
						redisFree(redis);
						return -2;
					}
				}
				freeReplyObject(reply);
			}

			// 发送获取数据命令
			char key_str_temp[WORD_LEN];
			if (other_flag){
				snprintf(key_str_temp, WORD_LEN, "GET %s", other_key);
			}else{
				snprintf(key_str_temp, WORD_LEN, "GET %"PRIu64, n_key);
			}
			reply = (redisReply*)redisCommand(redis, key_str_temp);
			if(NULL == reply){
				if(redis->err){
					LOG_ERROR("GET redis %s:%"PRIu32" error:%s", redis_ip, redis_port, redis->errstr);
					redisFree(redis);
					return -3;
				}

				reply = (redisReply*)redisCommand(redis, key_str_temp);
				if(reply == NULL){
					LOG_ERROR("GET redis %s:%"PRIu32" reply is NULL!", redis_ip, redis_port);
					redisFree(redis);
					return -2;
				}
			}

			if (reply->type != REDIS_REPLY_STRING){
				freeReplyObject(reply);
				redisFree(redis);
				return -3;
			}

			const char* redis_result = reply->str;
			if(NULL == redis_result){
				freeReplyObject(reply);
				redisFree(redis);
				return 0;
			}

			if(db_info_struct_.key_value_type_ == 0 && db_info_struct_.value_string_type_ == 0){
				if(db_info_struct_.compress_flag_){
					uLongf un_compr_len = REDIS_COMPRESS_LEN;
					int err = uncompress((Bytef*)un_compress_[un_compress_index_], &un_compr_len, (Bytef*)redis_result, reply->len);
					if (err != Z_OK)
					{
						LOG_ERROR("uncompress error: %d", err);
						freeReplyObject(reply);
						redisFree(redis);
						return -1;
					}
					un_compress_[un_compress_index_][un_compr_len] = '\0';
					p_result = un_compress_[un_compress_index_];
					un_compress_index_ ++;
				}else{
					strncpy(un_compress_[un_compress_index_], redis_result, REDIS_COMPRESS_LEN);
					//memcpy(un_compress_[un_compress_index_], redis_result, REDIS_COMPRESS_LEN);
					p_result = un_compress_[un_compress_index_];
					un_compress_index_ ++;
				}
				split_char = db_info_struct_.value_split_first_char_;
				second_split_char = db_info_struct_.value_split_second_char_;
			}

			gettimeofday(&tv_end, NULL);
			int time_cost = ((tv_end.tv_sec-tv_begin.tv_sec)*1000 + (tv_end.tv_usec-tv_begin.tv_usec)/1000);
			LOG_INFO("REDIS %s:%"PRIu32" GET %d ms", redis_ip, redis_port, time_cost);

			freeReplyObject(reply);
			redisFree(redis);
			return 1;
		}

		int s_mget(uint16_t type_id, const char* n_keys[], uint32_t key_num, MapStrResult& map_result,
				char& split_char, char& second_split_char){
			initialize();
			return s_mget_i(type_id, n_keys, key_num, map_result, split_char, second_split_char);
		}

		int mget(uint16_t type_id, uint64_t n_keys[], uint32_t key_num, MapMResult& map_result,
				char& split_char, char& second_split_char){
			initialize();
			uint32_t n_item_keys[1];
			if(db_info_struct_.mget_type_ == 1){
				return mget_i_multi(type_id, n_keys, key_num, n_item_keys, 0, map_result,
					split_char, second_split_char); 
			}else{
				return mget_i(type_id, n_keys, key_num, n_item_keys, 0, map_result, 
					split_char, second_split_char);
			}
		}

		int mget_i_multi(uint16_t type_id, uint64_t n_keys[], uint32_t key_num, 
				uint32_t n_item_keys[], uint32_t item_key_num,
				MapMResult& map_result, char& split_char, char& second_split_char){
			
			MapSplitIpPort map_ip_port;
			gets_ip_port(n_keys, key_num, map_ip_port);
		
			int num = map_ip_port.size();
			pthread_t thread_arr[num];
			RedisThreadPara redis_thread_para_arr[num];
			
			un_compress_index_  = 0;
			int index = 0;
			int step = db_info_struct_.mget_thread_num_;
			// 原有uncompress_这个多维数据是有限制的
			// 在redis并行获取过程中，需要限定每个线程最大访问数

			if(map_ip_port.size() > 0){			// 防止越界
				if ((REDIS_COMPRESS_NUM / int(map_ip_port.size())) < step){
					step = REDIS_COMPRESS_NUM / map_ip_port.size();
				}
			}

			for(MapSplitIpPort::iterator it = map_ip_port.begin(); it != map_ip_port.end(); it++){

				redis_thread_para_arr[index].p_redis_db_interface_ = this;
				redis_thread_para_arr[index].p_req_struct_ = new ReqStruct();
				redis_thread_para_arr[index].p_req_struct_->num_ = (*it).second.num_;
				redis_thread_para_arr[index].p_req_struct_->step_ = step;

				for(uint32_t i = 0; i < (*it).second.num_; i ++){
					redis_thread_para_arr[index].p_req_struct_->n_requsts_[i] = (*it).second.n_requsts_[i];
				}

				redis_thread_para_arr[index].p_svr_ip_port_ = new SvrIpPort();
				redis_thread_para_arr[index].p_svr_ip_port_->ip_index_ = (*it).first.ip_index_;
				redis_thread_para_arr[index].p_svr_ip_port_->port_index_ = (*it).first.port_index_;

				redis_thread_para_arr[index].p_vec_result_ = new VecMResult;
				redis_thread_para_arr[index].p_uncompress_ = &this->un_compress_[un_compress_index_];

				un_compress_index_ = un_compress_index_ + step;

				index ++;
			}

			map_ip_port.clear();

			for(int i = 0 ; i < index; i ++){
				pthread_create(&thread_arr[i], NULL, &HiRedisDbInterface::mget_i_internal_pthread, 
						&redis_thread_para_arr[i]);
			}

			for(int i = 0 ; i < index; i ++){
				pthread_join(thread_arr[i], NULL);
			}

			for(int i = 0; i < index; i ++){

				for(VecMResult::iterator it = redis_thread_para_arr[i].p_vec_result_->begin();
						it != redis_thread_para_arr[i].p_vec_result_->end(); it++){
					map_result.insert(MapMResult::value_type((*it).first, (*it).second));
				}

				redis_thread_para_arr[i].p_vec_result_->clear();
				delete redis_thread_para_arr[i].p_vec_result_;

				delete redis_thread_para_arr[i].p_req_struct_;
				delete redis_thread_para_arr[i].p_svr_ip_port_;
			}

			split_char = db_info_struct_.value_split_first_char_;
			second_split_char = db_info_struct_.value_split_second_char_;

			return 1;
		}

		static void* mget_i_internal_pthread(void* thread_para){
			
			RedisThreadPara* p_thread_para = (RedisThreadPara*)thread_para;

			if(NULL == p_thread_para || NULL == p_thread_para->p_redis_db_interface_){
				return NULL;
			}
			p_thread_para->p_redis_db_interface_->mget_i_internal(p_thread_para->p_svr_ip_port_,
					p_thread_para->p_req_struct_, p_thread_para->p_vec_result_, 
					p_thread_para->p_uncompress_, p_thread_para->n_spend_time_);

			return NULL;
		}

		int mget_i_internal(SvrIpPort* &p_svr_ip_port,  ReqStruct* &p_req_struct,
				VecMResult*& p_vec_result, char un_compress[][COMPRESS_LEN], int spend_time[]){

			struct timeval tv_begin, tv_end;
			gettimeofday(&tv_begin, NULL);

			if (NULL == p_req_struct || NULL == p_svr_ip_port || NULL == p_vec_result){
				return -1;
			}

			if(p_req_struct->num_ == 0){
				return -1;
			}
			
			redisContext* redis = get_redis_raw(*p_svr_ip_port);

			uint16_t total_index = p_svr_ip_port->ip_index_ + p_svr_ip_port->port_index_ * ip_num_;
			char* redis_ip = vec_redis_server_[total_index].ip_;
			uint32_t redis_port = vec_redis_server_[total_index].port_;

			if(redis == NULL){
				LOG_ERROR("CREATE redis %s:%"PRIu32" fail!", redis_ip, redis_port);
				return -1;
			}
				
			// 选定数据库
			redisReply *reply = NULL;
			if(db_info_struct_.db_index_ != 0){
				reply = (redisReply*)redisCommand(redis, "SELECT %d", db_info_struct_.db_index_);
				if(NULL == reply){
					if(redis->err){
						LOG_ERROR("SELECT redis error:%s", redis->errstr);
						redisFree(redis);
						return -3;
					}

					reply = (redisReply*)redisCommand(redis, "SELECT %d", db_info_struct_.db_index_);
					if(reply == NULL){
						LOG_ERROR("SELECT redis %s:%"PRIu32" db:%d fail!", redis_ip, redis_port, db_info_struct_.db_index_);
						redisFree(redis);
						return -2;
					}
				}
				freeReplyObject(reply);
			}

			int un_compress_index = 0;

			char mget[REDIS_COMPRESS_LEN];
			char* p_mget = mget;
			int len = snprintf(p_mget, WORD_LEN, "%s",  "MGET");
			p_mget = p_mget + len;

			uint32_t i = 0;
			for(i = 0; i < p_req_struct->num_; i ++){
				len = snprintf(p_mget, WORD_LEN, " %"PRIu64, p_req_struct->n_requsts_[i]);
				p_mget = p_mget + len;
			}
			*p_mget = '\0';

			reply = (redisReply*)redisCommand(redis, mget);
			
			if(reply == NULL){
				LOG_ERROR("MGET %s:%"PRIu32" err:%d, errstr:%s", redis_ip, redis_port, redis->err, redis->errstr);
				redisFree(redis);
				return -1;
			}

			int step = p_req_struct->step_;
			for(i = 0; i < reply->elements; i ++) {
				char *result = reply->element[i]->str;
				long result_len = reply->element[i]->len;

				if (result_len == 0 || result == NULL)
					continue;

				if(un_compress_index >= step){
					break;
				}
				if(db_info_struct_.key_value_type_ == 0 && db_info_struct_.value_string_type_ == 0){
					if(db_info_struct_.compress_flag_){
						uLongf un_compr_len = REDIS_COMPRESS_LEN;
						try{
							int err = uncompress((Bytef*)un_compress[un_compress_index], &un_compr_len, 
								(Bytef*)result, result_len);

							if (err != Z_OK){
								continue;
							}
						}catch(...){
							LOG_ERROR("uncompress error!");
						}

						un_compress[un_compress_index][un_compr_len] = '\0';
						char* p_result = un_compress[un_compress_index];
						un_compress_index ++;
			
						p_vec_result->push_back(pair<uint64_t, const char*>(p_req_struct->n_requsts_[i], p_result));

					}else{
						strncpy(un_compress[un_compress_index], result, REDIS_COMPRESS_LEN);
						char* p_result = un_compress[un_compress_index];
						un_compress_index ++;

						p_vec_result->push_back(pair<uint64_t, const char*>(p_req_struct->n_requsts_[i], p_result));
					}
				}
				else{
					strncpy(un_compress[un_compress_index], result, REDIS_COMPRESS_LEN);
					//memcpy(un_compress[un_compress_index], result, REDIS_COMPRESS_LEN);
					char* p_result = un_compress[un_compress_index];
					un_compress_index++;

					p_vec_result->push_back(pair<uint64_t, const char*>(p_req_struct->n_requsts_[i], p_result));
				}
			}

			gettimeofday(&tv_end, NULL);
			int time_cost = ((tv_end.tv_sec-tv_begin.tv_sec)*1000 + (tv_end.tv_usec-tv_begin.tv_usec)/1000);
			LOG_INFO("REDIS %s:%"PRIu32" MGET THREAD %d %d ms", redis_ip, redis_port, p_req_struct->num_, time_cost);

			freeReplyObject(reply);
			redisFree(redis);

			return 1;

		}

		int mget_i(uint16_t type_id, uint64_t n_keys[], uint32_t key_num, 
				uint32_t n_item_keys[], uint32_t item_key_num,
				MapMResult& map_result, char& split_char, char& second_split_char){

			struct timeval tv_begin, tv_end;
			gettimeofday(&tv_begin, NULL);

			MapSplitIpPort map_ip_port;
			gets_ip_port(n_keys, key_num, map_ip_port);
	
			for(MapSplitIpPort::iterator it = map_ip_port.begin(); it != map_ip_port.end(); it++){

				struct timeval inter_tv_begin, inter_tv_end;
				gettimeofday(&inter_tv_begin, NULL);

				SvrIpPort svr_ip_port = (*it).first;	
				ReqStruct& req_struct = (*it).second;

				if(req_struct.num_ == 0){
					continue;
				}

				redisContext* redis = get_redis_raw(svr_ip_port);

				uint16_t total_index = svr_ip_port.ip_index_ + svr_ip_port.port_index_ * ip_num_;
				char* redis_ip = vec_redis_server_[total_index].ip_;
				uint32_t redis_port = vec_redis_server_[total_index].port_;
				if(NULL == redis){
					LOG_ERROR("CREATE redis %s:%"PRIu32" fail!", redis_ip, redis_port);
					continue;
				}

				// 选定数据库
				redisReply *reply = NULL;
				if(db_info_struct_.db_index_ != 0){
					reply = (redisReply*)redisCommand(redis, "SELECT %d", db_info_struct_.db_index_);
					if(NULL == reply){
						if(redis->err){
							LOG_ERROR("SELECT redis error:%s", redis->errstr);
							redisFree(redis);
							continue;
						}

						reply = (redisReply*)redisCommand(redis, "SELECT %d", db_info_struct_.db_index_);
						if(reply == NULL){
							LOG_ERROR("SELECT redis %s:%"PRIu32" db:%d fail!", redis_ip, redis_port, db_info_struct_.db_index_);
							redisFree(redis);
							continue;
						}
					}
					freeReplyObject(reply);
				}

				// 发送获取数据命令
				char mget[REDIS_COMPRESS_LEN];
				char* p_mget = mget;
				int len = snprintf(p_mget, WORD_LEN, "%s",  "MGET");
				p_mget = p_mget + len;

				uint32_t i = 0;

				for(i = 0; i < req_struct.num_; i ++){
					len = snprintf(p_mget, WORD_LEN, " %"PRIu64, req_struct.n_requsts_[i]);
					p_mget = p_mget + len;
				}

				*p_mget = '\0';

				reply = (redisReply*)redisCommand(redis, mget);
				
				if(reply == NULL){
					LOG_ERROR("MGET %s:%"PRIu32" reply is NULL %s", redis_ip, redis_port, mget);
					redisFree(redis);
					continue;
				}

				for(i = 0; i < reply->elements; i ++) {
					char *result = reply->element[i]->str;
					long len = reply->element[i]->len;

					if (len == 0 || result == NULL)
						continue;

                    if(un_compress_index_ >= REDIS_COMPRESS_NUM){
						//LOG_DEBUG("out of num!");
                        break;
					}
					if(db_info_struct_.key_value_type_ == 0 && db_info_struct_.value_string_type_ == 0){
						if(db_info_struct_.compress_flag_){
							uLongf un_compr_len = REDIS_COMPRESS_LEN;
							try{
								int err = uncompress((Bytef*)un_compress_[un_compress_index_], &un_compr_len, 
									(Bytef*)result, len);

								if (err != Z_OK){
									LOG_ERROR("%"PRIu64" uncompress error!:%d:%d", req_struct.n_requsts_[i], err, un_compress_index_);
									un_compress_index_ ++;
									continue;
								}
							}catch(...){
								LOG_ERROR("uncompress error!");
							}

							un_compress_[un_compress_index_][un_compr_len] = '\0';
							char* p_result = un_compress_[un_compress_index_];
							un_compress_index_ ++;
						
							map_result.insert(MapMResult::value_type(req_struct.n_requsts_[i], p_result));

						}else{
							strncpy(un_compress_[un_compress_index_], result, REDIS_COMPRESS_LEN);
							//memcpy(un_compress_[un_compress_index_], result, REDIS_COMPRESS_LEN);
							char* p_result = un_compress_[un_compress_index_];
							un_compress_index_ ++;

                            map_result.insert(MapMResult::value_type(req_struct.n_requsts_[i], p_result));
						}
					}
					else{
						strncpy(un_compress_[un_compress_index_], result, REDIS_COMPRESS_LEN);
						//memcpy(un_compress_[un_compress_index_], result, REDIS_COMPRESS_LEN);
                        char* p_result = un_compress_[un_compress_index_];
						un_compress_index_ ++;

                        map_result.insert(MapMResult::value_type(req_struct.n_requsts_[i], p_result));
					}
				}

				gettimeofday(&inter_tv_end, NULL);
				int time_cost = ((inter_tv_end.tv_sec - inter_tv_begin.tv_sec) * 1000 + (inter_tv_end.tv_usec - inter_tv_begin.tv_usec) / 1000 );
				LOG_INFO("REDIS %s:%"PRIu32" MGET %d %d ms", redis_ip, redis_port, req_struct.num_, time_cost);

				freeReplyObject(reply);
				redisFree(redis);
			}
			
			gettimeofday(&tv_end, NULL);
			int time_cost = ((tv_end.tv_sec-tv_begin.tv_sec)*1000 + (tv_end.tv_usec-tv_begin.tv_usec)/1000);
			LOG_INFO("REDIS MGET ALL %d ms", time_cost);

			split_char = db_info_struct_.value_split_first_char_;
			second_split_char = db_info_struct_.value_split_second_char_;

			return 1;

		}

		int s_mget_i(uint16_t type_id, const char* n_keys[], const uint32_t key_num, 
				MapStrResult& map_result, char& split_char, char& second_split_char){

			struct timeval tv_begin, tv_end;
			gettimeofday(&tv_begin, NULL);

			MapSplitIpPort map_ip_port;
			// 由于请求是字符串，暂不支持hash规则，故随机选取ip和端口
			uint64_t tmp_uids[] = {0, 0};
			gets_ip_port(tmp_uids, 1, map_ip_port);
	
			for(MapSplitIpPort::iterator it = map_ip_port.begin(); it != map_ip_port.end(); it++){

				struct timeval inter_tv_begin, inter_tv_end;
				gettimeofday(&inter_tv_begin, NULL);

				SvrIpPort svr_ip_port = (*it).first;	
				redisContext* redis = get_redis_raw(svr_ip_port);

				uint16_t total_index = svr_ip_port.ip_index_ + svr_ip_port.port_index_ * ip_num_;
				char* redis_ip = vec_redis_server_[total_index].ip_;
				uint32_t redis_port = vec_redis_server_[total_index].port_;

                if(redis == NULL){
					LOG_ERROR("CREATE redis %s:%"PRIu32" fail!", redis_ip, redis_port);
					continue;
				}
				
				// 选定数据库
				redisReply *reply = NULL;
				if(db_info_struct_.db_index_ != 0){
					reply = (redisReply*)redisCommand(redis, "SELECT %d", db_info_struct_.db_index_);
					if(NULL == reply){
						if(redis->err){
							LOG_ERROR("SELECT error:%s", redis->errstr);
							redisFree(redis);
							return -3;
						}

						reply = (redisReply*)redisCommand(redis, "SELECT %d", db_info_struct_.db_index_);
						if(reply == NULL){
							LOG_ERROR("SELECT %d redis reply is NULL!", db_info_struct_.db_index_);
							redisFree(redis);
							return -2;
						}
					}
					freeReplyObject(reply);
				}

				// 发送获取数据命令
				char mget[REDIS_COMPRESS_LEN];
				char* p_mget = mget;
				int len = snprintf(p_mget, WORD_LEN, "%s",  "MGET");
				p_mget = p_mget + len;

				uint32_t i = 0;

				for(i = 0; i < key_num; i ++){
					len = snprintf(p_mget, WORD_LEN, " %s", n_keys[i]);
					p_mget = p_mget + len;
				}

				*p_mget = '\0';

				reply = (redisReply*)redisCommand(redis, mget);
				
				if(reply == NULL){
					LOG_ERROR("MGET %s:%"PRIu32" reply is NULL %s", redis_ip, redis_port, mget);
					redisFree(redis);
					continue;
				}

				for(i = 0; i < reply->elements; i ++) {
					char *result = reply->element[i]->str;
					long len = reply->element[i]->len;

					if (len == 0 || result == NULL)
						continue;

                    if(un_compress_index_ >= REDIS_COMPRESS_NUM){
						//LOG_DEBUG("out of num!");
                        break;
					}
					if(db_info_struct_.key_value_type_ == 0 && db_info_struct_.value_string_type_ == 0){
						if(db_info_struct_.compress_flag_){
							uLongf un_compr_len = REDIS_COMPRESS_LEN;
							try{
								int err = uncompress((Bytef*)un_compress_[un_compress_index_], &un_compr_len, 
									(Bytef*)result, len);

								if (err != Z_OK){
									LOG_ERROR("%s uncompress error!:%d:%d", n_keys[i], err, un_compress_index_);
									un_compress_index_ ++;
									continue;
								}
							}catch(...){
								LOG_ERROR("uncompress error!");
							}
							un_compress_[un_compress_index_][un_compr_len] = '\0';
							char* p_result = un_compress_[un_compress_index_];
							un_compress_index_ ++;
						
							map_result.insert(MapStrResult::value_type(n_keys[i], p_result));

						}else{
							strncpy(un_compress_[un_compress_index_], result, REDIS_COMPRESS_LEN);
							//memcpy(un_compress_[un_compress_index_], result, REDIS_COMPRESS_LEN);
							char* p_result = un_compress_[un_compress_index_];
							un_compress_index_ ++;

                            map_result.insert(MapStrResult::value_type(n_keys[i], p_result));
						}
					}
					else{
						strncpy(un_compress_[un_compress_index_], result, REDIS_COMPRESS_LEN);
						//memcpy(un_compress_[un_compress_index_], result, REDIS_COMPRESS_LEN);
                        char* p_result = un_compress_[un_compress_index_];
						un_compress_index_ ++;

                        map_result.insert(MapStrResult::value_type(n_keys[i], p_result));
					}
				}

				gettimeofday(&inter_tv_end, NULL);
				int time_cost = ((inter_tv_end.tv_sec - inter_tv_begin.tv_sec) * 1000 + (inter_tv_end.tv_usec - inter_tv_begin.tv_usec) / 1000 );
				LOG_INFO("REDIS %s:%"PRIu32" SMGET %d %d ms", redis_ip, redis_port, key_num, time_cost);

				freeReplyObject(reply);
				redisFree(redis);
			}
			split_char = db_info_struct_.value_split_first_char_;
			second_split_char = db_info_struct_.value_split_second_char_;

			gettimeofday(&tv_end, NULL);
			int time_cost = ((tv_end.tv_sec-tv_begin.tv_sec)*1000 + (tv_end.tv_usec-tv_begin.tv_usec)/1000);
			LOG_INFO("REDIS SMGET ALL %d ms", time_cost);

			return 1;
		}

	private:
		vector<Redis_Struct> vec_redis_server_;	
};

#endif
