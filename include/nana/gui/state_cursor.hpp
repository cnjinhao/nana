/*
 *	State Cursor
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2014 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/state_cursor.hpp
 */

#include <nana/gui/basis.hpp>
namespace nana
{
	class state_cursor
	{
		state_cursor(const state_cursor&) = delete;
		state_cursor& operator=(const state_cursor&) = delete;
	public:
		state_cursor(window, cursor);
		state_cursor(state_cursor&&);
		state_cursor& operator=(state_cursor&&);
		~state_cursor();
	private:
		window handle_;
	};
}