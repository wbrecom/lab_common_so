/**
 * woo framework 
 */
#ifndef __woo_binary_client__h__
#define __woo_binary_client__h__

#include <sys/time.h>
#include <stdint.h>

namespace woo {

struct _binary_client_t;
typedef struct _binary_client_t binary_client_t;

struct _binary_client_pool_t;
typedef struct _binary_client_pool_t binary_client_pool_t;

binary_client_t*
binary_client_create(const char *host, int port, int send_to, int recv_to);

binary_client_t*
binary_client_create(const char *servers, int send_to, int recv_to);

int
binary_client_sock(binary_client_t*);

void
binary_client_close(binary_client_t *client);

void
binary_client_destroy(binary_client_t *client);

ssize_t 
binary_client_send(binary_client_t *client, const char *data, uint32_t data_len, uint32_t log_id, bool reset_conn = false, long hash = -1, bool auto_conn = true);

ssize_t 
binary_client_recv(binary_client_t *client, char *buf, uint32_t *data_len, uint32_t buf_size, uint32_t *log_id, bool long_conn = true);

ssize_t 
binary_client_talk(binary_client_t *client, const char *req, uint32_t req_len,
		char *buf, uint32_t *data_len, uint32_t buf_size, uint32_t log_id, bool long_conn = true);

ssize_t 
safe_send(int s, const char *buf, size_t len, int flags);

binary_client_pool_t*
binary_client_pool_create(size_t max_client_num);

int
binary_client_pool_add(binary_client_pool_t *pool, const char *host, int port, 
		int send_to, int recv_to);

void
binary_client_pool_destroy(binary_client_pool_t *pool);

ssize_t 
binary_client_pool_talk(binary_client_pool_t *pool, const char *req, uint32_t req_len,
		char *buf, uint32_t *data_len, uint32_t buf_size, uint32_t log_id, uint32_t hash,
		int retry_times, int poll_times, bool long_conn = true);

}

#endif
