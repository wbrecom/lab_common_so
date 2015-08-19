#ifndef _EXAMPLE_WORK_INTERFACE_HEADER_
#define _EXAMPLE_WORK_INTERFACE_HEADER_

#include "work_interface.h"
#include "ini_file.h"
#include "woo/log.h"
#include <map>
#include <set>
#include <vector>
#include <fstream>
#include <iostream>
#include "json.h"
#include <time.h>
#ifdef __GNUC__
#include <ext/hash_map>
#else
#include <hash_map>
#endif

class ExampleWorkInterface : public WorkInterface{
	public:
		ExampleWorkInterface(DbCompany*& p_db_company, int interface_id):
		WorkInterface(p_db_company, interface_id){
		}
		~ExampleWorkInterface(){
		}

	public:
		int work_core(json_object *req_json, char* &p_out_string, int& n_out_len, int64_t req_id);
			
};
#endif
