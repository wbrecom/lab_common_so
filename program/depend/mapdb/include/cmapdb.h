#ifndef _CMAPDB_HEADER_
#define _CMAPDB_HEADER_
#include <signal.h>
#include "woo/log.h"
#include "woo/tcpserver.h"

#include "woo/keyblockfile2.h"
#include "woo/config.h"
#include "json/json.h"
#include "zlib.h"

typedef struct _data_index_t {
	uint64_t id;
	uint64_t off;
	uint32_t len;
} data_index_t;

struct data_index_less {
    bool operator()(const data_index_t &x , const data_index_t &y) {
		return x.id < y.id;
	}
};

typedef woo::KeyBlockFileMmap2<data_index_t, data_index_less> IndexedData;

typedef struct _indexed_data_info_t {
	IndexedData *indexed_data;
	uint64_t version;
	char index_path[PATH_MAX];
	char block_path[PATH_MAX];
} indexed_data_info_t; 

typedef std::vector<indexed_data_info_t> IndexedDataArray;

struct data_struct{
public:
	data_struct(const char* data_ptr, size_t data_len){
		data_ptr_  = data_ptr;
		data_len_ = data_len;
	}
	data_struct(){
		data_ptr_ = NULL;
		data_len_ = 0;
	}
	const char* data_ptr_;
	size_t data_len_;
};

struct data_index_t_k{
    data_index_t data_index_t_;
    int i_;
};

struct data_index_less_k {
    bool operator()(const data_index_t_k &x , const data_index_t_k &y) {
        return x.data_index_t_.id < y.data_index_t_.id;
    }
};

class MapDb{
	public:
		MapDb():indexed_data_array_(NULL), 
		lru_ut_cache_mgr_uc_(NULL), cache_buf_len_(0), nofound_result_len_(0){

		}
		~MapDb(){
			IndexedDataArray null_index_data_array;
			free_data(indexed_data_array_, &null_index_data_array);
		}
	private:
		char multi_data_path_[PATH_MAX];
		char nofound_result_[1024];
		
		char conf_[PATH_MAX];
		
		IndexedDataArray *indexed_data_array_;
		woo::LruUtCacheMgrUc* lru_ut_cache_mgr_uc_;
		
		int cache_buf_len_;
		size_t nofound_result_len_;

	private:
	
		IndexedDataArray* load_data(const char *path, IndexedDataArray *old);
	
		void free_data(IndexedDataArray *arr, IndexedDataArray *new_arr);
		
		void *monitor(void *arg);

		int query_indexed_data(IndexedDataArray *arr, uint64_t id, const char **data_ptr, size_t *data_len);
	
		int query_indexed_data_mget(IndexedDataArray *arr, uint64_t ids[], int num, data_struct array_data_struct[]);

	public:
		int get(uint64_t id, size_t& data_len, const char* &resp_body);

		int mget(uint64_t result_array[], int result_num, const char* resp_body[], int& resp_num);
		
		int initialize(const char* path);

		int load(const char*);

};
#endif
