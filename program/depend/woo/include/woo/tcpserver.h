/**
 * @file
 * @author  Li Jiong <lijiong@staff.sina.com.cn>
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * woo framework 
 */
#ifndef __woo_tcp_server__h__
#define __woo_tcp_server__h__

#include <vector>
#include <queue>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdint.h>

namespace woo {

struct _tcp_server_t;
typedef struct _tcp_server_t tcp_server_t;

struct _ip_filter_t {
	in_addr ip;
	in_addr mask;
};
typedef struct _ip_filter_t ip_filter_t;
typedef std::vector<ip_filter_t> *ip_filter_array_t;
ip_filter_array_t load_ip_filter(const char *path);

typedef int (*recv_handle_t)(void *handle_data, int sock, char *buf, size_t *data_len, 
		size_t buf_size);
typedef int (*proc_handle_t)(void *handle_data, char *input, uint32_t input_len, 
		char *output, uint32_t *output_len, char *msg, size_t msg_size);

tcp_server_t *tcp_server_create();
int tcp_server_open(tcp_server_t *server, const char *ip, int port,
		recv_handle_t recv, proc_handle_t proc, void **handle_data, int thread_num, 
		bool long_conn, size_t input_buf_size, size_t output_buf_size, int recv_timeout, int send_timeout);
int tcp_server_run(tcp_server_t *server, bool new_thread = false);
int tcp_server_wait(tcp_server_t *server);
void tcp_server_stop(tcp_server_t *server);
int tcp_server_destroy(tcp_server_t *server);
ip_filter_array_t
tcp_server_set_ip_filter(tcp_server_t *server, ip_filter_array_t ip_filter);

typedef struct _binary_head_t {
	uint32_t body_len;
	uint32_t log_id;
	uint64_t trans_id;
} binary_head_t;

int scgi_recv(void *handle_data, int sock, char *buf, size_t *data_len, size_t buf_size);
int binary_recv(void *handle_data, int sock, char *buf, size_t *data_len, size_t buf_size);

int safe_recv(int sock, char *buf, size_t len, int flags);

inline int diff_millisec(const struct timeval &end, const struct timeval &beg) {
	return (end.tv_sec - beg.tv_sec) * 1000
	   	+ (end.tv_usec - beg.tv_usec) / 1000;
}

int setnoblock(int fd);
}

#endif
