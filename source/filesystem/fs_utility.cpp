/*
 *	A FileSystem Utility Implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/filesystem/fs_utility.cpp
 *	@description:
 *		provide some interface for file managment
 */

#include <nana/filesystem/fs_utility.hpp>
#include <nana/filesystem/file_iterator.hpp>
#include <nana/deploy.hpp>
#include <vector>
#if defined(NANA_WINDOWS)
    #include <windows.h>

    #if defined(NANA_MINGW)
        #ifndef _WIN32_IE
            #define _WIN32_IE 0x0500
        #endif
    #endif

	#include <shlobj.h>
	#include <nana/datetime.hpp>
#elif defined(NANA_LINUX) || defined(NANA_MACOS)
	#include <nana/charset.hpp>
	#include <sys/stat.h>
	#include <sys/types.h>
	#include <dirent.h>
	#include <cstdio>
	#include <cstring>
	#include <errno.h>
	#include <unistd.h>
	#include <stdlib.h>
#endif

namespace nana
{
namespace filesystem
{
//Because of No wide character version of POSIX
#if defined(NANA_LINUX) || defined(NANA_MACOS)
	typedef std::string string_t;
	const char* splstr = "/\\";
#else
	typedef std::wstring string_t;
	const wchar_t* splstr = L"/\\";
#endif

	/*
	//class path
		path::path(){}

		path::path(const nana::string& text)
#if defined(NANA_WINDOWS)
			:text_(text)
		{
#else
			:text_(nana::charset(text))
		{
#endif
			auto pos = text_.find_last_of(splstr);
			for(; (pos != string_t::npos) && (pos + 1 == text_.size()); pos = text_.find_last_of(splstr))
				text_.erase(pos);
		}

		bool path::empty() const
		{
#if defined(NANA_WINDOWS)
			return (::GetFileAttributes(text_.c_str()) == INVALID_FILE_ATTRIBUTES);
#elif defined(NANA_LINUX) || defined(NANA_MACOS)
			struct stat sta;
			return (::stat(text_.c_str(), &sta) == -1);
#endif
		}

		path path::root() const
		{
		#if defined(NANA_WINDOWS)
			return path(filesystem::root(text_));
		#elif defined(NANA_LINUX) || defined(NANA_MACOS)
			return path(filesystem::root(nana::charset(text_)));
		#endif
		}

		int path::what() const
		{
#if defined(NANA_WINDOWS)
			unsigned long attr = ::GetFileAttributes(text_.c_str());
			if(INVALID_FILE_ATTRIBUTES == attr)
				return type::not_exist;

			if(FILE_ATTRIBUTE_DIRECTORY & attr)
				return type::directory;

			return type::file;
#elif defined(NANA_LINUX) || defined(NANA_MACOS)
			struct stat sta;
			if(-1 == ::stat(text_.c_str(), &sta))
				return type::not_exist;

			if((S_IFDIR & sta.st_mode) == S_IFDIR)
				return type::directory;

			if((S_IFREG & sta.st_mode) == S_IFREG)
				return type::file;

			return type::not_exist;
#endif
		}

		nana::string path::name() const
		{
			string_t::size_type pos = text_.find_last_of(splstr);
#if defined(NANA_WINDOWS)
			return text_.substr(pos + 1);
#else
			return nana::charset(text_.substr(pos + 1));
#endif
		}
	//end class path
	*/

	namespace detail
	{
		//rm_dir_recursive
		//@brief: remove a directory, if it is not empty, recursively remove it's subfiles and sub directories
		bool rm_dir_recursive(std::string&& dir)
		{
			std::vector<file_iterator::value_type> files;
			auto path = dir;
			path += '\\';

			std::copy(file_iterator(dir), file_iterator(), std::back_inserter(files));

			for(auto & f : files)
			{
				if(f.directory)
					rm_dir_recursive(path + f.name);
				else
					rmfile((path + f.name).c_str());
			}

			return rmdir(dir.c_str(), true);
		}

