/*
 *	Platform Specification Selector
 *	Copyright(C) 2003-2014 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Nana Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://nanapro.org/LICENSE_1_0.txt)
 *
 *	@file: nana/detail/platform_spec_selector.cpp
 *
 *  This file is used to support the Nana project of some cross-platform IDE,
 *	
 */

#include <nana/config.hpp>

#if defined(NANA_WINDOWS)
	#include "win32/platform_spec.cpp"
#elif defined(NANA_LINUX)
	#include "linux_X11/platform_spec.cpp"
#endif

