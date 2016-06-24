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

//#ifdef STD_put_time_NOT_SUPPORTED
#include  <ctime>
#include  <cwchar>
namespace std
{
	//Workaround for no implemenation of std::put_time in gcc < 5.
	/* std unspecified return type */
	//template< class CharT, class RTSTR >// let fail for CharT != char / wchar_t
	//RTSTR put_time(const std::tm* tmb, const CharT* fmt);

	//template<   >
	std::string put_time/*<char, std::string>*/(const std::tm* tmb, const char* fmt)
	{  
		std::size_t sz = 200;
		std::string str(sz, '\0'); 
		sz = std::strftime(&str[0], str.size() - 1, fmt, tmb);  
		str.resize(sz);
		return str;
	}
	//Defined in header <ctime>
	//	std::size_t strftime(char* str, std::size_t count, const char* format, const std::tm* time);
	//template<>
	//std::wstring put_time<wchar_t, std::wstring>(const std::tm* tmb, const wchar_t* fmt)
	//{
	//	unsigned sz = 200;
	//	std::wstring str(sz, L'\0');
	//	sz = std::wcsftime(&str[0], str.size() - 1, fmt, tmb); 
	//	str.resize(sz);
	//	return str;
	//}
	// http://en.cppreference.com/w/cpp/chrono/c/wcsftime
	// Defined in header <cwchar>
	//	std::size_t wcsftime(wchar_t* str, std::size_t count, const wchar_t* format, const std::tm* time);
	// Converts the date and time information from a given calendar time time to a null - terminated 
	// wide character string str according to format string format.Up to count bytes are written.
	//	Parameters
	//	str - pointer to the first element of the wchar_t array for output
	//	count - maximum number of wide characters to write
	//	format - pointer to a null - terminated wide character string specifying the format of conversion.

	}
//#endif  // STD_put_time_NOT_SUPPORTED

#include <iostream>

namespace nana
{
	bool is_utf8(const char* str, std::size_t len)
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

			if ((uv < 0xE0) && (end - ustr > 1))
				ustr += 2;
			else if ((uv < 0xF0) && (end - ustr > 2))
				ustr += 3;
			else if ((uv < 0x1F) && (end - ustr > 3))
				ustr += 4;
			else
				return false;
		}
		return true;
	}

	//class utf8_Error

#if defined(_MSC_VER)
#	if (_MSC_VER < 1900)
	//A workaround for lack support of C++11 inheriting constructors  for VC2013
	utf8_Error::utf8_Error(const std::string& msg)
		: std::runtime_error(msg)
	{}
#	endif
#endif

    void utf8_Error::emit()
		{
			if (use_throw)
				throw utf8_Error(*this);
			std::cerr << what();
		}

	//bool utf8_Error::use_throw{true}; 
	bool utf8_Error::use_throw{ false };
	//end class utf8_Error

	void throw_not_utf8(const std::string& text)
	{
		if (!is_utf8(text.c_str(), text.length()))
			return utf8_Error(std::string("\nThe text is not encoded in UTF8: ") + text).emit();
	}

	void throw_not_utf8(const char* text, std::size_t len)
	{
		if (!is_utf8(text, len))
			return utf8_Error(std::string("\nThe text is not encoded in UTF8: ") + std::string(text, len) ).emit();

		//throw std::invalid_argument("The text is not encoded in UTF8");
	}

	void throw_not_utf8(const char* text)
	{
		if (!is_utf8(text, std::strlen(text)))
			return utf8_Error(std::string("\nThe text is not encoded in UTF8: ") + text).emit();

		//throw std::invalid_argument("The text is not encoded in UTF8");
		
	}

	std::string recode_to_utf8(std::string no_utf8)
	{
		return nana::charset(std::move(no_utf8)).to_bytes(nana::unicode::utf8);
	}

	/// this text needed change, it needed review ??
	bool review_utf8(const std::string& text)
	{
		if (!is_utf8(text.c_str(), text.length()))
		{
			utf8_Error(std::string("\nThe const text is not encoded in UTF8: ") + text).emit();
			return true;   /// it needed change, it needed review !!
		}
		else
			return false;
	}

	/// this text needed change, it needed review ??
	bool review_utf8(std::string& text)
	{
		if (!is_utf8(text.c_str(), text.length()))
		{
			utf8_Error(std::string("\nThe text is not encoded in UTF8: ") + text).emit();
			text=recode_to_utf8(text);
			return true;   /// it needed change, it needed review !!
		}
		else
			return false;
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

#if defined(VERBOSE_PREPROCESSOR)
#	include <nana/verbose_preprocessor.hpp>
#endif