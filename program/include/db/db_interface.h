#ifndef _DB_INTERFACE_HEADER_
#define _DB_INTERFACE_HEADER_
#include "base_define.h"
#include "utility.h"
#include "woo/log.h"
#include "ini_file.h"
#include <map>
#include <ext/hash_map>
#include <vector>
#include <string>
#include <set>


using namespace std;

typedef enum{
	REDIS_DB_TYPE,
	WOO_DB_TYPE,
	API_DB_TYPE,
	MC_DB_TYPE,
} DB_TYPE;

typedef struct _Db_Info_Struct{
	char db_name_[WORD_LEN];
	char svr_ip_[PATH_MAX];
	char svr_port_[PORT_LEN];
	uint16_t db_index_;
	DB_TYPE db_type_;
	uint16_t db_id_;
	uint16_t timeout_;
	uint8_t select_type_; 
	uint8_t ip_type_; // 0 is for CNC; 1 is for CT
	//0 is for common, 1 is for port hash, 2 is for ip hash, 3 is for ip and port hash
	char ip_split_first_char_;
	char ip_split_second_char_;
    char port_split_char_;

	uint8_t key_value_type_;
	//0 is for string, 1 is for redis list, 2 is for redis set
	uint8_t value_string_type_; 
	//0 is for string, 1 is for list, 2 is for dict, 3 is for json
	char value_split_first_char_;
	char value_split_second_char_;

	uint8_t compress_flag_;
    //0 is for not compress, 1 is for compress
	//for api db
	char user_passwd_[MAX_BUFFER];
	bool passwd_flag_;
	bool token_flag_;


	//to control mget thread num and type
	uint8_t mget_type_;
	uint8_t mget_thread_num_;

	public:
		_Db_Info_Struct(){}
		_Db_Info_Struct(const _Db_Info_Struct& db_info_struct){
			strcpy(db_name_, db_info_struct.db_name_);
			strcpy(svr_ip_, db_info_struct.svr_ip_);
			strcpy(svr_port_, db_info_struct.svr_port_);
			db_index_ = db_info_struct.db_index_;
			db_type_ = db_info_struct.db_type_;
			db_id_ = db_info_struct.db_id_;
			timeout_ = db_info_struct.timeout_;
			select_type_ = db_info_struct.select_type_;
			ip_type_ = db_info_struct.ip_type_;
			ip_split_first_char_ = db_info_struct.ip_split_first_char_;
			port_split_char_ = db_info_struct.port_split_char_;
			key_value_type_ = db_info_struct.key_value_type_;
			value_string_type_ = db_info_struct.value_string_type_;
			compress_flag_ = db_info_struct.compress_flag_;
			value_split_first_char_ = db_info_struct.value_split_first_char_;
			value_split_second_char_ = db_info_struct.value_split_second_char_;	

			passwd_flag_ = db_info_struct.passwd_flag_;
			token_flag_ = db_info_struct.token_flag_;
			strcpy(user_passwd_, db_info_struct.user_passwd_);

			mget_type_ = db_info_struct.mget_type_;
			mget_thread_num_ = db_info_struct.mget_thread_num_;
		}

		int load_config(const char* file_name, const char* db_name){
			if(NULL == file_name || NULL == db_name){
				return -1;
			}
			strcpy(db_name_, db_name);
			
			if( !read_profile_string(db_name, "SVR_IP", 
					svr_ip_, PATH_MAX, "", file_name))
    	    {
				LOG_ERROR("load config svr_ip!");
				return -1;
        	}

			if( !read_profile_string(db_name, "SVR_PORT",
                    svr_port_, PORT_LEN, "6390,6391", file_name))
			{
				LOG_ERROR("load config svr_port!");
                return -1;
			}

			db_index_ = read_profile_int(db_name, "DB_INDEX",
                    0, file_name);

			db_type_ = (DB_TYPE)read_profile_int(db_name, "DB_TYPE",
                    0, file_name);

			db_id_ = read_profile_int(db_name, "DB_ID",
                    0, file_name);

			timeout_ = read_profile_int(db_name, "TIMEOUT",
					50, file_name);

			select_type_ = read_profile_int(db_name, "SELECT_TYPE",
                    3, file_name);	

			compress_flag_ = read_profile_int(db_name, "COMPRESS_FLAG",
                    0, file_name);
				
			ip_type_ = read_profile_int(db_name, "IP_TYPE",
                    0, file_name);

			ip_split_first_char_ = read_profile_int(db_name, "IP_SPLIT_FIRST_CHAR",
                    59, file_name);	
			
			ip_split_second_char_ = read_profile_int(db_name, "IP_SPLIT_SECOND_CHAR",
                    44, file_name);	


			port_split_char_ = read_profile_int(db_name, "PORT_SPLIT_CHAR",
                    44, file_name);

			key_value_type_ = read_profile_int(db_name, "KEY_VALUE_TYPE",
                    0, file_name);

			value_string_type_ = read_profile_int(db_name, "VALUE_STRING_TYPE",
                    0, file_name);

			value_split_first_char_ = read_profile_int(db_name, "VALUE_SPLIT_FIRST_CHAR",
                    44, file_name);

			value_split_second_char_ = read_profile_int(db_name, "VALUE_SPLIT_SECOND_CHAR",
                    58, file_name);
	
			if(db_type_ == API_DB_TYPE){
				read_profile_string(db_name, "USER_PASSWD",
						user_passwd_, MAX_BUFFER, "\0", file_name);

				passwd_flag_ = read_profile_int(db_name, "API_PASSWD_FLAG", 0, file_name) == 1 ? true : false;

				token_flag_ = read_profile_int(db_name, "API_TOKEN_FLAG", 0, file_name) == 1 ? true : false; 
			}
            
			mget_type_ = read_profile_int(db_name, "MGET_TYPE", 
					1, file_name);
			mget_thread_num_ = read_profile_int(db_name, "MGET_THREAD_NUM",
					64, file_name);

			return 1;

		}
	
} Db_Info_Struct;

