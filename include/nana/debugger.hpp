/*
 *	The Deploy Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2020 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/debugger.cpp
 *
 *	
 */

namespace nana
{
	namespace debugger
	{
		static bool enabled_debug = true;
		bool is_enabled_print_debug() {
			return enabled_debug;
		}
		void enable_print_debug(bool newValue) {
			enabled_debug = newValue;
		}
	}
}