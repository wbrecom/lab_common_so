#ifndef _API_DB_INTERFACE_HEADER_
#define _API_DB_INTERFACE_HEADER_
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <string>
#include "utility.h"
#include "base_define.h"
#include "json.h"
#include "db_interface.h"
#include <curl/curl.h>
#include <openssl/md5.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

using namespace std;

/*
 * 使用curl请求http获取资源
 * */

class ApiDbInterface : public DbInterface{
	public:
		ApiDbInterface(const Db_Info_Struct& db_info_struct):
			DbInterface(db_info_struct), handle_(NULL){
				// load configure
				output_buf_ = new _output_buf_t();
				if(output_buf_ != NULL){
					output_buf_->buf_ = NULL;
					output_buf_->size_ = 0;
					output_buf_->len_ = 0;
					output_buf_->key_ = NULL;
				}
			}

		virtual ~ApiDbInterface(){
			if(output_buf_ != NULL){
				delete output_buf_;
				output_buf_ = NULL;
			}

			if(handle_ != NULL){
				curl_easy_cleanup(handle_);
				handle_ = NULL;
			}
		}

		typedef struct _output_buf_t{
			char* buf_;
			size_t size_;
			size_t len_;
			char* key_;
		}output_buf_t;

		output_buf_t* output_buf_;
	protected:

		void md5_hex(char *out, const char *d, size_t n) {
			unsigned char md[MD5_DIGEST_LENGTH];
			MD5((const unsigned char *)d, n, md);
			for (n = 0; n < MD5_DIGEST_LENGTH; n ++) {
				out[n*2] = HEX_CHARS[md[n] >> 4];
				out[n*2 + 1]  = HEX_CHARS[md[n] & 0xf];
			}
			out[n*2] = '\0';
		}

		//unsigned char *MD5(const unsigned char *d, size_t n, unsigned char *md);	
		int get_authorization(const char* uid_str, const char* token_str, char base64_auth[], size_t& auth_len){

			//794198*/
			char uid_token[20];
			char auth[80];
			const char AUTH_PREFIX[] = "authorization: Token ";
			int AUTH_PREFIX_LEN = strlen(AUTH_PREFIX);

			int uid_token_len = snprintf(uid_token, sizeof(uid_token), "%s%s", uid_str, token_str);
			int uid_len = snprintf(auth, sizeof(auth), "%s:", uid_str);
			md5_hex(auth + uid_len, uid_token, uid_token_len);
			strcpy(base64_auth, AUTH_PREFIX);

			EVP_EncodeBlock((unsigned char *)base64_auth + AUTH_PREFIX_LEN, (unsigned char *)auth, 
					uid_len + MD5_DIGEST_LENGTH * 2);

			return 1;
		}

