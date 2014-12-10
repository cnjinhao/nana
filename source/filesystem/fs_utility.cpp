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
#elif defined(NANA_LINUX)
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
#if defined(NANA_LINUX)
	typedef std::string string_t;
	const char* splstr = "/\\";
#else
	typedef nana::string string_t;
	const nana::char_t* splstr = STR("/\\");
#endif
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
#elif defined(NANA_LINUX)
			struct stat sta;
			return (::stat(text_.c_str(), &sta) == -1);
#endif
		}

		path path::root() const
		{
		#if defined(NANA_WINDOWS)
			return path(filesystem::root(text_));
		#elif defined(NANA_LINUX)
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
#elif defined(NANA_LINUX)
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

	namespace detail
	{
		//rm_dir_recursive
		//@brief: remove a directory, if it is not empty, recursively remove it's subfiles and sub directories
		bool rm_dir_recursive(nana::string&& dir)
		{
			std::vector<file_iterator::value_type> files;
			nana::string path = dir;
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

		bool mkdir_helper(const nana::string& dir, bool & if_exist)
		{
#if defined(NANA_WINDOWS)
			if(::CreateDirectory(dir.c_str(), 0))
			{
				if_exist = false;
				return true;
			}

			if_exist = (::GetLastError() == ERROR_ALREADY_EXISTS);
#elif defined(NANA_LINUX)
			if(0 == ::mkdir(static_cast<std::string>(nana::charset(dir)).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
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

	bool file_attrib(const nana::string& file, attribute& attr)
	{
#if defined(NANA_WINDOWS)
		WIN32_FILE_ATTRIBUTE_DATA fad;
		if(::GetFileAttributesEx(file.c_str(), GetFileExInfoStandard, &fad))
		{
			LARGE_INTEGER li;
			li.u.LowPart = fad.nFileSizeLow;
			li.u.HighPart = fad.nFileSizeHigh;
			attr.bytes = li.QuadPart;
			attr.is_directory = (0 != (fad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));
			detail::filetime_to_c_tm(fad.ftLastWriteTime, attr.modified);
			return true;
		}
#elif defined(NANA_LINUX)
		struct stat fst;
		if(0 == ::stat(static_cast<std::string>(nana::charset(file)).c_str(), &fst))
		{
			attr.bytes = fst.st_size;
			attr.is_directory = (0 != (040000 & fst.st_mode));
			attr.modified = *(::localtime(&fst.st_ctime));
			return true;
		}
#endif
		return false;
	}

	long long filesize(const nana::string& file)
	{
#if defined(NANA_WINDOWS)
		//Some compilation environment may fail to link to GetFileSizeEx
		typedef BOOL (__stdcall *GetFileSizeEx_fptr_t)(HANDLE, PLARGE_INTEGER);
		GetFileSizeEx_fptr_t get_file_size_ex = reinterpret_cast<GetFileSizeEx_fptr_t>(::GetProcAddress(::GetModuleHandleA("Kernel32.DLL"), "GetFileSizeEx"));
		if(get_file_size_ex)
		{
			HANDLE handle = ::CreateFile(file.c_str(), GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
			if(INVALID_HANDLE_VALUE != handle)
			{
				LARGE_INTEGER li;
				if(!get_file_size_ex(handle, &li))
					li.QuadPart = 0;

				::CloseHandle(handle);
				return li.QuadPart;
			}
		}
		return 0;
#elif defined(NANA_LINUX)
		FILE * stream = ::fopen(static_cast<std::string>(nana::charset(file)).c_str(), "rb");
		long long size = 0;
		if(stream)
		{
			fseeko64(stream, 0, SEEK_END);
			size = ftello64(stream);
			fclose(stream);
		}
		return size;
#endif
	}

	bool modified_file_time(const nana::string& file, struct tm& t)
	{
#if defined(NANA_WINDOWS)
		WIN32_FILE_ATTRIBUTE_DATA attr;
		if(::GetFileAttributesEx(file.c_str(), GetFileExInfoStandard, &attr))
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
#elif defined(NANA_LINUX)
		struct stat attr;
		if(0 == ::stat(static_cast<std::string>(nana::charset(file)).c_str(), &attr))
		{
			t = *(::localtime(&attr.st_ctime));
			return true;
		}
#endif
		return false;
	}

	bool mkdir(const nana::string& path, bool & if_exist)
	{
		if_exist = false;
		if(path.size() == 0) return false;

		nana::string root;
#if defined(NANA_WINDOWS)
		if(path.size() > 3 && path[1] == STR(':'))
			root = path.substr(0, 3);
#elif defined(NANA_LINUX)
		if(path[0] == STR('/'))
			root = '/';
#endif
		bool mkstat = false;
		std::size_t beg = root.size();

		while(true)
		{
			beg = path.find_first_not_of(STR("/\\"), beg);
			if(beg == path.npos)
				break;

			std::size_t pos = path.find_first_of(STR("/\\"), beg + 1);
			if(pos != path.npos)
			{
				root += path.substr(beg, pos - beg);

				mkstat = detail::mkdir_helper(root, if_exist);
				if(mkstat == false && if_exist == false)
					return false;

#if defined(NANA_WINDOWS)
				root += STR('\\');
#elif defined(NANA_LINUX)
				root += STR('/');
#endif
			}
			else
			{
				if(beg + 1 < path.size())
				{
					root += path.substr(beg);
					mkstat = detail::mkdir_helper(root, if_exist);
				}
				break;
			}
			beg = pos + 1;
		}
		return mkstat;
	}

	bool rmfile(const nana::char_t* file)
	{
#if defined(NANA_WINDOWS)
		bool ret = false;
		if(file)
		{
			ret = (::DeleteFile(file) == TRUE);
			if(!ret)
				ret = (ERROR_FILE_NOT_FOUND == ::GetLastError());
		}

		return ret;
#elif defined(NANA_LINUX)
		if(std::remove(static_cast<std::string>(nana::charset(file)).c_str()))
			return (errno == ENOENT);
		return true;
#endif
	}

	bool rmdir(const nana::char_t* dir, bool fails_if_not_empty)
	{
		bool ret = false;
		if(dir)
		{
#if defined(NANA_WINDOWS)
			ret = (::RemoveDirectory(dir) == TRUE);
			if(!fails_if_not_empty && (::GetLastError() == ERROR_DIR_NOT_EMPTY))
				ret = detail::rm_dir_recursive(dir);
#elif defined(NANA_LINUX)
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
#elif defined(NANA_LINUX)
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
#elif defined(NANA_LINUX)
		const char * s = ::getenv("PWD");
		if(s)
			return nana::charset(std::string(s, std::strlen(s)), nana::unicode::utf8);
#endif
		return nana::string();
	}
}//end namespace filesystem
}//end namespace nana
