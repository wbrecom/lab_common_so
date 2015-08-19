/**
 * @file
 * @author  Li Jiong <lijiong@staff.sina.com.cn>
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * woo framework 
 */
#ifndef __woo_mutex_H__
#define __woo_mutex_H__

#include <pthread.h>

namespace woo {

class NullMutex {
public:
	NullMutex() { }
	~NullMutex() { }

	int init() {  return 0; }
	void destroy() { }

	int lock() { return 0; }
	int unlock() { return 0; }
};

class Mutex {
public:
	Mutex() { }
	~Mutex() { }

	int init() { 
		int ret = pthread_mutex_init(&mutex_, NULL); 
		LOG_DEBUG("pthread_mutex_init ret[%d]", ret);
		return ret;
	}
	void destroy() { 
		pthread_mutex_destroy(&mutex_); 
		LOG_DEBUG("pthread_mutex_destroy");
	}

	int lock() { 
		int ret = pthread_mutex_lock(&mutex_); 
		if (ret) {
			LOG_DEBUG("pthread_mutex_lock ret[%d]", ret);
		}
		return ret;
	}
	int unlock() { 
		int ret = pthread_mutex_unlock(&mutex_); 
		if (ret) {
			LOG_DEBUG("pthread_mutex_unlock ret[%d]", ret);
		}
		return ret;
	}
		
private:
	pthread_mutex_t mutex_;
};

template <typename MutexT>
class MutexGuard {
public:
	MutexGuard(MutexT &mutex): mutex_(mutex) { mutex_.lock(); }
	~MutexGuard() { mutex_.unlock(); }

private:
	MutexT &mutex_;
};

};

#endif 