		bool mkdir_helper(const std::string& dir, bool & if_exist)
		{
#if defined(NANA_WINDOWS)
			if(::CreateDirectory(utf8_cast(dir).c_str(), 0))
			{
				if_exist = false;
				return true;
			}

			if_exist = (::GetLastError() == ERROR_ALREADY_EXISTS);
#elif defined(NANA_LINUX) || defined(NANA_MACOS)
			if(0 == ::mkdir(dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
			{
				if_exist = false;
				return true;
			}

			if_exist = (errno == EEXIST);
#endif
			return false;
		}

#if defined(NANA_WINDOWS)
		void filetime_to_c_tm(FILETIME& ft, struct tm& t)
		{
			FILETIME local_file_time;
			if(::FileTimeToLocalFileTime(&ft, &local_file_time))
			{
				SYSTEMTIME st;
				::FileTimeToSystemTime(&local_file_time, &st);
				t.tm_year = st.wYear - 1900;
				t.tm_mon = st.wMonth - 1;
				t.tm_mday = st.wDay;
				t.tm_wday = st.wDayOfWeek - 1;
				t.tm_yday = nana::date::day_in_year(st.wYear, st.wMonth, st.wDay);

				t.tm_hour = st.wHour;
				t.tm_min = st.wMinute;
				t.tm_sec = st.wSecond;
			}
		}
#endif
	}//end namespace detail


	bool modified_file_time(const ::std::string& file, struct tm& t)
	{
#if defined(NANA_WINDOWS)
		WIN32_FILE_ATTRIBUTE_DATA attr;
		if(::GetFileAttributesExW(to_nstring(file).c_str(), GetFileExInfoStandard, &attr))
		{
			FILETIME local_file_time;
			if(::FileTimeToLocalFileTime(&attr.ftLastWriteTime, &local_file_time))
			{
				SYSTEMTIME st;
				::FileTimeToSystemTime(&local_file_time, &st);
				t.tm_year = st.wYear - 1900;
				t.tm_mon = st.wMonth - 1;
				t.tm_mday = st.wDay;
				t.tm_wday = st.wDayOfWeek - 1;
				t.tm_yday = nana::date::day_in_year(st.wYear, st.wMonth, st.wDay);

				t.tm_hour = st.wHour;
				t.tm_min = st.wMinute;
				t.tm_sec = st.wSecond;
				return true;
			}
		}
#elif defined(NANA_LINUX) || defined(NANA_MACOS)
		struct stat attr;
		if(0 == ::stat(file.c_str(), &attr))
		{
			t = *(::localtime(&attr.st_ctime));
			return true;
		}
#endif
		return false;
	}

	bool rmfile(const char* file)
	{
#if defined(NANA_WINDOWS)
		bool ret = false;
		if(file)
		{
			ret = (::DeleteFile(utf8_cast(file).c_str()) == TRUE);
			if(!ret)
				ret = (ERROR_FILE_NOT_FOUND == ::GetLastError());
		}

		return ret;
#elif defined(NANA_LINUX) || defined(NANA_MACOS)
		if(std::remove(file))
			return (errno == ENOENT);
		return true;
#endif
	}

	bool rmdir(const char* dir, bool fails_if_not_empty)
	{
		bool ret = false;
		if(dir)
		{
#if defined(NANA_WINDOWS)
			ret = (::RemoveDirectory(utf8_cast(dir).c_str()) == TRUE);
			if(!fails_if_not_empty && (::GetLastError() == ERROR_DIR_NOT_EMPTY))
				ret = detail::rm_dir_recursive(dir);
#elif defined(NANA_LINUX) || defined(NANA_MACOS)
			std::string mbstr = nana::charset(dir);
			if(::rmdir(mbstr.c_str()))
			{
				if(!fails_if_not_empty && (errno == EEXIST || errno == ENOTEMPTY))
					ret = detail::rm_dir_recursive(dir);
			}
			else
				ret = true;
#endif
		}
		return ret;
	}

	nana::string root(const nana::string& path)
	{
		std::size_t index = path.size();

		if(index)
		{
			const nana::char_t * str = path.c_str();

			for(--index; index > 0; --index)
			{
				nana::char_t c = str[index];
				if(c != '\\' && c != '/')
					break;
			}

			for(--index; index > 0; --index)
			{
				nana::char_t c = str[index];
				if(c == '\\' || c == '/')
					break;
			}
		}

		return index?path.substr(0, index + 1):nana::string();
	}

	nana::string path_user()
	{
#if defined(NANA_WINDOWS)
		nana::char_t path[MAX_PATH];
		if(SUCCEEDED(SHGetFolderPath(0, CSIDL_PROFILE, 0, SHGFP_TYPE_CURRENT, path)))
			return path;
#elif defined(NANA_LINUX) || defined(NANA_MACOS)
		const char * s = ::getenv("HOME");
		if(s)
			return nana::charset(std::string(s, std::strlen(s)), nana::unicode::utf8);
#endif
		return nana::string();
	}

	nana::string path_current()
	{
#if defined(NANA_WINDOWS)
		nana::char_t buf[MAX_PATH];
		DWORD len = ::GetCurrentDirectory(MAX_PATH, buf);
		if(len)
		{
			if(len > MAX_PATH)
			{
				nana::char_t * p = new nana::char_t[len + 1];
				::GetCurrentDirectory(len + 1, p);
				nana::string s = p;
				delete [] p;
				return s;
			}
			return buf;
		}
#elif defined(NANA_LINUX) || defined(NANA_MACOS)
		const char * s = ::getenv("PWD");
		if(s)
			return nana::charset(std::string(s, std::strlen(s)), nana::unicode::utf8);
#endif
		return nana::string();
	}
}//end namespace filesystem
}//end namespace nana
