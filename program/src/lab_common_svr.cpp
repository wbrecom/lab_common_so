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

typedef struct _conf_t {
	char log_path[PATH_MAX];
	char log_type[10];
	char log_flush_type[10];
	size_t log_buf_size;
	char ip[32];
	int query_port;
	int control_port;
	bool long_conn;
	int thread_num;
	size_t recv_buf_size;
	size_t send_buf_size;
	int recv_to;
	int send_to;
} conf_t;

void **query_handle_data = NULL;
conf_t g_conf;
pthread_mutex_t req_mutex = PTHREAD_MUTEX_INITIALIZER;
uint64_t req_id = 0;

typedef struct _thread_data_t {
	DbCompany* db_company_;
	WorkCompany* work_company_;
} thread_data_t;

GlobalDbCompany* p_global_db_company = NULL;

const int SLEEP_TIME = 15;			// 更新时交换指针后的睡眠时间，以保证旧指针无人使用

/*更新全局数据，更新global_db_config文件全部*/
int
update_global_handle_data(void** &query_handle_data, int num, char *output,
		            uint32_t& output_len){
	GlobalDbCompany* p_new_global_db_company = new GlobalDbCompany();
	if(NULL == p_new_global_db_company){
		 LOG_ERROR("new global db company is eror!");
		 return -1;
	}

	p_new_global_db_company->load_config("../conf/global_db_config.ini");
	
	void** handle_ptr_arr = query_handle_data;
	if (! handle_ptr_arr){
       	LOG_ERROR("calloc error");
		return -1;
    }
	
	for(int i = 0; i < num; ++i){
		thread_data_t *thr = (thread_data_t*)handle_ptr_arr[i];	
    	if(NULL == thr){
			continue;
		}

		thr->db_company_->initialize(p_new_global_db_company);
	}

	std::swap(p_global_db_company, p_new_global_db_company);
	sleep(SLEEP_TIME);
	delete p_new_global_db_company;
	p_new_global_db_company = NULL;

	/*使用malloc_trim 归还堆上的空间*/
	malloc_trim(0);

	return 1;

}

/*更新全局的部分数据，更新global_db_config中的某一个*/
int
update_global_single_handle_data(void** &query_handle_data, const char* db_name, 
		int num, char *output, uint32_t& output_len){
	if(NULL == p_global_db_company)
		return 1;

	GlobalDbInterface* p_new_db_interface = NULL;
	int result = p_global_db_company->update_global_db_interface
		(p_new_db_interface, db_name, "../conf/global_db_config.ini");
	
	if(result < 0)
		return result;
	else if(result == 1){
		sleep(SLEEP_TIME);
		//result == 1 is for already exists,0 is for new
		delete p_new_db_interface;
		p_new_db_interface = NULL;

		/*使用malloc_trim 归还堆上的空间*/
		malloc_trim(0);
	}

	return 1;

}


/*更新db_config文件*/
int
update_db_handle_data(void** &query_handle_data, int num, char *output,
		            uint32_t& output_len){
	void** handle_ptr_arr = query_handle_data;
	if (! handle_ptr_arr){
       	LOG_ERROR("calloc error");
		return -1;
    }

	WorkCompany* work_company_array[num];
	DbCompany* db_company_array[num];
	for(int i = 0; i < num; ++i){
		work_company_array[i] = NULL;
		db_company_array[i] = NULL;

		thread_data_t *thr = (thread_data_t*)handle_ptr_arr[i];	
    	if(NULL == thr){
			LOG_ERROR("company is NULL!");
			continue;
		}
		
		db_company_array[i] = new DbCompany();
		if(NULL == db_company_array[i]){
			LOG_ERROR("calloc dbcompany error!");
			continue;
		}

		db_company_array[i]->initialize(p_global_db_company);
		db_company_array[i]->load_config("../conf/db_config.ini");
		
		swap(thr->db_company_, db_company_array[i]);

   		work_company_array[i] = new WorkCompany(thr->db_company_);
    	if(NULL == work_company_array[i]){
			LOG_ERROR("calloc workcompany error!");
			delete db_company_array[i];
			continue;
    	}

    	work_company_array[i]->initialize("../conf/work_config.ini");
		std::swap(thr->work_company_, work_company_array[i]);
	}
	sleep(SLEEP_TIME);

	for(int i = 0; i < num; ++i){
		if(work_company_array[i] != NULL)
			delete work_company_array[i];
		if(db_company_array[i] != NULL)
			delete db_company_array[i];
	}
    return 1;
}