typedef struct _SvrIpPort {
	_SvrIpPort(){
		ip_index_ = 0;
		port_index_ = 0;
	}
	uint16_t ip_index_;
	uint16_t port_index_;

	void print(){
		LOG_DEBUG("%d:%d\n", ip_index_, port_index_);
	}
} SvrIpPort;

struct Hash_Key{
	size_t operator()(const SvrIpPort& svr_ip_port)const{
		return size_t((svr_ip_port.ip_index_ << 16) | svr_ip_port.port_index_);
	}
};

struct Equal_Key{
    bool operator()(const SvrIpPort& svr_ip_port_a, const SvrIpPort& svr_ip_port_b)const{
        return (svr_ip_port_a.ip_index_ == svr_ip_port_b.ip_index_ && 
				svr_ip_port_a.port_index_ == svr_ip_port_b.port_index_);
    }
};


typedef struct _ReqStruct{
public:
	_ReqStruct():num_(0){}
	uint64_t n_requsts_[PATH_MAX];
	uint32_t num_;
	int step_;

} ReqStruct;

typedef __gnu_cxx::hash_map<SvrIpPort, ReqStruct, Hash_Key, Equal_Key> MapSplitIpPort;

typedef map<uint64_t, const char*> MapMResult;
void release_map_m_result(MapMResult& map_m_result) {
	map_m_result.clear(); 
} 

typedef map<const char*, const char*> MapStrResult;
void release_map_str_result(MapStrResult& map_str_result) {
	map_str_result.clear(); 
} 

typedef pair<uint64_t, const char*> PairIC;
typedef vector<PairIC> VecMResult;

void print_map_mresult(const MapMResult& map_m_result){
	for(map<uint64_t, const char*>::const_iterator it = map_m_result.begin();
			it != map_m_result.end(); it++){
		if((*it).second  != NULL)
			LOG_DEBUG("%"PRIu64":%s", (*it).first, (*it).second);
	}
}

typedef set<uint64_t> MapKeyType;
void release_map_key_type(MapKeyType& map_key_type){
	map_key_type.clear();
}

class DbInterface{
	public:
		DbInterface(const Db_Info_Struct& db_info_struct):db_info_struct_(db_info_struct), ip_num_(0), 
			port_num_(0), un_compress_index_(0){
			vector<string> vec_str_ip_temp;
			split_string(vec_str_ip_temp, db_info_struct.svr_ip_, db_info_struct.ip_split_first_char_);

			for(vector<string>::iterator it = vec_str_ip_temp.begin(); it != vec_str_ip_temp.end(); it++){
				vector<string> vec_pair;
			
				split_string(vec_pair, (*it), db_info_struct.ip_split_second_char_);

				if(vec_pair.size() < (size_t)(db_info_struct.ip_type_ + 1))
					continue;

				vec_str_ip_.push_back(vec_pair[db_info_struct.ip_type_]);
			}

			ip_num_ = vec_str_ip_.size();

			split_string(vec_str_port_, db_info_struct.svr_port_, db_info_struct.port_split_char_);
			port_num_ = vec_str_port_.size();

		}
		virtual ~DbInterface(){
			vec_str_ip_.clear();
			vec_str_port_.clear();
		}

