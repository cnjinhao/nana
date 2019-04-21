/**
 *	Nana Configuration
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file  nana/config.hpp
 *
 *	@brief Provide switches to enable 3rd-party libraries for a certain feature.
 *
 *	External libraries:
 *	- NANA_LIBPNG, USE_LIBPNG_FROM_OS
 *	- NANA_LIBJPEG, USE_LIBJPEG_FROM_OS
 *  - NANA_ENABLE_AUDIO
 *
 *	messages:
 *	- VERBOSE_PREPROCESSOR, STOP_VERBOSE_PREPROCESSOR
 */

#ifndef NANA_CONFIG_HPP
#define NANA_CONFIG_HPP

#include "c++defines.hpp"

//The following basic configurations are ignored when NANA_IGNORE_CONF is defined.
//The NANA_IGNORE_CONF may be specified by CMake generated makefile.
#ifndef NANA_IGNORE_CONF

// Here defines some flags that tell Nana what features will be supported.

///////////////////////////
//  Support of std::thread
//  Boost.Thread is preferred.
//  NANA_ENABLE_MINGW_STD_THREADS_WITH_MEGANZ is only available on MinGW when STD_THREAD_NOT_SUPPORTED is defined.
//  if NANA_ENABLE_MINGW_STD_THREADS_WITH_MEGANZ is enabled, Boost.Thread will be replaced with meganz's mingw-std-threads.
//  https://github.com/meganz/mingw-std-threads
//#define NANA_ENABLE_MINGW_STD_THREADS_WITH_MEGANZ

//# The ISO C++ File System Technical Specification(ISO - TS, or STD) is optional.
//#              http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4100.pdf
//# This is not a workaround, but an user option.
//# The library maybe available in the std library in use or from Boost(almost compatible)
//#              http://www.boost.org/doc/libs/1_60_0/libs/filesystem/doc/index.htm
//# or you can choose to use the(partial, but functional) implementation provided by nana.
//# If you include the file <nana/filesystem/filesystem.hpp>
//# the selected option will be set by nana into std::experimental::filesystem
//# By default Nana will try to use the STD.If not available will try
//# to use boost if available.Nana own implementation will be use only none of them are available.
//# You can change that default if you change one of the following
//# (please don't define more than one of the _XX_FORCE options):
//
//#define BOOST_FILESYSTEM_AVAILABLE // "Is Boost filesystem available?"
//#define BOOST_FILESYSTEM_FORCE     // "Force use of Boost filesystem if available (over ISO and nana)
//#define STD_FILESYSTEM_FORCE       // "Use of STD filesystem?(a compilation error will occur if not available)" OFF)
//#define NANA_FILESYSTEM_FORCE      // "Force nana filesystem over ISO and boost?" OFF)
//
//	Make sure you (cmake?) provide the following where correspond (please find the correct values):
//	set CMAKE_BOOST_FILESYSTEM_INCLUDE_ROOT "Where to find <boost/filesystem.hpp>?" "../")
//	set CMAKE_BOOST_FILESYSTEM_LIB "Flag for the compiler to link: " "-lboost/fs")
//	include_directories CMAKE_BOOST_FILESYSTEM_INCLUDE_ROOT
//	APPEND flag LINKS CMAKE_BOOST_FILESYSTEM_LIB


///////////////////
//  Support of PCM playback
//
//#define NANA_ENABLE_AUDIO

///////////////////
//  Support for PNG
//	  Define the NANA_ENABLE_PNG to enable the support of PNG.
//
//#define NANA_ENABLE_PNG	//!
//#define USE_LIBPNG_FROM_OS // Un-Comment it to use libpng from operating system.
#if defined(NANA_ENABLE_PNG)
	#if !defined(USE_LIBPNG_FROM_OS)
		#define NANA_LIBPNG
	#endif
#endif

///////////////////
//  Support for JPEG
//	  Define the NANA_ENABLE_JPEG to enable the support of JPEG.
//
//#define NANA_ENABLE_JPEG	//!
//#define USE_LIBJPEG_FROM_OS // Un-Comment it to use libjpeg from operating system.
#if defined(NANA_ENABLE_JPEG)
	#if !defined(USE_LIBJPEG_FROM_OS)
		#define NANA_LIBJPEG
	#endif
#endif

///////////////////
//  Support for NANA_AUTOMATIC_GUI_TESTING
//	  Will cause the program to self-test the GUI. A default automatic GUI test 
//    will be added to all programs which don't have yet one defined. This default test will simple
//    wait 10 sec. (time to construct, show and execute the GUI) and then exit normally.
//
//#define NANA_AUTOMATIC_GUI_TESTING



#if !defined(VERBOSE_PREPROCESSOR)
//#define VERBOSE_PREPROCESSOR
#endif

#if !defined(STOP_VERBOSE_PREPROCESSOR)
//#define STOP_VERBOSE_PREPROCESSOR
#endif

#endif  // NANA_IGNORE_CONFIG
#endif  // NANA_CONFIG_HPP