		static size_t write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp)
		{
			size_t real_size = nmemb * size;
			output_buf_t* output_buf = (output_buf_t*)userp;

			if(output_buf != NULL){
				memcpy(output_buf->buf_ + output_buf->len_, contents, real_size);
				output_buf->len_ += real_size;
				output_buf->buf_[output_buf->len_] = '\0';
			}
			return real_size;
		}

		/*使用curl进行获取*/
		int get_data_by_curl(const char* url, const char* request_key, 
				const char* auth_header, char*& p_result){

			CURL *curl_handle = handle_;
			CURLcode res;
			/* init the curl session */
			if(NULL == curl_handle){
				curl_handle = curl_easy_init();

				if(NULL == curl_handle){
					LOG_ERROR("curl handle is error!");
					return -1;
				}

				//set timeout and connection timeout
				curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT_MS, db_info_struct_.timeout_);
				curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT_MS, db_info_struct_.timeout_);

				//set dns cache timeout
				curl_easy_setopt(curl_handle, CURLOPT_DNS_CACHE_TIMEOUT, 60 * 60 * 48);

				//set password
				if(db_info_struct_.passwd_flag_){
					curl_easy_setopt(curl_handle, CURLOPT_USERPWD, db_info_struct_.user_passwd_);
				}
				
				/* send all data to this function  */ 
				curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_memory_callback);

				//add below to resolve timeout multi-thread questions
				curl_easy_setopt(curl_handle, CURLOPT_NOSIGNAL, 1L);
				
				//curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, curl_error_msg_);

				handle_ = curl_handle;
			}
		    
			/* specify URL to get */ 
			char request_url[COMPRESS_NUM];
			snprintf(request_url, COMPRESS_NUM, "%s%s", url, request_key);
			curl_easy_setopt(curl_handle, CURLOPT_URL, request_url);

			//set token
			struct curl_slist *headers = NULL;
			if(db_info_struct_.token_flag_){
				headers = curl_slist_append(headers, auth_header);
				curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);
			}

			//memset(un_compress_[un_compress_index_], '\0', COMPRESS_LEN);
			un_compress_[un_compress_index_][0] = '\0'; //modify a bug to reslove data error
			/* we pass our 'chunk' struct to the callback function */ 
			if(output_buf_ == NULL){
				//curl_easy_cleanup(curl_handle);
				if(headers != NULL)
					curl_slist_free_all(headers);
				LOG_ERROR("output buf is null!");
				return -1;
			}

			output_buf_->buf_ = un_compress_[un_compress_index_];
			output_buf_->size_ = COMPRESS_LEN;
			output_buf_->len_ = 0;
			output_buf_->key_ = (char*)request_key;

			curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, output_buf_);//(void *)un_compress_[un_compress_index_]);
			
			char curl_error_msg[CURL_ERROR_SIZE] = "";
			curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, curl_error_msg);

			/* get it! */ 
			res = curl_easy_perform(curl_handle);
			
			/* check for errors */ 
			if(res != CURLE_OK) {
				curl_easy_strerror(res);
				LOG_ERROR("get url: %s error, no:%d", request_url, res);
				
				if(headers != NULL)
					curl_slist_free_all(headers);

				return -1;
			}
			else {
				p_result = un_compress_[un_compress_index_];
				un_compress_index_ ++;
			}
			/* cleanup curl stuff */ 
			//curl_easy_cleanup(curl_handle);
			if(headers != NULL)
				curl_slist_free_all(headers);
		
			return 1;
		}

	public :
		int s_get(uint16_t type_id, const char* p_str_key, char* &p_result, 
				char& split_char, char& second_split_char, uint64_t n_key){ 
			return get_i(type_id, p_str_key, 0, p_result, split_char, second_split_char);
		}

		// deal with item_key by char*
		int get_i(uint16_t type_id, const char* &p_str_key, uint32_t n_item_key, char* &p_result,
				char& split_char, char& second_split_char){

			initialize();

			if(vec_str_ip_.size() == 0){
				return 0;
			}

			char uid_str[PATH_MAX];
			char token_str[PATH_MAX];

			const char* p_temp = p_str_key;
			bool token_flag = false;
			size_t index = 0;
			while(*p_temp != '\0'){
				if (*p_temp == ':'){
					 uid_str[index] = '\0';
					token_flag = true;
					index = 0;
				}else{
					if (!token_flag){
						uid_str[index] = *p_temp;
					}else
						token_str[index] = *p_temp;
					index ++;
				}
				p_temp ++;
			}
			token_str[index] = '\0';

			char auth_str[160];
			size_t auth_len = 0;
			auth_str[0] = '\0';

			struct timeval tv_begin, tv_end;
			gettimeofday(&tv_begin, NULL);

			get_authorization(uid_str, token_str, auth_str, auth_len);

			gettimeofday(&tv_end, NULL);
			int time_auth = (tv_end.tv_sec - tv_begin.tv_sec)*1000 + (tv_end.tv_usec - tv_begin.tv_usec)/1000;

			gettimeofday(&tv_begin, NULL);
			int result = get_data_by_curl(vec_str_ip_[0].c_str(), uid_str, auth_str, p_result);

			gettimeofday(&tv_end, NULL);
			int time_cost = (tv_end.tv_sec - tv_begin.tv_sec)*1000 + (tv_end.tv_usec - tv_begin.tv_usec)/1000;
			LOG_INFO("API %s AUTH %d GET %d ms", vec_str_ip_[0].c_str(), time_auth, time_cost);

			if(result <= 0){
				return result;
			}

			split_char = db_info_struct_.value_split_first_char_;
			second_split_char = db_info_struct_.value_split_second_char_;

			return 1;

		}

		int get(uint16_t type_id, uint64_t n_key, char* &p_result, char& split_char, char& second_split_char){
			return get_i(type_id, n_key, 0, p_result, split_char, second_split_char);
		}

		int get_i(uint16_t type_id, uint64_t n_key, uint32_t n_item_key, char* &p_result,
				char& split_char, char& second_split_char, const char* other_key = NULL, bool other_flag = false) {
			char req[WORD_LEN];
			snprintf(req, WORD_LEN, "%"PRIu64, n_key);

			initialize();
			if(vec_str_ip_.size() == 0)
				return 0;

			struct timeval tv_begin, tv_end;
			gettimeofday(&tv_begin, NULL);

			get_data_by_curl(vec_str_ip_[0].c_str(), req, NULL, p_result);
			split_char = db_info_struct_.value_split_first_char_;
			second_split_char = db_info_struct_.value_split_second_char_;

			gettimeofday(&tv_end, NULL);
			int time_cost = (tv_end.tv_sec - tv_begin.tv_sec)*1000 + (tv_end.tv_usec - tv_begin.tv_usec)/1000;
			LOG_INFO("API %s GET %d ms", vec_str_ip_[0].c_str(), time_cost);

			return 1;
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

	private:
    protected:
		CURL* handle_;

};

#endif


