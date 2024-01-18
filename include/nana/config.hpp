/**
 *	Nana Configuration
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2020 Jinhao(cnjinhao@hotmail.com)
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
//  Support for GIF
//	  Define the NANA_ENABLE_GIF to enable the support of GIF.
//
//#define NANA_ENABLE_GIF	//!
//#define USE_LIBGIF_FROM_OS // Un-Comment it to use libgif from operating system.
#if defined(NANA_ENABLE_GIF)
	#if !defined(USE_LIBGIF_FROM_OS)
		#define NANA_LIBGIF
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
