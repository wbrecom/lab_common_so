/**
 * @file
 * @author  Li Jiong <lijiong@staff.sina.com.cn>
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * woo framework 
 */
#ifndef __woo_acceptor_connector_H__
#define __woo_acceptor_connector_H__

#include <deque>
#include <sys/epoll.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "woo/mutex.h"
#include "woo/reactor.h"

namespace woo {

typedef int (*proc_handle2_t)(void *handle_data, char *input, uint32_t input_len, 
		char *output, uint32_t *output_len, char *msg, size_t msg_size, int flag);

inline uint64_t getmillisecond() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

template <typename T, typename MutexT>
class PointerPool {
public:
	typedef T Element;

	PointerPool() {
		mutex_.init();
	}

	~PointerPool() {
		mutex_.destroy();
	}

	T* pop() {
		MutexGuard<MutexT> guard(mutex_);
		if (queue_.empty()) {
			return NULL;
		}
		T* v = queue_.front();
		queue_.pop_front();
		return v;
	}

	void push(T *v) {
		MutexGuard<MutexT> guard(mutex_);
		queue_.push_front(v);
	}

private:
	typedef std::deque<T*> Queue;
	Queue queue_;
	MutexT mutex_;
};

template <typename ConnectorPoolT>
class Acceptor {
public:
	int fd() {
		return fd_;
	}

	int accept() {
		int sock;
		struct sockaddr_in client_addr;
		socklen_t addrlen = sizeof(client_addr);
		sock = ::accept(fd_, (struct sockaddr *) &client_addr, &addrlen);
		LOG_TRACE("acceptor accept a connection[%d] from[%s] by listen fd[%d]", sock, inet_ntoa(client_addr.sin_addr), fd_);
		if(sock < 0){
			LOG_ERROR("accept connection error");
			return -1;
		}
		return sock;
	}

	void set_connector_pool(ConnectorPoolT *pool) {
		connector_pool_ = pool;
	}

	int open(const char *ip, int port) {
		struct sockaddr_in listen_addr;
		int listen_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		if(-1 == listen_sock) {
			LOG_ERROR("can not create socket");
			return -1;
		}
		int optval = 1;
		if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval,sizeof(int)) < 0) {
			LOG_ERROR("can not set socket");
			close(listen_sock);
			return -1;
		}
		memset(&listen_addr, 0, sizeof(listen_addr));

		listen_addr.sin_family = AF_INET;
		listen_addr.sin_port = htons(port);
		if (ip) {
			listen_addr.sin_addr.s_addr = inet_addr(ip);
		} else {
			listen_addr.sin_addr.s_addr = INADDR_ANY;
		}

		if(-1 == bind(listen_sock,(struct sockaddr *)&listen_addr, sizeof(listen_addr))) {
			LOG_ERROR("error bind failed");
			close(listen_sock);
			return -1;
		}

		if(-1 == listen(listen_sock, 512)) {
			LOG_ERROR("error listen failed");
			close(listen_sock);
			return -1;
		}
		if (setnoblock(listen_sock)) {
			LOG_ERROR("set listen no block failed");
			close(listen_sock);
			return -1;
		}
		fd_ = listen_sock;
		LOG_TRACE("listen fd[%d]", fd_);
		return 0;
	}

	int handle(woo::reactor_t *reactor) {
		//LOG_TRACE("accept from fd[%d]", fd_);

		int sock = accept();
		//LOG_TRACE("accept sock[%d] from fd[%d]", sock, fd_);
		if (setnoblock(sock)) {
			LOG_ERROR("set sock no block failed");
			::close(sock);
			sock = -1;
			return -1;
		}

		typename ConnectorPoolT::Element *connector = connector_pool_->pop();
		if (! connector) {
			LOG_DEBUG("connector_pool_ pop error");
			::close(sock);
			sock = -1;
			return -1;
		}

		connector->set_sock(sock);
		int ret = woo::reactor_add(reactor, EPOLLIN, ConnectorPoolT::Element::handle_callback, connector, sock, REACTOR_EVENT_FLAG_MAIN_THREAD_EXEC, 0);
		if (ret) {
			LOG_ERROR("reactor_add error[%d][%m]", errno);
			connector->close();
			connector_pool_->push(connector);
		}
		return 0;
	}

	static int handle_callback(woo::reactor_t *reactor, 
			void *handle_data, int handle_fd, int *flag) {
		Acceptor *acceptor = (Acceptor *)handle_data;
		return acceptor->handle(reactor);
	}

