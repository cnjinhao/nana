/**
 *	Predefined Symbols for C++
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2016-2020 Jinhao(cnjinhao@hotmail.com)
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
 *
 */

#ifndef NANA_CXX_DEFINES_INCLUDED
#define NANA_CXX_DEFINES_INCLUDED


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

#elif defined(__APPLE__) || defined(APPLE)	//Mac OS X
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
#	define _SCL_SECURE_NO_WARNNGS
#	define _CRT_SECURE_NO_DEPRECATE
#	pragma warning(disable : 4996)

    static_assert(_MSC_VER >= 1912, "Nana requires Visual Studio 2017 Update 5 and later");
#	if defined(_MSVC_LANG)
		static_assert(_MSVC_LANG >= 201703L, "Nana requires C++17, please specify /std:C++17 for the compiler option.");
#	else
		static_assert(false, "_MSVC_LANG is not defined");
#	endif
#elif defined(__clang__)	//Clang
#elif defined(__GNUC__) // GCC
#	if defined(NANA_MINGW)
        static_assert(__GNUC__ * 100 + __GNUC_MINOR__  >= 803, "Nana requires MinGW/GCC 8.3 and later");
#	endif
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
