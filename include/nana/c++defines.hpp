/**
 *	Predefined Symbols for C++
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2016-2018 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file  nana/c++defines.hpp
 *
 *	@brief Provide switches to adapt to the target OS, use of external libraries or workarounds compiler errors or lack of std C++ support.
 *
 *	To control target OS/compiler:
 *	- NANA_WINDOWS
 *	- NANA_MINGW
 *	- NANA_POSIX
 *	- NANA_LINUX
 *	- NANA_MACOS
 *	- NANA_X11
 *
 *	External libraries:
 *	- NANA_LIBPNG, USE_LIBPNG_FROM_OS
 *	- NANA_LIBJPEG, USE_LIBJPEG_FROM_OS
 *
 * (see: Feature-testing recommendations for C++
 *       in http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/p0096r0.html
 *       for example: __cpp_lib_experimental_filesystem  = 201406 in <experimental/filesystem)
 *
 *	Workaround to known compiler errors, unnecessary warnings or lack of C++11/14/17 support:
 *	- _SCL_SECURE_NO_WARNNGS, _CRT_SECURE_NO_DEPRECATE (VC)
 *	- STD_CODECVT_NOT_SUPPORTED (VC RC, <codecvt> is a known issue on libstdc++, it works on libc++)
 *	- STD_THREAD_NOT_SUPPORTED (GCC < 4.8.1)
 *	- STD_NUMERIC_CONVERSIONS_NOT_SUPPORTED  (MinGW with GCC < 4.8.1)
 *	- STD_NUMERIC_CONVERSIONS_NOT_SUPPORTED (MinGW with GCC < 4.8.1)
 *	- STD_TO_STRING_NOT_SUPPORTED (MinGW with GCC < 4.8)
 *	- STD_FILESYSTEM_NOT_SUPPORTED (GCC < 5.3) ....
 *	- CXX_NO_INLINE_NAMESPACE (Visual C++ < 2015)
 *
 *	There are two kinds of flags:
 *	* _nana_std_xxx indicates that nana provides a standard-like class for workaround of lack of C++ support.
 *  * _nana_std_has_xxx indicates that nana detects whether a C++ feature is supported. Nana doesn't provide a standard-like class for this missing feature.
 *
 *	- _nana_std_make_unique (__cpluscplus < 201402)
 *	- _nana_std_put_time (GCC < 5)
 *  - _nana_std_clamp (Visual C++ < 2017)
 */

#ifndef NANA_CXX_DEFINES_INCLUDED
#define NANA_CXX_DEFINES_INCLUDED
#define STD_FILESYSTEM_NOT_SUPPORTED

//C++ language
#if defined(_MSC_VER)
#	if (_MSC_VER < 1900)
#		//About std.experimental.filesystem.
#		//Through VC2013 has provided <filesystem>, but all the names are given in namespace std. It's hard to alias these names into std::experimental,
#		//So Nana use nana.filesystem implement instead for VC2013
#
#		//Nana defines some macros for lack of support of keywords
#		define _ALLOW_KEYWORD_MACROS
#
#		define CXX_NO_INLINE_NAMESPACE //no support of C++11 inline namespace until Visual C++ 2015
#		define noexcept		//no support of noexcept until Visual C++ 2015

#		define constexpr	//no support of constexpr until Visual C++ 2015 ? const ??
#	else
#		undef STD_FILESYSTEM_NOT_SUPPORTED
#	endif
#elif defined(__GNUC__) && not defined(__clang__)
#	if (__GNUC__ == 4 && __GNUC_MINOR__ < 6)
#		define noexcept		//no support of noexcept until GCC 4.6
#	endif
#endif

// Set this to "UTF-32" at the command-line for big endian.
#ifndef NANA_UNICODE
    // much of the world runs intel compatible processors so default to LE.
	#define NANA_UNICODE "UTF-32LE"
#endif

// Select platform  ......
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)	//Microsoft Windows
	#define NANA_WINDOWS
	typedef unsigned long thread_t;

	// MINGW ...
	#if defined(__MINGW32__) || defined(__MINGW64__) || defined(MINGW)
		#define NANA_MINGW
	#endif // MINGW

#elif defined(APPLE)	//Mac OS X
	//Symbols for MACOS
	#define NANA_MACOS
	#define NANA_POSIX
	#define NANA_X11
	typedef unsigned long thread_t;
#elif defined(__FreeBSD__)
	#define NANA_POSIX
	#define NANA_X11
	typedef unsigned long thread_t;
#elif (defined(linux) || defined(__linux) || defined(__linux__) || defined(__GNU__) || defined(__GLIBC__)) && !defined(_CRAYC)	//Linux
	#define NANA_LINUX
	#define NANA_POSIX
	#define NANA_X11
	typedef unsigned long thread_t;
#else
	static_assert(false, "Only Windows and Linux are supported now (Mac OS and BSD are experimental)");
#endif

// Select compiler ...
#if defined(_MSC_VER)	//Microsoft Visual C++
	#define _SCL_SECURE_NO_WARNNGS
	#define _CRT_SECURE_NO_DEPRECATE
	#pragma warning(disable : 4996)

#	if (_MSC_VER >= 1900)
		// google: break any code that tries to use codecvt<char16_t> or codecvt<char32_t>.
		// google: It appears the C++ libs haven't been compiled with native char16_t/char32_t support.
		// google: Those definitions are for codecvt<wchar_t>::id, codecvt<unsigned short>::id and codecvt<char>::id respectively.
		// However, the codecvt<char16_t>::id and codecvt<char32_t>::id definitions aren't there, and indeed, if you look at locale0.cpp in the CRT source code you'll see they're not defined at all.
		// google: That's a known issue, tracked by an active bug (DevDiv#1060849). We were able to update the STL's headers in response to char16_t/char32_t, but we still need to update the separately compiled sources.
