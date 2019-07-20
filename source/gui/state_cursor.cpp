/*
 *	State Cursor
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2014 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/state_cursor.cpp
 */

#include "detail/basic_window.hpp"
#include <nana/gui/state_cursor.hpp>
#include <nana/gui/detail/bedrock.hpp>
#include <nana/gui/detail/window_manager.hpp>

namespace nana
{
	state_cursor::state_cursor(window wd, cursor cur)
		: handle_(wd)
	{
		auto & brock = detail::bedrock::instance();
		if (brock.wd_manager().available(wd))
			brock.define_state_cursor(wd, cur, nullptr);
		else
			handle_ = nullptr;
	}

	state_cursor::state_cursor(state_cursor&& rhs)
		: handle_(rhs.handle_)
	{
		rhs.handle_ = nullptr;
	}

	state_cursor& state_cursor::operator = (state_cursor&& rhs)
	{
		if (this != &rhs)
		{
			if (handle_)
			{
				nana::internal_scope_guard lock;
				auto & brock = detail::bedrock::instance();
				brock.undefine_state_cursor(handle_, nullptr);
			}
			handle_ = rhs.handle_;
			rhs.handle_ = nullptr;
		}
		return *this;
	}

	state_cursor::~state_cursor()
	{
		if (handle_)
		{
			nana::internal_scope_guard lock;
			auto & brock = detail::bedrock::instance();
			brock.undefine_state_cursor(handle_, nullptr);
		}
	}
}