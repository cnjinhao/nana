/*
 *	The Deploy Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
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
#include <cstdlib>
#include <stdexcept>
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

	int stoi(const std::string& str, std::size_t * pos, int base)
	{
#if defined(NANA_MINGW)
		auto sptr = str.c_str();
		char *end;
		errno = 0;
		auto result = std::strtol(sptr, &end, base);

		if (sptr == end)
			throw std::invalid_argument("invalid stoi argument");
		if (errno == ERANGE)
			throw std::out_of_range("stoi argument out of range");

		if (pos)
			*pos = (std::size_t)(end - sptr);
		return ((int)result);
#else
		return std::stoi(str, pos, base);
#endif
	}

	int stoi(const std::wstring& str, std::size_t* pos, int base)
	{
#if defined(NANA_MINGW)
		auto sptr = str.data();
		wchar_t *end;
		errno = 0;
		auto result = std::wcstol(sptr, &end, base);

		if (sptr == end)
			throw std::invalid_argument("invalid stoi argument");
		if (errno == ERANGE)
			throw std::out_of_range("stoi argument out of range");

		if (pos)
			*pos = (std::size_t)(end - sptr);
		return ((int)result);
#else
		return std::stoi(str, pos, base);
#endif
	}

	double stod(const std::string& str, std::size_t * pos)
	{
#ifdef NANA_MINGW
		auto *ptr = str.data();
		errno = 0;
		char *end;
		auto result = std::strtod(ptr, &end);

		if (ptr == end)
			throw std::invalid_argument("invalid stod argument");
		if (errno == ERANGE)
			throw std::out_of_range("stod argument out of range");
		if (pos)
			*pos = (std::size_t)(end - ptr);
		return result;
#else
		return std::stod(str, pos);
#endif
	}

	double stod(const std::wstring& str, std::size_t* pos)
	{
#ifdef NANA_MINGW
		auto *ptr = str.data();
		errno = 0;
		wchar_t *end;
		auto result = std::wcstod(ptr, &end);

		if (ptr == end)
			throw std::invalid_argument("invalid stod argument");
		if (errno == ERANGE)
			throw std::out_of_range("stod argument out of range");
		if (pos)
			*pos = (std::size_t)(end - ptr);
		return result;
#else
		return std::stod(str, pos);
#endif
	}

	std::wstring to_wstring(double v)
	{
#ifdef NANA_MINGW
		std::wstringstream ss;
		ss << v;
		return ss.str();
#else
		return std::to_wstring(v);
#endif	
	}

	std::wstring to_wstring(long double v)
	{
#ifdef NANA_MINGW
		std::wstringstream ss;
		ss << v;
		return ss.str();
#else
		return std::to_wstring(v);
#endif		
	}

	std::wstring to_wstring(unsigned v)
	{
#ifdef NANA_MINGW
		std::wstringstream ss;
		ss << v;
		return ss.str();
#else
		return std::to_wstring(v);
#endif	
	}

	std::wstring to_wstring(int v)
	{
#ifdef NANA_MINGW
		std::wstringstream ss;
		ss << v;
		return ss.str();
#else
		return std::to_wstring(v);
#endif	
	}

	std::wstring to_wstring(long v)
	{
#ifdef NANA_MINGW
		std::wstringstream ss;
		ss << v;
		return ss.str();
#else
		return std::to_wstring(v);
#endif		
	}

	std::wstring to_wstring(unsigned long v)
	{
#ifdef NANA_MINGW
		std::wstringstream ss;
		ss << v;
		return ss.str();
#else
		return std::to_wstring(v);
#endif	
	}

	std::wstring to_wstring(long long v)
	{
#ifdef NANA_MINGW
		std::wstringstream ss;
		ss << v;
		return ss.str();
#else
		return std::to_wstring(v);
#endif		
	}

	std::wstring to_wstring(unsigned long long v)
	{
#ifdef NANA_MINGW
		std::wstringstream ss;
		ss << v;
		return ss.str();
#else
		return std::to_wstring(v);
#endif	
	}

	std::wstring to_wstring(float v)
	{
#ifdef NANA_MINGW
		std::wstringstream ss;
		ss << v;
		return ss.str();
#else
		return std::to_wstring(v);
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
