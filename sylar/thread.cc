#include "thread.h"
#include "log.h"
#include "util.h"

namespace sylar{
	
	static thread_local Thread* t_thread = nullptr; // 定义线程局部变量，每个线程拥有一份独立的变量副本，生命周
	static thread_local std::string t_thread_name = "UNKNOW";	// 期与本线程一致，该关键字只能用于静态或全局变量
	static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");	
		
	Semaphore::Semaphore(uint32_t count) {
		if(sem_init(&m_semaphore, 0, count)) {
			throw std::logic_error("sem_init error");
		}
	}
	
	Semaphore::~Semaphore() {
	  sem_destroy(&m_semaphore);	
	}
	//如果信号量大于0，则信号量减1，如果信号量小于0，则该线程阻塞在此处
	void Semaphore::wait() {
		if(sem_wait(&m_semaphore)) {
			throw std::logic_error("sem_wait error");
		}
	}
	//信号量加1，如果大于0，唤醒阻塞在sem_wait()处的线程
	void Semaphore::notify() {
		if(sem_post(&m_semaphore)) {
			throw std::logic_error("sem_post error");
		}
	}
		
	//返回thread类实例化的指针
	Thread* Thread::GetThis() {
		return t_thread;
	}
	//获取线程名称
	const std::string& Thread::GetName() {
		return t_thread_name;
	}
  //设置线程名称
  void Thread::SetName(const std::string& name) {
  		if(t_thread) {
  			t_thread->m_name = name;
  		}
  		t_thread_name = name;
  }
  //构造变量赋值，并且创建一个新线程，线程中运行m_cb
	Thread::Thread(std::function<void()> cb, const std::string& name)
		:m_cb(cb)
		, m_name(name) {
		if(name.empty()) {
			m_name = "UNKNOW";
		}
		int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
		if(rt) {
			SYLAR_LOG_ERROR(g_logger) << "pthread_creat thread fail, rt=" << rt
					<< " name=" << m_name;
			throw std::logic_error("pthread_create error");
		}
		m_semaphore.wait();
	}
	
	Thread::~Thread() {
		if(m_thread) {
			pthread_detach(m_thread);
		}
	}
	//pthread_join()会一直阻塞调用它的进程，直至目标线程执行结束
	void Thread::join() {
		if(m_thread) {
			int rt = pthread_join(m_thread, nullptr);
			if(rt) {
				SYLAR_LOG_ERROR(g_logger) << "pthread_join thread fail, rt=" << rt
					<< " name=" << m_name;
			throw std::logic_error("pthread_join error");
			}
			m_thread = 0;
		}
	}
	
	void* Thread::run(void* arg) {
		Thread* thread = (Thread*)arg;
		t_thread = thread;
		t_thread_name = thread->m_name;
		thread->m_id = sylar::GetThreadId();
		pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());
		
		std::function<void()> cb;
		cb.swap(thread->m_cb); // cb.swap(m_cb),交换两个对象的地址
		
		thread->m_semaphore.notify();
		cb();
		return 0;
	}
		
		
	
}