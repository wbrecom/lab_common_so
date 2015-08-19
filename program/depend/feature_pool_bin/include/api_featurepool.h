#ifndef _API_FEATURE_POOL_HEADER_
#define _API_FEATURE_POOL_HEADER_

#include "virtual_featurepool.h"

class ApiFeaturePool : public VirtualFeaturePool{

public:
	ApiFeaturePool();

	~ApiFeaturePool();

public:
	int initialize(const char* path);
	int load(const char*);

public:
	int size(int& size, const char* dbid);

	int incrby(MAIN_KEY_TYPE key, ITEM_KEY_TYPE code, 
			ADD_VALUE_TYPE add_score, const char* dbid){
		return 0;
	}

	int incrbys(ItemResultMap& result_map, MAIN_KEY_TYPE key, 
			const CodeItemMapAdd& code_item_map, const char* dbid){
		return 0;
	}

	int mincrbys(KeyItemResultMap& key_item_result_map, 
			const KeyCodeItemMapAdd& key_code_item_map, const char* dbid){
		return 0;
	}

	int contains(bool& flag, MAIN_KEY_TYPE key, const char* dbid);

	int exists(bool& flag, MAIN_KEY_TYPE key, 
			ITEM_KEY_TYPE code, const char* dbid);

	int add(MAIN_KEY_TYPE key, ITEM_KEY_TYPE code, 
			ITEM_VALUE_TYPE score, const char* dbid){
		return 0;
	}

	int adds(ItemResultMap& result_map, MAIN_KEY_TYPE key, 
			const CodeItemMap& code_item_map, const char* dbid){
		return 0;
	}

	int rem(MAIN_KEY_TYPE key, const char* dbid){
		return 0;
	}

	int get(ITEM_VALUE_TYPE& return_value, MAIN_KEY_TYPE key, 
			ITEM_KEY_TYPE code, const char* dbid);

	int gets(CodeItemMap& map_return_value, MAIN_KEY_TYPE key, 
			const ItemVec& vec_code, const char* dbid);

	int mgets(KeyCodeItemMap& map_return_value, const KeyVec& vec_key, 
			const ItemVec& vec_code, const char* dbid);

	int mgets_i(KeyCodeItemMap& map_return_value, 
			const KeyItemVec& key_item_vec, const char* dbid);

	int update(MAIN_KEY_TYPE key, ITEM_KEY_TYPE code, 
			ITEM_VALUE_TYPE new_score, const char* dbid){
		return 0;
	}

	int updates(ItemResultMap& result_map, MAIN_KEY_TYPE key, 
			const CodeItemMap& code_item_map, const char* dbid){
		return 0;
	}

	int mupdates(KeyItemResultMap& key_item_result_map, 
			const KeyCodeItemMap& key_code_item_map, const char* dbid){
		return 0;
	}
//
};
#endif
