#ifndef _INTER_ALGORITHM_INTERFACE_HEADER_
#define _INTER_ALGORITHM_INTERFACE_HEADER_

#include "algorithm_interface.h"

class InterAlgorithmInterface : public AlgorithmInterface{
	public:
		InterAlgorithmInterface(DbCompany*& p_db_company, int interface_id):
			AlgorithmInterface(p_db_company, interface_id){
		}
		~InterAlgorithmInterface(){
		}

	public:
		int algorithm_core(int64_t req_id, 
				const AccessStr& access_str,
				VEC_CAND& vec_cand);
};
#endif
