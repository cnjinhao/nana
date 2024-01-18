/*
 *	The Debugger Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2020 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/debugger.cpp
 *
 *	What follows is dependent on what defined in nana/config.hpp
 */

#include <nana/debugger.hpp>

namespace nana
{
	bool debugger::enabled_debug = false;

	bool debugger::is_enabled_print_debug() {
		return enabled_debug;
	}
	void debugger::enable_print_debug(bool newValue) {
		enabled_debug = newValue;
	}
}//end namespace nana
