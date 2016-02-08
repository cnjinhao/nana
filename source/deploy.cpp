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
#include <cstring> //std::strlen
#include <stdexcept>

#if defined(NANA_WINDOWS)
	#include <windows.h>
#elif defined(NANA_LINUX) || defined(NANA_MACOS)
	#include <string.h>
	#include <nana/detail/platform_spec_selector.hpp>
#endif

//Implement workarounds for GCC/MinGW which version is below 4.8.2
#if defined(STD_NUMERIC_CONVERSIONS_NOT_SUPPORTED)
#include <sstream>
namespace std
{
	int stoi(const std::string& str, std::size_t * pos, int base)
	{
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
	}

	int stoi(const std::wstring& str, std::size_t* pos, int base)
	{
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
	}
	using ::strtof;
	using ::strtold;
	using ::wcstold;
	using ::strtoll;
	using ::wcstoll;
	using ::strtoull;
	using ::wcstoull;

	float stof(const std::string& str, std::size_t * pos)
	{
		auto *ptr = str.data();
		errno = 0;
		char *end;
		auto result = std::strtof(ptr, &end);

		if (ptr == end)
			throw std::invalid_argument("invalid stod argument");
		if (errno == ERANGE)
			throw std::out_of_range("stof argument out of range");
		if (pos)
			*pos = (std::size_t)(end - ptr);
		return result;
	}

	float stof(const std::wstring& str, std::size_t* pos)
	{
		auto *ptr = str.data();
		errno = 0;
		wchar_t *end;
		auto result = std::wcstof(ptr, &end);

		if (ptr == end)
			throw std::invalid_argument("invalid stod argument");
		if (errno == ERANGE)
			throw std::out_of_range("stof argument out of range");
		if (pos)
			*pos = (std::size_t)(end - ptr);
		return result;
	}

	double stod(const std::string& str, std::size_t * pos)
	{
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
	}

	double stod(const std::wstring& str, std::size_t* pos)
	{
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
	}

	long double stold(const std::string& str, std::size_t * pos)
	{
		auto *ptr = str.data();
		errno = 0;
		char *end;
		auto result = std::strtold(ptr, &end);

		if (ptr == end)
			throw std::invalid_argument("invalid stod argument");
		if (errno == ERANGE)
			throw std::out_of_range("stold argument out of range");
		if (pos)
			*pos = (std::size_t)(end - ptr);
		return result;
	}

	long double stold(const std::wstring& str, std::size_t* pos)
	{
		auto *ptr = str.data();
		errno = 0;
		wchar_t *end;
		auto result = std::wcstold(ptr, &end);

		if (ptr == end)
			throw std::invalid_argument("invalid stod argument");
		if (errno == ERANGE)
			throw std::out_of_range("stold argument out of range");
		if (pos)
			*pos = (std::size_t)(end - ptr);
		return result;
	}

	long stol(const std::string& str, std::size_t* pos, int base)
	{
		auto *ptr = str.data();
		errno = 0;
		char *end;
		auto result = std::strtol(ptr, &end, base);

		if (ptr == end)
			throw std::invalid_argument("invalid stod argument");
		if (errno == ERANGE)
			throw std::out_of_range("stol argument out of range");
		if (pos)
			*pos = (std::size_t)(end - ptr);
		return result;
	}

	long stol(const std::wstring& str, std::size_t* pos, int base)
	{
		auto *ptr = str.data();
		errno = 0;
		wchar_t *end;
		auto result = std::wcstol(ptr, &end, base);

		if (ptr == end)
			throw std::invalid_argument("invalid stod argument");
		if (errno == ERANGE)
			throw std::out_of_range("stol argument out of range");
		if (pos)
			*pos = (std::size_t)(end - ptr);
		return result;
	}

	//Workaround for no implemenation of std::stoll in MinGW.
	long long stoll(const std::string& str, std::size_t* pos, int base)
	{
		auto *ptr = str.data();
		errno = 0;
		char* end;
		auto result = std::strtoll(ptr, &end, base);

		if (ptr == end)
			throw std::invalid_argument("invalid stod argument");
		if (errno == ERANGE)
			throw std::out_of_range("stoll argument out of range");
		if (pos)
			*pos = (std::size_t)(end - ptr);
		return result;
	}

