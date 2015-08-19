#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ext/hash_set>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "woo/log.h"
#include "woo/tcpserver.h"
#include "woo/config.h"
#include <limits.h>
#include "db_company.h"
#include "work_company.h"
#include "global_db_company.h"
#include "time.h"
#include "malloc.h"
#include <vector>

typedef struct _conf_t {
	char log_path[PATH_MAX];
	char output_file_path[PATH_MAX];
	char keys_file_path[PATH_MAX];
	size_t log_buf_size;
	int thread_num;
} conf_t;

conf_t g_conf;
GlobalDbCompany* p_global_db_company;

typedef struct _thread_data_t {
	DbCompany* db_company_;
	WorkCompany* work_company_;
	const char* output_file_;
	std::vector<uint64_t> vec_keys_;

} thread_data_t;

/*主要为服务使用*/
/*json版本的服务接口*/
static void* query_req_handle(void *handle_data) {
	char* req_body = new char[PATH_MAX];
	char* resp_body = new char[BUF_SIZE];

	thread_data_t* thr = (thread_data_t*)handle_data;
    if( thr != NULL){
		FILE* out_fp = fopen(thr->output_file_, "w");
		if(NULL == out_fp){
			LOG_ERROR("open file is error!");
			return NULL;
		}

		WorkCompany* p_api_adapter = thr->work_company_;
		if(p_api_adapter != NULL){
			for(std::vector<uint64_t>::iterator it = thr->vec_keys_.begin();
					it != thr->vec_keys_.end(); it ++){

				char uid_str[64];
				size_t uid_len = sprintf(uid_str, "%"PRIu64, (*it));
				sprintf(req_body, "{\"api\":\"simpleframe\", \"cmd\":\"query\", \"uid\":\"%s\"}", uid_str);
				
				int n_out_len = 0;	
				p_api_adapter->work_core((const char*&)req_body, (char* &)resp_body, n_out_len, 0);
				resp_body[n_out_len] = '\n';
				n_out_len ++;

				fwrite(uid_str, 1, uid_len, out_fp);
				fwrite("\t", 1, 1, out_fp);
				fwrite(resp_body, 1, n_out_len, out_fp);

			}
		}
		else{
			LOG_ERROR("error!");
		}
		fclose(out_fp);
	}
	else{
		LOG_ERROR("error!");
	}

	delete [] resp_body;
	delete [] req_body;
	return NULL;
}

/*更新全局数据*/
int
update_global_handle_data(thread_data_t thread_data_t_array[], int num, char *output,
		            uint32_t& output_len){
	GlobalDbCompany* p_new_global_db_company = new GlobalDbCompany();
	if(NULL == p_new_global_db_company){
		 LOG_ERROR("new global db company is eror!");
		 return -1;
	}

	p_new_global_db_company->load_config("../conf/global_db_config.ini");
	
	for(int i = 0; i < num; ++i){
		thread_data_t *thr = &thread_data_t_array[i];	
    	if(NULL == thr){
			continue;
		}

		LOG_TRACE("initialize new data!");
		thr->db_company_->initialize(p_new_global_db_company);
	}
	sleep(1);

	std::swap(p_global_db_company, p_new_global_db_company);
	delete p_new_global_db_company;
	p_new_global_db_company = NULL;

	/*使用malloc_trim 归还堆上的空间*/
	malloc_trim(0);

	return 1;

}

int load_conf(const char *path) {
	woo::Conf *conf = woo::conf_load(path);
	strncpy(g_conf.log_path, woo::conf_get_str(conf, "log_path", "../log/lab_common_main.log"), 
			sizeof(g_conf.log_path));

	strncpy(g_conf.output_file_path, woo::conf_get_str(conf, "output_file_path", "../result/output_file"), 
			sizeof(g_conf.log_path));
	
	strncpy(g_conf.keys_file_path, woo::conf_get_str(conf, "keys_file_path", "../data/user_list"), 
			sizeof(g_conf.log_path));

	g_conf.log_buf_size = woo::conf_get_int(conf, "log_buf_size", 1024 * 128);
	g_conf.thread_num = woo::conf_get_int(conf, "thread_num", 24);

	woo::conf_destroy(conf);
	return 0;
}

