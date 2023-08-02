#include "iomanager.h"
#include "macro.h"
#include<unistd.h>
#include<sys/epoll.h>
#include "log.h"
#include<fcntl.h>
#include<errno.h>
#include<string.h>

namespace sylar {
	
	static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");
	//根据Event类型返回对应的EventContext事件
	IOManager::FdContext::EventContext& IOManager::FdContext::getContext(IOManager::Event event) {
		switch(event) {
			case IOManager::READ:
				return read;
			case IOManager::WRITE:
				return write;
			default:
				SYLAR_ASSERT2(false, "getContext");
		}
	}
	//重置EventContext ctx的内容
	void IOManager::FdContext::resetContext(EventContext& ctx) {
		ctx.scheduler = nullptr;
		ctx.fiber.reset();
		ctx.cb = nullptr;
	}
	//像events中添加event事件，如果event不为空，如果对应事件的方法已经定义，调度这个方法，没有则调度相应协程
	void IOManager::FdContext::triggerEvent(IOManager::Event event) {
		SYLAR_ASSERT(events & event);
		events = (Event)(events & ~event);
		EventContext& ctx = getContext(event);
		if(ctx.cb) {
			ctx.scheduler->schedule(&ctx.cb);
		}
		else {
			ctx.scheduler->schedule(&ctx.fiber);
		}
		ctx.scheduler = nullptr;
		return;
	}
	//创建一个epoll示例，创建一个管道，设置管道的读端文件描述符的属性，设定epoll_event类型触发事件
	//将该文件描述符和epoll_event类型事件注册到epoll示例中
	IOManager::IOManager(size_t threads, bool use_caller, const std::string& name)
		:Scheduler(threads, use_caller, name) {
		m_epfd = epoll_create(5000); // m_epfd接受一个文件描述符,用于后续对epoll示例的操作
		SYLAR_ASSERT(m_epfd > 0);
		
		int rt = pipe(m_tickleFds);
		SYLAR_ASSERT(!rt);
		epoll_event event;
		memset(&event, 0, sizeof(epoll_event));
		event.events = EPOLLIN | EPOLLET; // EPOLLIN: 关注可读事件 EPOLLET: 边缘触发
		event.data.fd = m_tickleFds[0];
		//fcntl: 修改文件描述符的属性
		rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK); //将文件描述符m_tickleFds[0]的属性设置为
		SYLAR_ASSERT(!rt);										              //非阻塞模式
		
		rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event); //将m_tickleFds[0]文件描述符添加到epoll示例
		SYLAR_ASSERT(!rt);								
		contextResize(32);															 //m_epfd中，设定相应的事件和属性为event

		start();
	}
	
	IOManager::~IOManager() {
		stop();
		close(m_epfd);
		close(m_tickleFds[0]);
		close(m_tickleFds[1]);
		
		for(size_t i = 0; i < m_fdContexts.size(); ++i) {
			if(m_fdContexts[i]) {
				delete m_fdContexts[i];
			}
		}
	}
	//重置m_fdContexts容器
	void IOManager::contextResize(size_t size) {
		m_fdContexts.resize(size);
		
		for(size_t i = 0; i < m_fdContexts.size(); ++i) {
			if(!m_fdContexts[i]) {
				m_fdContexts[i] = new FdContext;
				m_fdContexts[i]->fd = i;
			}
		}
	}
	
	//向epoll示例中对fd文件描述符注册event事件，并向该文件描述符添加cb方法来处理事件
	//0 success, -1 error
	int IOManager::addEvent(int fd, Event event, std::function<void()> cb) {
		FdContext* fd_ctx = nullptr;
		RWMutexType::ReadLock lock(m_mutex);
		if((int)m_fdContexts.size() > fd) {
			fd_ctx = m_fdContexts[fd];
			lock.unlock();
		}
		else {
			lock.unlock();
			RWMutexType::WriteLock lock2(m_mutex);
			contextResize(fd * 1.5);
			fd_ctx = m_fdContexts[fd];
		}
		
		FdContext::MutexType::Lock lock2(fd_ctx->mutex);
		if(fd_ctx->events & event) {
			SYLAR_LOG_ERROR(g_logger) << "addEvent assert fd=" << fd
					<< " event=" << event
					<< " fd_ctx.event=" << fd_ctx->events;
			SYLAR_ASSERT(!(fd_ctx->events & event));
		}
		
		int op = fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
		epoll_event epevent;
		epevent.events = EPOLLET | fd_ctx->events | event;
		epevent.data.ptr = fd_ctx;
		
		int rt = epoll_ctl(m_epfd, op, fd, &epevent);
		if(rt) {
			SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
					<< op << "," << fd << ", " << epevent.events << "):"
					<< rt << " (" << errno << ") (" << strerror(errno) << ")";
			return -1;		
		}
		
		++m_pendingEventCount;
		fd_ctx->events = (Event)(fd_ctx->events | event);
		FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
		SYLAR_ASSERT(!event_ctx.scheduler && !event_ctx.fiber && !event_ctx.cb);
		event_ctx.scheduler = Scheduler::GetThis();
		if(cb) {
			event_ctx.cb.swap(cb);
		}
		else {
			event_ctx.fiber = Fiber::GetThis();
			SYLAR_ASSERT(event_ctx.fiber->getState() == Fiber::EXEC);
		}
		return 0;
	}
	
	//fd文件描述符删除event事件
	bool IOManager::delEvent(int fd, Event event) {
		RWMutexType::ReadLock lock(m_mutex);
		if((int)m_fdContexts.size() <= fd) {
			return false;
		}
		FdContext* fd_ctx = m_fdContexts[fd];
		lock.unlock();
		FdContext::MutexType::Lock lock2(fd_ctx->mutex);
		if(!(fd_ctx->events & event)) {
			return false;
		}
		
		Event new_events = (Event)(fd_ctx->events & ~event);
		int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
		epoll_event epevent;
		epevent.events = EPOLLET | new_events;
		epevent.data.ptr = fd_ctx;
		
		int rt = epoll_ctl(m_epfd, op, fd, &epevent);
		if(rt) {
			SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
					<< op << "," << fd << ", " << epevent.events << "):"
					<< rt << " (" << errno << ") (" << strerror(errno) << ")";
			return false;
		}
		--m_pendingEventCount;
		fd_ctx->events = new_events;
		FdContext::EventContext& event_ctx = fd_ctx->getContext(event);
		fd_ctx->resetContext(event_ctx);
		return true;
	}
	
	//在epoll实例中删除fd中注册的event事件，并且触发该事件
	bool IOManager::cancelEvent(int fd, Event event) {
		RWMutexType::ReadLock lock(m_mutex);
		if((int)m_fdContexts.size() <= fd) {
			return false;
		}
		FdContext* fd_ctx = m_fdContexts[fd];
		lock.unlock();
		FdContext::MutexType::Lock lock2(fd_ctx->mutex);
		if(!(fd_ctx->events & event)) {
			return false;
		}
		
		Event new_events = (Event)(fd_ctx->events & ~event);
		int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
		epoll_event epevent;
		epevent.events = EPOLLET | new_events;
		epevent.data.ptr = fd_ctx;
		
		int rt = epoll_ctl(m_epfd, op, fd, &epevent);
		if(rt) {
			SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
					<< op << "," << fd << ", " << epevent.events << "):"
					<< rt << " (" << errno << ") (" << strerror(errno) << ")";
			return false;
		}
		fd_ctx->triggerEvent(event);
		--m_pendingEventCount;
		return true;
	}
	
	//删除fd在epoll实例中注册的所有事件，并一一触发它们
	bool IOManager::cancelAll(int fd) {
		RWMutexType::ReadLock lock(m_mutex);
		if((int)m_fdContexts.size() <= fd) {
			return false;
		}
		FdContext* fd_ctx = m_fdContexts[fd];
		lock.unlock();
		FdContext::MutexType::Lock lock2(fd_ctx->mutex);
		if(!fd_ctx->events) {
			return false;
		}
		
		int op = EPOLL_CTL_DEL;
		epoll_event epevent;
		epevent.events = 0;
		epevent.data.ptr = fd_ctx;
		
		int rt = epoll_ctl(m_epfd, op, fd, &epevent);
		if(rt) {
			SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
					<< op << "," << fd << ", " << epevent.events << "):"
					<< rt << " (" << errno << ") (" << strerror(errno) << ")";
			return false;
		}
		
		if(fd_ctx->events & READ) {
			fd_ctx->triggerEvent(READ);
			--m_pendingEventCount;
		}
		if(fd_ctx->events & WRITE) {
			fd_ctx->triggerEvent(WRITE);
			--m_pendingEventCount;
		}
		SYLAR_ASSERT(fd_ctx->events == 0);
		return true;
	}
	
	IOManager* IOManager::GetThis() {
		return dynamic_cast<IOManager*>(Scheduler::GetThis());
	}
	
	//在管道写端写入数据
	void IOManager::tickle() {
		if(!hasIdleThreads()) {
			return;
		}
		//如果写入成功，rt为写入字节数,这段代码表示像m_tickleFds[1]所关联的文件描述符
		//写入长度为1的内容，内容为"T""
		int rt = write(m_tickleFds[1], "T", 1);
		SYLAR_ASSERT(rt == 1);
	}
	
  bool IOManager::stopping() {
  	uint64_t timeout = 0;
  	return stopping(timeout);
  }
  
  //空闲则等待事件发生然后调度它们
	void IOManager::idle() {
		epoll_event* events = new epoll_event[64]();
		//自定义删除器的共享指针
		std::shared_ptr<epoll_event> shared_events(events, [](epoll_event* ptr){
			delete[] ptr;
		});
		
		while(true) {
			uint64_t next_timeout = 0;
			//停止则结束执行
			if(stopping(next_timeout)) {
				SYLAR_LOG_INFO(g_logger) << "name=" << getName() << " idle stopping exit";
				break;			
			}
			
			int rt = 0;
			//等待至少一个事件发生
			do {
				static const int MAX_TIMEOUT = 3000;
				if(next_timeout != ~0ull) {
					next_timeout = (int)next_timeout > MAX_TIMEOUT ? MAX_TIMEOUT : next_timeout;
				}
				else {
					next_timeout = MAX_TIMEOUT;
				}
				//rt表示等待期间发生的事件个数,epoll_wait将阻塞在次数，直到m_epfd关联的文件描述符
				//至少有一个事件发生。第二个参数events为事件类型的数组，存储事件信息，后续为数组大小，最后为超时时间
				//如果没有事件发生，返回值为0
				rt = epoll_wait(m_epfd, events, 64, (int)next_timeout);
				
				if(rt < 0 && errno == EINTR) {
					
				}
				else {
					break;
				}
			}
			while(true);
			std::vector<std::function<void()>> cbs;
			listExpiredCb(cbs);
			if(!cbs.empty()) {
				schedule(cbs.begin(), cbs.end());
				cbs.clear();
			}
			//
			for(int i = 0; i < rt; ++i) {
				epoll_event& event = events[i];
				if(event.data.fd == m_tickleFds[0]) {
					uint8_t dummy;
					//读取m_fickleFds[0]的文件描述符中的内容，并将变量存储到dummy变量中
					//如果读取成功，read()返回读取的字节长度
					while(read(m_tickleFds[0], &dummy, 1) == 1); 
					continue;
				}
				FdContext* fd_ctx = (FdContext*)event.data.ptr;
				FdContext::MutexType::Lock lock(fd_ctx->mutex);
				if(event.events & (EPOLLERR | EPOLLHUP)) {
					event.events |= EPOLLIN | EPOLLOUT;
				}
				int real_events = NONE;
				if(event.events & EPOLLIN) {
					real_events |= READ;
				}
				if(event.events & EPOLLOUT) {
					real_events |= WRITE;
				}
				
				if(!(fd_ctx->events & real_events)) {
					continue;
				}
				
				int left_events = (fd_ctx->events & ~real_events);
				int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
				event.events = EPOLLET | left_events;
				
				int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
				if(rt2) {
					SYLAR_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
					<< op << "," << fd_ctx->fd << ", " << event.events << "):"
					<< rt2 << " (" << errno << ") (" << strerror(errno) << ")";
					continue;
				}
				
				if(real_events & READ) {
					fd_ctx->triggerEvent(READ);
					--m_pendingEventCount;
				}
				if(real_events & WRITE) {
					fd_ctx->triggerEvent(WRITE);
					--m_pendingEventCount;
				}
			}
			
			Fiber::ptr cur = Fiber::GetThis();
			auto raw_ptr = cur.get();
			cur.reset();
			raw_ptr->swapOut();
		}
	}
	
	void IOManager::onTimerInsertedAtFront() {
		tickle();
	}
	
	bool IOManager::stopping(uint64_t& timeout) {
		timeout = getNextTimer();
		return timeout == ~0ull
			&& m_pendingEventCount == 0
			&& Scheduler::stopping();
	}
}