/*
 *	A Thread Pool Implementation
 *	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *
 *	@file: nana/threads/pool.cpp
 */

#include <nana/threads/pool.hpp>
#include <nana/system/platform.hpp>
#include <time.h>
#include <deque>
#include <vector>
#include <atomic>

#if defined(STD_THREAD_NOT_SUPPORTED)
    #include <nana/std_mutex.hpp>
    #include <nana/std_condition_variable.hpp>

#else
    #include <condition_variable>
    #include <mutex>
#endif

#if defined(NANA_WINDOWS)
	#include <windows.h>
	#include <process.h>
#elif defined(NANA_LINUX) || defined(NANA_MACOS)
	#include <pthread.h>
#endif

namespace nana
{
namespace threads
{
	//class pool
		//struct task
			pool::task::task(t k) : kind(k){}
			pool::task::~task(){}
		//end struct task

		//struct task_signal
		struct pool::task_signal
			: task
		{
			task_signal()
				: task(task::signal)
			{}

			void run()
			{}
		};//end struct task_signal

		class pool::impl
		{
			enum class state{init, idle, run, finished};

			struct pool_throbj
			{
#if defined(NANA_WINDOWS)
				typedef HANDLE thread_t;
#elif defined(NANA_POSIX)
				typedef pthread_t thread_t;
#endif
				impl * pool_ptr;
				task * task_ptr;
				thread_t	handle;
				std::atomic<state>	thr_state;
				time_t	timestamp;
#if defined(NANA_POSIX)
				std::mutex wait_mutex;
				std::condition_variable wait_cond;
				std::atomic<bool> suspended;
#endif
			};
		public:
			impl(std::size_t thr_number)
			{
				if(0 == thr_number) thr_number = 4;

				for(; thr_number; --thr_number)
				{
					pool_throbj * pto = new pool_throbj;
					pto->pool_ptr = this;
					pto->thr_state = state::init;
					pto->task_ptr = nullptr;
#if defined(NANA_WINDOWS)
					pto->handle = (HANDLE)::_beginthreadex(0, 0, reinterpret_cast<unsigned(__stdcall*)(void*)>(&impl::_m_thr_starter), pto, 0, 0);
#elif defined(NANA_POSIX)
					pto->suspended = false;
					::pthread_create(&(pto->handle), 0, reinterpret_cast<void*(*)(void*)>(&impl::_m_thr_starter), pto);
#endif
					container_.threads.push_back(pto);
				}
			}

			~impl()
			{
				runflag_ = false;

				while(true)
				{
					bool all_finished = true;
					{
						for(auto thr: container_.threads)
						{
							if(state::finished != thr->thr_state)
							{
								all_finished = false;
								break;
							}
						}
					}

					if(all_finished)
						break;

					while(true)
					{
						auto idle_thr = _m_pick_up_an_idle();
						if(idle_thr)
							_m_resume(idle_thr);
						else
							break;
					}
					nana::system::sleep(100);
				}

				std::vector<pool_throbj*> dup(std::move(container_.threads));

				for(auto thr: dup)
				{
#if defined(NANA_WINDOWS)
					::WaitForSingleObject(thr->handle, INFINITE);
					::CloseHandle(thr->handle);
#elif defined(NANA_POSIX)
					::pthread_join(thr->handle, 0);
					::pthread_detach(thr->handle);
#endif
				}

				std::lock_guard<decltype(mutex_)> lock(mutex_);
				for(auto task_ptr : container_.tasks)
				{
					delete task_ptr;
				}
			}

			void push(task * taskptr)
			{
				if(false == runflag_)
				{
					delete taskptr;
					throw std::runtime_error("Nana.Pool: Do not accept task now");
				}

				pool_throbj * pto = _m_pick_up_an_idle();

				if(pto)
				{
					pto->task_ptr = taskptr;
					_m_resume(pto);
				}
				else
				{
					std::lock_guard<decltype(mutex_)> lock(mutex_);
					container_.tasks.push_back(taskptr);
				}
			}

			void wait_for_signal()
			{
				std::unique_lock<std::mutex> lock(signal_.mutex);
				signal_.cond.wait(lock);
			}

