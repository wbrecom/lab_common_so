#ifndef _CMN_FEATURE_POOL_HEADER_
#define _CMN_FEATURE_POOL_HEADER_

#include "interface_featurepool.h"
#define MAX_BUFFER 102400
#define ARRAY_NUM 64

/*在sorted set之上的一个封装，同时继承interfacefeaturepool*/
/*现在的这个实例，主要采用json格式进行接口处理*/
class CmnFeaturePool : public InterfaceFeaturePool{

public:
	CmnFeaturePool(std::map<std::string, std::string>& p_db_name, const char* p_dump_folder) : 
	InterfaceFeaturePool(p_db_name, p_dump_folder){
	}

	~CmnFeaturePool(){
	}

//inherit from parent class
public:
	int pool_oper_core_query(const char* p_req_body, char* &p_out_string, int& n_out_len);
	int pool_oper_core_control(const char* p_req_body, char* &p_out_string, int& n_out_len);
	int load(const char* p_file_name, const char *dbid);
	int reload(const char *dbid);
	int pool_oper_core_query(json_object *req_json, char* &p_out_string, json_object* &new_json);

private:
	/*根据不同情况，加载外存文件，有别于序列化*/
	int load_file(const char* p_file_name, FeaturePoolInstance* p_featurepool);
	int load_user_file(const char* p_file_name, FeaturePoolInstance* p_featurepool);

	int get_attribute(MAIN_KEY_TYPE& key, ITEM_KEY_TYPE& item_key, ITEM_VALUE_TYPE& score, 
			json_object* req_json, int type = 1);
	int get_attribute_added(MAIN_KEY_TYPE& key, ITEM_KEY_TYPE& item_key, ADD_VALUE_TYPE& score, 
			json_object* req_json, int type = 1);

// inherit from parent class
private:
	int oper_core_size(char* &p_out_string, json_object *req_json, json_object* &new_json,
		FeaturePoolInstance* const p_featurepool);
	int oper_core_incrby(char* &p_out_string, json_object *req_json, json_object* &new_json,
		FeaturePoolInstance* const p_featurepool);
	int oper_core_incrbys(char* &p_out_string, json_object *req_json, json_object* &new_json,
        FeaturePoolInstance* const p_featurepool);
	int oper_core_mincrbys(char* &p_out_string, json_object *req_json, json_object* &new_json,
        FeaturePoolInstance* const p_featurepool);
	int oper_core_contains(char* &p_out_string, json_object *req_json, json_object* &new_json,
		FeaturePoolInstance* const p_featurepool);
	int oper_core_exists(char* &p_out_string, json_object *req_json, json_object* &new_json,
		FeaturePoolInstance* const p_featurepool);
	int oper_core_add(char* &p_out_string, json_object *req_json, json_object* &new_json,
		FeaturePoolInstance* const p_featurepool);
	int oper_core_adds(char* &p_out_string, json_object *req_json, json_object* &new_json,
        FeaturePoolInstance* const p_featurepool);
	int oper_core_rem(char* &p_out_string, json_object *req_json, json_object* &new_json,
		FeaturePoolInstance* const p_featurepool);
	int oper_core_get(char* &p_out_string, json_object *req_json, json_object* &new_json,
		FeaturePoolInstance* const p_featurepool);	
	int oper_core_gets(char* &p_out_string, json_object *req_json, json_object* &new_json,
        FeaturePoolInstance* const p_featurepool);
	int oper_core_mgets(char* &p_out_string, json_object *req_json, json_object* &new_json,
        FeaturePoolInstance* const p_featurepool);
	int oper_core_mgets_i(char* &p_out_string, json_object *req_json, json_object* &new_json,
        FeaturePoolInstance* const p_featurepool);
	int oper_core_update(char* &p_out_string, json_object *req_json, json_object* &new_json,
    	FeaturePoolInstance* const p_featurepool);	
	int oper_core_updates(char* &p_out_string, json_object *req_json, json_object* &new_json,
        FeaturePoolInstance* const p_featurepool);
	int oper_core_mupdates(char* &p_out_string, json_object *req_json, json_object* &new_json,
        FeaturePoolInstance* const p_featurepool);

	int return_json_str(char* &p_out_string, int n_return_code,
			    json_object *req_json, json_object* &new_json);

	const char *get_req_dbid(json_object* req_json)
	{
		json_object *dbid_json = json_object_object_get(req_json, "dbid");
		if (dbid_json == NULL)
			return NULL;

		const char *dbid_str = json_object_get_string(dbid_json); 
		return dbid_str;
	}
//
};
#endif
