#include "sylar/sylar.h"
#include<assert.h>

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

//assert(int expression)：断言，一种在程序中插入的条件检查，当表达式expression为0时，
//assert()将会发出一条错误消息，并终止程序的执行错误消息包括了断言失败的位置、文件名和行号等信息。
void test_assert() {
	SYLAR_LOG_INFO(g_logger) << sylar::BacktraceToString(10);
	//SYLAR_ASSERT(false);
	SYLAR_ASSERT2(0 == 1, "abcdefg xx");
}

int main(int argc, char** argv) {
	test_assert();
	return 0;
}