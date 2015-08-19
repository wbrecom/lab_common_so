#ifndef _VIRTUAL_FEATURE_POOL_HEADER_
#define _VIRTUAL_FEATURE_POOL_HEADER_

#include <functional>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include "woo/log.h"
#include "feature_pool.h"
//#include "feature_pool_2.h"
#include "json.h"
#include "time.h"
#include <errno.h>
#include <sys/wait.h> 

#define DEFAULT_DB_NAME "sample_feature_pool"
#define DEFAULT_DUMP_FOLDER "./"

/*封装feature pool的interfacefeaturepool*/
/*现在的这个实例，主要采用json格式进行接口处理*/
class VirtualFeaturePool{

public:
	typedef FeaturePool FeaturePoolInstance;
public:
	VirtualFeaturePool() {
	}

	virtual ~VirtualFeaturePool(){
		for(std::map<std::string, FeaturePoolInstance*>::iterator it = p_featurepools_.begin();
				it != p_featurepools_.end(); it++){
			
			if((*it).second != NULL){
				delete (*it).second;
				(*it).second = NULL;
			}
		}
		p_featurepools_.clear();
		str_db_names_.clear();
	}

public:
	int prepare(std::map<std::string, std::string>& p_db_name, 
			const char* p_dump_folder){
	
		std::map<std::string, std::string>::iterator it;
		for(it = p_db_name.begin(); it != p_db_name.end(); it++)
		{
			str_db_names_.insert(std::pair<std::string, std::string>(it->first, it->second));
			FeaturePoolInstance *p_featurepool = new FeaturePoolInstance;
			p_featurepools_.insert(std::pair<std::string, FeaturePoolInstance*>(it->first, p_featurepool));
		}

		if(p_dump_folder != NULL)
			strcpy(str_dump_folder_, p_dump_folder);
		else
			strcpy(str_dump_folder_, DEFAULT_DUMP_FOLDER);

		return 1;

	}
	/*子类实现，加载外部文件，添加新数据*/
	virtual int load(const char* p_file_name){
		return 1;
	}

	/*子类实现，重新加载外部文件，删除旧的*/
	virtual int reload(const char* p_file_name){
		return 1;
	}

	/*内部加载函数*/
	virtual int _load(const char* p_dir){
		char path[PATH_MAX];

		if(NULL == p_dir)
			return -3;
		std::map<std::string, std::string>::iterator it;

		for(it = str_db_names_.begin(); it != str_db_names_.end(); it++)
		{
			const char *dbname = it->second.c_str();
			const char *dbid = it->first.c_str();

			snprintf(path, sizeof(path), "%s/%s", p_dir, dbname);
			LOG_TRACE("to load:%s", path);

			FeaturePoolInstance *p_featurepool = get_feature_pool(dbid);
			if (NULL == p_featurepool){
				return -2;
			}

			bool b_flag = p_featurepool->unseralize(path);
			if(b_flag){
				LOG_DEBUG("load ok!");
			}

		} 
		
		return 1;

	}

	/*内部dump函数，写时复制功能*/
	virtual int _dump(const char* p_dir, bool lock_flag = false){
		char path[PATH_MAX];
		char tmp_path[PATH_MAX];
		int ret = 0;
		int len;

		std::map<std::string, std::string>::iterator it;
		for(it = str_db_names_.begin(); it != str_db_names_.end(); it++)
		{
			const char *dbid = it->first.c_str();
			char db_name[PATH_MAX];
			get_db_name(dbid, db_name, len);
			if (len == 0)
			{
				LOG_TRACE("not found the db:%s", dbid);
				exit(1);
			}

			snprintf(path, sizeof(path), "%s/%s", p_dir, db_name);
			snprintf(tmp_path, sizeof(tmp_path), "%s/%s.dumping", p_dir, db_name);
			LOG_TRACE("to dump:%s", path);

			FeaturePoolInstance *p_featurepool = get_feature_pool(dbid);
			if(NULL == p_featurepool)
				return -1;

			pid_t pid;
			pid = fork();
			if(pid < 0){
				LOG_ERROR("error in fork!");
			} else if (pid == 0) {
				LOG_DEBUG("dump in fork");
				p_featurepool->seralize(tmp_path, lock_flag);
				ret = rename(tmp_path, path);
				if (ret) {
					LOG_ERROR("rename [%s] to [%s] error[%d][%m]", tmp_path, path, errno);
				}
				LOG_DEBUG("dump ok ");
				exit(0);
			}
			/*waitpid非常重要，否则会产生僵尸子进程*/
			if (waitpid(pid, NULL, 0) != pid){
				fprintf(stderr,"Waitpid error!\n");
				exit(-1);
			}
		}
	
		return 1;
	}

