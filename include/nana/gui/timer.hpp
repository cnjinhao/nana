/*
 *	A Timer Implementation
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/timer.hpp
 *	@description:
 *		A timer can repeatedly call a piece of code. The duration between 
 *	calls is specified in milliseconds. Timer is different from other graphics
 *	controls, it has no graphics interface.
 *
 *	@contributors: rbrugo(#417)
 */

#ifndef NANA_GUI_TIMER_HPP
#define NANA_GUI_TIMER_HPP
#include <nana/gui/detail/general_events.hpp>
#include <nana/push_ignore_diagnostic>

namespace nana
{  

	class timer;

	struct arg_elapse
		: public event_arg
	{
		timer*	sender; //indicates which timer emitted this notification
	};

	/// Can repeatedly call a piece of code.
	class timer
	{
		struct implement;

		timer(const timer&) = delete;
		timer& operator=(const timer&) = delete;
		timer(timer&&) = delete;
		timer& operator=(timer&&) = delete;
	public:
		timer();
		explicit timer(std::chrono::milliseconds ms);
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

		void interval(std::chrono::milliseconds ms);

		template <typename Duration = std::chrono::milliseconds>
		inline Duration interval() const
		{
			return std::chrono::duration_cast<Duration>(std::chrono::milliseconds{ _m_interval() });
		}
	private:
		unsigned _m_interval() const;
	private:
		nana::basic_event<arg_elapse> elapse_;
		implement * const impl_;
	};
}//end namespace nana
#include <nana/pop_ignore_diagnostic>
#endif
