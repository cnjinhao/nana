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
 *	Set VERBOSE_PREPROCESSOR to 1 to show the messages or to 0 for a normal build.
 *	Normally set to 0. Set to 1 only in case you want to debug the build system because it is extremely repetitive and slow.
 *
 *  @authors  Ariel Vina-Rodriguez (qPCR4vir)
 *
 */
// Created by ariel.rodriguez on 08.12.2015.
//

#ifndef NANA_VERBOSE_PREPROCESSOR_H
#define NANA_VERBOSE_PREPROCESSOR_H

#if defined(VERBOSE_PREPROCESSOR)

    #define STRING2(x) #x
	#define STRING(x) STRING2(x)
	#pragma message ( "\nVerbose preprocessor =" STRING(VERBOSE_PREPROCESSOR)" , \n STOP_VERBOSE_PREPROCESSOR=" STRING(STOP_VERBOSE_PREPROCESSOR)  )

	#pragma message ( "\nWindows: \n _WIN32=" STRING(_WIN32) ", \n __WIN32__ =" STRING(__WIN32__) " , \n WIN32=" STRING(WIN32)" , \n NANA_WINDOWS=" STRING(NANA_WINDOWS)   )

	#pragma message ( "\nUNICODE: \n NANA_UNICODE=" STRING(NANA_UNICODE) ", \n _UNICODE =" STRING(_UNICODE) " , \n UNICODE=" STRING(UNICODE)  )

	#pragma message ( "\nMinGW: \n __MINGW32__=" STRING(__MINGW32__) ", \n __MINGW64__=" STRING(__MINGW64__) " , \n MINGW=" STRING(MINGW) )

	#pragma message ( "\nGNU: \n __GNUC__=" STRING(__GNUC__) ", \n __GNUC_MINOR__=" STRING(__GNUC_MINOR__) " , \n __GNUC_PATCHLEVEL__=" STRING(__GNUC_PATCHLEVEL__) )

    #pragma message ( "\nSTD: \nSTD_CODECVT_NOT_SUPPORTED=" STRING(STD_CODECVT_NOT_SUPPORTED) " , \nSTD_THREAD_NOT_SUPPORTED=" STRING(STD_THREAD_NOT_SUPPORTED) )

	#pragma message ( "\nSTD: \nUSE_github_com_meganz_mingw_std_threads=" STRING(USE_github_com_meganz_mingw_std_threads) " , \nSTD_THREAD_NOT_SUPPORTED=" STRING(STD_THREAD_NOT_SUPPORTED) )

	#pragma message ( "\nClang compiler: \n__clang__=" STRING(__clang__)  ", \n__GLIBCPP__=" STRING(__GLIBCPP__) " , \n__GLIBCXX__=" STRING(__GLIBCXX__) )

	#pragma message ( "\nMSC: \n_MSC_VER=" STRING(_MSC_VER) ", \n_MSC_FULL_VER=" STRING(_MSC_FULL_VER ) )

    #if defined(STOP_VERBOSE_PREPROCESSOR)
        #error ("\nCompilation stopped to avoid annoying messages")
    #endif

#endif // VERBOSE_PREPROCESSOR



#endif //NANA_VERBOSE_PREPROCESSOR_H