/*主要为服务使用，更新work_config文件*/
int
update_work_handle_data(void** &query_handle_data, int num, char *output,
		            uint32_t& output_len) {
	void** handle_ptr_arr = query_handle_data;
	if (! handle_ptr_arr){
       	LOG_ERROR("calloc error");
		return -1;
    }

	WorkCompany* work_company_array[num];
	for(int i = 0; i < num; ++i){
		thread_data_t *thr = (thread_data_t*)handle_ptr_arr[i];	
    	if(NULL == thr){
			work_company_array[i] = NULL;
			continue;
		}

   		work_company_array[i] = new WorkCompany(thr->db_company_);
    	if(NULL == work_company_array[i]){
			continue;
    	}

    	work_company_array[i]->initialize("../conf/work_config.ini");
		std::swap(thr->work_company_, work_company_array[i]);
	}
	sleep(SLEEP_TIME);

	for(int i = 0; i < num; ++i){
		if(work_company_array[i] != NULL)
			delete work_company_array[i];	
	}
    return 1;
}


/*json版本的服务接口*/
int query_req_handle(void *handle_data, char *input, uint32_t input_len, char *output, 
			uint32_t *output_len, char *msg, size_t msg_size) {
	woo::binary_head_t *req_head = (woo::binary_head_t *)input;
	char *req_body = input + sizeof(woo::binary_head_t);
	woo::binary_head_t *resp_head = (woo::binary_head_t *)output;
	char *resp_body = output + sizeof(woo::binary_head_t);

	memset(resp_head, 0, sizeof(woo::binary_head_t));
	resp_head->log_id = req_head->log_id;
	uint64_t log_id = resp_head->log_id;			// 20140903新增
	req_body[req_head->body_len] = '\0';	
	
	thread_data_t *thr = (thread_data_t *)handle_data;
    int n_out_len = 0;
    *resp_body = '\0';

	//pthread_mutex_lock(&req_mutex);
	//req_id++;
	//pthread_mutex_unlock(&req_mutex);

	LOG_DEBUG("input:%u:%s", req_head->body_len, req_body);
    if( thr != NULL){
		WorkCompany* p_api_adapter = thr->work_company_;
		if(p_api_adapter != NULL)
        	p_api_adapter->work_core((const char*&)req_body, resp_body, n_out_len, log_id);
		else
			LOG_ERROR("error!");
	}
	else
		LOG_ERROR("error!");
	
	
	resp_head->body_len = n_out_len;
	*output_len = sizeof(woo::binary_head_t) + resp_head->body_len;

	output[*output_len] = '\0';
	//LOG_DEBUG("output:%u:%s", resp_head->body_len, resp_body);
	LOG_DEBUG("output:%u", resp_head->body_len);

	return 0;
}

/*json版本的控制接口*/
int control_req_handle(void *handle_data, char *input, uint32_t input_len, char *output, 
		uint32_t *output_len, char *msg, size_t msg_size) {
    woo::binary_head_t *req_head = (woo::binary_head_t *)input;
    char *req_body = input + sizeof(woo::binary_head_t);
    woo::binary_head_t *resp_head = (woo::binary_head_t *)output;
    char *resp_body = output + sizeof(woo::binary_head_t);

    req_body[req_head->body_len] = '\0';
    LOG_DEBUG("input:%u:%s", req_head->body_len, req_body);

	uint32_t n_out_len = 0;
    *resp_body = '\0';

	int result = 0;
	if(strcasecmp(req_body, "update_work") == 0){
		result = update_work_handle_data(query_handle_data, g_conf.thread_num, resp_body, n_out_len);
	}else if(strcasecmp(req_body, "update_global") == 0){
		result = update_global_handle_data(query_handle_data, g_conf.thread_num, resp_body, n_out_len);
	}else if(strcasecmp(req_body, "update_db") == 0){
		result = update_db_handle_data(query_handle_data, g_conf.thread_num, resp_body, n_out_len);
	}else{
		result = update_global_single_handle_data(query_handle_data, req_body, g_conf.thread_num, resp_body, n_out_len);
	}

	if(1 == result)
		n_out_len = sprintf(resp_body, "%s:%s", req_body, "success");
	else
		n_out_len = sprintf(resp_body, "%s:%s", req_body, "fail");

    memset(resp_head, 0, sizeof(woo::binary_head_t));
    resp_head->body_len = n_out_len;
    resp_head->log_id = req_head->log_id;
    *output_len = sizeof(woo::binary_head_t) + resp_head->body_len;

    output[*output_len] = '\0';
    LOG_DEBUG("output:%u:%s", resp_head->body_len, resp_body);

    return 0;
}

