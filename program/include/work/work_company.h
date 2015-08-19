#ifndef _WORK_COMPANY_HEADER_
#define _WORK_COMPANY_HEADER_

#include "db_company.h"
#include "work_interface_factory.h"
#include "ini_file.h"
#include "woo/log.h"
#include <map>
#include <set>
#include <vector>
#include <fstream>
#include <iostream>
#include "json.h"
#include <time.h>

class WorkCompany{
	public:
		WorkCompany(DbCompany*& p_db_company):work_interface_factory_(NULL){
			work_interface_factory_ = new WorkInterfaceFactory(p_db_company);	
		}

		~WorkCompany(){
			if( work_interface_factory_ != NULL){
				delete work_interface_factory_;
				work_interface_factory_ = NULL;
			}
		}

	private:
		WorkInterfaceFactory* work_interface_factory_;

	public:
		int work_core(const char*& request_str, char* &p_out_string, int& n_out_len, uint64_t req_id){
			if(NULL == request_str){
				return_fail(405, "request str is NULL!", p_out_string, n_out_len); //modified by wcp
                return 0;
			}

			// deal with json
            json_object* req_json = json_tokener_parse(request_str);
			if (NULL == req_json){
				LOG_ERROR("%s", "json is error!");
				return_fail(405, "json is error!", p_out_string, n_out_len); //modified by wcp
				return -1;
			}

            if(is_error(req_json)){
				LOG_ERROR("%s", request_str);
				return_fail(405, "parse request error!", p_out_string, n_out_len); //modified by wcp
				json_object_put(req_json);
                return 0;
            }

			json_object* api_json = json_object_object_get(req_json, "api");
			char* api_str = NULL;
			if(NULL == api_json){
				return_fail(405, "need api!", p_out_string, n_out_len); //modified by wcp
				json_object_put(req_json);
				return -1;
			}else{
				api_str = (char*)json_object_get_string(api_json);
			}

			int result = 0;
            WorkInterface* work_interface  = work_interface_factory_->get_interface(api_str);
			if( work_interface == NULL){
				return_fail(406, "can not support api!", p_out_string, n_out_len);
				json_object_put(req_json);
				return -1;
			}else{
				result = work_interface->work_core(req_json, p_out_string, n_out_len, req_id);
			}

			json_object_put(req_json);
			return result;
		}
	public:
		int initialize(const char* work_config){

			if(work_interface_factory_ != NULL){
				return work_interface_factory_->initialize(work_config);
			}
			return 0;
		}

};
#endif
