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
#include <limits.h>

int req_handle(void *handle_data, char *input, uint32_t input_len, char *output, uint32_t *output_len, char *msg, size_t msg_size) {
	woo::binary_head_t *req_head = (woo::binary_head_t *)input;
	char *req_body = input + sizeof(woo::binary_head_t);
	woo::binary_head_t *resp_head = (woo::binary_head_t *)output;
	char *resp_body = output + sizeof(woo::binary_head_t);

	input[input_len] = '\0';
	LOG_DEBUG("input:%u:%s", req_head->body_len, req_body);

	for (uint32_t i = 0; i < req_head->body_len; ++ i) {
		resp_body[i*2] = req_body[i];
		resp_body[i*2 + 1] = '-';
	}

	memset(resp_head, 0, sizeof(woo::binary_head_t));
	resp_head->body_len = req_head->body_len * 2;
	resp_head->log_id = req_head->log_id;
	*output_len = sizeof(woo::binary_head_t) + resp_head->body_len;

	output[*output_len] = '\0';
	LOG_DEBUG("output:%u:%s", resp_head->body_len, resp_body);

	return 0;
}

typedef struct _conf_t {
	char log_path[PATH_MAX];
	char ip[32];
	int port;
	bool long_conn;
	int thread_num;
	size_t recv_buf_size;
	size_t send_buf_size;
	int recv_to;
	int send_to;
} conf_t;

conf_t g_conf;

int main(int argc, char ** argv) {
	signal(SIGPIPE, SIG_IGN);

	if (argc < 2) {
		printf("usage:%s port \n", argv[0]);
		return -1;
	}

	const char *ip_filter_path = NULL;
	woo::ip_filter_array_t ip_filter = NULL;
	if (argc >= 3) {
		ip_filter_path = argv[2];
	}

	strncpy(g_conf.ip, "0.0.0.0", sizeof(g_conf.ip));
	g_conf.port = atoi(argv[1]);
	g_conf.thread_num = 3;
	g_conf.long_conn = false;
	strncpy(g_conf.log_path, "./binaryserver.log", sizeof(g_conf.log_path));
   	g_conf.recv_buf_size = 1024*1000;
   	g_conf.send_buf_size = 1024*1000;
	g_conf.recv_to = 1000000;
   	g_conf.send_to = 1000000;


	woo::open_log(g_conf.log_path, 1024 *100, "ewidt", "ewidt");
	woo::tcp_server_t *server = woo::tcp_server_create();
	if (ip_filter_path) {
		LOG_DEBUG("to load ip filter [%s]", ip_filter_path);
		ip_filter = woo::load_ip_filter(ip_filter_path);
		if (ip_filter) {
			woo::tcp_server_set_ip_filter(server, ip_filter);
		}
	}

	woo::tcp_server_open(server, g_conf.ip, g_conf.port, woo::binary_recv, req_handle, NULL, 
			g_conf.thread_num, g_conf.long_conn, 
			g_conf.recv_buf_size, g_conf.send_buf_size, g_conf.recv_to, g_conf.send_to);
	woo::tcp_server_run(server);
	woo::tcp_server_wait(server);
	woo::tcp_server_destroy(server);

	return 0;
}
