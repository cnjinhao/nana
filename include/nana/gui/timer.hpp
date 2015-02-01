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

		void interval(unsigned milliseconds);   ///< Set the duration between calls (millisec ??)
		unsigned interval() const;
	private:
		nana::basic_event<arg_elapse> elapse_;
		implement * const impl_;
	};
}//end namespace nana
#endif
