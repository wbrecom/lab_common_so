#ifndef _PTHRAD_RW_LOCKER_HEADER_
#define _PTHRAD_RW_LOCKER_HEADER_
#include <pthread.h>
#include "woo/log.h"

// read and write locker
class PthreadRWLocker{

public:
	PthreadRWLocker(){
		int return_value = pthread_rwlock_init(&rwlock_t_, NULL);
        if(return_value != 0){
			LOG_ERROR("pthread init return Error num:[%d]", return_value);
        }
	}
	~PthreadRWLocker(){
		int return_value = pthread_rwlock_unlock(&rwlock_t_);
		if(return_value != 0){
			LOG_ERROR("pthread unlock return Error num:[%d]", return_value);
		}

		return_value = pthread_rwlock_destroy(&rwlock_t_);
		if(return_value != 0){
			LOG_ERROR("pthread destroy return Error num:[%d]", return_value);
		}
	}

	int rdlock() { 
		int return_value = pthread_rwlock_rdlock(&rwlock_t_); 
		if(return_value != 0){
			LOG_ERROR("pthread lock return Error num:[%d]", return_value);
		}

		return return_value;
	}

	int wrlock() { 
		int return_value = pthread_rwlock_wrlock(&rwlock_t_); 
		if(return_value != 0){
			LOG_ERROR("pthread lock return Error num:[%d]", return_value);
		}

		return return_value;
	}

	int unlock() { 
		int return_value = pthread_rwlock_unlock(&rwlock_t_); 
		if(return_value != 0){
			LOG_ERROR("pthread unlock return Error num:[%d]", return_value);
		}

		return return_value;
	}

private:
	pthread_rwlock_t rwlock_t_;
};

#endif
