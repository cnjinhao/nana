/*
 *	A File Iterator Implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/filesystem/file_iterator.cpp
 *	@description:
 *		file_iterator is a toolkit for applying each file and directory in a
 *	specified path.
 */

#include <nana/filesystem/file_iterator.hpp>

namespace nana
{
namespace filesystem
{
	//struct fileinfo
		fileinfo::fileinfo()
			:size(0), directory(false)
		{}
#if defined(NANA_WINDOWS)
		fileinfo::fileinfo(const WIN32_FIND_DATA& wfd)
			:   name(wfd.cFileName),    size(wfd.nFileSizeLow),
                directory((FILE_ATTRIBUTE_DIRECTORY & wfd.dwFileAttributes) == FILE_ATTRIBUTE_DIRECTORY)
		{
        }
#elif defined(NANA_LINUX)
		fileinfo::fileinfo(const nana::string& name, const struct stat& fst)
			:name(name), size(fst.st_size), directory(0 != S_ISDIR(fst.st_mode))
		{
        }
#endif
	//end struct fileinfo

}//end namespace filesystem
}//end namespace nana