private:
	int fd_;
	ConnectorPoolT *connector_pool_;
};

template <typename HandlerPoolT>
class BinaryConnector {
public:
	typedef PointerPool<BinaryConnector, Mutex> ConnectorPool;

	BinaryConnector() : connector_pool_(NULL), handler_pool_(NULL) {}

	void set_connector_pool(ConnectorPool *pool) {
		connector_pool_ = pool;
	}

	void set_handler_pool(HandlerPoolT *pool) {
		handler_pool_ = pool;
	}

	int init(size_t recv_buf_size, size_t send_buf_size) {
		recv_buf_size_ = recv_buf_size;
		send_buf_size_ = send_buf_size;
		send_buf_  = (char *)malloc(send_buf_size);
		if (! send_buf_) {
			LOG_ERROR("malloc send_buf_size[%zu] error", send_buf_size);
			return -1;
		}
		recv_buf_  = (char *)malloc(recv_buf_size);
		if (! recv_buf_) {
			LOG_ERROR("malloc recv_buf_size[%zu] error", recv_buf_size);
			return -1;
		}

		set_sock(-1);

		return 0;
	}

	void close() {
		if (sock_ >= 0) {
			::close(sock_);
			sock_ = -1;
		}
	}

	void destroy() {
		if (recv_buf_) {
			free(recv_buf_);
			recv_buf_ = NULL;
		}
		if (send_buf_) {
			free(send_buf_);
			send_buf_ = NULL;
		}
		if (sock_ >= 0) {
			close(sock_);
			sock_ = -1;
		}
	}

	void set_sock(int sock) {
		recv_len_ = 0;
		send_len_ = 0;
		input_data_len_ = sizeof(woo::binary_head_t);
		output_data_len_ = 0;

		recv_times_ = 0;
		send_times_ = 0;
		init_beg_ = 0;
		recv_beg_ = 0;
		recv_end_ = 0;
		proc_beg_ = 0;
		proc_end_ = 0;
		send_beg_ = 0;
		send_end_ = 0;

		sock_ = sock;

		msg_[0] = '\0';

		init_beg_ = getmillisecond();
	}


