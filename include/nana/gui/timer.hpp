/*
 *	A Timer Implementation
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/timer.hpp
 *	@description:
 *		A timer can repeatedly call a piece of code. The duration between 
 *	calls is specified in milliseconds. Timer is defferent from other graphics
 *	controls, it has no graphics interface.
 */

#ifndef NANA_GUI_TIMER_HPP
#define NANA_GUI_TIMER_HPP
#include <nana/gui/detail/general_events.hpp>
#include <nana/push_ignore_diagnostic>

namespace nana
{  
       /// Can repeatedly call a piece of code.

	struct arg_elapse
		: public event_arg
	{
		long long id;	//timer identifier;
	};

	class timer
	{
		struct implement;

		timer(const timer&) = delete;
		timer& operator=(const timer&) = delete;
		timer(timer&&) = delete;
		timer& operator=(timer&&) = delete;
	public:
		timer();
		[[deprecated("prefer a std::chrono::duration (like std::chrono::milliseconds) over "
			     "raw integers for durations")]]
		timer(unsigned int ms) : timer{} { interval_(ms); } /// Accepts an initial interval in ms
		template <typename Rep, typename Period> /// Accepts an initial interval in any chrono unit
		explicit timer(std::chrono::duration<Rep, Period> const & time) : timer{} { interval(time); }

		~timer();

		template<typename Function>
		void elapse(Function && fn)
		{
			elapse_.connect(std::forward<Function>(fn));
		}

		void reset();
		void start();
		bool started() const;
		void stop();

		[[deprecated("prefer a std::chrono::duration (like std::chrono::milliseconds) over "
			     "raw integers for durations")]]
		inline void interval(unsigned milliseconds) { interval_(milliseconds); }
		template <typename Rep, typename Period>
		inline void interval(std::chrono::duration<Rep, Period> const & time_interval) ///< Set the duration between calls, accepts std::chrono
		{
		    interval_(std::chrono::duration_cast<std::chrono::milliseconds>(time_interval).count());
		}
		unsigned interval() const;
		template <typename Duration = std::chrono::milliseconds>
		inline Duration interval() const
		{
		    return std::chrono::duration_cast<Duration>(std::chrono::milliseconds(interval));
		}
	private:
		void interval_(unsigned milliseconds);   ///< Set the duration between calls (millisec ??)
	private:
		nana::basic_event<arg_elapse> elapse_;
		implement * const impl_;
	};
}//end namespace nana
#include <nana/pop_ignore_diagnostic>
#endif
