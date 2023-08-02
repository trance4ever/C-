#ifndef __SYLAR_IOMANAGER_H__
#define __SYLAR_IOMANAGER_H__

#include "scheduler.h"
#include "timer.h"

namespace sylar {

class IOManager : public Scheduler, public TimerManager {
public:
	typedef std::shared_ptr<IOManager> ptr;
	typedef RWMutex RWMutexType;
	enum Event { //�¼�
		NONE  = 0x0,
		READ  = 0x1, //EPOLLIN
		WRITE = 0x4, //EPOLLOUT
	};
private:
	//�ļ�������
	struct FdContext {
		typedef Mutex MutexType;
		struct EventContext {
			Scheduler* scheduler = nullptr; 		//�¼�ִ�е�scheduler
			Fiber::ptr fiber;										//�¼���Э��
			std::function<void()> cb; 					//�¼��Ļص�����
		};
		
		EventContext& getContext(Event event);
		void resetContext(EventContext& ctx);
		void triggerEvent(Event event);
		EventContext read;										//���¼�
		EventContext write;										//д�¼�
		int fd = 0;												   	//�¼������ľ��
		Event events = NONE;  								//�Ѿ�ע����¼�
		MutexType mutex;					
	};
	
public:
	IOManager(size_t threads = 1, bool use_caller = true, const std::string& name = "");
	~IOManager();
	
	//0 success -1 error
	int addEvent(int fd, Event event, std::function<void()> cb = nullptr);
	bool delEvent(int fd, Event event);	
	bool cancelEvent(int fd, Event event);
	
	bool cancelAll(int fd);
	static IOManager* GetThis();

protected:
	void tickle() override;
  bool stopping() override;
	void idle() override;
	void onTimerInsertedAtFront() override;
	
	void contextResize(size_t size);
	bool stopping(uint64_t& timeout);
private:
	int m_epfd = 0;      //epollʾ�����ļ�������
	int m_tickleFds[2];  //m_tickleFds[0]��m_tickleFds[0]�ֱ�Ϊ�ܵ����˺�д�˵��ļ�������
	
	std::atomic<size_t> m_pendingEventCount = {0};
	RWMutexType m_mutex;
	std::vector<FdContext*> m_fdContexts;
};	
	
}

#endif