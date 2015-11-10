/*
 *	Selector of Platform Specification
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/detail/platform_spec_selector.hpp
 *
 *	Selects the proper platform_spec header file for the current platform
 */

#include <nana/config.hpp>

#if defined(NANA_WINDOWS)
#include <nana/detail/win32/platform_spec.hpp>
#elif defined(NANA_LINUX) || defined(NANA_MACOS)
#include <nana/detail/linux_X11/platform_spec.hpp>
#endif