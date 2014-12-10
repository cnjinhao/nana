/*
 *	The Deploy Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2014 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/deploy.hpp
 *
 *	What follow are dependented on what defined in nana/config.hpp
 */

#ifndef NANA_DEPLOY_HPP
#define NANA_DEPLOY_HPP

#include <nana/config.hpp>
#include <nana/charset.hpp>
#if defined(NANA_LINUX)
#undef NANA_WINDOWS
#endif

#ifndef NANA_UNICODE
	namespace nana
	{
		typedef char		char_t;
		typedef std::string string; ///< An alias of std::wstring or std::string, depending on the macro NANA_UNICODE
	}
	#define STR(string) string
#else
	namespace nana
	{
		typedef wchar_t			char_t;
		typedef std::wstring	string; ///< An alias of std::wstring or std::string, depending on the macro NANA_UNICODE
	}
	#define STR(string) L##string
#endif

namespace nana
{
	std::size_t strlen(const char_t* str);
	double strtod(const char_t* str, char_t** endptr);
	char_t* strcpy(char_t* dest, const char_t* source);
}

#if defined(NANA_WINDOWS)
	#define NANA_SHARED_EXPORT	extern "C" _declspec(dllexport)
#elif defined(NANA_LINUX)
	#define NANA_SHARED_EXPORT	extern "C"
#endif

namespace nana
{
	bool is_incomplete(const nana::string& str, unsigned pos);

	inline unsigned make_rgb(unsigned char red, unsigned char green, unsigned char blue)
	{

		return ((unsigned(red) << 16)|((unsigned(green)<<8))|blue);
	}
}

#define NANA_RGB(a)	(((DWORD)(a) & 0xFF)<<16) |  ((DWORD)(a) & 0xFF00) | (((DWORD)(a) & 0xFF0000) >> 16 )


#endif //NANA_MACROS_HPP
