#include "util.h"
#include<execinfo.h>
#include<sys/time.h>
#include "log.h"
#include "fiber.h"
namespace sylar {
	
	static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
	// 获取线程Id
	pid_t GetThreadId() {
		return syscall(SYS_gettid);
	}
	
	//获取协程Id
	uint32_t GetFiberId() {
	  return sylar::Fiber::GetFiberId();
	}
	//skip：跳过的层数
	void Backtrace(std::vector<std::string>& bt, int size, int skip) {
		void** array = (void**)malloc(sizeof(void*) * size);
		size_t s = ::backtrace(array, size); //::前空白保证是全局作用域的backtrack()函数
		
		char** strings = backtrace_symbols(array, s);
		if(strings == NULL) {
			SYLAR_LOG_ERROR(g_logger) << "backtrace_symbols error";
			return;
		}
		
		for(size_t i = skip; i < s; ++i) {
			bt.push_back(strings[i]);
		}
		free(strings);
		free(array);
	}
	
	std::string BacktraceToString(int size, int skip, const std::string& prefix) {
		std::vector<std::string> bt;
		Backtrace(bt, size, skip);
		std::stringstream ss;
		for(size_t i = 0; i < bt.size(); ++i) {
			ss << prefix << bt[i] << std::endl;
		}
		return ss.str();
	}
	
	uint64_t GetCurrentMS() {
		struct timeval tv;
		gettimeofday(&tv, NULL);
		return tv.tv_sec * 1000ul + tv.tv_usec / 1000;
	}
	uint64_t GetCurrentUS() {
		struct timeval tv;
		gettimeofday(&tv, NULL);
		return tv.tv_sec * 1000 * 1000ul + tv.tv_usec;
	}
}