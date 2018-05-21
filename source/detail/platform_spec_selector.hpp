/**
 *	Selector of Platform Specification
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2018 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file nana/detail/platform_spec_selector.hpp
 *
 *	@brief Selects the proper platform_spec header file for the current platform
 */

#include <nana/config.hpp>

#if defined(NANA_WINDOWS)
#include "mswin/platform_spec.hpp"
#elif defined(NANA_POSIX)
#include "posix/platform_spec.hpp"
#endif