	public:
		void initialize(){
			un_compress_index_ = 0;
		}
	
	protected :
		Db_Info_Struct db_info_struct_; //db info
		uint16_t ip_num_;
		uint16_t port_num_;
		int un_compress_index_;
		char un_compress_[COMPRESS_NUM][COMPRESS_LEN];
		
		vector<string> vec_str_ip_;
		vector<string> vec_str_port_;

	protected:
		bool compress_add(){
			if(un_compress_index_ >= COMPRESS_NUM - 1)
				return false;
			else{
				un_compress_index_ ++;
				return true;
			}
		}
		virtual uint64_t Hash_Func(const char*& n_key){
			uint64_t n_key_id = strtoull(n_key, NULL, 10);
			return n_key_id;
		}

		int gets_ip_port(uint64_t n_keys[], uint32_t num, MapSplitIpPort& map_ip_port){
			switch(db_info_struct_.select_type_){ // for common, random ip, random port
				case 0:
					{
						SvrIpPort svr_ip_port;
						srand((unsigned)time(NULL));
						svr_ip_port.ip_index_ = rand() % ip_num_;
						svr_ip_port.port_index_ = rand() % port_num_;

						ReqStruct req_struct;	
						for(uint32_t i = 0; i < num; i++){
							req_struct.n_requsts_[req_struct.num_] = n_keys[i];
							req_struct.num_ ++;
						}

						map_ip_port.insert(make_pair(svr_ip_port, req_struct));
					}
					return 1;
				
				case 1:
					{
						srand((unsigned)time(NULL));
						uint16_t ip_index = rand() % ip_num_;

						for(uint32_t i = 0; i < num; i++){
							SvrIpPort svr_ip_port;
							svr_ip_port.ip_index_ = ip_index;
							uint64_t n_key_id = n_keys[i];
							svr_ip_port.port_index_ = n_key_id % port_num_;

							MapSplitIpPort::iterator pos = map_ip_port.find(svr_ip_port);
							if(pos != map_ip_port.end()){
								(*pos).second.n_requsts_[(*pos).second.num_] = n_keys[i];
								(*pos).second.num_ ++;
							}
							else{
								ReqStruct req_struct;
								req_struct.n_requsts_[0] = n_keys[i];
								req_struct.num_ ++;
								map_ip_port.insert(make_pair(svr_ip_port, req_struct));
							}
						}
					}
					return 1;

				case 2:
					{
						srand((unsigned)time(NULL));
						uint16_t port_index = rand() % port_num_;

						for(uint32_t i = 0; i < num; i++){
							SvrIpPort svr_ip_port;
							uint64_t n_key_id = n_keys[i];
							svr_ip_port.ip_index_ = n_key_id % ip_num_;
							svr_ip_port.port_index_ = port_index;

							MapSplitIpPort::iterator pos = map_ip_port.find(svr_ip_port);
                            if(pos != map_ip_port.end()){
								(*pos).second.n_requsts_[(*pos).second.num_] = n_keys[i];
                                (*pos).second.num_ ++;
                            }
                            else{
								ReqStruct req_struct;
                                req_struct.n_requsts_[0] = n_keys[i];
                                req_struct.num_ ++;
                                map_ip_port.insert(make_pair(svr_ip_port, req_struct));
                            }
						}
					}
					return 1;

				case 3:
					{
						for(uint32_t i = 0; i < num; i++){
							SvrIpPort svr_ip_port;
							uint64_t n_key_id = n_keys[i];
							svr_ip_port.ip_index_ = n_key_id % ip_num_;
							svr_ip_port.port_index_ = (n_key_id / ip_num_) % port_num_;

							MapSplitIpPort::iterator pos = map_ip_port.find(svr_ip_port);
                            if(pos != map_ip_port.end()){
								(*pos).second.n_requsts_[(*pos).second.num_] = n_keys[i];
                                (*pos).second.num_ ++;
                            }
                            else{
								ReqStruct req_struct;
                                req_struct.n_requsts_[0] = n_keys[i];
                                req_struct.num_ ++;
                                map_ip_port.insert(make_pair(svr_ip_port, req_struct));
                            }
						}
					}
					return 1;

				default:
					{
						SvrIpPort svr_ip_port;
						svr_ip_port.ip_index_ = 0;
						svr_ip_port.port_index_ = 0;

						ReqStruct req_struct;
                        for(uint32_t i = 0; i < num; i++){
                            req_struct.n_requsts_[req_struct.num_] = n_keys[i];
                            req_struct.num_ ++;
                        }

                        map_ip_port.insert(make_pair(svr_ip_port, req_struct));
					}
					return 0;
			}

			return -1;
		}

