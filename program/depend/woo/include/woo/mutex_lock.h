/**
 * @file
 * @author  Wang Chuanpeng <chuanpeng@staff.sina.com.cn>
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * woo framework 
 */
#ifndef __mutex_lock_H__
#define __mutex_lock_H__

#include <pthread.h>
#include "woo/log.h"

namespace woo {
	class MutexLock {
		public:
			MutexLock() {
				int return_value = pthread_mutex_init(&plock_, NULL);
				if(return_value != 0){
					LOG_ERROR("pthread init return Error num:[%d]", return_value);
				}
			}

			~MutexLock() { 
				int return_value = pthread_mutex_unlock(&plock_);
				if(return_value != 0){
				    LOG_ERROR("pthread unlock return Error num:[%d]", return_value);
				}

				return_value = pthread_mutex_destroy(&plock_);
				if(return_value != 0){
				    LOG_ERROR("pthread destroy return Error num:[%d]", return_value);
				}
			}

			int lock() { 
				int return_value = pthread_mutex_lock(&plock_); 
				if(return_value != 0){
				    LOG_ERROR("pthread lock return Error num:[%d]", return_value);
				}

				return return_value;
			}
			int unlock() { 
				int return_value = pthread_mutex_unlock(&plock_); 
				if(return_value != 0){
				    LOG_ERROR("pthread unlock return Error num:[%d]", return_value);
				}

				return return_value;
			}
		
		private:
			pthread_mutex_t plock_;
	};

};

#endif 