	int handle(reactor_t *reactor, int handle_fd, int flag) {
		//LOG_DEBUG("connect handle recv_times[%d] send_times[%d]", recv_times_, send_times_);
		int ret;
		woo::binary_head_t *req_head = (woo::binary_head_t *)recv_buf_;
		if (recv_len_ >= ssize_t(sizeof(woo::binary_head_t))) {
			set_log_id(req_head->log_id);
		} else {
			set_log_id(0);
		}

		if (recv_beg_ == 0) {
			recv_beg_ = getmillisecond();
		}

		while (recv_len_ < input_data_len_) {
			ret = recv(sock_, recv_buf_ + recv_len_, input_data_len_ - recv_len_, 0);
			++ recv_times_;
			if (ret < 0) {
				if (EAGAIN == errno || EWOULDBLOCK == errno) {
					ret = reactor_add(reactor, EPOLLIN, BinaryConnector::handle_callback, this, sock_, REACTOR_EVENT_FLAG_MAIN_THREAD_EXEC, 0);
					if (ret) {
						LOG_ERROR("reactor_add error[%d][%m]", errno);
						close();
						ret = -1;
						goto out_log;
					}
					set_log_id(0);
					LOG_DEBUG("recv part[%zd] need[%zd]", recv_len_, input_data_len_);
					return 0;
				} else {
					LOG_TRACE("sock recv ret[%d] error[%d][%m]", ret, errno);
					close();
					ret = -1;
					goto out_log;
				}
			} else if (0 == ret) {
				LOG_TRACE("sock recv ret[%d] closed by peer", ret);
				close();
				ret = -1;
				goto out_log;
			}
			recv_len_ += ret;
			if (recv_len_ == input_data_len_ 
					&& recv_len_ == sizeof(woo::binary_head_t)) {
				input_data_len_
					= req_head->body_len + sizeof(woo::binary_head_t);
				if (input_data_len_ >= recv_buf_size_) {
					LOG_ERROR("req body_len[%u] is too larger than recv_buf_size[%d]",
						   req_head->body_len, recv_buf_size_);
					close();
					ret = -1;
					goto out_log;
				}
			}
			if (recv_len_ == input_data_len_) {
				ret = reactor_add(reactor, EPOLLOUT, BinaryConnector::handle_callback, this, sock_, 0, 0);
				if (ret) {
					LOG_ERROR("reactor_add error[%d][%m]", errno);
					close();
					ret = -1;
					goto out_log;
				}
				if (recv_end_ == 0) {
					recv_end_ = getmillisecond();
				}
				set_log_id(0);
				LOG_DEBUG("recv ok[%zd], send to thread pool procc", input_data_len_);
				return 0;
			}
		}
		if (recv_len_ >= ssize_t(sizeof(woo::binary_head_t))) {
			set_log_id(req_head->log_id);
		} else {
			set_log_id(0);
		}

		if (proc_beg_ == 0) {
			proc_beg_ = getmillisecond();
		}
		if (0 == output_data_len_) {
			uint32_t output_data_len = send_buf_size_;
			typename HandlerPoolT::Element *handler = handler_pool_->pop();
			if (! handler) {
				LOG_ERROR("get handler error");
				close();
				ret = -1;
				goto out_log;
			}
			handler->handle(recv_buf_, recv_len_, send_buf_, &output_data_len, 
					msg_, sizeof(msg_), flag);
			output_data_len_ = output_data_len;
			handler_pool_->push(handler);
			proc_end_ = getmillisecond();
		}

		if (send_beg_ == 0) {
			send_beg_ = getmillisecond();
		}
		while (send_len_ < output_data_len_) {
			ret = send(sock_, send_buf_ + send_len_, 
					output_data_len_ - send_len_, 0);
			++ send_times_;
			if (ret <= 0) {
				if (EAGAIN == errno || EWOULDBLOCK == errno) {
					ret = reactor_add(reactor, EPOLLOUT, BinaryConnector::handle_callback, this, sock_, REACTOR_EVENT_FLAG_MAIN_THREAD_EXEC, 0);
					if (ret) {
						LOG_ERROR("reactor_add error[%d][%m]", errno);
						close();
						//connector_pool_->push(this);
						ret = -1;
						goto out_log;
					}
					set_log_id(0);
					LOG_DEBUG("send part[%zd] need[%zd]", send_len_, output_data_len_);
					return 0;
				} else {
					LOG_WARN("sock send ret[%d] error[%d][%m]", ret, errno);
					close();
					ret = -1;
					goto out_log;
				}
			}
			send_len_ += ret;
		}
		if (send_end_ == 0) {
			send_end_ = getmillisecond();
		}
		close();
		ret = 0;
out_log:
		LOG_INFO("ret[%d] total_cost[%lu] cost:(%lu-%lu-%lu-%lu-%lu-%lu) handle_times[%d,%d] recv[%zu] send[%zu] %s",
				ret, send_end_ - init_beg_, recv_beg_ - init_beg_, recv_end_ - recv_beg_, 
				proc_beg_ - recv_end_, proc_end_ - proc_beg_, send_beg_ - proc_end_,
				send_end_ - send_beg_,
			   	recv_times_, send_times_,
				input_data_len_, output_data_len_,
				msg_
				);
		connector_pool_->push(this);
		set_log_id(0);
		return ret;
	}

