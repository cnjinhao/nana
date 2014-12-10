/*
*	Forward Declaration of Internal Scope Guard
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2014 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file: nana/gui/detail/internal_scope_guard.hpp
*/
#ifndef NANA_GUI_DETAIL_INTERNAL_SCOPE_GUARD_HPP
#define NANA_GUI_DETAIL_INTERNAL_SCOPE_GUARD_HPP

namespace nana
{
	//Implemented in bedrock
	class internal_scope_guard
	{
		internal_scope_guard(const internal_scope_guard&) = delete;
		internal_scope_guard(internal_scope_guard&&) = delete;

		internal_scope_guard& operator=(const internal_scope_guard&) = delete;
		internal_scope_guard& operator=(internal_scope_guard&&) = delete;
	public:
		internal_scope_guard();
		~internal_scope_guard();
	};
}

#endif