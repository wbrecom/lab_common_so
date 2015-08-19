/*
 * 本文件是一些功能函数的封装
 * */

#ifndef _UTILITY_HEADER_
#define _UTILITY_HEADER_
#include <inttypes.h>
#include <strings.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <vector>
#include <string>
#include <map>
#include <set>
#include "zlib.h"
#include "math.h"
#include "base_define.h"
#include "woo/log.h"

#ifdef __GNUC__
#include <ext/hash_set>
#else
#include <hash_set>
#endif

using namespace std;

// 计算耗时，使用方式见work_tool/src下example_work_interface.cpp
struct timeval calc_spend_time(struct timeval tv_beg, const char* message,
    int& msec, bool begin_flag = false){

    struct timeval tv_temp;
    gettimeofday(&tv_temp, NULL);

    if(begin_flag)
        msec = -1;
    else
        msec = (tv_temp.tv_sec - tv_beg.tv_sec) * 1000 + (tv_temp.tv_usec - tv_beg.tv_usec) / 1000;

    LOG_INFO("%s: %d", message, msec);
    return tv_temp;
}

// 将字符串按照分隔符分割，转换成数字存放到集合中，若limit!=0，则最多存放limit个
void split_string_set_ids(__gnu_cxx::hash_set<uint64_t>& set_ids, uint32_t& id_num, 
			const char*& str_input, char sep_char, uint32_t limit = 0){
	const char* p_temp = str_input;

	id_num = 0;
	uint64_t id = 0;
	while((*p_temp) != '\0'){
		if(limit != 0 && id_num >= limit){
			return;
		}

		if ((*p_temp) >= '0' && (*p_temp) <= '9'){
			id = id * 10 + (*p_temp) - '0';
		}else if((*p_temp) == sep_char){
			set_ids.insert(id);
			id_num ++;
			id = 0;
		}
		p_temp ++;
	}
	set_ids.insert(id);
	id_num ++;
}

// 将字符串按照分隔符分割，转换成数字存放到数组中，若limit!=0，则最多存放limit个
void split_string_ids(uint64_t ids[], uint32_t& id_num, const char* str_input, 
			char sep_char, uint32_t limit = 0){
	const char* p_temp = str_input;

	id_num = 0;
	uint64_t id = 0;
	while((*p_temp) != '\0'){
		if(limit != 0 && id_num >= limit){
			return;
		}

		if ((*p_temp) >= '0' && (*p_temp) <= '9'){
			id = id * 10 + (*p_temp) - '0';
		}else if((*p_temp) == sep_char){
			ids[id_num] = id;
			id_num ++;
			id = 0;
		}
		p_temp ++;
	}
	ids[id_num] = id;
	id_num ++;
}

// 对于有两重分隔符的字符串进行处理，格式为<uint64_t, float>
typedef pair<uint64_t, float> PAIR_INT_FLOAT;
void split_string_vec_pair_ids(vector<PAIR_INT_FLOAT>& vec_ids, const char*& str_input, 
		char sep_char, char second_sep_char, uint32_t limit = 0){
	const char* p_temp = str_input;

	uint32_t id_num = 0;
	uint64_t id = 0;
	float score = 0.0f;
	int score_index = -1;
	bool score_flag = false;
	while((*p_temp) != '\0'){
		if(limit != 0 && id_num >= limit)
			return;

		if ((*p_temp) >= '0' && (*p_temp) <= '9'){
			if (score_flag){
				score = score * 10 + (*p_temp) - '0';
				score_index ++ ;
			}else{
				id = id * 10 + (*p_temp) - '0';
			}
		}else if((*p_temp) == sep_char){
			if(score_index == -1)
				score = score;
			else
				score = score / float(pow(10, score_index));

			pair<uint64_t, float> pair_r(id, score);
			vec_ids.push_back(pair_r);
			id_num ++;
			
			id = 0;
			score = 0.0f;
			score_flag = false;
			score_index = -1;
		}else if((*p_temp) == '.'){
			score_index = 0;
		}else if((*p_temp) == second_sep_char){
			score_flag = true;
		}
			p_temp ++;
	}
	if(score_index == -1)
		score = score;
	else
		score = score / float(pow(10, score_index));

	pair<uint64_t, float> pair_r(id, score);
	vec_ids.push_back(pair_r);
	id_num ++;
}

