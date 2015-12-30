/*
 *	A ISO C++ FileSystem Implementation
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/filesystem/filesystem.cpp
 *	@description:
 *		provide some interface for file managment
 */

#include <nana/filesystem/filesystem.hpp>
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

namespace nana {	namespace experimental {
	namespace filesystem
	{
		//Because of No wide character version of POSIX
#if defined(NANA_LINUX) || defined(NANA_MACOS)
		const char* splstr = "/";
#else
		const wchar_t* splstr = L"/\\";
#endif

	//class file_status
		file_status::file_status(file_type ft, perms prms)
			: value_{ft}, perms_{prms}
		{}

		file_type file_status::type() const
		{
			return value_;
		}

		void file_status::type(file_type ft)
		{
			value_ = ft;
		}

		perms file_status::permissions() const
		{
			return perms_;
		}

		void file_status::permissions(perms prms)
		{
			perms_ = prms;
		}
	//end filestatus

		//class path
		path::path() {}

		int path::compare(const path& p) const
		{
			return pathstr_.compare(p.pathstr_);
		}

		bool path::empty() const
		{
#if defined(NANA_WINDOWS)
			return (::GetFileAttributes(pathstr_.c_str()) == INVALID_FILE_ATTRIBUTES);
#elif defined(NANA_LINUX) || defined(NANA_MACOS)
			struct stat sta;
			return (::stat(pathstr_.c_str(), &sta) == -1);
#endif
		}

		path path::extension() const
		{
#if defined(NANA_WINDOWS)
			auto pos = pathstr_.find_last_of(L"\\/.");
#else
			auto pos = pathstr_.find_last_of("\\/.");
#endif
			if ((pos == pathstr_.npos) || (pathstr_[pos] != '.'))
				return path();

				
			if (pos + 1 == pathstr_.size())
				return path();

			return path(pathstr_.substr(pos));
		}

		path path::parent_path() const
		{
			return{filesystem::parent_path(pathstr_)};
		}

		file_type path::what() const
		{
#if defined(NANA_WINDOWS)
			unsigned long attr = ::GetFileAttributes(pathstr_.c_str());
			if (INVALID_FILE_ATTRIBUTES == attr)
				return file_type::not_found; //??

			if (FILE_ATTRIBUTE_DIRECTORY & attr)
				return file_type::directory;

			return file_type::regular;
#elif defined(NANA_LINUX) || defined(NANA_MACOS)
			struct stat sta;
			if (-1 == ::stat(pathstr_.c_str(), &sta))
				return file_type::not_found; //??

			if ((S_IFDIR & sta.st_mode) == S_IFDIR)
				return file_type::directory;

			if ((S_IFREG & sta.st_mode) == S_IFREG)
				return file_type::regular;

			return file_type::none;
#endif
		}

		path path::filename() const
		{
			auto pos = pathstr_.find_last_of(splstr);
			if (pos != pathstr_.npos)
			{
				if (pos + 1 == pathstr_.size())
				{
					value_type tmp[2] = {preferred_separator, 0};

					if (pathstr_.npos != pathstr_.find_last_not_of(splstr, pos))
						tmp[0] = '.';

					return{ tmp };
				}
				return{ pathstr_.substr(pos + 1) };
			}

			return{ pathstr_ };
		}

		const path::value_type* path::c_str() const
		{
			return native().c_str();
		}

		const path::string_type& path::native() const
		{
			return pathstr_;
		}
			
		path::operator string_type() const
		{
			return native();
		}

		void path::_m_assign(const std::string& source_utf8)
		{
#if defined(NANA_WINDOWS)
			pathstr_ = utf8_cast(source_utf8);
#else
			pathstr_ = source_utf8;
#endif
		}

		void path::_m_assign(const std::wstring& source)
		{
#if defined(NANA_WINDOWS)
			pathstr_ = source;
#else
			pathstr_ = utf8_cast(source);
#endif			
		}
		//end class path

		bool operator==(const path& lhs, const path& rhs)
		{
			return (lhs.compare(rhs) == 0);
		}

		bool operator!=(const path& lhs, const path& rhs)
		{
			return (lhs.native() != rhs.native());
		}

		bool operator<(const path& lhs, const path& rhs)
		{
			return (lhs.compare(rhs) < 0);
		}

