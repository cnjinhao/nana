/*
 *	A ISO C++ FileSystem Implementation
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/filesystem/filesystem.cpp
 *	@description:
 *		provide some interface for file management
 */

#include <nana/filesystem/filesystem_ext.hpp>
#include <vector>
#include <sstream>

#include <nana/config.hpp>
#ifdef _nana_std_put_time
	#include <nana/stdc++.hpp>
#else
	#include <iomanip>
#endif

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

#if NANA_USING_NANA_FILESYSTEM

namespace nana_fs = nana::experimental::filesystem;

namespace nana {	namespace experimental {	namespace filesystem
	{
#ifndef CXX_NO_INLINE_NAMESPACE
			inline namespace v1 {
#endif

		//class filesystem_error
			filesystem_error::filesystem_error(const std::string& msg, std::error_code err)
				: std::system_error(err, msg)
			{}

			filesystem_error::filesystem_error(const std::string& msg, const path& path1, std::error_code err)
				:	std::system_error(err, msg),
					path1_(path1)
			{}

			filesystem_error::filesystem_error(const std::string& msg, const path& path1, const path& path2, std::error_code err)
				:	std::system_error(err, msg),
					path1_(path1),
					path2_(path2)
			{}

			const path& filesystem_error::path1() const noexcept
			{
				return path1_;
			}

			const path& filesystem_error::path2() const noexcept
			{
				return path2_;
			}
		//end class filesystem_error


		//Because of No wide character version of POSIX
#if defined(NANA_POSIX)
		const char* separators = "/";
		const char	separator = '/';
		const char* punt = ".";
#else
		const wchar_t* separators = L"/\\";
		const char	separator = '\\';
		const wchar_t* punt = L".";
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
		int path::compare(const path& p) const
		{
			return pathstr_.compare(p.pathstr_);
		}

		/// true if the path is empty, false otherwise. ??
		bool path::empty() const noexcept
		{
			return pathstr_.empty();
		}

		bool path::is_absolute() const
		{
#ifdef NANA_WINDOWS
			return has_root_name() && has_root_directory();
#else
			return has_root_directory();
#endif
		}

		bool path::is_relative() const
		{
			return !is_absolute();
		}

		path path::extension() const
		{
			// todo: make more global
#if defined(NANA_WINDOWS)
            auto SLorP=L"\\/.";
			auto P=L'.';
#else
			auto SLorP="\\/.";
			auto P='.';
#endif
			auto pos = pathstr_.find_last_of(SLorP);

			if (    ( pos == pathstr_.npos)
				 || ( pathstr_[pos] != P )
				 || ( pos + 1 == pathstr_.size()  ))
			   return path();

			return path(pathstr_.substr(pos));
		}

		bool is_directory_separator(path::value_type ch)
		{
			return (ch == '/')
#ifdef NANA_WINDOWS
				|| (ch == path::preferred_separator)
#endif
				;
		}



		path path::root_name() const
		{
			auto pos = pathstr_.find_first_not_of(separators);
			if (pathstr_.npos != pos)
			{
				pos = pathstr_.find_first_of(separators, pos + 1);
				if (pathstr_.npos != pos)
				{
					if ((is_directory_separator(pathstr_[0]) && is_directory_separator(pathstr_[1]))
#ifdef NANA_WINDOWS
						|| (pathstr_[pos - 1] == ':')
#endif
						)
						return pathstr_.substr(0, pos);
				}
			}

			return path{};
		}

		std::size_t root_directory_pos(const path::string_type& str)
		{
#ifdef NANA_WINDOWS
			// case "c:/"
			if (str.size() > 2
				&& str[1] == ':'
				&& is_directory_separator(str[2])) return 2;
#endif

			// case "//"
			if (str.size() == 2
				&& is_directory_separator(str[0])
				&& is_directory_separator(str[1])) return path::string_type::npos;

#ifdef NANA_WINDOWS
			// case "\\?\"
			if (str.size() > 4
				&& is_directory_separator(str[0])
				&& is_directory_separator(str[1])
				&& str[2] == '?'
				&& is_directory_separator(str[3]))
			{
				auto pos = str.find_first_of(separators, 4);
				return pos < str.size() ? pos : str.npos;
			}
#endif

			// case "//net {/}"
			if (str.size() > 3
				&& is_directory_separator(str[0])
				&& is_directory_separator(str[1])
				&& !is_directory_separator(str[2]))
			{
				auto pos = str.find_first_of(separators, 2);
				return pos < str.size() ? pos : str.npos;
			}

			// case "/"
			if (str.size() > 0 && is_directory_separator(str[0])) return 0;

			return str.npos;
		}

		path path::root_directory() const
		{
			auto pos = root_directory_pos(pathstr_);

			return pos == string_type::npos
				? path()
				: path(pathstr_.substr(pos, 1));
		}

		path path::root_path() const
		{
			return root_name().pathstr_ + root_directory().pathstr_;
		}

		path path::relative_path() const
		{
			if (!empty())
			{
				auto pos = root_directory_pos(pathstr_);

				pos = pathstr_.find_first_not_of(separators, pos);
				if (pathstr_.npos != pos)
					return path{ pathstr_.substr(pos) };
			}
			return{};
		}

		path path::parent_path() const
		{
			return{nana_fs::parent_path(pathstr_)};
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
#elif defined(NANA_POSIX)
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
			auto pos = pathstr_.find_last_of(separators);
			if (pos != pathstr_.npos)
			{
				if (pos + 1 == pathstr_.size())
				{
					value_type tmp[2] = {preferred_separator, 0};

					if (pathstr_.npos != pathstr_.find_last_not_of(separators, pos))
						tmp[0] = '.';

					return{ tmp };
				}
				return{ pathstr_.substr(pos + 1) };
			}

			return{ pathstr_ };
		}

        path path::stem() const
        {
            auto pos = pathstr_.find_last_of(separators);
            auto ext = pathstr_.find_last_of(punt);

            if (pos == pathstr_.npos)
                pos = 0;
            else
                pos++;

            if (ext == pathstr_.npos || ext < pos)
                return path(pathstr_.substr(pos));
            else
                return path(pathstr_.substr(pos, ext-pos));
        }

		void path::clear() noexcept
		{
			pathstr_.clear();
		}

		path& path::make_preferred()
		{
#ifdef NANA_WINDOWS
			std::replace(pathstr_.begin(), pathstr_.end(), L'/', L'\\');
#else
			std::replace(pathstr_.begin(), pathstr_.end(), '\\', '/');
#endif
			return *this;
		}

		path& path::remove_filename()
		{
#ifdef NANA_WINDOWS
			wchar_t seps[] = L"/\\";
#else
			char seps[] = "/\\";
#endif
			auto pos = pathstr_.find_last_of(seps);
			if (pos != pathstr_.npos)
			{
				pos = pathstr_.find_last_not_of(seps, pos);
				if (pos != pathstr_.npos)
				{
#ifdef NANA_WINDOWS
					if (pathstr_[pos] == L':')
					{
						pathstr_.erase(pos + 1);
						pathstr_ += L'\\';

						return *this;
					}
#endif
					++pos;
				}
				else
					pos = 0;
			}
			else
				pos = 0;

			pathstr_.erase(pos);

			return *this;
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

		std::string path::string() const
		{
			return to_osmbstr(to_utf8(pathstr_));
		}

		std::wstring path::wstring() const
		{
			return to_wstring(pathstr_);
		}

		std::string path::u8string() const
		{
			return to_utf8(pathstr_);
		}
		std::string path::generic_string() const
		{
			auto str = string();
			std::replace(str.begin(), str.end(), '\\', '/');
			return str;
		}
		std::wstring path::generic_wstring() const
		{
			auto str = wstring();
			std::replace(str.begin(), str.end(), L'\\', L'/');
			return str;
		}
		std::string path::generic_u8string() const // uppss ...
		{
			auto str = pathstr_;
			std::replace(str.begin(), str.end(), '\\', '/');  // uppss ...  revise this !!!!!
			return to_utf8(str);
		}

		path path::lexically_normal() const
		{
			if (pathstr_.empty())
				return *this;

			std::vector<path> elements;
			path temp{ pathstr_ };
			while (!temp.empty())
			{
				elements.emplace_back(temp.filename());
				temp.remove_filename();
			}

			auto start = elements.begin();
			auto last = elements.end();
			auto stop = last--;
			for (auto itr(start); itr != stop; ++itr)
			{
				// ignore "." except at start and last
				if (itr->native().size() == 1
					&& (itr->native())[0] == '.'
					&& itr != start
					&& itr != last) continue;

				// ignore a name and following ".."
				if (!temp.empty()
					&& itr->native().size() == 2
					&& (itr->native())[0] == '.'
					&& (itr->native())[1] == '.') // dot dot
				{
					string_type lf(temp.filename().native());
					if (lf.size() > 0
						&& (lf.size() != 1
						|| (lf[0] != '.'
						&& (lf[0] != '/' && lf[0] != '\\')))
						&& (lf.size() != 2
						|| (lf[0] != '.'
						&& lf[1] != '.'
#             ifdef NANA_WINDOWS
						&& lf[1] != ':'
#             endif
						)
						)
						)
					{
						temp.remove_filename();
						auto next = itr;
						if (temp.empty() && ++next != stop
							&& next == last && last->string() == ".")
						{
							temp /= ".";
						}
						continue;
					}
				}

				temp /= *itr;
			};

			if (temp.empty())
				temp = ".";
			return temp;
		}

		path & path::operator/=(const path& p)
		{
			if (p.empty())
				return *this;

			if (this == &p)
			{
				auto other = p.pathstr_;
				if (!is_directory_separator(other.front()))
				{
					if (!pathstr_.empty() && !is_directory_separator(pathstr_.back()))
						pathstr_.append(1, path::preferred_separator);
				}

				pathstr_ += other;
			}
			else
			{
				auto & other = p.pathstr_;
				if (!is_directory_separator(other.front()))
				{
					if (!pathstr_.empty() && !is_directory_separator(pathstr_.back()))
						pathstr_.append(1, path::preferred_separator);
				}

				pathstr_ += p.pathstr_;
			}
			return *this;
		}

		void path::_m_assign(const std::string& source_utf8)
		{
			pathstr_ = to_nstring(source_utf8);
		}

		void path::_m_assign(const std::wstring& source)
		{
			pathstr_ = to_nstring(source);
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

		path operator/(const path& lhs, const path& rhs)
		{
			auto path = lhs;
			return (path /= rhs);
		}

		//class directory_entry
			directory_entry::directory_entry(const nana_fs::path& p)
				:path_{ p }
			{}

			//modifiers
			void directory_entry::assign(const  nana_fs::path& p)
			{
				path_ = p;
			}

			void directory_entry::replace_filename(const  nana_fs::path& p)
			{
				path_ = path_.parent_path() / p;
			}

			//observers
			file_status directory_entry::status() const
			{
				return nana_fs::status(path_);
			}

			//directory_entry::operator const nana_fs::path&() const
			//{
			//	return path_;
			//}

			const nana_fs::path& directory_entry::path() const
			{
				return path_;
			}
		//end class directory_entry


		//class directory_iterator
			struct inner_handle_deleter
			{
				void operator()(void** handle)
				{
#if defined(NANA_WINDOWS)
                    ::FindClose(*handle);
#elif defined(NANA_POSIX)
                    ::closedir(*reinterpret_cast<DIR**>(handle));
#endif
				}
			};

				directory_iterator::directory_iterator() noexcept
					:	end_(true),
						handle_(nullptr)
				{}

				directory_iterator::directory_iterator(const path& file_path)
				{
					_m_prepare(file_path);
				}

				directory_iterator::directory_iterator(const path& p, directory_options opt):
					option_(opt)
				{
					_m_prepare(p);
				}

				const directory_iterator::value_type& directory_iterator::operator*() const { return value_; }

				const directory_iterator::value_type*
					directory_iterator::operator->() const { return &(operator*()); }

				directory_iterator& directory_iterator::operator++()
				{
					_m_read(); return *this;
				}

				directory_iterator directory_iterator::operator++(int)
				{
					directory_iterator tmp = *this;
					_m_read();
					return tmp;
				}

				bool directory_iterator::equal(const directory_iterator& x) const
				{
					if (end_ && (end_ == x.end_)) return true;
					return (value_.path().filename() == x.value_.path().filename());
				}


				void directory_iterator::_m_prepare(const path& file_path)
				{
					path_ = file_path.native();
#if defined(NANA_WINDOWS)
					if (!path_.empty() && (path_.back() != L'/' && path_.back() != L'\\'))
						path_ += L'\\';

					auto pat = path_;
					DWORD attr = ::GetFileAttributes(pat.data());
					if ((attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY))
						pat += L"*";

					WIN32_FIND_DATAW wfd;
					::HANDLE handle = ::FindFirstFile(pat.data(), &wfd);

					if (handle == INVALID_HANDLE_VALUE)
					{
						end_ = true;
						return;
					}

					while (_m_ignore(wfd.cFileName))
					{
						if (::FindNextFile(handle, &wfd) == 0)
						{
							end_ = true;
							::FindClose(handle);
							return;
						}
					}

					value_ = value_type(path(path_ + wfd.cFileName));

#elif defined(NANA_POSIX)
					if (path_.size() && (path_.back() != '/'))
						path_ += '/';
					auto handle = opendir(path_.c_str());
					end_ = true;
					if (handle)
					{
						struct dirent * dnt = readdir(handle);
						if (dnt)
						{
							while (_m_ignore(dnt->d_name))
							{
								dnt = readdir(handle);
								if (dnt == 0)
								{
									closedir(handle);
									return;
								}
							}

							value_ = value_type(path_ + dnt->d_name);
							end_ = false;
						}
					}
#endif
					if (false == end_)
					{
						find_ptr_ = std::shared_ptr<find_handle>(new find_handle(handle), inner_handle_deleter());
						handle_ = handle;
					}
				}

				void directory_iterator::_m_read()
				{
					if (handle_)
					{
#if defined(NANA_WINDOWS)
						WIN32_FIND_DATAW wfd;
						if (::FindNextFile(handle_, &wfd) != 0)
						{
							while (_m_ignore(wfd.cFileName))
							{
								if (::FindNextFile(handle_, &wfd) == 0)
								{
									end_ = true;
									return;
								}
							}
							value_ = value_type(path(path_ + wfd.cFileName));
						}
						else
							end_ = true;
#elif defined(NANA_POSIX)
						struct dirent * dnt = readdir(reinterpret_cast<DIR*>(handle_));
						if (dnt)
						{
							while (_m_ignore(dnt->d_name))
							{
								dnt = readdir(reinterpret_cast<DIR*>(handle_));
								if (!dnt)
								{
									end_ = true;
									return;
								}
							}

							value_ = value_type(path(path_ + dnt->d_name));
						}
						else
							end_ = true;
#endif
					}
				}
		//end class directory_iterator

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

		namespace detail
		{
			bool rm_file(const path& p)
			{
				if (p.empty())
					return false;
#if defined(NANA_WINDOWS)
				return (FALSE != ::DeleteFileW(p.c_str()));
#elif defined(NANA_POSIX)
				return (!std::remove(p.c_str()));
#endif
			}

			bool rm_dir(const path& p)
			{
#if defined(NANA_WINDOWS)
				return (FALSE != ::RemoveDirectoryW(p.c_str())) || not_found_error(static_cast<int>(::GetLastError()));
#elif defined(NANA_POSIX)
				return (!::rmdir(p.c_str())) || not_found_error(errno);
#endif
			}

			bool rm_dir(const path& p, bool fails_if_not_empty);

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

					if (is_directory(f))
						rm_dir_recursive(subpath.c_str());
					else
						rm_file(subpath.c_str());
				}
				return rm_dir(dir, true);
			}

			bool rm_dir(const path& p, bool fails_if_not_empty)
			{
				if (p.empty())
					return false;

#if defined(NANA_WINDOWS)
				if (FALSE != ::RemoveDirectoryW(p.c_str()))
					return true;

				if (!fails_if_not_empty && (ERROR_DIR_NOT_EMPTY == ::GetLastError()))
					return detail::rm_dir_recursive(p.c_str());

				return false;
#elif defined(NANA_POSIX)
				if (::rmdir(p.c_str()))
				{
					if (!fails_if_not_empty && (errno == EEXIST || errno == ENOTEMPTY))
						return detail::rm_dir_recursive(p.c_str());

					return false;
				}
				return true;
#endif
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


		file_status status(const path& p)
		{
			std::error_code err;
			auto stat = status(p, err);

			if (err != std::error_code())
				throw filesystem_error("nana::filesystem::status", p, err);

			return stat;
		}

		file_status status(const path& p, std::error_code& ec)
		{
			ec = std::error_code();
#if defined(NANA_WINDOWS)
			auto attr = ::GetFileAttributesW(p.c_str());
			if (INVALID_FILE_ATTRIBUTES == attr)
			{
				if (not_found_error(static_cast<int>(::GetLastError())))
					return file_status{ file_type::not_found };
				return file_status{ file_type::unknown };
			}
			return file_status{ (FILE_ATTRIBUTE_DIRECTORY & attr) ? file_type::directory : file_type::regular, perms::all };
#elif defined(NANA_POSIX)
			struct stat path_stat;
			if (0 != ::stat(p.c_str(), &path_stat))
			{
				if (errno == ENOENT || errno == ENOTDIR)
					return file_status{ file_type::not_found };

				return file_status{ file_type::unknown };
			}

			auto prms = static_cast<perms>(path_stat.st_mode & static_cast<unsigned>(perms::mask));

			if (S_ISREG(path_stat.st_mode))
				return file_status{ file_type::regular, prms };

			if (S_ISDIR(path_stat.st_mode))
				return file_status{ file_type::directory, prms };

			if (S_ISLNK(path_stat.st_mode))
				return file_status{ file_type::symlink, prms };

			if (S_ISBLK(path_stat.st_mode))
				return file_status{ file_type::block, prms };

			if (S_ISCHR(path_stat.st_mode))
				return file_status{ file_type::character, prms };

			if (S_ISFIFO(path_stat.st_mode))
				return file_status{ file_type::fifo, prms };

			if (S_ISSOCK(path_stat.st_mode))
				return file_status{ file_type::socket, prms };

			return file_status{ file_type::unknown };
#endif
		}

		bool is_directory(const path& p)
		{
			return (status(p).type() == file_type::directory);
		}

		bool is_directory(const path& p, std::error_code& ec) noexcept
		{
			return (status(p, ec).type() == file_type::directory);
		}

		std::uintmax_t file_size(const path& p)
		{
			std::error_code err;
			auto bytes = file_size(p, err);
			if (err)
				throw filesystem_error("nana::filesystem::status", p, err);

			return bytes;
		}

		std::uintmax_t file_size(const path& p, std::error_code& ec) noexcept
		{
#if defined(NANA_WINDOWS)
			//Some compilation environment may fail to link to GetFileSizeEx
			typedef BOOL(__stdcall *GetFileSizeEx_fptr_t)(HANDLE, PLARGE_INTEGER);
			GetFileSizeEx_fptr_t get_file_size_ex = reinterpret_cast<GetFileSizeEx_fptr_t>(::GetProcAddress(::GetModuleHandleA("Kernel32.DLL"), "GetFileSizeEx"));
			if (get_file_size_ex)
			{
				HANDLE handle = ::CreateFile(p.c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
				if (INVALID_HANDLE_VALUE != handle)
				{
					LARGE_INTEGER li;
					if (!get_file_size_ex(handle, &li))
						li.QuadPart = 0;

					::CloseHandle(handle);
					return li.QuadPart;
				}
			}
			ec.assign(static_cast<int>(::GetLastError()), std::generic_category());
#elif defined(NANA_POSIX)
			FILE * stream = ::fopen(p.c_str(), "rb");
			if (stream)
			{
				long long bytes = 0;
#	if defined(NANA_LINUX)
				fseeko64(stream, 0, SEEK_END);
				bytes = ftello64(stream);
#	elif defined(NANA_POSIX)
				fseeko(stream, 0, SEEK_END);
				bytes = ftello(stream);
#	endif
				::fclose(stream);
				return bytes;
			}
			ec.assign(static_cast<int>(errno), std::generic_category());
#endif
			return static_cast<std::uintmax_t>(-1);
		}


		file_time_type last_write_time(const path& p)
		{
			struct tm t;
			nana::filesystem_ext::modified_file_time(p, t);
			std::chrono::system_clock::time_point dateTime =std::chrono::system_clock::from_time_t( mktime(&t) );
			return 	dateTime;
		}

			bool create_directory(const path& p)
			{
#if defined(NANA_WINDOWS)
				return (FALSE != ::CreateDirectoryW(p.c_str(), 0));
#elif defined(NANA_POSIX)
				return (0 == ::mkdir(p.c_str(), static_cast<int>(perms::all)));
#endif
			}

			bool remove(const path& p)
			{
				auto stat = status(p);
				if (stat.type() == file_type::directory)
					return detail::rm_dir(p);

				return detail::rm_file(p);
			}

			bool remove(const path& p, std::error_code & ec)
			{
				ec.clear();
				auto stat = status(p);
				if (stat.type() == file_type::directory)
					return detail::rm_dir(p);

				return detail::rm_file(p);
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

#ifndef CXX_NO_INLINE_NAMESPACE
		} //end namespace v1
#endif
		}//end namespace filesystem
	} //end namespace experimental
}//end namespace nana

namespace std
{
	namespace filesystem
	{
#if defined(NANA_FILESYSTEM_FORCE) || \
    (defined(_MSC_VER) && ((!defined(_MSVC_LANG)) || (_MSVC_LANG < 201703)))
		path absolute(const path& p)
		{
			if (p.empty())
				return p;

			auto abs_base = current_path();

			//  store expensive to compute values that are needed multiple times
			path p_root_name(p.root_name());
			path base_root_name(abs_base.root_name());
			path p_root_directory(p.root_directory());

			if (!p_root_name.empty())  // p.has_root_name()
			{
				if (p_root_directory.empty())  // !p.has_root_directory()
					return p_root_name / abs_base.root_directory()
					/ abs_base.relative_path() / p.relative_path();
				// p is absolute, so fall through to return p at end of block
			}
			else if (!p_root_directory.empty())  // p.has_root_directory()
			{
#ifdef NANA_POSIX
				// POSIX can have root name it it is a network path
				if (base_root_name.empty())   // !abs_base.has_root_name()
					return p;
#endif
				return base_root_name / p;
			}
			else
				return abs_base / p;

			return p;  // p.is_absolute() is true
		}

		path absolute(const path& p, std::error_code& /*err*/)
		{
			return absolute(p);
		}

		path canonical(const path& p, std::error_code* err)
		{
			path source(p.is_absolute() ? p : absolute(p));
			path root(source.root_path());
			path result;

			std::error_code local_ec;
			file_status stat(status(source, local_ec));

			if (stat.type() == file_type::not_found)
			{
				if (nullptr == err)
					throw (filesystem_error(
					"nana::filesystem::canonical", source,
					error_code(static_cast<int>(errc::no_such_file_or_directory), generic_category())));
				err->assign(static_cast<int>(errc::no_such_file_or_directory), generic_category());
				return result;
			}
			else if (local_ec)
			{
				if (nullptr == err)
					throw (filesystem_error(
					"nana::filesystem::canonical", source, local_ec));
				*err = local_ec;
				return result;
			}


			auto tmp_p = source;

			std::vector<path> source_elements;
			while (tmp_p != root)
			{
				source_elements.emplace(source_elements.begin(), tmp_p.filename());
				tmp_p.remove_filename();
			}

			result = root;

			for(auto & e : source_elements)
			{
				auto str = e.string();
				if("." == str)
					continue;
				else if(".." == str)
				{
					if(result != root)
						result.remove_filename();
					continue;
				}

				result /= e;
			}

			if (err)
				err->clear();

			return result;
		}

		path canonical(const path& p)
		{
			return canonical(p, nullptr);
		}

		path canonical(const path& p, std::error_code& err)
		{
			return canonical(p, &err);
		}

		bool try_throw(int err_val, const path& p, std::error_code* ec, const char* message)
		{
			if (0 == err_val)
			{
				if (ec) ec->clear();
			}
			else
			{	//error
				if (nullptr == ec)
					throw (filesystem_error(
						message, p,
						error_code(err_val, generic_category())));
				else
					ec->assign(err_val, system_category());
			}
			return err_val != 0;
		}

		path weakly_canonical(const path& p, std::error_code* err)
		{
			path head{ p };

			std::error_code tmp_err;
			std::vector<path> elements;
			while (!head.empty())
			{
				auto head_status = status(head, tmp_err);

				if (head_status.type() == file_type::unknown)
				{
					if (try_throw(static_cast<int>(errc::invalid_argument), head, err, "nana::filesystem::weakly_canonical"))
						return path{};
				}
				if (head_status.type() != file_type::not_found)
					break;

				elements.emplace_back(head.filename());
				head.remove_filename();
			}

			bool tail_has_dots = false;
			path tail;

			for (auto & e : elements)
			{
				tail /= e;
				// for a later optimization, track if any dot or dot-dot elements are present
				if (e.native().size() <= 2
					&& e.native()[0] == '.'
					&& (e.native().size() == 1 || e.native()[1] == '.'))
					tail_has_dots = true;
			}

			if (head.empty())
				return p.lexically_normal();
			head = canonical(head, tmp_err);
			if (try_throw(tmp_err.value(), head, err, "nana::filesystem::weakly_canonical"))
				return path();
			return tail.empty()
				? head
				: (tail_has_dots  // optimization: only normalize if tail had dot or dot-dot element
				? (head / tail).lexically_normal()
				: head / tail);
		}

		path weakly_canonical(const path& p)
		{
			return weakly_canonical(p, nullptr);
		}

		path weakly_canonical(const path& p, std::error_code& err)
		{
			return weakly_canonical(p, &err);
		}
#endif

#if defined(NANA_FILESYSTEM_FORCE) || defined(NANA_MINGW)
    bool exists( std::filesystem::file_status s ) noexcept
    {
        return s.type() != file_type::not_found;
    }

    bool exists( const std::filesystem::path& p )
    {
        return exists(status(p));
    }

    bool exists( const std::filesystem::path& p, std::error_code& ec ) noexcept
    {
        return exists(status(p, ec));
    }
#endif
	}//end namespace filesystem
}//end namespace std

#else 	//NANA_USING_NANA_FILESYSTEM
#	if defined(NANA_USING_STD_EXPERIMENTAL_FILESYSTEM)

	//Defines the functions that are not provided by experimental/filesystem
	namespace std
	{
		namespace filesystem
		{
			#if (defined(_MSC_VER) && (_MSC_VER > 1912)) || \
				(!defined(__clang__) && defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__ < 801))

			namespace detail
			{
				bool try_throw(int err_val, const path& p, std::error_code* ec, const char* message)
				{
					if (0 == err_val)
					{
						if (ec) ec->clear();
					}
					else
					{	//error
						if (nullptr == ec)
							throw (filesystem_error(
								message, p,
								error_code(err_val, generic_category())));
						else
							ec->assign(err_val, system_category());
					}
					return err_val != 0;
				}

				path lexically_normal(path p)
				{
					if (p.empty())
						return p;

					std::vector<path> elements;

					while (!p.empty())
					{
						elements.emplace_back(p.filename());
						p.remove_filename();
					}

					auto start = elements.begin();
					auto last = elements.end();
					auto stop = last--;
					for (auto itr(start); itr != stop; ++itr)
					{
						// ignore "." except at start and last
						if (itr->native().size() == 1
							&& (itr->native())[0] == '.'
							&& itr != start
							&& itr != last) continue;

						// ignore a name and following ".."
						if (!p.empty()
							&& itr->native().size() == 2
							&& (itr->native())[0] == '.'
							&& (itr->native())[1] == '.') // dot dot
						{
							auto lf(p.filename().native());
							if (lf.size() > 0
								&& (lf.size() != 1
									|| (lf[0] != '.'
										&& (lf[0] != '/' && lf[0] != '\\')))
								&& (lf.size() != 2
									|| (lf[0] != '.'
										&& lf[1] != '.'
	#             ifdef NANA_WINDOWS
										&& lf[1] != ':'
	#             endif
										)
									)
								)
							{
								p.remove_filename();
								auto next = itr;
								if (p.empty() && ++next != stop
									&& next == last && last->string() == ".")
								{
									p /= ".";
								}
								continue;
							}
						}

						p /= *itr;
					};

					if (p.empty())
						p = ".";
					return p;
				}
			}

			path weakly_canonical(const path& p, std::error_code* err)
			{
				path head{ p };

				std::error_code tmp_err;
				std::vector<path> elements;
				while (!head.empty())
				{
					auto head_status = status(head, tmp_err);

					if (head_status.type() == file_type::unknown)
					{
						if (detail::try_throw(static_cast<int>(errc::invalid_argument), head, err, "nana::filesystem::weakly_canonical"))
							return path{};
					}
					if (head_status.type() != file_type::not_found)
						break;

					elements.emplace_back(head.filename());
					head.remove_filename();
				}

				bool tail_has_dots = false;
				path tail;

				for (auto & e : elements)
				{
					tail /= e;
					// for a later optimization, track if any dot or dot-dot elements are present
					if (e.native().size() <= 2
						&& e.native()[0] == '.'
						&& (e.native().size() == 1 || e.native()[1] == '.'))
						tail_has_dots = true;
				}

				if (head.empty())
					return detail::lexically_normal(p);
				head = canonical(head, tmp_err);
				if (detail::try_throw(tmp_err.value(), head, err, "nana::filesystem::weakly_canonical"))
					return path();
				return tail.empty()
					? head
					: (tail_has_dots  // optimization: only normalize if tail had dot or dot-dot element
						? detail::lexically_normal(head / tail)
						: head / tail);
			}

			path weakly_canonical(const path& p)
			{
				return weakly_canonical(p, nullptr);
			}

			path weakly_canonical(const path& p, std::error_code& err)
			{
				return weakly_canonical(p, &err);
			}
			#endif
		}
	}
#	endif

#endif //NANA_USING_NANA_FILESYSTEM

