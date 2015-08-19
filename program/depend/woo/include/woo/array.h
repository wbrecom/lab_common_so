/**
 * @file
 * @author  Li Jiong <lijiong@staff.sina.com.cn>
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * woo framework 
 */
#ifndef __woo_array_H___
#define __woo_array_H___

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include "woo/log.h"

namespace woo {

template<typename T>
class DArray {
	public:
		DArray(size_t step=16) : row_num_(0), size_(0), step_(step), array_(NULL) {
		}

		T* put_back(T v) {
			size_t row_idx = size_ / step_;
			size_t col_idx = size_ % step_;

			if (! ex_capacity(size_)) {
				return NULL;
			}
			array_[row_idx][col_idx] = v; 
			++ size_;
			return array_[row_idx] + col_idx;
		}

		T& operator [](size_t i) {
			if (! ex_capacity(i)) {
				throw 1;
			}
			if (i >= size_) {
				size_ = i + 1;
			}
			return array_[i / step_][i % step_];
		}

		T* get(size_t i) {
			if (! ex_capacity(i)) {
				return NULL;
			}
			return array_[i / step_] + (i % step_);
		}

		T* set(size_t i, const T& v) {
			if (! ex_capacity(i)) {
				return NULL;
			}
			if (i >= size_) {
				size_ = i + 1;
			}
			return array_[i / step_][i % step_] = v;
		}

		bool ex_capacity(size_t i) {
			size_t row_idx = i / step_;
			if (row_idx >= row_num_) {
				if (! array_) {
					T **array = (T **)calloc(row_idx + 1, sizeof(T*));
					if (! array) {
						LOG_ERROR("calloc error, %u %m", errno);
						return false;
					}
					array_ = array;
				} else {
					T **array = (T **)calloc(row_idx + 1, sizeof(T*));
					if (! array) {
						LOG_ERROR("calloc error, %u %m", errno);
						return false;
					}
					memcpy(array, array_, sizeof(T*) * row_num_);
					T **old_array = array_;
					array_ = array;
					free(old_array);
				}
				row_num_ = row_idx + 1;
			}
			if (! array_[row_idx]) {
				T *col = (T *)malloc(sizeof(T)* step_);
				if (! col) {
					LOG_ERROR("malloc error, %u %m", errno);
					return false;
				}
				array_[row_idx] = col;
			}
			return true;
		}

	private:
		size_t row_num_;
		size_t size_;
		size_t step_;
		T **array_;
};

}

#endif
