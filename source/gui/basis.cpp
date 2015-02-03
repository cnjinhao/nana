/*
 *	Basis Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/basis.cpp
 *
 *	This file provides basis class and data structrue that required by gui
 */

#include <nana/gui/basis.hpp>

namespace nana
{
	//struct appearance
	//@brief: Window appearance structure
		appearance::appearance()
			:taskbar(true), floating(false), no_activate(false),
			 minimize(true), maximize(true), sizable(true),
			 decoration(true)
		{}

		appearance::appearance(bool has_decorate, bool taskbar, bool is_float, bool no_activate, bool min, bool max, bool sizable)
			:	taskbar(taskbar), floating(is_float), no_activate(no_activate),
				minimize(min), maximize(max), sizable(sizable),
				decoration(has_decorate)
		{}
	//end struct appearance
}//end namespace nana

