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



    #define STRING2(x) #x
	#define STRING(x) STRING2(x)
    #define SHOW_VALUE(x)  "\n x=" STRING2(x) 

	#pragma message ( "\n ---> SVerbose preprocessor \nVERBOSE_PREPROCESSOR=" STRING(VERBOSE_PREPROCESSOR)" , \n STOP_VERBOSE_PREPROCESSOR=" STRING(STOP_VERBOSE_PREPROCESSOR)  )

	#pragma message ( "\n ---> OS: \nWindows: \n _WIN32=" STRING(_WIN32) ", \n __WIN32__ =" STRING(__WIN32__) " , \n WIN32=" STRING(WIN32)" , \n NANA_WINDOWS=" STRING(NANA_WINDOWS)   )

	#pragma message ( "\nNIX: \n NANA_LINUX=" STRING(NANA_LINUX) ", \n  NANA_POSIX=" STRING(NANA_POSIX) " , \n  NANA_X11=" STRING(NANA_X11) " , \n  APPLE=" STRING(APPLE) " , \n NANA_MACOS =" STRING(NANA_MACOS) " , \n NANA_IGNORE_CONF=" STRING(NANA_IGNORE_CONF) )

	#pragma message ( "\n ---> Compilers: \n MinGW: \n __MINGW32__=" STRING(__MINGW32__) ", \n __MINGW64__=" STRING(__MINGW64__) " , \n MINGW=" STRING(MINGW) )

	#pragma message ( "\nMSC: \n _MSC_VER=" STRING(_MSC_VER) ", \n _MSC_FULL_VER=" STRING(_MSC_FULL_VER ) )

	#pragma message ( "\nGNU: \n __GNUC__=" STRING(__GNUC__) ", \n __GNUC_MINOR__=" STRING(__GNUC_MINOR__) " , \n __GNUC_PATCHLEVEL__=" STRING(__GNUC_PATCHLEVEL__) )

	#pragma message ( "\nClang compiler: \n __clang__=" STRING(__clang__)  ", \n __GLIBCPP__=" STRING(__GLIBCPP__) " , \n __GLIBCXX__=" STRING(__GLIBCXX__) )

    #pragma message ( "\n ---> STD: \n STD_CODECVT_NOT_SUPPORTED=" STRING(STD_CODECVT_NOT_SUPPORTED) " , \n STD_THREAD_NOT_SUPPORTED=" STRING(STD_THREAD_NOT_SUPPORTED) )

    #pragma message ( "\n STD_NUMERIC_CONVERSIONS_NOT_SUPPORTED=" STRING(STD_NUMERIC_CONVERSIONS_NOT_SUPPORTED) " , \n STD_TO_STRING_NOT_SUPPORTED=" STRING(STD_TO_STRING_NOT_SUPPORTED) ", \n STD_TO_WSTRING_NOT_SUPPORTED=" STRING(STD_TO_WSTRING_NOT_SUPPORTED))

	#pragma message ( "\n USE_github_com_meganz_mingw_std_threads=" STRING(USE_github_com_meganz_mingw_std_threads) ", \n NANA_ENABLE_MINGW_STD_THREADS_WITH_MEGANZ=" STRING(NANA_ENABLE_MINGW_STD_THREADS_WITH_MEGANZ)"  , \n STD_THREAD_NOT_SUPPORTED=" STRING(STD_THREAD_NOT_SUPPORTED) )

    #pragma message ( "\n STD_put_time_NOT_SUPPORTED=" STRING(STD_put_time_NOT_SUPPORTED) " , \n STD_MAKE_UNIQUE_NOT_SUPPORTED=" STRING(STD_MAKE_UNIQUE_NOT_SUPPORTED)" , \n STD_FILESYSTEM_NOT_SUPPORTED=" STRING(STD_FILESYSTEM_NOT_SUPPORTED))

	#pragma message ( "\n BOOST_FILESYSTEM_AVAILABLE=" STRING(BOOST_FILESYSTEM_AVAILABLE) ", \n BOOST_FILESYSTEM_FORCE =" STRING(BOOST_FILESYSTEM_FORCE) " , \n STD_FILESYSTEM_FORCE=" STRING(STD_FILESYSTEM_FORCE) ", \n NANA_FILESYSTEM_FORCE=" STRING(NANA_FILESYSTEM_FORCE)   )

	#pragma message ( "\n CXX_NO_INLINE_NAMESPACE=" STRING(CXX_NO_INLINE_NAMESPACE) ", \n __has_include=" STRING(__has_include)"  , \n __cpp_lib_experimental_filesystem=" STRING(__cpp_lib_experimental_filesystem) )

    #pragma message ( "\n NANA_USING_NANA_FILESYSTEM=" STRING(NANA_USING_NANA_FILESYSTEM) ", \n NANA_USING_STD_FILESYSTEM=" STRING(NANA_USING_STD_FILESYSTEM)"  , \n NANA_USING_BOOST_FILESYSTEM=" STRING(NANA_USING_BOOST_FILESYSTEM) )

    #include <nana/filesystem/filesystem_selector.hpp>

    #pragma message ( "\n ...including filesystem_selector: \n STD_put_time_NOT_SUPPORTED=" STRING(STD_put_time_NOT_SUPPORTED) " , \n STD_FILESYSTEM_NOT_SUPPORTED=" STRING(STD_FILESYSTEM_NOT_SUPPORTED))

	#pragma message ( "\n BOOST_FILESYSTEM_AVAILABLE=" STRING(BOOST_FILESYSTEM_AVAILABLE) ", \n BOOST_FILESYSTEM_FORCE =" STRING(BOOST_FILESYSTEM_FORCE) " , \n STD_FILESYSTEM_FORCE=" STRING(STD_FILESYSTEM_FORCE) ", \n NANA_FILESYSTEM_FORCE=" STRING(NANA_FILESYSTEM_FORCE)   )

	#pragma message ( "\n CXX_NO_INLINE_NAMESPACE=" STRING(CXX_NO_INLINE_NAMESPACE) ", \n __has_include=" STRING(__has_include)"  , \n __cpp_lib_experimental_filesystem=" STRING(__cpp_lib_experimental_filesystem) )

    #pragma message ( "\n NANA_USING_NANA_FILESYSTEM=" STRING(NANA_USING_NANA_FILESYSTEM) ", \n NANA_USING_STD_FILESYSTEM=" STRING(NANA_USING_STD_FILESYSTEM)"  , \n NANA_USING_BOOST_FILESYSTEM=" STRING(NANA_USING_BOOST_FILESYSTEM) )
   
   // #pragma message ( "\n =" STRING() ", \n =" STRING()"  , \n =" STRING() )
   
    #pragma message ( "\n Show value: =>\n " SHOW_VALUE(NANA_USING_NANA_FILESYSTEM)   )


	#pragma message ( "\nUNICODE: \n NANA_UNICODE=" STRING(NANA_UNICODE) ", \n _UNICODE =" STRING(_UNICODE) " , \n UNICODE=" STRING(UNICODE)  )

	#pragma message ( "\n ---> Libraries: \n NANA_ENABLE_AUDIO=" STRING(NANA_ENABLE_AUDIO) ", \n NANA_ENABLE_PNG =" STRING(NANA_ENABLE_PNG) " , \n USE_LIBPNG_FROM_OS=" STRING(USE_LIBPNG_FROM_OS) " , \n NANA_LIBPNG=" STRING(NANA_LIBPNG) )

	#pragma message ( "\n NANA_ENABLE_JPEG=" STRING(NANA_ENABLE_JPEG) ", \n USE_LIBJPEG_FROM_OS =" STRING(USE_LIBJPEG_FROM_OS) " , \n NANA_LIBJPEG=" STRING(NANA_LIBJPEG) )


    #if defined(STOP_VERBOSE_PREPROCESSOR)
        #error ("\nCompilation stopped to avoid annoying messages")
    #endif

#endif // VERBOSE_PREPROCESSOR


#endif //NANA_VERBOSE_PREPROCESSOR_H
