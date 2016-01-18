/*
 *	A FileSystem Utility Implementation
 *	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
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

	namespace detail
	{
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

	std::string path_user()
	{
#if defined(NANA_WINDOWS)
		wchar_t path[MAX_PATH];
		if(SUCCEEDED(SHGetFolderPath(0, CSIDL_PROFILE, 0, SHGFP_TYPE_CURRENT, path)))
			return to_utf8(path);
#elif defined(NANA_LINUX) || defined(NANA_MACOS)
		const char * s = ::getenv("HOME");
		if(s)
			return s;
#endif
		return std::string();
	}
}//end namespace filesystem
}//end namespace nana