void split_string(vector<string>& vec_str, const string& str_input, char sep_char){
    string::size_type pos1, pos2;
    pos2 = str_input.find(sep_char);
    pos1 = 0;
    while (string::npos != pos2)
    {
        vec_str.push_back(str_input.substr(pos1, pos2 - pos1));

        pos1 = pos2 + 1;
        pos2 = str_input.find(sep_char, pos1);
    }
    vec_str.push_back(str_input.substr(pos1));
}

void split_map_string(map<std::string, std::string>& map_result, const string& str_input,
		char first_char, char second_char, int16_t limit = -1){
	vector<string> vec_str;

	int16_t index = 0;
	split_string(vec_str, str_input, first_char);
	for(vector<string>::iterator it = vec_str.begin(); it != vec_str.end(); it++){
		if(limit != -1 && index > limit){
			break;
		}
		vector<string> temp_str;
		split_string(temp_str, (*it), second_char);

		if(temp_str.size() < 2)
			map_result.insert(map<std::string, std::string>::value_type(temp_str[0], "1"));
		else
			map_result.insert(map<std::string, std::string>::value_type(temp_str[0], temp_str[1]));
		index ++;
	}
}

typedef std::vector<std::string> VEC_STR;
typedef std::map<int, VEC_STR> MAP_VEC_STR;

void split_map_vec_string(map<int, VEC_STR>& map_result, const string& str_input,
		char first_char, char second_char, int16_t limit = -1){
	vector<string> vec_str;

	int16_t index = 0;
	split_string(vec_str, str_input, first_char);
	for(vector<string>::iterator it = vec_str.begin(); it != vec_str.end(); it++){
		if(limit != -1 && index > limit){
			break;
		}
		vector<string> temp_str;
		split_string(temp_str, (*it), second_char);

		size_t temp_size = temp_str.size();
		if(temp_size < 2){
			continue;
		}else if(temp_size < 3){
			//std::vector<std::string> temp_vec;
			VEC_STR temp_vec;
			temp_vec.push_back(temp_str[1]);
			temp_vec.push_back("1");
			map_result.insert(map<int, 
					VEC_STR>::value_type(atoi(temp_str[0].c_str()), temp_vec));
		}
		else{
			//std::vector<std::string> temp_vec;
			VEC_STR temp_vec;
			temp_vec.push_back(temp_str[1]);
			temp_vec.push_back(temp_str[2]);

			map_result.insert(map<int,
					VEC_STR>::value_type(atoi(temp_str[0].c_str()), temp_vec));
		}
		index ++;
	}
}

int get_char_num(const char* p_src, char find_char){
	int result = 0;

	const char* p_src_temp = p_src;

	while(*p_src_temp != '\0'){
		if(*p_src_temp == find_char)
			result ++;
		p_src_temp ++;
	}
	return result;
}

bool strstr_k(const char* p_src, const char* p_sub, char end_char){
	const char* p_src_temp = p_src;
	const char* p_sub_temp = p_sub;
	
	bool b_flag = false;
	while(*p_src_temp != '\0' && *p_sub_temp != '\0'){
		if((*p_src_temp) != (*p_sub_temp)){
			p_src_temp ++;
			p_sub_temp = p_sub;
		}else{
			p_src_temp ++;
			p_sub_temp ++;

			if(*p_sub_temp == '\0' && 
				(*p_src_temp == '\0' || *p_src_temp == end_char)){
				b_flag = true;
				break;
			}
		}
	}
	return b_flag;
}


// 对32位，将大端模式转成小端模式
uint32_t transformatUInt32(uint32_t value){
	return (value & 0x000000FFU) << 24 | (value & 0x0000FF00U) << 8 |
		(value & 0x00FF0000U) >> 8 | (value & 0xFF000000U) >> 24;
}

// 对64位，将大端模式转成小端模式
// 先将64位的低32位转成小端模式，再将64位的高32位转成小端模式
// 在将原来的低32位放置到高32位，原来的高32位放置到低32位
uint64_t transformatUInt64(uint64_t value){
	uint64_t high_uint64 = (uint64_t)(transformatUInt32(uint32_t(value)));         // 低32位转成小端
	uint64_t low_uint64 = (uint64_t)(transformatUInt32(uint32_t(value >> 32)));  // 高32位转成小端
    return (high_uint64 << 32) + low_uint64;
}

#endif
