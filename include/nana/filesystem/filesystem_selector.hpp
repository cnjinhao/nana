/**
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file nana\filesystem\filesystem_selector.hpp
*   @autor by Ariel Vina-Rodriguez:
*	@brief A "ISO C++" filesystem Implementation selector
*    The ISO C++ File System Technical Specification is optional.
*               http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4100.pdf
*    This is not a workaround, but an user option.
*    The library maybe available in the std library in use or from Boost (almost compatible)
*               http://www.boost.org/doc/libs/1_60_0/libs/filesystem/doc/index.htm
*    or you can choose to use the (partial, but functional) implementation provided by nana.
*    If you include the file <nana/filesystem/filesystem_selector.hpp>
*    The selected option will be set by nana into std::experimental::filesystem
*    By default Nana will use the ISO TS if available, or nana if not.
*    Boost will be use only if explicitily changed 
*    nana Now mimic std::experimental::filesystem::v1   (boost v3)
*    
*/

#ifndef NANA_FILESYSTEM_SELECTOR
#define NANA_FILESYSTEM_SELECTOR

#include <nana/config.hpp>

#if defined(NANA_BOOST_FILESYSTEM_AVAILABLE) && ( defined(NANA_BOOST_FILESYSTEM_FORCE) || (defined(STD_FILESYSTEM_NOT_SUPPORTED) && defined(NANA_BOOST_FILESYSTEM_PREFERRED) ) )

#   include <boost/filesystem.hpp>
	// add boost::filesystem into std::experimental::filesystem
	namespace std {
		namespace experimental {
#       ifdef CXX_NO_INLINE_NAMESPACE
			using namespace boost::experimental;
#       else
			using namespace boost::experimental::v3;
#       endif

		}

#elif defined(STD_FILESYSTEM_NOT_SUPPORTED)

#   include <nana/filesystem/filesystem.hpp>
	namespace std {
		namespace experimental {
#       ifdef CXX_NO_INLINE_NAMESPACE
			using namespace nana::experimental;
#       else
			using namespace nana::experimental::v1;
#       endif
		}
}

#else
#    include <filesystem>
#endif

#ifndef __cpp_lib_experimental_filesystem
#   define __cpp_lib_experimental_filesystem 1
#endif

#if defined(NANA_WINDOWS)
constexpr auto def_root = "C:";
constexpr auto def_rootstr = "C:\\";
constexpr auto def_rootname = "Local Drive(C:)";
#elif defined(NANA_LINUX)
constexpr auto def_root = "/";
constexpr auto def_rootstr = "/";
constexpr auto def_rootname = "Root/";
#endif

#endif // NANA_FILESYSTEM_SELECTOR             // "Force use of Boost filesystem if available (over ISO)?
 




