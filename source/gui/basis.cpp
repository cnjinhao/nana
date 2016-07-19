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

using namespace nana;
using namespace nana::parameters;

//struct appearance
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

#if defined(NANA_WINDOWS)
#	include <windows.h>
#endif

mouse_wheel::mouse_wheel()
	: lines(3), characters(3)
{
#if defined(NANA_WINDOWS)
    // https://msdn.microsoft.com/en-us/library/ms997498.aspx
    //#define SPI_SETWHEELSCROLLCHARS   0x006D
#	ifndef SPI_GETWHEELSCROLLCHARS
#		define SPI_GETWHEELSCROLLCHARS   0x006C
#	endif
	::SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &lines, 0);
	::SystemParametersInfo(SPI_GETWHEELSCROLLCHARS, 0, &characters, 0);
#endif
}

