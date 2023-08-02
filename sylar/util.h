#ifndef _SYLAR_UTIL_H_H
#define _SYLAR_UTIL_H_H
#include<pthread.h>
#include<sys/types.h>
#include<sys/syscall.h>
#include<stdio.h>
#include<unistd.h>
#include<stdint.h>
#include<vector>
#include<string>

namespace sylar {
	
	pid_t GetThreadId();
	uint32_t GetFiberId();
	
	void Backtrace(std::vector<std::string>& bt, int size = 64, int skip = 1);
	std::string BacktraceToString(int size = 64, int skip = 2, const std::string& prefix = "");
		
	//Ê±¼ä
	uint64_t GetCurrentMS();
	uint64_t GetCurrentUS();
}

#endif