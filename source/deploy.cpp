/*
 *	The Deploy Implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/depoly.cpp
 *
 *	What follow are dependented on what defined in nana/config.hpp
 */

#include <nana/deploy.hpp>

#if defined(NANA_WINDOWS)
	#include <windows.h>
#elif defined(NANA_LINUX)
	#include <string.h>
	#include PLATFORM_SPEC_HPP
#endif

namespace nana
{
	std::size_t strlen(const char_t* str)
	{
#if defined(NANA_UNICODE)
		return ::wcslen(str);
#else
		return ::strlen(str);
#endif
	}

	double strtod(const char_t* str, char_t** endptr)
	{
#if defined(NANA_UNICODE)
		return ::wcstod(str, endptr);
#else
		return ::strtod(str, endptr);
#endif
	}

	char_t* strcpy(char_t* dest, const char_t* source)
	{
#if defined(NANA_UNICODE)
		return ::wcscpy(dest, source);
#else
		return ::strcpy(dest, source);
#endif	
	}

	bool is_incomplete(const nana::string& str, unsigned pos)
	{
#ifndef NANA_UNICODE
		if(pos > str.size())
			pos = static_cast<unsigned>(str.size());
		const nana::char_t * pstr = str.c_str();
		if(pstr[pos] < 0)
		{
			bool incomp = false;
			for(unsigned i = 0; i < pos; ++i)
			{
				if(pstr[i] < 0)
					incomp = !incomp;
				else
					incomp = false;
			}
			return incomp;
		}
#endif
		return false;
	}
}
