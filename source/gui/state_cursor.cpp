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
#include <nana/gui/state_cursor.hpp>
#include <nana/gui/detail/bedrock.hpp>
#include <nana/gui/detail/basic_window.hpp>
#include <nana/gui/detail/window_manager.hpp>

namespace nana
{
	state_cursor::state_cursor(window handle, cursor cur)
		: handle_(handle)
	{
		auto & brock = detail::bedrock::instance();
		auto wd = reinterpret_cast<detail::basic_window*>(handle);
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
				auto & brock = detail::bedrock::instance();
				auto wd = reinterpret_cast<detail::basic_window*>(handle_);
				if (brock.wd_manager().available(wd))
					brock.undefine_state_cursor(wd, nullptr);
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
			auto & brock = detail::bedrock::instance();
			auto wd = reinterpret_cast<detail::basic_window*>(handle_);
			if (brock.wd_manager().available(wd))
				brock.undefine_state_cursor(wd, nullptr);
		}
	}
}