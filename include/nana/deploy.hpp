/*
 *	The Deploy Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
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
#if defined(VERBOSE_PREPROCESSOR)
	#include <nana/verbose_preprocessor.hpp>
#endif

#include <stdexcept>
#include <nana/charset.hpp>

//Implement workarounds for GCC/MinGW which version is below 4.8.2
#if defined(STD_NUMERIC_CONVERSIONS_NOT_SUPPORTED)
namespace std
{
	//Workaround for no implemenation of std::stoi in MinGW.
	int stoi(const std::string&, std::size_t * pos = nullptr, int base = 10);
	int stoi(const std::wstring&, std::size_t* pos = nullptr, int base = 10);

	//Workaround for no implemenation of std::stof in MinGW.
	float stof(const std::string&, std::size_t * pos = nullptr);
	float stof(const std::wstring&, std::size_t* pos = nullptr);

	//Workaround for no implemenation of std::stod in MinGW.
	double stod(const std::string&, std::size_t * pos = nullptr);
	double stod(const std::wstring&, std::size_t* pos = nullptr);

	//Workaround for no implemenation of std::stold in MinGW.
	long double stold(const std::string&, std::size_t * pos = nullptr);
	long double stold(const std::wstring&, std::size_t* pos = nullptr);

	//Workaround for no implemenation of std::stol in MinGW.
	long stol(const std::string&, std::size_t* pos = nullptr, int base = 10);
	long stol(const std::wstring&, std::size_t* pos = nullptr, int base = 10);

	//Workaround for no implemenation of std::stoll in MinGW.
	long long stoll(const std::string&, std::size_t* pos = nullptr, int base = 10);
	long long stoll(const std::wstring&, std::size_t* pos = nullptr, int base = 10);

	//Workaround for no implemenation of std::stoul in MinGW.
	unsigned long stoul(const std::string&, std::size_t* pos = nullptr, int base = 10);
	unsigned long stoul(const std::wstring&, std::size_t* pos = nullptr, int base = 10);

	//Workaround for no implemenation of std::stoull in MinGW.
	unsigned long long stoull(const std::string&, std::size_t* pos = nullptr, int base = 10);
	unsigned long long stoull(const std::wstring&, std::size_t* pos = nullptr, int base = 10);
}
#endif //STD_NUMERIC_CONVERSIONS_NOT_SUPPORTED

#ifdef STD_TO_STRING_NOT_SUPPORTED
namespace std
{
	//Workaround for no implemenation of std::to_string/std::to_wstring in MinGW.
	std::string to_string(long double);
	std::string to_string(double);
	std::string to_string(unsigned);
	std::string to_string(int);
	std::string to_string(long);
	std::string to_string(unsigned long);
	std::string to_string(long long);
	std::string to_string(unsigned long long);
	std::string to_string(float);
}
#endif

#ifdef STD_TO_WSTRING_NOT_SUPPORTED
namespace std
{
	std::wstring to_wstring(long double);
	std::wstring to_wstring(double);
	std::wstring to_wstring(unsigned);
	std::wstring to_wstring(int);
	std::wstring to_wstring(long);
	std::wstring to_wstring(unsigned long);
	std::wstring to_wstring(long long);
	std::wstring to_wstring(unsigned long long);
	std::wstring to_wstring(float);
}
#endif

namespace nana
{
	/// Checks whether a specified text is utf8 encoding
	bool is_utf8(const char* str, unsigned len);
	void throw_not_utf8(const std::string& text);
	void throw_not_utf8(const char*, unsigned len);
	void throw_not_utf8(const char*);

	const std::string& to_utf8(const std::string&);
	std::string to_utf8(const std::wstring&);

	std::wstring to_wstring(const std::string& utf8_str);
	const std::wstring& to_wstring(const std::wstring& wstr);
	std::wstring&& to_wstring(std::wstring&& wstr);

#if defined(NANA_WINDOWS)
	std::string to_osmbstr(const std::string& text_utf8);
#else
	std::string to_osmbstr(std::string text_utf8);
#endif


	namespace detail
	{
#if defined(NANA_WINDOWS)
		using native_string_type = std::wstring;
#else	//POSIX
		using native_string_type = std::string;
#endif
	}

#if defined(NANA_WINDOWS)
	const detail::native_string_type to_nstring(const std::string&);
	const detail::native_string_type& to_nstring(const std::wstring&);
	detail::native_string_type to_nstring(std::string&&);
	detail::native_string_type&& to_nstring(std::wstring&&);
#else	//POSIX
	const detail::native_string_type& to_nstring(const std::string&);
	const detail::native_string_type to_nstring(const std::wstring&);
	detail::native_string_type&& to_nstring(std::string&&);
	detail::native_string_type to_nstring(std::wstring&&);
#endif
	detail::native_string_type to_nstring(int);
	detail::native_string_type to_nstring(double);
	detail::native_string_type to_nstring(std::size_t);
}


namespace nana
{
	inline unsigned make_rgb(unsigned char red, unsigned char green, unsigned char blue)
	{
		return ((unsigned(red) << 16)|((unsigned(green)<<8))|blue);
	}
}

#define NANA_RGB(a)	(((DWORD)(a) & 0xFF)<<16) |  ((DWORD)(a) & 0xFF00) | (((DWORD)(a) & 0xFF0000) >> 16 )


#ifdef STD_MAKE_UNIQUE_NOT_SUPPORTED
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3656.htm

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

namespace std {
	template<class T> struct _Unique_if {
		typedef unique_ptr<T> _Single_object;
	};

	template<class T> struct _Unique_if<T[]> {
		typedef unique_ptr<T[]> _Unknown_bound;
	};

	template<class T, size_t N> struct _Unique_if<T[N]> {
		typedef void _Known_bound;
	};

	template<class T, class... Args>
	typename _Unique_if<T>::_Single_object
	make_unique(Args&&... args) {
		return unique_ptr<T>(new T(std::forward<Args>(args)...));
	}

	template<class T>
	typename _Unique_if<T>::_Unknown_bound
	make_unique(size_t n) {
		typedef typename remove_extent<T>::type U;
		return unique_ptr<T>(new U[n]());
	}

	template<class T, class... Args>
	typename _Unique_if<T>::_Known_bound
			make_unique(Args&&...) = delete;
}
#endif //STD_make_unique_NOT_SUPPORTED

#endif //NANA_MACROS_HPP
