/**
 * @file
 * @author  Li Jiong <lijiong@staff.sina.com.cn>
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * woo framework 
 */
#ifndef __woo_multi_merge_H__
#define __woo_multi_merge_H__

#include <stdint.h>
#include <stdlib.h>
#include "woo/log.h"


namespace woo {

template<typename InputT, typename OutputT>
int multi_merge(InputT** inputs, int input_num, OutputT* output) {
	int n = 0;
	for (int i = 0; i < input_num; ++ i) {
		if (inputs[i]->have()) {
			if (i != n) {
				inputs[n] = inputs[i];
			}
			n ++;
		}
	}
	int live_num = n;
	int hit_num;
	int hit_arr[input_num];
	int ret;

	while (live_num) {
		hit_num = 0;
		if (live_num > 2) {
			int min_idx = 0;
			hit_arr[hit_num ++] = 0;
			for (int i = 1; i < live_num; ++ i) {
				if (inputs[i]->key() < inputs[min_idx]->key()) {
					min_idx = i;
					hit_arr[0] = i;
					hit_num = 1;
				} else if ((! (inputs[i]->key() < inputs[min_idx]->key())) 
						&& (! (inputs[min_idx]->key() < inputs[i]->key()))) {
					hit_arr[hit_num ++] = i;
				}
			}
		} else if (2 == live_num) {
			if (inputs[0]->key() < inputs[1]->key()) {
				hit_arr[hit_num ++] = 0;
			} else if (inputs[1]->key() < inputs[0]->key()) {
				hit_arr[hit_num ++] = 1;
			} else {
				hit_arr[hit_num ++] = 0;
				hit_arr[hit_num ++] = 1;
			}
		} else {
			hit_arr[hit_num ++] = 0;
		}

		ret = output->merge(inputs, hit_arr, hit_num);
		if (ret < 0) {
			LOG_ERROR("merge error");
			return -1;
		}

		bool have_end = false;
		for (int i = 0; i < hit_num; ++ i) {
			ret = inputs[hit_arr[i]]->next();
			if (ret == 0) {
				have_end = true;
			} else if (ret < 0) {
				LOG_ERROR("read next error");
				return -1;
			}
		}

		if (have_end) {
			n = 0;
			for (int i = 0; i < live_num; ++ i) {
				if (inputs[i]->have()) {
					if (i != n) {
						inputs[n] = inputs[i];
					}
					n ++;
				}
			}
			live_num = n;
		}
	}
	return 0;
}

}

#endif
