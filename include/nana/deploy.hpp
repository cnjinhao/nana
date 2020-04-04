/*
 *	The Deploy Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2020 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/deploy.hpp
 *
 *	What follows is dependent on what defined in nana/config.hpp
 */

#ifndef NANA_DEPLOY_HPP
#define NANA_DEPLOY_HPP
#include <nana/push_ignore_diagnostic>

#include <nana/config.hpp>
#include <nana/charset.hpp>

#include <stdexcept>
#include <string_view>

namespace nana
{
	/// move to *.h ??
	struct utf8_Error : std::runtime_error
	{
		static bool use_throw; ///< def { true }; use carefully - it is a global variable !! \todo initialize from a #define ?

		using std::runtime_error::runtime_error;

#if defined(_MSC_VER)
#	if (_MSC_VER < 1900)
		//A workaround for lack support of C++11 inheriting constructors  for VC2013
		explicit utf8_Error(const std::string& msg);
#	endif
#endif

		void emit();
	};


	/// Checks whether a specified text is utf8 encoding
	bool is_utf8(std::string_view str);
	void throw_not_utf8(std::string_view str);

	/// this text needed change, it needed review ??
	bool review_utf8(const std::string& text);

	/// this text needed change, it needed review ??
	bool review_utf8(std::string& text);

	const std::string& to_utf8(const std::string&);

	std::string to_utf8(std::wstring_view sv);
	std::wstring to_wstring(std::string_view utf8_str);

	const std::wstring& to_wstring(const std::wstring& wstr);
	std::wstring&& to_wstring(std::wstring&& wstr);

#ifdef __cpp_char8_t
	std::string 	to_string(std::u8string_view);
	std::wstring	to_wstring(std::u8string_view s);
	std::u8string	to_u8str(std::string_view s);
	std::u8string	to_u8str(std::wstring_view s);
#endif

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

#ifdef __cpp_char8_t
		detail::native_string_type to_nstring(std::u8string_view);
#endif
	}
}


namespace nana
{
	inline unsigned make_rgb(unsigned char red, unsigned char green, unsigned char blue)
	{
		return ((unsigned(red) << 16)|((unsigned(green)<<8))|blue);
	}
}

#include <nana/pop_ignore_diagnostic>
#endif //NANA_DEPLOY_HPP
