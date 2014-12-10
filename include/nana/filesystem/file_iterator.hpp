/*
 *	A File Iterator Implementation
 *	Copyright(C) 2003 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: stdex/filesystem/file_iterator.hpp
 *	@description:
 *		file_iterator is a toolkit for applying each file and directory in a
 *	specified path.
 */

#ifndef NANA_FILESYSTEM_FILE_ITERATOR_HPP
#define NANA_FILESYSTEM_FILE_ITERATOR_HPP
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

	template<typename FileInfo>
	class basic_file_iterator
		:public std::iterator<std::input_iterator_tag, FileInfo>
	{
	public:
		typedef FileInfo value_type;

		basic_file_iterator():end_(true), handle_(nullptr){}

		basic_file_iterator(const nana::string& file_path)
			:end_(false), handle_(nullptr)
		{
			_m_prepare(file_path);
		}

		const value_type&
		operator*() const { return value_; }

		const value_type*
		operator->() const { return &(operator*()); }

		basic_file_iterator& operator++()
		{ _m_read(); return *this; }

		basic_file_iterator operator++(int)
		{
			basic_file_iterator tmp = *this;
			_m_read();
			return tmp;
		}

		bool equal(const basic_file_iterator& x) const
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

	template<typename Value_Type>
	inline bool operator==(const basic_file_iterator<Value_Type> & x, const basic_file_iterator<Value_Type> & y)
	{
	   return x.equal(y);
	}

	template<typename Value_Type>
	inline bool operator!=(const basic_file_iterator<Value_Type> & x, const basic_file_iterator<Value_Type> & y)
	{
	   return !x.equal(y);
	}

	typedef basic_file_iterator<fileinfo> file_iterator;
}//end namespace filesystem
}//end namespace nana

#endif