int load_conf(const char *path) {
	woo::Conf *conf = woo::conf_load(path);
	strncpy(g_conf.log_path, woo::conf_get_str(conf, "log_path", "./log/lab_common.log"), 
			sizeof(g_conf.log_path));
	strncpy(g_conf.log_type, woo::conf_get_str(conf, "log_type", "ewidt"),
			            sizeof(g_conf.log_type));
	strncpy(g_conf.log_flush_type, woo::conf_get_str(conf, "log_flush_type", "ewi"),
			            sizeof(g_conf.log_flush_type));

	g_conf.log_buf_size = woo::conf_get_int(conf, "log_buf_size", 1024 * 128);
	strncpy(g_conf.ip, woo::conf_get_str(conf, "ip", "0.0.0.0"), 
			sizeof(g_conf.ip));
	g_conf.query_port = woo::conf_get_int(conf, "query_port", 22035);
	g_conf.control_port = woo::conf_get_int(conf, "control_port", 22036);
	g_conf.thread_num = woo::conf_get_int(conf, "thread_num", 24);
	g_conf.recv_buf_size = woo::conf_get_int(conf, "recv_buf_size", 1024 * 1000);
	g_conf.send_buf_size = woo::conf_get_int(conf, "send_buf_size", 1024 * 1000);
	g_conf.recv_to = woo::conf_get_int(conf, "recv_timeout", 200000);
	g_conf.send_to = woo::conf_get_int(conf, "send_timeout", 200000);

	g_conf.long_conn = woo::conf_get_int(conf, "long_conn", 1) == 0 ? false : true;
	woo::conf_destroy(conf);
	return 0;
}

woo::tcp_server_t *query_server = NULL;
woo::tcp_server_t *control_server = NULL;
/*可以通过发送信号来优雅的停止服务*/
bool g_quit = false;
void stop_server(int sig) {
	LOG_TRACE("recv sig[%d]", sig);
	g_quit = true;
	if (sig == SIGTERM || sig == SIGINT) {
		LOG_TRACE("to stop server");
		if (control_server) {
            woo::tcp_server_stop(control_server);
            LOG_TRACE("notify control_server to stop");
        }
		if (query_server) {
			woo::tcp_server_stop(query_server);
			LOG_TRACE("notify query_server to stop");
		}
     	LOG_TRACE("notify stop finished");
	}
}

/*to be continue 销毁数据*/
int 
destroy_thread_data(void **query_handle_data, int num){
	return 0;
}

/*主要为服务使用*/
int
init_thread_data(thread_data_t *thr) {
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
	return 0;
}

/*提供进程共有数据，主要为服务使用*/
void **
create_handle_data(int num) {
    thread_data_t *handle_arr = (thread_data_t *)calloc(num, sizeof(thread_data_t));
    if (! handle_arr) {
        LOG_ERROR("calloc error");
        return NULL;
    }
    void **handle_ptr_arr = (void **) calloc(num, sizeof(void *));
    if (! handle_ptr_arr) {
        LOG_ERROR("calloc error");
        return NULL;
    }
    for (int i = 0; i < num; ++ i) {
        handle_ptr_arr[i] = handle_arr + i;
        init_thread_data(handle_arr + i);
    }
    return handle_ptr_arr;
}

int main(int argc, char ** argv) {
	signal(SIGPIPE, SIG_IGN);
    signal(SIGTERM, stop_server);
    signal(SIGINT, stop_server);

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

	woo::open_log(g_conf.log_path, g_conf.log_buf_size, g_conf.log_type, g_conf.log_flush_type);

	p_global_db_company = new GlobalDbCompany();
	if(NULL == p_global_db_company){
		 LOG_ERROR("new global db company is eror!");
		 return -1;
	}

	p_global_db_company->load_config("../conf/global_db_config.ini");	

	query_handle_data = create_handle_data(g_conf.thread_num);
    if (! query_handle_data) {
        LOG_ERROR("create query handle error");
        return -1;
    }

	time_t now	= time(NULL);
	req_id  = now << 32L;

	/*控制服务*/
    control_server = woo::tcp_server_create();
    if (! control_server) {
        LOG_ERROR("create tcp server error");
        return -1;
    }

	/*查询服务*/
	query_server = woo::tcp_server_create();
	if (! query_server) {
		LOG_ERROR("create tcp server error");
		return -1;
	}

	int ret = 0;

	ret = woo::tcp_server_open(query_server, g_conf.ip, g_conf.query_port, woo::binary_recv, 
		query_req_handle, query_handle_data, g_conf.thread_num, g_conf.long_conn, 
		g_conf.recv_buf_size, g_conf.send_buf_size, g_conf.recv_to, g_conf.send_to);
	if (ret) {
		LOG_ERROR("open query tcp server error");
		return -1;
	}

	ret = woo::tcp_server_open(control_server, g_conf.ip, g_conf.control_port, woo::binary_recv,
                control_req_handle, NULL, 1, g_conf.long_conn,
                g_conf.recv_buf_size, g_conf.send_buf_size, g_conf.recv_to, g_conf.send_to);
	if (ret) {
		LOG_ERROR("open update tcp server error");
		return -1;
	}

	if (woo::tcp_server_run(control_server, true)) {
        LOG_ERROR("update tcp server run error");
        return -1;
    }

	if (woo::tcp_server_run(query_server, true)) {
		LOG_ERROR("query tcp server run error");
		return -1;
	}
	
	LOG_TRACE("query server started");
    woo::tcp_server_wait(control_server);
    LOG_TRACE("update server stoped");
	woo::tcp_server_wait(query_server);
	LOG_TRACE("query server stoped");

	return 0;
}