			void wait_for_finished()
			{
				while(true)
				{
					{
						std::lock_guard<decltype(mutex_)> lock(mutex_);
						if(container_.tasks.empty())
						{
							bool finished = true;
							for(auto thr : container_.threads)
							{
								if(state::run == thr->thr_state)
								{
									finished = false;
									break;
								}
							}
							if(finished)
								return;
						}
					}
					nana::system::sleep(100);
				}
			}
		private:
			pool_throbj* _m_pick_up_an_idle()
			{
				for(auto thr : container_.threads)
					if(state::idle == thr->thr_state)
					{
						std::lock_guard<decltype(mutex_)> lock(mutex_);
						if(state::idle == thr->thr_state)
						{
							thr->thr_state = state::run;
							return thr;
						}
					}
				return nullptr;
			}

			void _m_suspend(pool_throbj* pto)
			{
				pto->thr_state = state::idle;
#if defined(NANA_WINDOWS)
				::SuspendThread(pto->handle);
#elif defined(NANA_POSIX)
				std::unique_lock<std::mutex> lock(pto->wait_mutex);
				pto->suspended = true;
				pto->wait_cond.wait(lock);
				pto->suspended = false;
#endif
			}

			void _m_resume(pool_throbj* pto)
			{
#if defined(NANA_WINDOWS)
				while(true)
				{
					DWORD n = ::ResumeThread(pto->handle);
					if(n == 1 || n == static_cast<DWORD>(-1))
						break;
				}
#elif defined(NANA_POSIX)
				while(false == pto->suspended)
					;
				std::unique_lock<std::mutex> lock(pto->wait_mutex);
				pto->wait_cond.notify_one();
#endif
			}

			bool _m_read(pool_throbj* pto)
			{
				pto->task_ptr = nullptr;
				if(runflag_)
				{
					std::lock_guard<decltype(mutex_)> lock(mutex_);
					if(container_.tasks.size())
					{
						pto->task_ptr = container_.tasks.front();
						container_.tasks.pop_front();
					}
				}
				else
					return false;

				if(nullptr == pto->task_ptr)
				{
					//The task queue is empty, so that
					//suspend and wait for a task.
					_m_suspend(pto);
				}

				return (nullptr != pto->task_ptr);
			}

			void _m_thr_runner(pool_throbj* pto)
			{
				while(_m_read(pto))
				{
					pto->timestamp = time(nullptr);
					switch(pto->task_ptr->kind)
					{
					case task::general:
						try
						{
							pto->task_ptr->run();
						}catch(...){}
						break;
					case task::signal:
						while(true)
						{
							bool finished = true;
							{
								std::lock_guard<decltype(mutex_)> lock(mutex_);
								for(auto thr : container_.threads)
								{
									if((thr != pto) && (state::run == thr->thr_state) && (thr->timestamp <= pto->timestamp))
									{
										finished = false;
										break;
									}
								}
							}

							if(finished)
								break;
							nana::system::sleep(100);
						}

						//wait till the cond is waiting.
						signal_.cond.notify_one();
						break;
					}
					delete pto->task_ptr;
					pto->task_ptr = nullptr;
				}

				pto->thr_state = state::finished;
			}

			//Here defines the a function used for creating a thread.
			//This is platform-specified.
#if defined(NANA_WINDOWS)
			static unsigned __stdcall _m_thr_starter(pool_throbj * pto)
			{
				pto->pool_ptr->_m_thr_runner(pto);
				::_endthreadex(0);
				return 0;
			}
#elif defined(NANA_POSIX)
			static void * _m_thr_starter(pool_throbj * pto)
			{
				pto->pool_ptr->_m_thr_runner(pto);
				return nullptr;
			}
#endif
		private:
			std::atomic<bool> runflag_{ true };
			std::recursive_mutex mutex_;

			struct signal
			{
				std::mutex mutex;
				std::condition_variable cond;
			}signal_;

			struct container
			{
				std::deque<task*> tasks;
				std::vector<pool_throbj*> threads;
			}container_;
		};//end class impl

		pool::pool()
			: impl_(new impl(4))
		{
		}

		pool::pool(std::size_t thread_number)
			: impl_(new impl(thread_number))
		{
		}

		pool::~pool()
		{
			delete impl_;
		}

		void pool::signal()
		{
			task * task_ptr = nullptr;
			try
			{
				task_ptr = new task_signal;
				_m_push(task_ptr);
			}
			catch(std::bad_alloc&)
			{
				delete task_ptr;
			}
		}

		void pool::wait_for_signal()
		{
			impl_->wait_for_signal();
		}

		void pool::wait_for_finished()
		{
			impl_->wait_for_finished();
		}

		void pool::_m_push(task* task_ptr)
		{
			impl_->push(task_ptr);
		}
	//end class pool

}//end namespace threads
}//end namespace nana