/*主要为服务使用*/
int
init_thread_data(thread_data_t *thr, int thread_index) {
	if(NULL == thr)
		return -1;

	thr->db_company_ = new DbCompany();
	if(NULL == thr->db_company_)
		return -1;

	thr->db_company_->initialize(p_global_db_company);
	thr->db_company_->load_config("../conf/db_config.ini");

	thr->work_company_ = new WorkCompany(thr->db_company_);
	if(NULL == thr->work_company_){
		delete thr->db_company_;
		thr->db_company_ = NULL;
		return -1;
	}

	thr->work_company_->initialize("../conf/work_config.ini");

	char* output_file = new char[PATH_MAX];
	if(output_file == NULL){
		delete thr->work_company_;
		thr->work_company_ = NULL;

		delete thr->db_company_;
		thr->db_company_ = NULL;
		return -1;
	}

	sprintf(output_file, "%s_%d", g_conf.output_file_path, thread_index);
	thr->output_file_ = output_file;

	return 0;
}

/*提供进程共有数据，主要为服务使用*/
void
create_handle_data(thread_data_t thread_data_t_array[], int num, const char* keys_file_path) {
    for (int i = 0; i < num; ++ i) {
        init_thread_data(&thread_data_t_array[i], i);
    }

	FILE *fd = NULL;  
	char buf[64];  
	fd = fopen(keys_file_path, "r");  
	if(NULL == fd){
		LOG_ERROR("%s read is error!", keys_file_path);
	}  
	int i = 0;
	while(fgets(buf, 64, fd) != NULL){  
		uint64_t id = strtoull(buf, NULL, 10);
		//mod id
		LOG_DEBUG("%"PRIu64":%d", id, i);
		thread_data_t_array[id % num].vec_keys_.push_back(id);
		i ++ ;
	}  
	fclose(fd);  

}

/*提供回收thread*/
void
destroy_handle_data(thread_data_t thread_data_t_array[], int num){
	for (int i = 0; i < num; ++ i){
		if(thread_data_t_array[i].db_company_ != NULL)
			delete thread_data_t_array[i].db_company_;
		if(thread_data_t_array[i].work_company_ != NULL)
			delete thread_data_t_array[i].work_company_;
		if(thread_data_t_array[i].output_file_ != NULL)
			delete [] thread_data_t_array[i].output_file_;
		thread_data_t_array[i].vec_keys_.clear();
	}
}

int main(int argc, char ** argv) {

	const char *conf_path = NULL;
	if (argc < 2) {
		printf("usage:%s conf_path\n", argv[0]);
		return -1;
	}

	conf_path = argv[1];
	if (load_conf(conf_path)) {
		LOG_ERROR("read %s error", conf_path);
		return -1;
	}

	woo::open_log(g_conf.log_path, g_conf.log_buf_size,  "ewi", "ewi");

	curl_global_init(CURL_GLOBAL_ALL);

	p_global_db_company = new GlobalDbCompany();
	if(NULL == p_global_db_company){
		LOG_ERROR("new db company is failed!");
		return -1;
	}

	p_global_db_company->load_config("../conf/global_db_config_main.ini");	

	thread_data_t thread_data_t_array[g_conf.thread_num];
	create_handle_data(thread_data_t_array, g_conf.thread_num, g_conf.keys_file_path);

	pthread_t thread_arr[g_conf.thread_num];
	for(int i = 0 ; i < g_conf.thread_num; i ++){
		pthread_create(&thread_arr[i], NULL, &query_req_handle, &thread_data_t_array[i]);
	}

	for(int i = 0 ; i < g_conf.thread_num; i ++){
		pthread_join(thread_arr[i], NULL);
	}

	char* output = NULL;
	uint32_t output_len = 0;

	LOG_TRACE("update global data!");
	update_global_handle_data(thread_data_t_array, g_conf.thread_num, output, output_len);

	destroy_handle_data(thread_data_t_array, g_conf.thread_num);

	delete p_global_db_company;
	p_global_db_company = NULL;

	curl_global_cleanup();

	return 0;
}
