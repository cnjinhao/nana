/*
 *	Nana Configuration
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/config.hpp
 */

#ifndef NANA_CONFIG_HPP
#define NANA_CONFIG_HPP


#if defined(_MSC_VER)
	#define _SCL_SECURE_NO_WARNINGS
	#define _CRT_SECURE_NO_DEPRECATE
    #pragma warning(disable : 4996)

	#if (_MSC_VER < 1900)
        // is this a good idea?
        #define NOT_IMPLEMENTED_KEYWORD_noexcept 
	#endif // _MSC_VER < 1900 
	#if (_MSC_VER == 1900)
		 // google: break any code that tries to use codecvt<char16_t> or codecvt<char32_t>.
		 // google: It appears the C++ libs haven't been compiled with native char16_t/char32_t support.
		 // google: Those definitions are for codecvt<wchar_t>::id, codecvt<unsigned short>::id and codecvt<char>::id respectively. 
		 // However, the codecvt<char16_t>::id and codecvt<char32_t>::id definitions aren't there, and indeed, if you look at locale0.cpp in the CRT source code you'll see they're not defined at all.
		 // google: That's a known issue, tracked by an active bug (DevDiv#1060849). We were able to update the STL's headers in response to char16_t/char32_t, but we still need to update the separately compiled sources.
		#define STD_CODECVT_NOT_SUPPORTED
	#endif // _MSC_VER == 1900 
#endif // _MSVC

//Select platform automatically
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
//Windows:
	#define NANA_WINDOWS	1
	#define PLATFORM_SPEC_HPP <nana/detail/win32/platform_spec.hpp>

	//Test if it is MINGW
	#if defined(__MINGW32__) || defined(__MINGW64__)
		#define NANA_MINGW
		#define STD_CODECVT_NOT_SUPPORTED
		#if (__GNUC__ == 4) && ((__GNUC_MINOR__ < 8) || (__GNUC_MINOR__ == 8 && __GNUC_PATCHLEVEL__ < 1))
			//Use this flag if MinGW version is older than 4.8.1
			#define STD_THREAD_NOT_SUPPORTED
		#endif
	#endif
#elif (defined(linux) || defined(__linux) || defined(__linux__) || defined(__GNU__) || defined(__GLIBC__)) && !defined(_CRAYC)
//Linux:
	#define NANA_LINUX	1
	#define NANA_X11	1
	#define PLATFORM_SPEC_HPP <nana/detail/linux_X11/platform_spec.hpp>
	#define STD_CODECVT_NOT_SUPPORTED
#else
#	static_assert(false, "Only Windows and Unix are supported now");
#endif

#if defined(NANA_MINGW) || defined(NANA_LINUX)
	#if (__GNUC__ == 4) && (__GNUC_MINOR__ == 8) && (__GNUC_PATCHLEVEL__ <= 1)
		//Some functions which are specified in 21.5 Numeric conversions in Strings library have not yet implemented
		#define STD_NUMERIC_CONVERSIONS_NOT_SUPPORTED
	#endif
#endif

//Here defines some flags that tell Nana what features will be supported.
#define NANA_UNICODE

#if defined(NANA_UNICODE) && defined(NANA_WINDOWS)
	#ifndef _UNICODE
		#define _UNICODE
	#endif

	#ifndef UNICODE
		#define UNICODE
	#endif
#endif

//Support for PNG
//	Comment it to disable the feature of support for PNG.
//#define NANA_ENABLE_PNG
#if defined(NANA_ENABLE_PNG)
	//Comment it to use libpng from operating system.
	#define NANA_LIBPNG
#endif


#endif	//NANA_CONFIG_HPP