		bool operator>(const path& lhs, const path& rhs)
		{
			return (rhs.compare(lhs) < 0);
		}

		namespace detail
		{
				//rm_dir_recursive
				//@brief: remove a directory, if it is not empty, recursively remove it's subfiles and sub directories
				template<typename CharT>
				bool rm_dir_recursive(const CharT* dir)
				{
					std::vector<directory_iterator::value_type> files;
					std::basic_string<CharT> path = dir;
					path += '\\';

					std::copy(directory_iterator(dir), directory_iterator(), std::back_inserter(files));

					for (auto & f : files)
					{
						auto subpath = path + f.path().filename().native();
						if (f.attr.directory)
							rm_dir_recursive(subpath.c_str());
						else
							rmfile(subpath.c_str());
					}

					return rmdir(dir, true);
				}

#if defined(NANA_WINDOWS)
				void filetime_to_c_tm(FILETIME& ft, struct tm& t)
				{
					FILETIME local_file_time;
					if (::FileTimeToLocalFileTime(&ft, &local_file_time))
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

		bool not_found_error(int errval)
		{
#if defined(NANA_WINDOWS)
			switch (errval)
			{
			case ERROR_FILE_NOT_FOUND:
			case ERROR_PATH_NOT_FOUND:
			case ERROR_INVALID_NAME:
			case ERROR_INVALID_DRIVE:
			case ERROR_NOT_READY:
			case ERROR_INVALID_PARAMETER:
			case ERROR_BAD_PATHNAME:
			case ERROR_BAD_NETPATH:
				return true;
			}
			return false;
#elif defined(NANA_POSIX)
			return (errval == ENOENT || errval == ENOTDIR);
#else
			static_assert(false, "Only Windows and Unix are supported now (Mac OS is experimental)");
#endif
		}

		file_status status(const path& p)
		{
#if defined(NANA_WINDOWS)
			auto attr = ::GetFileAttributesW(p.c_str());
			if (INVALID_FILE_ATTRIBUTES == attr)
			{
				if (not_found_error(static_cast<int>(::GetLastError())))
					return file_status{file_type::not_found};
				return file_status{ file_type::unknown };
			}
			return file_status{(FILE_ATTRIBUTE_DIRECTORY & attr) ? file_type::directory : file_type::regular, perms::all};	
#elif defined(NANA_POSIX)
			struct stat path_stat;
			if(0 != ::stat(p.c_str(), &path_stat))
			{
				if(errno == ENOENT || errno == ENOTDIR)
					return file_status{file_type::not_found};
				
				return file_status{file_type::unknown};
			}

			auto prms = static_cast<perms>(path_stat.st_mode & static_cast<unsigned>(perms::mask));

			if(S_ISREG(path_stat.st_mode))
				return file_status{file_type::regular, prms};

			if(S_ISDIR(path_stat.st_mode))
				return file_status{file_type::directory, prms};

			if(S_ISLNK(path_stat.st_mode))
				return file_status{file_type::symlink, prms};

			if(S_ISBLK(path_stat.st_mode))
				return file_status{file_type::block, prms};

			if(S_ISCHR(path_stat.st_mode))
				return file_status{file_type::character, prms};

			if(S_ISFIFO(path_stat.st_mode))
				return file_status{file_type::fifo, prms};

			if(S_ISSOCK(path_stat.st_mode))
				return file_status{file_type::socket, prms};

			return file_status{file_type::unknown};
#endif
		}

			bool is_directory(const path& p)
			{
				return (status(p).type() == file_type::directory);
			}

			std::uintmax_t file_size(const path& p)
			{
#if defined(NANA_WINDOWS)
				//Some compilation environment may fail to link to GetFileSizeEx
				typedef BOOL(__stdcall *GetFileSizeEx_fptr_t)(HANDLE, PLARGE_INTEGER);
				GetFileSizeEx_fptr_t get_file_size_ex = reinterpret_cast<GetFileSizeEx_fptr_t>(::GetProcAddress(::GetModuleHandleA("Kernel32.DLL"), "GetFileSizeEx"));
				if (get_file_size_ex)
				{
					HANDLE handle = ::CreateFile(p.c_str(), GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
					if (INVALID_HANDLE_VALUE != handle)
					{
						LARGE_INTEGER li;
						if (!get_file_size_ex(handle, &li))
							li.QuadPart = 0;

						::CloseHandle(handle);
						return li.QuadPart;
					}
				}
				return 0;
#elif defined(NANA_POSIX)
				FILE * stream = ::fopen(p.c_str(), "rb");
				long long size = 0;
				if (stream)
				{
#	if defined(NANA_LINUX)
					fseeko64(stream, 0, SEEK_END);
					size = ftello64(stream);
#	elif defined(NANA_MACOS)
					fseeko(stream, 0, SEEK_END);
					size = ftello(stream);
#	endif
					::fclose(stream);
				}
				return size;
#endif
			}


			bool modified_file_time(const std::wstring& file, struct tm& t)
			{
#if defined(NANA_WINDOWS)
				WIN32_FILE_ATTRIBUTE_DATA attr;
				if (::GetFileAttributesEx(file.c_str(), GetFileExInfoStandard, &attr))
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
#elif defined(NANA_LINUX) || defined(NANA_MACOS)
				struct stat attr;
				if (0 == ::stat(static_cast<std::string>(nana::charset(file)).c_str(), &attr))
				{
					t = *(::localtime(&attr.st_ctime));
					return true;
				}
#endif
				return false;
			}


			bool create_directory(const path& p)
			{
#if defined(NANA_WINDOWS)
				return (FALSE != ::CreateDirectoryW(p.c_str(), 0));
#elif defined(NANA_POSIX)
				return (0 == ::mkdir(p.c_str(), static_cast<int>(perms::all)));
#endif
			}

			bool rmfile(const path& p)
			{
				if(p.empty())
					return false;
#if defined(NANA_WINDOWS)	
				if (FALSE == ::DeleteFileW(p.c_str()))
					return (ERROR_FILE_NOT_FOUND == ::GetLastError());

				return true;
#elif defined(NANA_POSIX)
				if (std::remove(p.c_str()))
					return (errno == ENOENT);
				return true;
#endif
			}


			bool rmdir(const path& p, bool fails_if_not_empty)
			{
				if(p.empty())
					return false;

#if defined(NANA_WINDOWS)
				if(FALSE != ::RemoveDirectoryW(p.c_str()))
					return true;

				if(!fails_if_not_empty && (ERROR_DIR_NOT_EMPTY == ::GetLastError()))
					return detail::rm_dir_recursive(p.c_str());

				return false;
#elif defined(NANA_POSIX)
				if(::rmdir(p.c_str()))
				{
					if (!fails_if_not_empty && (errno == EEXIST || errno == ENOTEMPTY))
						return detail::rm_dir_recursive(p.c_str());
				
					return false;
				}
				return true;	
#endif
			}

			path path_user()
			{
#if defined(NANA_WINDOWS)
				wchar_t pstr[MAX_PATH];
				if (SUCCEEDED(SHGetFolderPath(0, CSIDL_PROFILE, 0, SHGFP_TYPE_CURRENT, pstr)))
					return pstr;
#elif defined(NANA_LINUX) || defined(NANA_MACOS)
				const char * pstr = ::getenv("HOME");
				if (pstr)
					return pstr;
#endif
				return path();
			}

			path current_path()
			{
#if defined(NANA_WINDOWS)
				wchar_t buf[MAX_PATH];
				DWORD len = ::GetCurrentDirectoryW(MAX_PATH, buf);
				if (len)
				{
					if (len > MAX_PATH)
					{
						wchar_t * p = new wchar_t[len + 1];
						::GetCurrentDirectoryW(len + 1, p);
						std::wstring s = p;
						delete[] p;
						return s;
					}
					return buf;
				}
#elif defined(NANA_POSIX)
				char buf[260];
				auto pstr = ::getcwd(buf, 260);
				if (pstr)
					return pstr;
				
				int bytes = 260 + 260;
				while (ERANGE == errno)
				{
					std::unique_ptr<char[]> buf(new char[bytes]);
					auto pstr = ::getcwd(buf.get(), bytes);
					if (pstr)
						return path(pstr);

					bytes += 260;
				}
#endif
				return path();
			}

			void current_path(const path& p)
			{
#if defined(NANA_WINDOWS)
				::SetCurrentDirectoryW(p.c_str());
#elif defined(NANA_POSIX)
				::chdir(p.c_str());
#endif
			}
		}//end namespace filesystem
	} //end namespace experimental
}//end namespace nana
