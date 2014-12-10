/*
 *	Timepiece Implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file:			nana/system/timepiece.hpp
 *	@description:	a time counter
 */

#ifndef NANA_SYSTEM_TIMEPIECE_HPP
#define NANA_SYSTEM_TIMEPIECE_HPP

namespace nana
{
namespace system
{           ///  used for measuring and signaling the end of time intervals.
	class timepiece
	{
	public:
		timepiece();
		timepiece(const volatile timepiece&);
		~timepiece();
		timepiece & operator=(const volatile timepiece &);
		void start() volatile;              ///<  	Set the begin time.
		double calc() const volatile;       ///<    Get the intervals from the begin time.
	private:
		struct impl_t;
		impl_t * impl_;
	};

}//end namespace system
}//end namespace nana

#endif

