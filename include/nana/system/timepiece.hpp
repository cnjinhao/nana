/*
 *	Timepiece Implementation
 *	Copyright(C) 2003-2018 Jinhao(cnjinhao@hotmail.com)
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

#include "../c++defines.hpp"

namespace nana
{
namespace system
{           ///  used for measuring and signaling the end of time intervals.
	class timepiece
	{
	public:
		timepiece();
		timepiece(const timepiece&);
		~timepiece();
		timepiece & operator=(const timepiece &);
		void start() noexcept;              ///<  	Set the begin time.
		double calc() const noexcept;       ///<    Get the intervals from the begin time.
	private:
		struct impl_t;
		impl_t * impl_;
	};

}//end namespace system
}//end namespace nana

#endif

