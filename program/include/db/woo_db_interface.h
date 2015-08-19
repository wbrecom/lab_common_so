#ifndef _WOO_DB_INTERFACE_HEADER_
#define _WOO_DB_INTERFACE_HEADER_
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <string>
#include "utility.h"
#include "base_define.h"
#include "json.h"
#include "db_interface.h"
#include "woo/binaryclient.h"
#include <errno.h>

using namespace std;

/*
 * 使用自研的woo协议获取数据
 * */

typedef struct _Woo_Struct{
	char ip_[IP_WORD_LEN];
	uint32_t port_;
} Woo_Struct;

class WooDbInterface : public DbInterface{
	public:
		WooDbInterface(const Db_Info_Struct& db_info_struct):
			DbInterface(db_info_struct){
				// load configure

				for(vector<string>::iterator port_it = vec_str_port_.begin();
						port_it != vec_str_port_.end(); port_it++){
					uint32_t n_port = strtoul((*port_it).c_str(), NULL, 10);
					for(vector<string>::iterator ip_it = vec_str_ip_.begin();
							 ip_it != vec_str_ip_.end(); ip_it++){
						Woo_Struct woo_struct;
						strncpy(woo_struct.ip_, (*ip_it).c_str(), IP_WORD_LEN);
						woo_struct.port_ = n_port;
						vec_server_.push_back(woo_struct);

					}
				}
			}

		virtual ~WooDbInterface(){
			vec_server_.clear();
			for(vector<woo::binary_client_t*>::iterator it = vec_client_t_.begin();
					it != vec_client_t_.end(); it++){
				if((*it) != NULL)
					woo::binary_client_destroy((*it));
			}
			vec_client_t_.clear();
		}
	protected:

		int get_woo_result(uint64_t n_key, const char* req, char* &resp_buf){
			SvrIpPort svr_ip_port;
			get_ip_port(n_key, svr_ip_port);
			return get_woo_result(svr_ip_port, req, resp_buf);
		}

		int get_woo_result(const SvrIpPort& svr_ip_port, const char* req, char* &resp_buf){
			uint16_t total_index = svr_ip_port.ip_index_ + svr_ip_port.port_index_ * ip_num_;

			struct timeval tv_begin, tv_end;
			gettimeofday(&tv_begin, NULL);

			int64_t log_id = woo::get_log_id();//1234567;
			
			resp_buf = un_compress_[un_compress_index_];
			if(!resp_buf) {
				return -1;
			}
			un_compress_index_ ++;

			uint32_t resp_buf_len = 0;
			char servers[MAX_BUFFER];
			snprintf(servers, sizeof(servers), "%s:%"PRIu16, vec_server_[total_index].ip_,
					vec_server_[total_index].port_);

			woo::binary_client_t *cli = woo::binary_client_create(servers, db_info_struct_.timeout_ * 1000,
					db_info_struct_.timeout_ * 1000);

			if(NULL==cli){
				LOG_ERROR("CREATE woo %s fail!", servers);
				return -1;
			}

			ssize_t ret = 0;
			ret = woo::binary_client_talk(cli, req, strlen(req), resp_buf, &resp_buf_len, BUF_SIZE, log_id);

			woo::binary_client_destroy(cli);

			gettimeofday(&tv_end, NULL);
			int time_cost = (tv_end.tv_sec - tv_begin.tv_sec)*1000 + (tv_end.tv_usec - tv_begin.tv_usec)/1000;
			LOG_INFO("WOO %s GET %d ms", servers, time_cost);
			return ret;
		}

	public :
		int s_get(uint16_t type_id, const char* p_str_key, char* &p_result, 
				char& split_char, char& second_split_char, uint64_t n_key = 0){ 
			//return get_i(type_id, p_str_key, 0, p_result, split_char, second_split_char);
			return get_i(type_id, n_key, 0, p_result, split_char, second_split_char, p_str_key, true);
		}

		// deal with item_key by char*
		int get_i(uint16_t type_id, const char* &p_str_key, uint32_t n_item_key, char* &p_result,
				char& split_char, char& second_split_char){

			initialize();

			get_woo_result(0, p_str_key, p_result);
			split_char = db_info_struct_.value_split_first_char_;
			second_split_char = db_info_struct_.value_split_second_char_;

			return 1;
		}

		int get(uint16_t type_id, uint64_t n_key, char* &p_result, char& split_char, char& second_split_char){
			return get_i(type_id, n_key, 0, p_result, split_char, second_split_char);			
		}

		int get_i(uint16_t type_id, uint64_t n_key, uint32_t n_item_key, char* &p_result,
				char& split_char, char& second_split_char, const char* other_key = NULL, bool other_flag = false) {
			initialize();
		
			if(!other_flag){
				char req[WORD_LEN];
				snprintf(req, WORD_LEN, "{\"id\":%"PRIu64"}", n_key);
				get_woo_result(n_key, req, p_result);
			}else{
				get_woo_result(n_key, other_key, p_result);
			}

			split_char = db_info_struct_.value_split_first_char_;
			second_split_char = db_info_struct_.value_split_second_char_;

			return 1;
		}

		int mget(uint16_t type_id, uint64_t n_keys[], uint32_t key_num, MapMResult& map_result,
				char& split_char, char& second_split_char){

			uint32_t n_item_keys[1];
			initialize();
			return mget_i(type_id, n_keys, key_num, n_item_keys, 0, map_result, 
					split_char, second_split_char);
		}

