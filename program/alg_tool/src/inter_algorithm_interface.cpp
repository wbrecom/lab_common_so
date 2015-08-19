#include "inter_algorithm_interface.h"

DYN_ALGORITHMS(InterAlgorithmInterface);

int InterAlgorithmInterface::algorithm_core(int64_t req_id, 
		const AccessStr& access_str,
		VEC_CAND& vec_cand){
		
	LOG_DEBUG("inter algorithm interface!");
	return 1;
}
