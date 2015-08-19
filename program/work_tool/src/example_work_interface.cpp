#include "example_work_interface.h"

DYN_WORK(ExampleWorkInterface);

int ExampleWorkInterface::work_core(json_object *req_json, 
		char* &p_out_string, int& n_out_len, int64_t req_id){
	int result = 0;

	json_object* cmd_json = json_object_object_get(req_json, "cmd");
	char* cmd_str = NULL;
	if(NULL == cmd_json){
		cmd_str = "query";
	}else{
		cmd_str = (char*)json_object_get_string(cmd_json);
	}
	
	if(strcasecmp(cmd_str, "query") == 0){
		char* p_out_temp = p_out_string;

		json_object* body_json = json_object_object_get(req_json, "body");
		char* body_str = NULL;
		if(NULL == body_json){
			body_str = "hello word!";
		}else{
			body_str = (char*)json_object_get_string(body_json);
		}

		int len = sprintf(p_out_temp, "%s", body_str);
		n_out_len = len;
	}
	return result;
}
