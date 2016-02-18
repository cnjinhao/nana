/**
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file nana\filesystem\filesystem_ext.hpp
*   @autor by Ariel Vina-Rodriguez:
*	@brief Some convenient extensions to the filesystem library.
*
*/

#ifndef NANA_FILESYSTEM_EXT_HPP
#define NANA_FILESYSTEM_EXT_HPP

#if defined(NANA_WINDOWS)
    constexpr auto def_root = "C:";
    constexpr auto def_rootstr = "C:\\";
    constexpr auto def_rootname = "Local Drive(C:)";
#elif defined(NANA_LINUX)
    constexpr auto def_root = "/";
    constexpr auto def_rootstr = "/";
    constexpr auto def_rootname = "Root/";
#endif

// nana::experimental::filesystem::path_user());   //  REPLACE !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! to filesystem_ext.hhp

#endif //NANA_FILESYSTEM_EXT_HPP