	long long stoll(const std::wstring& str, std::size_t* pos, int base)
	{
		auto *ptr = str.data();
		errno = 0;
		wchar_t* end;
		auto result = std::wcstoll(ptr, &end, base);

		if (ptr == end)
			throw std::invalid_argument("invalid stod argument");
		if (errno == ERANGE)
			throw std::out_of_range("stoll argument out of range");
		if (pos)
			*pos = (std::size_t)(end - ptr);
		return result;
	}

	unsigned long long stoull(const std::string& str, std::size_t* pos, int base)
	{
		auto *ptr = str.data();
		errno = 0;
		char* end;
		auto result = std::strtoull(ptr, &end, base);

		if (ptr == end)
			throw std::invalid_argument("invalid stod argument");
		if (errno == ERANGE)
			throw std::out_of_range("stoull argument out of range");
		if (pos)
			*pos = (std::size_t)(end - ptr);
		return result;
	}

	unsigned long long stoull(const std::wstring& str, std::size_t* pos, int base)
	{
		auto *ptr = str.data();
		errno = 0;
		wchar_t* end;
		auto result = std::wcstoull(ptr, &end, base);

		if (ptr == end)
			throw std::invalid_argument("invalid stod argument");
		if (errno == ERANGE)
			throw std::out_of_range("stoull argument out of range");
		if (pos)
			*pos = (std::size_t)(end - ptr);
		return result;
	}

	//Workaround for no implemenation of std::stoul in MinGW.
	unsigned long stoul(const std::string& str, std::size_t* pos, int base)
	{
		auto *ptr = str.data();
		errno = 0;
		char* end;
		auto result = std::strtoul(ptr, &end, base);

		if (ptr == end)
			throw std::invalid_argument("invalid stod argument");
		if (errno == ERANGE)
			throw std::out_of_range("stoul argument out of range");
		if (pos)
			*pos = (std::size_t)(end - ptr);
		return result;
	}

	unsigned long stoul(const std::wstring& str, std::size_t* pos, int base)
	{
		auto *ptr = str.data();
		errno = 0;
		wchar_t* end;
		auto result = std::wcstoul(ptr, &end, base);

		if (ptr == end)
			throw std::invalid_argument("invalid stod argument");
		if (errno == ERANGE)
			throw std::out_of_range("stoul argument out of range");
		if (pos)
			*pos = (std::size_t)(end - ptr);
		return result;
	}
}//end namespace std
#endif //STD_NUMERIC_CONVERSIONS_NOT_SUPPORTED

#ifdef STD_TO_STRING_NOT_SUPPORTED
#include <sstream>
namespace std
{
	std::string to_string(double v)
	{
		std::stringstream ss;
		ss << v;
		return ss.str();
	}

	std::string to_string(long double v)
	{
		std::stringstream ss;
		ss << v;
		return ss.str();
	}

	std::string to_string(unsigned v)
	{
		std::stringstream ss;
		ss << v;
		return ss.str();
	}

	std::string to_string(int v)
	{
		std::stringstream ss;
		ss << v;
		return ss.str();
	}

	std::string to_string(long v)
	{
		std::stringstream ss;
		ss << v;
		return ss.str();
	}

	std::string to_string(unsigned long v)
	{
		std::stringstream ss;
		ss << v;
		return ss.str();
	}

	std::string to_string(long long v)
	{
		std::stringstream ss;
		ss << v;
		return ss.str();
	}

	std::string to_string(unsigned long long v)
	{
		std::stringstream ss;
		ss << v;
		return ss.str();
	}

	std::string to_string(float v)
	{
		std::stringstream ss;
		ss << v;
		return ss.str();
	}
}
#endif // STD_TO_STRING_NOT_SUPPORTED

#ifdef STD_TO_WSTRING_NOT_SUPPORTED
#include <sstream>
namespace std
{
	std::wstring to_wstring(double v)
	{
		std::wstringstream ss;
		ss << v;
		return ss.str();
	}

	std::wstring to_wstring(long double v)
	{
		std::wstringstream ss;
		ss << v;
		return ss.str();
	}

	std::wstring to_wstring(unsigned v)
	{
		std::wstringstream ss;
		ss << v;
		return ss.str();
	}

	std::wstring to_wstring(int v)
	{
		std::wstringstream ss;
		ss << v;
		return ss.str();
	}

	std::wstring to_wstring(long v)
	{
		std::wstringstream ss;
		ss << v;
		return ss.str();
	}

