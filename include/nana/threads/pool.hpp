/*
 *	A Thread Pool Implementation
 *	Copyright(C) 2003-2018 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *
 *	@file: nana/threads/pool.hpp
 */

#ifndef NANA_THREADS_POOL_HPP
#define NANA_THREADS_POOL_HPP

#include <nana/c++defines.hpp>
#include <nana/traits.hpp>
#include <functional>
#include <cstddef>

#ifndef STD_THREAD_NOT_SUPPORTED
#	include <thread>
#endif

namespace nana{
   /// Some mutex classes for synchronizing.
namespace threads
{    /// A thread pool manages a group threads for a large number of tasks processing.
	class pool
	{
		struct task
		{
			enum t{general, signal};

			const t kind;

			task(t);
			virtual ~task() = 0;
			virtual void run() = 0;
		};

		template<typename Function>
		struct task_wrapper
			: task
		{
			typedef Function function_type;
			function_type taskobj;

			task_wrapper(const function_type& f)
				: task(task::general), taskobj(f)
			{}

			void run()
			{
				taskobj();
			}
		};

		struct task_signal;
		class impl;

		pool(const pool&) = delete;
		pool& operator=(const pool&) = delete;
	public:
#ifndef STD_THREAD_NOT_SUPPORTED
		pool(unsigned thread_number = std::thread::hardware_concurrency());    ///< Creates a group of threads.
#else
		pool(unsigned thread_number = 0);
#endif
		pool(pool&&);
		~pool();    ///< waits for the all running tasks till they are finished and skips all the queued tasks.

		pool& operator=(pool&&);

		template<typename Function>
		void push(const Function& f)
		{
			task * taskptr = nullptr;

			try
			{
				taskptr = new task_wrapper<typename std::conditional<std::is_function<Function>::value, Function*, Function>::type>(f);
				_m_push(taskptr);
			}
			catch(std::bad_alloc&)
			{
				delete taskptr;
			}
		}

		void signal(); ///< Make a signal that will be triggered when the tasks which are pushed before it are finished.
		void wait_for_signal();     ///< Waits for a signal until the signal processed.
		void wait_for_finished();
	private:
		void _m_push(task* task_ptr);
	private:
		impl * impl_;
	};//end class pool

            /// Manages a group threads for a large number of tasks processing.
	template<typename Function>
	class pool_pusher
	{
	public:
           /// same as Function if Function is not a function prototype, otherwise value_type is a pointer type of function
		typedef typename std::conditional<std::is_function<Function>::value, Function*, Function>::type value_type;

		pool_pusher(pool& pobj, value_type fn)
			:pobj_(pobj), value_(fn)
		{}

		void operator()() const
		{
			pobj_.push(value_);
		}
	private:
		pool & pobj_;
		value_type value_;
	};

	template<typename Function>
	pool_pusher<Function> pool_push(pool& pobj, const Function& fn)
	{
		return pool_pusher<Function>(pobj, fn);
	}

	template<typename Class, typename Concept>
	pool_pusher<std::function<void()> > pool_push(pool& pobj, Class& obj, void(Concept::*mf)())
	{
		return pool_pusher<std::function<void()> >(pobj, std::bind(mf, &obj));
	}

}//end namespace threads
}//end namespace nana
#endif