		int mget_i(uint16_t type_id, uint64_t n_keys[], uint32_t key_num, 
				uint32_t n_item_keys[], uint32_t item_key_num,
				MapMResult& map_result, char& split_char, char& second_split_char){

			MapSplitIpPort map_ip_port;
			gets_ip_port(n_keys, key_num, map_ip_port);
			for(MapSplitIpPort::iterator it = map_ip_port.begin(); it != map_ip_port.end(); it++){
				SvrIpPort svr_ip_port = (*it).first;
				ReqStruct& req_struct = (*it).second;

				for(uint32_t i = 0; i < req_struct.num_; i ++){
					char req[WORD_LEN];
					snprintf(req, WORD_LEN, "{\"uids\":[%"PRIu64"]}", req_struct.n_requsts_[i]);
					char* p_result = NULL;
					get_woo_result(svr_ip_port, req, p_result);

					map_result.insert(MapMResult::value_type(req_struct.n_requsts_[i], p_result));
				}
			}

			split_char = db_info_struct_.value_split_first_char_;
			second_split_char = db_info_struct_.value_split_second_char_;
			return 1;
		}

		//return ip list number to get key_num
		uint16_t get_ip_num(){
			return ip_num_;
		}

		//added by wcp
		int smget_i(uint16_t type_id, char* p_keys[], uint32_t key_num,
				MapMResult& map_result, char& split_char, char& second_split_char){

			initialize();
			MapSplitIpPort map_ip_port;

			//这里有一个比较特殊的技巧，需要key_num会和后台的服务器数量一致
			//或者小于它: key_num <= ip list number
			//同时需要保证select_type_ = 2进行hash化
			//n_keys是人为放入的，对于后台的服务器也需要看成是完全相同的
			//
			if(key_num > ip_num_){
				LOG_ERROR("key_num is more than ip list number!");
				key_num = ip_num_;
			}

			uint64_t n_keys[key_num];//0,1,2,....key_num - 1
			for(uint32_t index = 0; index < key_num; index++){
				n_keys[index] = index;
			}

			this->db_info_struct_.select_type_ = 2;//保证hash化
			gets_ip_port(n_keys, key_num, map_ip_port);

			fd_set rfds;
			int nfds = -1;
			FD_ZERO(&rfds);
			struct timeval tv;
			tv.tv_sec = 0;
			tv.tv_usec = db_info_struct_.timeout_ * 1000;

			woo::binary_client_t *cli[key_num];

			uint32_t total_i = 0;
			int64_t log_id = woo::get_log_id();
			for(MapSplitIpPort::iterator it = map_ip_port.begin(); it != map_ip_port.end(); it ++){
				if(total_i >= key_num)
					break;

				SvrIpPort svr_ip_port = (*it).first;
				uint16_t total_index = svr_ip_port.ip_index_ + svr_ip_port.port_index_ * ip_num_;

				char servers[MAX_BUFFER];
				snprintf(servers, sizeof(servers), "%s:%"PRIu16, vec_server_[total_index].ip_,
						vec_server_[total_index].port_);

				cli[total_i] = woo::binary_client_create(servers, db_info_struct_.timeout_ * 1000,
						db_info_struct_.timeout_ * 1000);

				if(NULL == cli[total_i]){
					LOG_ERROR("client create is error!:%s", servers);
					continue;
				}

				if(NULL == p_keys[total_i])
					continue;

				int ret = binary_client_send(cli[total_i], p_keys[total_i], strlen(p_keys[total_i]) + 1,
						log_id, true);

				if(ret < 0){
					LOG_ERROR("send is error!:%d errorno:%d", ret, errno);
					continue;
				}

				total_i ++;
			}

			if(key_num != total_i){
				LOG_ERROR("map ip port is not equal to key_num");
				LOG_INFO("%"PRIu32":%"PRIu32, key_num, total_i);
			}

			while(1){
				nfds = -1;
				FD_ZERO(&rfds);

				for(uint32_t index = 0 ; index < total_i; index++){
					binary_client_fd_set(cli[index], &rfds, &nfds);
				}

				if (nfds < 0) {
					LOG_DEBUG("no binary client need select");
					break;
				}
				
				int ret = select(1 + nfds, &rfds, NULL, NULL, &tv);
				if(ret < 0){
					break;
				}else if (ret == 0){
					LOG_ERROR("select timeout!");
					//return ret;
					break;
				}else{
					for(uint32_t index = 0; index < total_i; index ++){
						char* resp_buf = un_compress_[un_compress_index_];
						if(!resp_buf) {
							LOG_ERROR("resp is error!");
							//return -1;
							continue;
						}

						uint32_t resp_len = 0;
						if(binary_client_fd_isset(cli[index], &rfds) == 0){
							ret = binary_client_recv(cli[index], resp_buf, &resp_len, COMPRESS_LEN, NULL, false);
							if(ret == 0){
								//LOG_INFO("%"PRIu32":%s", index, resp_buf);
								map_result.insert(MapMResult::value_type(index, resp_buf));
								un_compress_index_ ++;
							}
						}
					}
				}
			}

			for(uint32_t index = 0 ; index < total_i; index++){
				if(cli[index] != NULL)
					binary_client_destroy(cli[index]);
			}

			return 1;
		}
	private:
		int binary_client_fd_isset(woo::binary_client_t *client, fd_set *rfds) {
			if (woo::binary_client_sock(client) >= 0) { 
				if (FD_ISSET(woo::binary_client_sock(client), rfds)) {
					return 0;
				}	
			}
			return -1;
		}

		int binary_client_fd_set(woo::binary_client_t *client, fd_set *rfds, int *nfds) {
			if (woo::binary_client_sock(client) >= 0) { 
				FD_SET(woo::binary_client_sock(client), rfds); 
				if (*nfds <= woo::binary_client_sock(client)) { 
					*nfds = woo::binary_client_sock(client); 
				} 
				return 0;
			} else {
				return -1;
			}
		}
    protected:
		vector<Woo_Struct> vec_server_;
		vector<woo::binary_client_t*> vec_client_t_;

};

#endif


