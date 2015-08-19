/*
 * 本文件定义了算法入口使用的参数
 * */

#ifndef __CANDUSER_H__
#define __CANDUSER_H__

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "zlib.h"
#include <string>
#include <vector>

const static int MAX_BUF_SIZE = 1024*1024*10;

//推荐候选，用户id，总得分，四种桥梁理由
struct candidate_item_t {
	uint64_t uid;
	uint64_t tscore;
	uint32_t utype;                     // 用户类型
};

typedef std::vector<candidate_item_t*> VEC_CAND; //定义算法的入口

bool cmp_candidate_by_tscore(const candidate_item_t* a, const candidate_item_t* b){
	if(a != NULL && b != NULL){
		return a->tscore > b->tscore;
	}
	return true;
}


#endif