#		define STD_CODECVT_NOT_SUPPORTED
#	endif // _MSC_VER == 1900

#elif defined(__clang__)	//Clang

	#include <iosfwd>	//Introduces some implement-specific flags of ISO C++ Library
	#if defined(__GLIBCPP__) || defined(__GLIBCXX__)
		//<codecvt> is a known issue on libstdc++, it works on libc++
		#define STD_CODECVT_NOT_SUPPORTED
	#endif
#elif defined(__GNUC__) //GCC

	#include <iosfwd>	//Introduces some implement-specific flags of ISO C++ Library
	#if defined(__GLIBCPP__) || defined(__GLIBCXX__)
		//<codecvt> is a known issue on libstdc++, it works on libc++
		#define STD_CODECVT_NOT_SUPPORTED

		//It's a known issue of libstdc++ on MinGW
		//introduce to_string/to_wstring workarounds for disabled capacity of stdlib
		#ifdef _GLIBCXX_HAVE_BROKEN_VSWPRINTF
			#if (__GNUC__ < 5)
			#	define STD_TO_STRING_NOT_SUPPORTED
			#endif

			#define STD_TO_WSTRING_NOT_SUPPORTED
		#endif
	#endif


#	if ((__GNUC__ < 5)   )
#		define _nana_std_put_time
#	endif

#   if ((__GNUC__ > 5) || ((__GNUC__ == 5) && (__GNUC_MINOR__ >= 3 ) ) )
#	    undef STD_FILESYSTEM_NOT_SUPPORTED
#   endif

	#if (__GNUC__ == 4)
		#if ((__GNUC_MINOR__ < 8) || (__GNUC_MINOR__ == 8 && __GNUC_PATCHLEVEL__ < 1))
			#define STD_THREAD_NOT_SUPPORTED
		#endif

		#if defined(NANA_MINGW)
			#ifndef __MINGW64_VERSION_MAJOR
				//It's a known issue under MinGW(except MinGW-W64)
				#define STD_NUMERIC_CONVERSIONS_NOT_SUPPORTED
			#endif
		#endif

		#if (__GNUC_MINOR__ < 8)
			//introduce to_string/to_wstring workaround for lack of stdlib definitions
			#ifndef STD_TO_STRING_NOT_SUPPORTED
			#	define STD_TO_STRING_NOT_SUPPORTED
			#endif

			#ifndef STD_TO_WSTRING_NOT_SUPPORTED
			#	define STD_TO_WSTRING_NOT_SUPPORTED
			#endif
		#endif
	#endif
#endif

//Assume the std::thread is not implemented on MinGW,
//unless it was compiled with POSIX threading support.
//But some toolchains may implement std::thread.
#ifdef NANA_MINGW
#	ifndef STD_THREAD_NOT_SUPPORTED
#		define STD_THREAD_NOT_SUPPORTED
#	endif
#endif


//Detects the feature std::make_unique
//std::make_unique has been provided by Visual C++ 2013 and later
#undef _nana_std_make_unique
#if (defined(__clang__) && (__cplusplus < 201305L || (__cplusplus == 201305L && (__clang_major__ * 100 + __clang_minor__ < 304 )))) \
	|| ((!defined(__clang__)) && defined(__GNUC__) && __cplusplus < 201300L)
#	define _nana_std_make_unique
#endif

//Detects the feature std::clamp
//Visual C++ 2017 with /std:c++latest provides the std::clamp
#undef _nana_std_clamp
#if (defined(_MSC_VER) && ((!defined(_MSVC_LANG)) || _MSVC_LANG < 201403L))	\
	|| (defined(__clang__) && (__cplusplus < 201406L))						\
	|| (defined(__GNUC__) && (!defined(__clang__)) && (__cplusplus < 201703))
#	define _nana_std_clamp
#endif


#undef _nana_std_optional
#if ((defined(_MSC_VER) && ((!defined(_MSVC_LANG)) || _MSVC_LANG < 201703))) ||	\
	((!defined(_MSC_VER)) && ((__cplusplus < 201703L) || \
		(defined(__clang__) && (__clang_major__ * 100 + __clang_minor__ < 400)) ||				\
		(!defined(__clang__) && defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__ < 701)))	\
	)
#	define _nana_std_optional
#endif

#undef _nana_std_has_string_view
#undef _nana_std_has_emplace_return_type
#if ((defined(_MSC_VER) && (_MSC_VER >= 1912) && defined(_MSVC_LANG) && _MSVC_LANG >= 201703)) ||				\
	((__cplusplus >= 201703L) && \
		(defined(__clang__) && (__clang_major__ * 100 + __clang_minor__ >= 400) ||		\
		(!defined(__clang__) && defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__ >= 701))) \
	)
#	define _nana_std_has_string_view
#	define _nana_std_has_emplace_return_type
#endif


#if defined(NANA_WINDOWS)
	#ifndef _UNICODE
		#define _UNICODE
	#endif

	#ifndef UNICODE
		#define UNICODE
	#endif
#endif

// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/p0061r0.html

#if defined(__cpp_lib_experimental_filesystem) && (__cpp_lib_experimental_filesystem == 201406)
#	undef STD_FILESYSTEM_NOT_SUPPORTED
#endif


#ifdef __has_include
#  if __has_include(<filesystem>)
#    undef STD_FILESYSTEM_NOT_SUPPORTED
#  endif
#  if __has_include(<mutex>) 
#    if !(defined(NANA_MINGW) && !defined(_GLIBCXX_HAS_GTHREADS))
//See the comment above regarding MinGW's threading support
#      undef STD_THREAD_NOT_SUPPORTED
#    endif
#  endif
#endif

#endif  // NANA_CXX_DEFINES_INCLUDED
