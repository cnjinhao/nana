/*
 *	A filesystem Implementation
 *	Copyright(C) 2003 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: stdex/filesystem/filesystem.hpp
 *	@description:
 *		file_iterator is a toolkit for applying each file and directory in a
 *	specified path.
 */

// http://en.cppreference.com/w/cpp/experimental/fs
// http://cpprocks.com/introduction-to-tr2-filesystem-library-in-vs2012/  --- TR2 filesystem in VS2012
// https://msdn.microsoft.com/en-us/library/hh874694%28v=vs.140%29.aspx   ---  C++ 14, the <filesystem> header VS2015
// https://msdn.microsoft.com/en-us/library/hh874694%28v=vs.120%29.aspx   --- <filesystem> header VS2013
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4100.pdf      --- last pdf of std draft N4100    2014-07-04
// http://cplusplus.github.io/filesystem-ts/working-draft.html            --- in html format
// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4099.html     --- in html format
// http://article.gmane.org/gmane.comp.lib.boost.devel/256220             --- The filesystem TS unanimously approved by ISO.
// http://theboostcpplibraries.com/boost.filesystem                       --- Boost docs
// http://www.boost.org/doc/libs/1_58_0/libs/filesystem/doc/index.htm     --- 
// http://www.boost.org/doc/libs/1_34_0/libs/filesystem/doc/index.htm
// http://www.boost.org/doc/libs/1_58_0/boost/filesystem.hpp 


#ifndef NANA_FILESYSTEM_HPP
#define NANA_FILESYSTEM_HPP
#include <iterator>
#include <memory>

#include <nana/deploy.hpp>

#ifdef NANA_WINDOWS
	#include <windows.h>
	typedef HANDLE find_handle_t;
#elif defined(NANA_LINUX)
	#include <sys/stat.h>
	#include <sys/types.h>
	#include <dirent.h>
	typedef DIR* find_handle_t;
#endif

 // namespace std { namespace experimental { namespace filesystem { inline namespace v1 {

namespace nana
{
namespace filesystem
{
	struct fileinfo
	{
		fileinfo();
#ifdef NANA_WINDOWS
		fileinfo(const WIN32_FIND_DATA& wfd);
#elif NANA_LINUX
		fileinfo(const nana::string& filename, const struct stat &);
#endif
		nana::string name;

		unsigned long size;
		bool directory;
	};


    /// an iterator for a sequence of directory_entry elements representing the files in a directory, not an recursive_directory_iterator
	//template<typename FileInfo>
	class directory_iterator 		:public std::iterator<std::input_iterator_tag, fileinfo>
	{
	public:
		typedef fileinfo value_type;

		directory_iterator():end_(true), handle_(nullptr){}

		directory_iterator(const nana::string& file_path)
			:end_(false), handle_(nullptr)
		{
			_m_prepare(file_path);
		}

		const value_type&
		operator*() const { return value_; }

		const value_type*
		operator->() const { return &(operator*()); }

		directory_iterator& operator++()
		{ _m_read(); return *this; }

		directory_iterator operator++(int)
		{
			directory_iterator tmp = *this;
			_m_read();
			return tmp;
		}

		bool equal(const directory_iterator& x) const
		{
			if(end_ && (end_ == x.end_)) return true;
			return (value_.name == x.value_.name);
		}
	private:
		template<typename Char>
		static bool _m_ignore(const Char * p)
		{
			while(*p == '.')
				++p;
			return (*p == 0);
		}

		void _m_prepare(const nana::string& file_path)
		{
		#if defined(NANA_WINDOWS)
			path_ = file_path;
			auto pat = file_path;
			DWORD attr = ::GetFileAttributes(pat.data());
			if((attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY))
				pat += STR("\\*");

			::HANDLE handle = ::FindFirstFile(pat.data(), &wfd_);

			if(handle == INVALID_HANDLE_VALUE)
			{
				end_ = true;
				return;
			}

			while(_m_ignore(wfd_.cFileName))
			{
				if(::FindNextFile(handle, &wfd_) == 0)
				{
					end_ = true;
					::FindClose(handle);
					return;
				}
			}
			value_ = value_type(wfd_);
		#elif defined(NANA_LINUX)
			path_ = nana::charset(file_path);
			if(path_.size() && (path_[path_.size() - 1] != '/'))
				path_ += '/';
			find_handle_t handle = opendir(path_.c_str());
			end_ = true;
			if(handle)
			{
				struct dirent * dnt = readdir(handle);
				if(dnt)
				{
					while(_m_ignore(dnt->d_name))
					{
						dnt = readdir(handle);
						if(dnt == 0)
						{
							closedir(handle);
							return;
						}
					}

					struct stat fst;
					if(stat((path_ + dnt->d_name).c_str(), &fst) == 0)
					{
						value_ = value_type(nana::charset(dnt->d_name), fst);
					}
					else
					{
						value_.name = nana::charset(dnt->d_name);
						value_.size = 0;
						value_.directory = false;
					}
					end_ = false;
				}
			}
		#endif
			if(false == end_)
			{
				find_ptr_ = std::shared_ptr<find_handle_t>(new find_handle_t(handle), inner_handle_deleter());
				handle_ = handle;
			}
		}

		void _m_read()
		{
			if(handle_)
			{
			#if defined(NANA_WINDOWS)
				if(::FindNextFile(handle_, &wfd_) != 0)
				{
					while(_m_ignore(wfd_.cFileName))
					{
						if(::FindNextFile(handle_, &wfd_) == 0)
						{
							end_ = true;
							return;
						}
					}
					value_ = value_type(wfd_);
				}
				else
					end_ = true;
			#elif defined(NANA_LINUX)
				struct dirent * dnt = readdir(handle_);
				if(dnt)
				{
					while(_m_ignore(dnt->d_name))
					{
						dnt = readdir(handle_);
						if(dnt == 0)
						{
							end_ = true;
							return;
						}
					}
					struct stat fst;
					if(stat((path_ + "/" + dnt->d_name).c_str(), &fst) == 0)
						value_ = value_type(nana::charset(dnt->d_name), fst);
					else
						value_.name = nana::charset(dnt->d_name);
				}
				else
					end_ = true;
			#endif
			}
		}
	private:
		struct inner_handle_deleter
		{
			void operator()(find_handle_t * handle)
			{
				if(handle && *handle)
				{
  				#if defined(NANA_WINDOWS)
					::FindClose(*handle);
            	#elif defined(NANA_LINUX)
					::closedir(*handle);
				#endif
				}
				delete handle;
			}
		};
	private:
		bool	end_;

#if defined(NANA_WINDOWS)
		WIN32_FIND_DATA		wfd_;
		nana::string	path_;
#elif defined(NANA_LINUX)
		std::string	path_;
#endif
		std::shared_ptr<find_handle_t> find_ptr_;

		find_handle_t	handle_;
		value_type	value_;
	};

	//template<typename Value_Type>
	inline bool operator==(const directory_iterator/*<Value_Type>*/ & x, const directory_iterator/*<Value_Type>*/ & y)
	{
	   return x.equal(y);
	}

	//template<typename Value_Type>
	inline bool operator!=(const directory_iterator/*<Value_Type>*/ & x, const directory_iterator/*<Value_Type>*/ & y)
	{
	   return !x.equal(y);
	}

	//using directory_iterator = directory_iterator<fileinfo> ;
}//end namespace filesystem
}//end namespace nana

#endif