	static int handle_callback(woo::reactor_t *reactor, 
			void *handle_data, int handle_fd, int *flag) {
		BinaryConnector *connector = (BinaryConnector *)handle_data;
		//LOG_DEBUG("connect handle %p", connector);
		return connector->handle(reactor, handle_fd, *flag);
	}

private:
	int sock_;
	size_t recv_len_;
	size_t send_len_;
	size_t input_data_len_;
	size_t output_data_len_;

	size_t recv_buf_size_;
	size_t send_buf_size_;
	char *recv_buf_;
	char *send_buf_;

	int recv_times_;
	int send_times_;

	uint64_t init_beg_;
	uint64_t recv_beg_;
	uint64_t recv_end_;
	uint64_t proc_beg_;
	uint64_t proc_end_;
	uint64_t send_beg_;
	uint64_t send_end_;

	char msg_[1024];

	ConnectorPool *connector_pool_;
	HandlerPoolT *handler_pool_;
};

class HandlerProxy {
public:
	HandlerProxy(proc_handle2_t handle = NULL, void *data = NULL)
		: handle_(handle), data_(data) {}

	int handle(char *input, uint32_t input_len, 
		char *output, uint32_t *output_len, char *msg, size_t msg_size, int flag) {
		//LOG_DEBUG("handle proxy");
		return handle_(data_, input, input_len, output, output_len, msg, msg_size, flag);
	}

private:
	proc_handle2_t handle_;
	void *data_;
};

typedef PointerPool<HandlerProxy, Mutex> HandlerProxyPool;
typedef BinaryConnector<HandlerProxyPool> PoolBinaryConnector;
typedef PoolBinaryConnector::ConnectorPool BinaryConnectorPool;
typedef Acceptor<BinaryConnectorPool> BinaryAcceptor;

inline BinaryAcceptor*
create_binary_acceptor(const char *ip, int port, int connector_pool_size, int recv_buf_size, int send_buf_size, int handler_pool_size, proc_handle2_t handle_func, void **handle_data) {

	int ret;

	HandlerProxyPool *handler_pool = new HandlerProxyPool();
	if (! handler_pool) {
		LOG_ERROR("create HandlerPool error");
		return NULL;
	}
	HandlerProxy *handler;
	for (int i = 0; i < handler_pool_size; ++ i) {
		if (handle_data) {
			handler = new HandlerProxy(handle_func, handle_data[i]);
		} else {
			handler = new HandlerProxy(handle_func, NULL);
		}
		if (! handler) {
			LOG_ERROR("create HandlerProxy error");
			return NULL;
		}
		handler_pool->push(handler);
	}


	BinaryConnectorPool *connector_pool = new BinaryConnectorPool();
	if (! connector_pool) {
		LOG_ERROR("create connector_pool error");
		return NULL;
	}
	for (int i = 0; i < connector_pool_size; ++ i) {
		PoolBinaryConnector *connector =  new PoolBinaryConnector();
		if (! connector) {
			LOG_ERROR("create connector error");
			return NULL;
		}
		ret = connector->init(recv_buf_size, send_buf_size);
		if (ret) {
			LOG_ERROR("connector init error");
			return NULL;
		}
		connector->set_connector_pool(connector_pool);
		connector->set_handler_pool(handler_pool);
		connector_pool->push(connector);
	}

	BinaryAcceptor *acceptor = new BinaryAcceptor();
	if (! acceptor) {
		LOG_ERROR("create acceptor error");
		return NULL;
	}
	acceptor->set_connector_pool(connector_pool);
	ret = acceptor->open(ip,  port);
	if (ret) {
		LOG_DEBUG("acceptor open error");
		return NULL;
	}

	return acceptor;
}

}
#endif
