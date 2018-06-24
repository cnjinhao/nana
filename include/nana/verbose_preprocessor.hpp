/**
 *	Nana Verbose preprocessor
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file     nana/verbose_preprocessor.hpp
 *
 *	@brief    show the values of configuration constants during compilation to facilitate build debugging.
 *
 *	Define VERBOSE_PREPROCESSOR to show the messages or undefine for a normal build.
 *	Normally undefined. Define in case you want to debug the build system.
 *
 *  @authors  Ariel Vina-Rodriguez (qPCR4vir)
 *
 */
// Created by ariel.rodriguez on 08.12.2015.
//

#ifndef NANA_VERBOSE_PREPROCESSOR_H
#define NANA_VERBOSE_PREPROCESSOR_H

#if defined(VERBOSE_PREPROCESSOR)

    #include <nana/config.hpp>
    #include <nana/deploy.hpp>



    #define STRING2(...) #__VA_ARGS__
	#define STRING(x) STRING2(x)
    #define SHOW_VALUE(x) " " #x "  =  " STRING2(x)

	#pragma message ( "\n -----> Verbose preprocessor"  )
	#pragma message (  SHOW_VALUE(VERBOSE_PREPROCESSOR)  )
	#pragma message (  SHOW_VALUE(STOP_VERBOSE_PREPROCESSOR)  )

	#pragma message ( "\n -----> OS: \n   --Windows: "    )
    #pragma message (  SHOW_VALUE(_WIN32)  )
    #pragma message (  SHOW_VALUE(__WIN32__)  )
    #pragma message (  SHOW_VALUE(WIN32)  )
	#pragma message (  SHOW_VALUE(NANA_WINDOWS)  )

	#pragma message ( "\n   ---NIX: "  )
	#pragma message (  SHOW_VALUE(NANA_LINUX)  )
	#pragma message (  SHOW_VALUE(NANA_POSIX)  )
	#pragma message (  SHOW_VALUE(NANA_X11)  )
	#pragma message (  SHOW_VALUE(APPLE)  )
	#pragma message (  SHOW_VALUE(NANA_IGNORE_CONF)  )


	#pragma message ( "\n -----> Compilers: \n  MinGW: "  )
	#pragma message (  SHOW_VALUE(__MINGW32__)  )
	#pragma message (  SHOW_VALUE(__MINGW64__)  )
	#pragma message (  SHOW_VALUE(MINGW)  )

	#pragma message ( "\n  ---MSC: " )
	#pragma message (  SHOW_VALUE(_MSC_VER)     )
	#pragma message (  SHOW_VALUE(_MSC_FULL_VER)  )

	#pragma message ( "\n  ---GNU: " )
	#pragma message (  SHOW_VALUE(__GNUC__)  )
	#pragma message (  SHOW_VALUE(__GNUC_MINOR__)  )
	#pragma message (  SHOW_VALUE(__GNUC_PATCHLEVEL__)  )

	#pragma message ( "\n  ---Clang compiler: " )
	#pragma message (  SHOW_VALUE(__clang__)  )
	#pragma message (  SHOW_VALUE(__GLIBCPP__)  )
	#pragma message (  SHOW_VALUE(__GLIBCXX__)  )

    #pragma message ( "\n -----> STD:  " )
	#pragma message (  SHOW_VALUE(STD_CODECVT_NOT_SUPPORTED)  )
	#pragma message (  SHOW_VALUE(STD_THREAD_NOT_SUPPORTED)  )
	#pragma message (  SHOW_VALUE(STD_NUMERIC_CONVERSIONS_NOT_SUPPORTED)  )
	#pragma message (  SHOW_VALUE(STD_TO_STRING_NOT_SUPPORTED)  )
	#pragma message (  SHOW_VALUE(STD_TO_WSTRING_NOT_SUPPORTED)  )
	#pragma message (  SHOW_VALUE(USE_github_com_meganz_mingw_std_threads)  )
	#pragma message (  SHOW_VALUE(NANA_ENABLE_MINGW_STD_THREADS_WITH_MEGANZ)  )
	#pragma message (  SHOW_VALUE(STD_THREAD_NOT_SUPPORTED)  )
	#pragma message (  SHOW_VALUE(_nana_std_put_time)  )
	#pragma message (  SHOW_VALUE(STD_MAKE_UNIQUE_NOT_SUPPORTED)  )

	#pragma message (  SHOW_VALUE(STD_FILESYSTEM_NOT_SUPPORTED)  )
	#pragma message (  SHOW_VALUE(BOOST_FILESYSTEM_AVAILABLE)  )
	#pragma message (  SHOW_VALUE(BOOST_FILESYSTEM_FORCE)  )
	#pragma message (  SHOW_VALUE(STD_FILESYSTEM_FORCE)  )
	#pragma message (  SHOW_VALUE(NANA_FILESYSTEM_FORCE)  )
	#pragma message (  SHOW_VALUE(CXX_NO_INLINE_NAMESPACE)  )
	//#pragma message (  SHOW_VALUE(__has_include)  )
	#pragma message (  SHOW_VALUE(__cpp_lib_experimental_filesystem)  )
	#pragma message (  SHOW_VALUE(NANA_USING_NANA_FILESYSTEM)  )
	#pragma message (  SHOW_VALUE(NANA_USING_STD_FILESYSTEM)  )
	#pragma message (  SHOW_VALUE(NANA_USING_BOOST_FILESYSTEM)  )

    #pragma message ( "\n#include <nana/filesystem/filesystem.hpp> " )
    #include <nana/filesystem/filesystem.hpp>

	#pragma message (  SHOW_VALUE(STD_MAKE_UNIQUE_NOT_SUPPORTED)  )
	#pragma message (  SHOW_VALUE(STD_FILESYSTEM_NOT_SUPPORTED)  )
	#pragma message (  SHOW_VALUE(BOOST_FILESYSTEM_AVAILABLE)  )
	#pragma message (  SHOW_VALUE(BOOST_FILESYSTEM_FORCE)  )
	#pragma message (  SHOW_VALUE(STD_FILESYSTEM_FORCE)  )
	#pragma message (  SHOW_VALUE(NANA_FILESYSTEM_FORCE)  )
	#pragma message (  SHOW_VALUE(CXX_NO_INLINE_NAMESPACE)  )
	//#pragma message (  SHOW_VALUE(__has_include)  )
	#pragma message (  SHOW_VALUE(__cpp_lib_experimental_filesystem)  )
	#pragma message (  SHOW_VALUE(NANA_USING_NANA_FILESYSTEM)  )
	#pragma message (  SHOW_VALUE(NANA_USING_STD_FILESYSTEM)  )
	#pragma message (  SHOW_VALUE(NANA_USING_BOOST_FILESYSTEM)  )

	#pragma message (  SHOW_VALUE(NANA_UNICODE)  )
	#pragma message (  SHOW_VALUE(_UNICODE)  )
	#pragma message (  SHOW_VALUE(UNICODE)  )

    #pragma message ( "\n -----> Libraries: " )
	#pragma message (  SHOW_VALUE(NANA_ENABLE_AUDIO)  )
	#pragma message (  SHOW_VALUE(NANA_ENABLE_PNG)  )
	#pragma message (  SHOW_VALUE(USE_LIBPNG_FROM_OS)  )
	#pragma message (  SHOW_VALUE(NANA_LIBPNG)  )
	#pragma message (  SHOW_VALUE(NANA_ENABLE_JPEG)  )
	#pragma message (  SHOW_VALUE(USE_LIBJPEG_FROM_OS)  )
	#pragma message (  SHOW_VALUE(NANA_LIBJPEG)  )


   // #pragma message ( "\n =" STRING() ", \n =" STRING()"  , \n =" STRING() )

    #if defined(STOP_VERBOSE_PREPROCESSOR)
        #error ("\nCompilation stopped to avoid annoying messages")
    #endif

#endif // VERBOSE_PREPROCESSOR


#endif //NANA_VERBOSE_PREPROCESSOR_H