		int get_ip_port(uint64_t n_key,  SvrIpPort& svr_ip_port){
			uint64_t n_key_id = 0;

			switch(db_info_struct_.select_type_){ // for common, random ip, random port
				case 0:
					srand((unsigned)time(NULL));
					svr_ip_port.ip_index_ = rand() % ip_num_;
					svr_ip_port.port_index_ = rand() % port_num_;

					return 1;
				case 1:
					srand((unsigned)time(NULL));
					svr_ip_port.ip_index_ = rand() % ip_num_;
					n_key_id = n_key;
					svr_ip_port.port_index_ = n_key_id % port_num_;

					return 1;
				case 2:
					srand((unsigned)time(NULL));
					n_key_id = n_key;
					svr_ip_port.ip_index_ = n_key_id % ip_num_;
					svr_ip_port.port_index_ = rand() % port_num_;

					return 1;
				case 3:
					n_key_id = n_key;
					svr_ip_port.ip_index_ = n_key_id % ip_num_;
					svr_ip_port.port_index_ = (n_key_id / ip_num_) % port_num_;
				
					return 1;
				case 4:
					n_key_id = n_key;
					svr_ip_port.ip_index_ = n_key_id % ip_num_;
					svr_ip_port.port_index_ = n_key_id % port_num_;

					return 1;
				case 5: // 一种hash规则
					srand((unsigned)time(NULL));
					n_key_id = n_key;
					svr_ip_port.ip_index_ = ((n_key_id / 10) % 24) / 4; 
					svr_ip_port.port_index_ = rand() % port_num_;

					return 1;
				case 6: // 
					srand((unsigned)time(NULL));
					n_key_id = n_key;
					svr_ip_port.ip_index_ = (n_key_id % 12) / 2;
					svr_ip_port.port_index_ = rand() % port_num_;
					 return 1;

				case 7: {
						uint32_t total_num = 0;
						n_key_id = n_key;
						while(n_key_id > 0){
							total_num += n_key_id % 10;
							n_key_id = n_key_id / 10;
						}

						svr_ip_port.ip_index_ = total_num % ip_num_;
						svr_ip_port.port_index_ = total_num % port_num_;
						return 1;
					}
				case 8: 
					srand((unsigned)time(NULL));
					n_key_id = n_key;
					svr_ip_port.ip_index_ = (n_key_id / 4) % ip_num_; 
					svr_ip_port.port_index_ = rand() % port_num_;

					return 1;
				case 9: 
					srand((unsigned)time(NULL));
					n_key_id = n_key;
					svr_ip_port.ip_index_ = (n_key_id / 10 ) % ip_num_;
					svr_ip_port.port_index_ = rand() % port_num_;
					return 1;

				default:
					svr_ip_port.ip_index_ = 0 ;
					svr_ip_port.port_index_ = 0 ;

					return 0;
			}
			svr_ip_port.ip_index_ = 0 ;
			svr_ip_port.port_index_ = 0 ;

			return 0;
		}

	public:
		// deal with item_key by bat
		virtual int mget_i(uint16_t type_id, uint64_t n_keys[], uint32_t key_num,
				uint32_t n_item_keys[], uint32_t item_key_num,
				MapMResult& map_result, char& split_char, char& second_split_char) = 0;
		
		// deal with item_key by char*
		virtual int get_i(uint16_t type_id, const char* &p_str_key, uint32_t n_item_key, char* &p_result,
				char& split_char, char& second_split_char) = 0;

		// deal with item_key
		virtual int get_i(uint16_t type_id, uint64_t n_key, uint32_t n_item_key, char* &p_result,
				char& split_char, char& second_split_char, const char* other_key = NULL, bool other_flag = false) = 0;

		// mget data
		virtual int mget(uint16_t type_id, uint64_t n_keys[], uint32_t key_num, 
				MapMResult& map_result, char& split_char, char& second_split_char) = 0;

		// mget data by char* 
		virtual int s_mget(uint16_t type_id, const char* p_str_keys[], uint32_t key_num, 
				MapStrResult& map_result, char& split_char, char& second_split_char){
			return 1;
		}

		// get data by char* modified by wcp to add n_key
		virtual int s_get(uint16_t type_id, const char* p_str_key, char* &p_result,
				char& split_char, char& second_split_char, uint64_t n_key = 0) = 0;

		// get data
		virtual int get(uint16_t type_id, uint64_t n_key, char* &p_result,
				char& split_char, char& second_split_char) = 0;
};

#endif