	void get_db_name(const char *dbid, char db_name[], int& len){
		std::map<std::string, std::string>::iterator it = str_db_names_.find(dbid);
		if (it != str_db_names_.end())
		{
			strcpy(db_name, it->second.c_str());
			len = strlen(it->second.c_str());
		}
		else 
			len = 0; 
	}

	FeaturePoolInstance* get_feature_pool(const char *dbid)
	{
		if (!dbid)
			return NULL;
		std::map<std::string, FeaturePoolInstance*>::iterator it = p_featurepools_.find(dbid);
		if (it != p_featurepools_.end())
		{
			return it->second;
		}
		return NULL;

	}

	void get_dump_folder(char dump_folder[], int& len){
		strcpy(dump_folder, str_dump_folder_);
		len = strlen(str_dump_folder_);
	}

protected:

	std::map<std::string, FeaturePoolInstance*> p_featurepools_;
	std::map<std::string, std::string> str_db_names_; // db的名称，主要用于生成序列化文件名字

	char str_dump_folder_[PATH_MAX]; // 存放序列化文件的目录

private:
	/*以下接口返回，由子类进行实例化，纯虚函数*/
	virtual int size(int& size, const char* dbid) = 0;

	virtual int incrby(MAIN_KEY_TYPE key, ITEM_KEY_TYPE code, 
			ADD_VALUE_TYPE add_score, const char* dbid) = 0;

	virtual int incrbys(ItemResultMap& result_map, MAIN_KEY_TYPE key, 
			const CodeItemMapAdd& code_item_map, const char* dbid) = 0;

	virtual int mincrbys(KeyItemResultMap& key_item_result_map, 
			const KeyCodeItemMapAdd& key_code_item_map, const char* dbid) = 0;
	
	virtual int contains(bool& flag, MAIN_KEY_TYPE key, const char* dbid) = 0;

	virtual int exists(bool& flag, MAIN_KEY_TYPE key, 
			ITEM_KEY_TYPE code, const char* dbid) = 0;
	virtual int add(MAIN_KEY_TYPE key, ITEM_KEY_TYPE code, 
			ITEM_VALUE_TYPE score, const char* dbid) = 0;

	virtual int adds(ItemResultMap& result_map, MAIN_KEY_TYPE key, 
			const CodeItemMap& code_item_map, const char* dbid) = 0;

	virtual int rem(MAIN_KEY_TYPE key, const char* dbid) = 0;
	
	virtual int get(ITEM_VALUE_TYPE& return_value, MAIN_KEY_TYPE key, 
			ITEM_KEY_TYPE code, const char* dbid) = 0;	

	virtual int gets(CodeItemMap& map_return_value, MAIN_KEY_TYPE key, 
			const ItemVec& vec_code, const char* dbid) = 0;

	virtual int mgets(KeyCodeItemMap& map_return_value, const KeyVec& vec_key, 
			const ItemVec& vec_code, const char* dbid) = 0;

	virtual int mgets_i(KeyCodeItemMap& map_return_value, 
			const KeyItemVec& key_item_vec, const char* dbid) = 0;
	
	virtual int update(MAIN_KEY_TYPE key, ITEM_KEY_TYPE code, 
			ITEM_VALUE_TYPE new_score, const char* dbid) = 0;

	virtual int updates(ItemResultMap& result_map, MAIN_KEY_TYPE key, 
			const CodeItemMap& code_item_map, const char* dbid) = 0;

	virtual int mupdates(KeyItemResultMap& key_item_result_map, 
			const KeyCodeItemMap& key_code_item_map, const char* dbid) = 0;	
};
#endif