	std::wstring to_wstring(unsigned long v)
	{
		std::wstringstream ss;
		ss << v;
		return ss.str();
	}

	std::wstring to_wstring(long long v)
	{
		std::wstringstream ss;
		ss << v;
		return ss.str();
	}

	std::wstring to_wstring(unsigned long long v)
	{
		std::wstringstream ss;
		ss << v;
		return ss.str();
	}

	std::wstring to_wstring(float v)
	{
		std::wstringstream ss;
		ss << v;
		return ss.str();
	}
}
#endif

namespace nana
{
	bool is_utf8(const char* str, unsigned len)
	{
		auto ustr = reinterpret_cast<const unsigned char*>(str);
		auto end = ustr + len;

		while (ustr < end)
		{
			const auto uv = *ustr;
			if (uv < 0x80)
			{
				++ustr;
				continue;
			}

			if (uv < 0xC0)
				return false;

			if ((uv < 0xE0) && (ustr + 1 < end))
				ustr += 2;
			else if (uv < 0xF0 && (ustr + 2 <= end))
				ustr += 3;
			else if (uv < 0x1F && (ustr + 3 <= end))
				ustr += 4;
			else
				return false;
		}

		return true;
	}

	void throw_not_utf8(const std::string& text)
	{
		if (!is_utf8(text.c_str(), text.length()))
			throw std::invalid_argument("The text is not encoded in UTF8");
	}

	void throw_not_utf8(const char* text, unsigned len)
	{
		if (!is_utf8(text, len))
			throw std::invalid_argument("The text is not encoded in UTF8");
	}

	void throw_not_utf8(const char* text)
	{
		if (!is_utf8(text, std::strlen(text)))
			throw std::invalid_argument("The text is not encoded in UTF8");
		
	}

	const std::string& to_utf8(const std::string& str)
	{
		return str;
	}

	std::string to_utf8(const std::wstring& text)
	{
		return ::nana::charset(text).to_bytes(::nana::unicode::utf8);
	}

	std::wstring to_wstring(const std::string& utf8_str)
	{
		return ::nana::charset(utf8_str, ::nana::unicode::utf8);
	}

	const std::wstring& to_wstring(const std::wstring& wstr)
	{
		return wstr;
	}

	std::wstring&& to_wstring(std::wstring&& wstr)
	{
		return static_cast<std::wstring&&>(wstr);
	}

#if defined(NANA_WINDOWS)
	std::string to_osmbstr(const std::string& text_utf8)
	{
		return ::nana::charset(text_utf8, ::nana::unicode::utf8);
	}
#else
	std::string to_osmbstr(std::string text_utf8)
	{
		return text_utf8;
	}
#endif

#if defined(NANA_WINDOWS)
	const detail::native_string_type to_nstring(const std::string& text)
	{
		return ::nana::charset(text, ::nana::unicode::utf8);
	}

	const detail::native_string_type& to_nstring(const std::wstring& text)
	{
		return text;
	}

	detail::native_string_type to_nstring(std::string&& text)
	{
		return ::nana::charset(text, ::nana::unicode::utf8);
	}

	detail::native_string_type&& to_nstring(std::wstring&& text)
	{
		return std::move(text);
	}

	detail::native_string_type to_nstring(int n)
	{
		return std::to_wstring(n);
	}

	detail::native_string_type to_nstring(double d)
	{
		return std::to_wstring(d);
	}

	detail::native_string_type to_nstring(std::size_t d)
	{
		return std::to_wstring(d);
	}
#else	//POSIX
	const detail::native_string_type& to_nstring(const std::string& text)
	{
		return text;
	}

	const detail::native_string_type to_nstring(const std::wstring& text)
	{
		return ::nana::charset(text).to_bytes(::nana::unicode::utf8);
	}

	detail::native_string_type&& to_nstring(std::string&& text)
	{
		return std::move(text);
	}

	detail::native_string_type to_nstring(std::wstring&& text)
	{
		return ::nana::charset(text).to_bytes(::nana::unicode::utf8);
	}

	detail::native_string_type to_nstring(int n)
	{
		return std::to_string(n);
	}

	detail::native_string_type to_nstring(double d)
	{
		return std::to_string(d);
	}

	detail::native_string_type to_nstring(std::size_t d)
	{
		return std::to_string(d);
	}
#endif


}

