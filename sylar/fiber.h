#ifndef __SYLAR_FIBER_H__
#define __SYLAR_FIBER_H__

#include<memory>
#include "thread.h"
#include<functional>
#include<ucontext.h>

namespace sylar {
class Scheduler;
class Fiber : public std::enable_shared_from_this<Fiber> { //�̳��˸��࣬���޷���ջ�Ͻ�������,���ڳ�Ա��ͨ��
friend class Scheduler;                                    //shared_from_this()������ȡ��ǰ���������ָ��	
public:																								 
		typedef std::shared_ptr<Fiber> ptr;
		enum State {
			INIT,
			HOLD,
			EXEC,
			TERM,
			READY,
			EXCEPT
		};
private:
	Fiber();
public:
	Fiber(std::function<void()> cb, size_t stacksize = 0, bool user_caller = false);
	~Fiber();
	
	//����Э�̺�����������״̬
	//INIT,TERM
	void reset(std::function<void()> cb);
	//�л�����ǰЭ��ִ��
	void swapIn();
	//�л�����ִ̨��
	void swapOut();
	void call();
	void back();
	uint64_t getId() const {return m_id;}
	State getState() const { return m_state;}
public:
	//���ص�ǰЭ��
	static Fiber::ptr GetThis();
	//���õ�ǰЭ��
	static void SetThis(Fiber* f);		
	//Э���л�����̨����������ΪReady״̬
	static void YieldToReady();
	//Э���л�����̨����������ΪHold״̬
	static void YieldToHold();
	//��Э����
	static uint64_t TotalFibers();
	
	static void MainFunc();
	static void CallerMainFunc();
	static uint64_t GetFiberId();
private:
	uint64_t m_id = 0;
	uint32_t m_stacksize = 0; //ջ��С
	State m_state = INIT;
	
	ucontext_t m_ctx; //������
	void* m_stack = nullptr; //ջ�ռ�
	
	std::function<void()> m_cb;
};

}
#endif