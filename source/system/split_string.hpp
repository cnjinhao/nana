/*
 *	The Deploy Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2018 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/system/split_string.hpp
 *
 *	What follows is dependent on what defined in nana/config.hpp
 */

#ifndef NANA_SYSTEM_SPLITSTRING_HPP
#define NANA_SYSTEM_SPLITSTRING_HPP

#include <nana/config.hpp>
#include <vector>
#ifdef _nana_std_has_string_view
#	include <string_view>
#else
#	include <string>
#endif

namespace nana
{
namespace system
{
#ifdef _nana_std_has_string_view
typedef std::string_view split_string_type;
#else
typedef std::string split_string_type;
#endif

std::vector<split_string_type> split_string (const split_string_type& text, char sep);

}
}

#endif
