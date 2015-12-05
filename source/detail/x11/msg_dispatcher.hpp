/*
 *	Message Dispatcher Implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/detail/msg_dispatcher.hpp
 *
 *  @DO NOT INCLUDE THIS HEADER FILE IN YOUR SOURCE FILE!!
 *
 *	This class msg_dispatcher provides a simulation of Windows-like message
 *	dispatcher. Every event is dispatched into its own message queue for
 *	corresponding thread.
 */

#ifndef NANA_DETAIL_MSG_DISPATCHER_HPP
#define NANA_DETAIL_MSG_DISPATCHER_HPP
#include <nana/detail/linux_X11/msg_packet.hpp>
#include <nana/system/platform.hpp>
#include <list>
#include <set>
#include <map>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <thread>

namespace nana
{
namespace detail
{
	class msg_dispatcher
	{
		struct thread_binder
		{
			unsigned tid;
			std::mutex	mutex;
			std::condition_variable	cond;
			std::list<msg_packet_tag>	msg_queue;
			std::set<Window> window;
		};

	public:
		typedef msg_packet_tag	msg_packet;
		typedef void (*timer_proc_type)(unsigned tid);
		typedef void (*event_proc_type)(Display*, msg_packet_tag&);
		typedef int (*event_filter_type)(XEvent&, msg_packet_tag&);

		typedef std::list<msg_packet_tag> msg_queue_type;

		msg_dispatcher(Display* disp)
			: display_(disp), is_work_(false)
		{
			proc_.event_proc = 0;
			proc_.timer_proc = 0;
			proc_.filter_proc = 0;
		}

		void set(timer_proc_type timer_proc, event_proc_type event_proc, event_filter_type filter)
		{
			proc_.timer_proc = timer_proc;
			proc_.event_proc = event_proc;
			proc_.filter_proc = filter;
		}

		void insert(Window wd)
		{
			unsigned tid = nana::system::this_thread_id();

			bool start_driver;

			{
				std::lock_guard<decltype(table_.mutex)> lock(table_.mutex);

				//No thread is running, so msg dispatcher should start the msg driver.
				start_driver = (0 == table_.thr_table.size());
				thread_binder * thr;

				std::map<unsigned, thread_binder*>::iterator i = table_.thr_table.find(tid);
				if(i == table_.thr_table.end())
				{
					thr = new thread_binder;
					thr->tid = tid;
					table_.thr_table.insert(std::make_pair(tid, thr));
				}
				else
					thr = i->second;

				thr->mutex.lock();
				thr->window.insert(wd);
				thr->mutex.unlock();
			
				table_.wnd_table[wd] = thr;
			}

			if(start_driver && proc_.event_proc && proc_.timer_proc)
			{
				//It should start the msg driver, before starting it, the msg driver must be inactive.
				if(thrd_)
				{
					is_work_ = false;
					thrd_->join();
				}
				is_work_ = true;
				thrd_ = std::unique_ptr<std::thread>(new std::thread([this](){ this->_m_msg_driver(); }));
			}
		}

		void erase(Window wd)
		{
			std::lock_guard<decltype(table_.mutex)> lock(table_.mutex);
			
			auto i = table_.wnd_table.find(wd);
			if(i != table_.wnd_table.end())
			{
				thread_binder * const thr = i->second;
				std::lock_guard<decltype(thr->mutex)> lock(thr->mutex);
				for(auto li = thr->msg_queue.begin(); li != thr->msg_queue.end();)
				{
					if(wd == _m_window(*li))
						li = thr->msg_queue.erase(li);
					else
						++li;
				}

				table_.wnd_table.erase(i);
				thr->window.erase(wd);
				
				//There still is at least one window alive.
				if(thr->window.size())
				{
					//Make a cleanup msg packet to infor the dispatcher the window is closed.
					msg_packet_tag msg;
					msg.kind = msg.kind_cleanup;
					msg.u.packet_window = wd;
					thr->msg_queue.push_back(msg);
				}
			}
		}

