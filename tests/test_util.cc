#include "sylar/sylar.h"
#include<assert.h>

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

//assert(int expression)�����ԣ�һ���ڳ����в����������飬�����ʽexpressionΪ0ʱ��
//assert()���ᷢ��һ��������Ϣ������ֹ�����ִ�д�����Ϣ�����˶���ʧ�ܵ�λ�á��ļ������кŵ���Ϣ��
void test_assert() {
	SYLAR_LOG_INFO(g_logger) << sylar::BacktraceToString(10);
	//SYLAR_ASSERT(false);
	SYLAR_ASSERT2(0 == 1, "abcdefg xx");
}

int main(int argc, char** argv) {
	test_assert();
	return 0;
}