/*
 *	A platform API implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/system/platform.hpp
 *	@description:
 *		this implements some API for platform-independent programming
 */

#ifndef NANA_SYSTEM_PLATFORM_HPP
#define NANA_SYSTEM_PLATFORM_HPP
#include <nana/deploy.hpp>

namespace nana
{
namespace system
{
	//sleep
	//@brief: suspend current thread for a specified milliseconds.
	//its precision is depended on hardware.
	void sleep(unsigned milliseconds);

	//this_thread_id
	//@brief: get the identifier of calling thread.
	thread_t this_thread_id();

	//timestamp
	//@brief: it retrieves the timestamp at the time the function is called.
	//		it is easy for get the number of milliseconds between calls.
	unsigned long timestamp();

	//get_async_mouse_state
	//@brief: it determines whether a mouse button was pressed at the time the function is called.
	bool get_async_mouse_state(int button);

	//open an url through a default browser
	void open_url(const std::string& url);

}//end namespace system
}//end namespace nana

#endif
