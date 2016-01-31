/**
 *	Predefined Symbols for C++
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2016 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file  nana/config.hpp
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
 */

#ifndef NANA_CXX_DEFINES_INCLUDED
#define NANA_CXX_DEFINES_INCLUDED

//C++ language
#if defined(_MSC_VER)
#	if (_MSC_VER < 1900)
#		//Nana defines some macros for lack of support of keywords
#		define _ALLOW_KEYWORD_MACROS
#		
#		define noexcept		//no support of noexcept until Visual C++ 2015
#		define constexpr	//no support of constexpr until Visual C++ 2015
#	endif
#elif defined(__GNUC__)
#	if (__GNUC__ == 4 && __GNUC_MINOR__ < 6)
#		define noexcept		//no support of noexcept until GCC 4.6
#	endif
#endif

// Select platform  ......
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)	//Microsoft Windows
	#define NANA_WINDOWS

	// MINGW ...
	#if defined(__MINGW32__) || defined(__MINGW64__) || defined(MINGW)
		#define NANA_MINGW
	#endif // MINGW

#elif defined(APPLE)	//Mac OS X
	//Symbols for MACOS

	#define NANA_MACOS
	#define NANA_X11

#elif (defined(linux) || defined(__linux) || defined(__linux__) || defined(__GNU__) || defined(__GLIBC__)) && !defined(_CRAYC)	//Linux
	#define NANA_LINUX
	#define NANA_X11
#else
#	static_assert(false, "Only Windows and Unix are supported now (Mac OS is experimental)");
#endif

//Define a symbol for POSIX operating system.
#if defined(NANA_LINUX) || defined(NANA_MACOS)
	#define NANA_POSIX
#endif



// Select compiler ...
#if defined(_MSC_VER)	//Microsoft Visual C++
	#define _SCL_SECURE_NO_WARNNGS
	#define _CRT_SECURE_NO_DEPRECATE
	#pragma warning(disable : 4996)

	#if (_MSC_VER == 1900)
		// google: break any code that tries to use codecvt<char16_t> or codecvt<char32_t>.
		// google: It appears the C++ libs haven't been compiled with native char16_t/char32_t support.
		// google: Those definitions are for codecvt<wchar_t>::id, codecvt<unsigned short>::id and codecvt<char>::id respectively.
		// However, the codecvt<char16_t>::id and codecvt<char32_t>::id definitions aren't there, and indeed, if you look at locale0.cpp in the CRT source code you'll see they're not defined at all.
		// google: That's a known issue, tracked by an active bug (DevDiv#1060849). We were able to update the STL's headers in response to char16_t/char32_t, but we still need to update the separately compiled sources.
		#define STD_CODECVT_NOT_SUPPORTED
	#endif // _MSC_VER == 1900

#elif defined(__clang__)	//Clang

	#include <iosfwd>	//Introduces some implement-specific flags of ISO C++ Library
	#if defined(__GLIBCPP__) || defined(__GLIBCXX__)
		//<codecvt> is a known issue on libstdc++, it works on libc++
		#define STD_CODECVT_NOT_SUPPORTED


		#ifndef STD_MAKE_UNIQUE_NOT_SUPPORTED
			#define STD_MAKE_UNIQUE_NOT_SUPPORTED
		#endif

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

	#if (__GNUC__ == 4)
		#if ((__GNUC_MINOR__ < 8) || (__GNUC_MINOR__ == 8 && __GNUC_PATCHLEVEL__ < 1))
			#define STD_THREAD_NOT_SUPPORTED
		#endif

		#if (__GNUC_MINOR__ < 9)
			#define STD_MAKE_UNIQUE_NOT_SUPPORTED
		#endif

		#if defined(NANA_MINGW)
			//It's a knonwn issue under MinGW
			#define STD_NUMERIC_CONVERSIONS_NOT_SUPPORTED
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


#if defined(NANA_WINDOWS)
	#ifndef _UNICODE
		#define _UNICODE
	#endif

	#ifndef UNICODE
		#define UNICODE
	#endif
#endif

#endif  // NANA_CXX_DEFINES_INCLUDED
