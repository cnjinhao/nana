/**
 *	A ISO C++ FileSystem Implementation
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file  nana/filesystem/filesystem.cpp
 *	@description
 *		provide some interface for file management
 */

#include <nana/config.hpp>
#include <nana/filesystem/filesystem_ext.hpp>
#include <vector>
#include <sstream>
#include <string>
#include <iomanip>

#if defined(NANA_WINDOWS)
    #include <windows.h>

    #if defined(NANA_MINGW)
        #ifndef _WIN32_IE
            #define _WIN32_IE 0x0500
        #endif
    #endif

	#include <shlobj.h>
	#include <nana/datetime.hpp>
#elif defined(NANA_POSIX)
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

namespace fs = std::filesystem;

namespace nana
{
	namespace filesystem_ext
	{

		fs::path path_user()
		{
#if defined(NANA_WINDOWS)
			wchar_t pstr[MAX_PATH];
			if (SUCCEEDED(SHGetFolderPath(0, CSIDL_PROFILE, 0, SHGFP_TYPE_CURRENT, pstr)))
				return pstr;
#elif defined(NANA_POSIX)
			const char * pstr = ::getenv("HOME");
			if (pstr)
				return pstr;
#endif
			return fs::path();
		}

		std::string pretty_file_size(const fs::path& path)
		{
			try {
				auto bytes = fs::file_size(path);
				const char * ustr[] = { " KB", " MB", " GB", " TB" };
				std::stringstream ss;
				if (bytes < 1024)
					ss << bytes << " Bytes";
				else
				{
					double cap = bytes / 1024.0;
					std::size_t uid = 0;
					while ((cap >= 1024.0) && (uid < sizeof(ustr) / sizeof(char *)))
					{
						cap /= 1024.0;
						++uid;
					}
					ss << cap;
					auto s = ss.str();
					auto pos = s.find('.');
					if (pos != s.npos)
					{
						if (pos + 2 < s.size())
							s.erase(pos + 2);
					}
					return s + ustr[uid];
				}

				return ss.str();
			}
			catch (...) {}
			return{};
		}

		std::string pretty_file_date(const fs::path& path) // todo: move to .cpp
		{
			struct tm t;
			if (modified_file_time(path, t))
			{
				std::stringstream tm;
				tm << std::put_time(&t, "%Y-%m-%d, %H:%M:%S");
				return tm.str();
			}
			return {};
		}

		bool modified_file_time(const fs::path& p, struct tm& t)
		{
#if defined(NANA_WINDOWS)
			WIN32_FILE_ATTRIBUTE_DATA attr;
			if (::GetFileAttributesEx(p.c_str(), GetFileExInfoStandard, &attr))
			{
				FILETIME local_file_time;
				if (::FileTimeToLocalFileTime(&attr.ftLastWriteTime, &local_file_time))
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
#elif defined(NANA_POSIX)
			struct stat attr;
			if (0 == ::stat(p.c_str(), &attr))
			{
				t = *(::localtime(&attr.st_ctime));
				return true;
			}
#endif
			return false;
		}
	}
}