		void dispatch(Window modal)
		{
			unsigned tid = nana::system::this_thread_id();
			msg_packet_tag msg;
			int qstate;
			
			//Test whether the thread is registered for window, and retrieve the queue state for event
			while((qstate = _m_read_queue(tid, msg, modal)))
			{
				//the queue is empty
				if(-1 == qstate)
				{
					if(false == _m_wait_for_queue(tid))
						proc_.timer_proc(tid);
				}
				else
				{
					proc_.event_proc(display_, msg);
				}
			}
		}
	private:
		void _m_msg_driver()
		{
			int fd_X11 = ConnectionNumber(display_);

			msg_packet_tag msg_pack;
			XEvent event;
			while(is_work_)
			{
				int pending;
				{
					nana::detail::platform_scope_guard lock;
					pending = ::XPending(display_);
					if(pending)
					{
						::XNextEvent(display_, &event);
						if(::XFilterEvent(&event, None))
							continue;
					}
				}

				if(0 == pending)
				{
					fd_set fdset;
					FD_ZERO(&fdset);
					FD_SET(fd_X11, &fdset);

					struct timeval tv;
					tv.tv_usec = 10000;
					tv.tv_sec = 0;
					::select(fd_X11 + 1, &fdset, 0, 0, &tv);
				}
				else
				{
					switch(proc_.filter_proc(event, msg_pack))
					{
					case 0:
						msg_pack.kind = msg_pack.kind_xevent;
						msg_pack.u.xevent = event;
					case 1:
						_m_msg_dispatch(msg_pack);
					}
				}
			}
		}
	private:
		static Window _m_event_window(const XEvent& event)
		{
			switch(event.type)
			{
			case MapNotify:
			case UnmapNotify:
				return event.xmap.window;
			}
			return event.xkey.window;
		}

		static Window _m_window(const msg_packet_tag& pack)
		{
			switch(pack.kind)
			{
			case msg_packet_tag::kind_xevent:
				return _m_event_window(pack.u.xevent);
			case msg_packet_tag::kind_mouse_drop:
				return pack.u.mouse_drop.window;
			default:
				break;
			}
			return 0;
		}

		void _m_msg_dispatch(const msg_packet_tag &msg)
		{
			std::lock_guard<decltype(table_.mutex)> lock(table_.mutex);
			auto i = table_.wnd_table.find(_m_window(msg));
			if(i != table_.wnd_table.end())
			{
				thread_binder * const thr = i->second;
				
				std::lock_guard<decltype(thr->mutex)> lock(thr->mutex);
				thr->msg_queue.push_back(msg);
				thr->cond.notify_one();
			}
		}

		//_m_read_queue
		//@brief:Read the event from a specified thread queue.
		//@return: 0 = exit the queue, 1 = fetch the msg, -1 = no msg
		int _m_read_queue(unsigned tid, msg_packet_tag& msg, Window modal)
		{
			bool stop_driver = false;

			{
				std::lock_guard<decltype(table_.mutex)> lock(table_.mutex);
				//Find the thread whether it is registered for the window.
				auto i = table_.thr_table.find(tid);
				if(i != table_.thr_table.end())
				{
					if(i->second->window.size())
					{
						msg_queue_type & queue = i->second->msg_queue;
						if(queue.size())
						{
							msg = queue.front();
							queue.pop_front();

							//Check whether the event dispatcher is used for the modal window
							//and when the modal window is closing, the event dispatcher would
							//stop event pumping.
							if((modal == msg.u.packet_window) && (msg.kind == msg.kind_cleanup))
								return 0;

							return 1;
						}
						else
							return -1;
					}

					delete i->second;
					table_.thr_table.erase(i);
					stop_driver = (table_.thr_table.size() == 0);
				}
			}
			if(stop_driver)
			{
				is_work_ = false;
				thrd_->join();
				thrd_.reset();
			}
			return 0;
		}

		//_m_wait_for_queue
		//	wait for the insertion of queue.
		//return@ it returns true if the queue is not empty, otherwise the wait is timeout.
		bool _m_wait_for_queue(unsigned tid)
		{
			thread_binder * thr = nullptr;
			{
				std::lock_guard<decltype(table_.mutex)> lock(table_.mutex);
				auto i = table_.thr_table.find(tid);
				if(i != table_.thr_table.end())
				{
					if(i->second->msg_queue.size())
						return true;
					thr = i->second;
				}
			}
			
			//Waits for notifying the condition variable, it indicates a new msg is pushing into the queue.
			std::unique_lock<decltype(thr->mutex)> lock(thr->mutex);
			return (thr->cond.wait_for(lock, std::chrono::milliseconds(10)) != std::cv_status::timeout);
		}
		
	private:
		Display * display_;
		volatile bool is_work_;
		std::unique_ptr<std::thread> thrd_;

		struct table_tag
		{
			std::recursive_mutex mutex;
			std::map<unsigned, thread_binder*> thr_table;
			std::map<Window, thread_binder*> wnd_table;
		}table_;

		struct proc_tag
		{
			timer_proc_type	timer_proc;
			event_proc_type	event_proc;
			event_filter_type filter_proc;
		}proc_;
	};
}//end namespace detail
}//end namespace nana

#endif

