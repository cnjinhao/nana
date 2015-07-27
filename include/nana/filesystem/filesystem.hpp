/*
 *	A ISO C++ filesystem Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/filesystem/filesystem.hpp
 *	@description:
 *		file_iterator is a toolkit for applying each file and directory in a
 *	    specified path.
 *  Modiffied by Ariel Vina-Rodriguez:
 *  Now mimic std::experimental::filesystem::v1   (boost v3)
 *  and need VC2015 or a C++11 compiler. With a few correction will be compiler by VC2013
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
// https://gcc.gnu.org/onlinedocs/libstdc++/manual/status.html#status.iso.200x --- Table 1.4. g++ C++ Technical Specifications Implementation Status

#ifndef NANA_FILESYSTEM_HPP
#define NANA_FILESYSTEM_HPP
#include <iterator>
#include <memory>
#include <chrono>
#include <cstddef>

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

namespace nana  { namespace experimental
{
namespace filesystem
{
    enum class file_type 
    { 
        none = 0,   ///< has not been determined or an error occurred while trying to determine
        not_found = -1, ///< Pseudo-type: file was not found. Is not considered an error
        regular = 1,
        directory = 2  ,
        symlink =3, ///< Symbolic link file
        block =4,  ///< Block special file
        character= 5 ,  ///< Character special file
        fifo = 6 ,  ///< FIFO or pipe file
        socket =7,
        unknown= 8  ///< The file does exist, but is of an operating system dependent type not covered by any of the other
    };
    
    enum class perms 
    {
        none =0, ///< There are no permissions set for the file.
        unknown = 0xFFFF  ///<  not known, such as when a file_status object is created without specifying the permissions
    };
    //enum class copy_options;
    //enum class directory_options;

    // class filesystem_error;
    enum class error  	{	none = 0 		};  // deprecate ?? 

	struct attribute  // deprecate ??
	{
        uintmax_t size     {};
        bool      directory{};
        tm        modified {};

        attribute() {} ;
        attribute( uintmax_t size, bool is_directory) :size{size}, directory{is_directory} {}
	};

    struct space_info
    {
        uintmax_t capacity;
        uintmax_t free;
        uintmax_t available;
    };
    using file_time_type = std::chrono::time_point< std::chrono::system_clock>;// trivial-clock> ;

    class file_status
    {
        file_type m_ft = file_type::none;
        perms     m_prms = perms::unknown;

       public:
        explicit file_status(file_type ft = file_type::none, perms prms = perms::unknown) 
            :m_ft{ft}, m_prms{prms}
        {}

        file_status(const file_status& fs) : m_ft{fs.m_ft}, m_prms{fs.m_prms}{} // = default;  
        file_status(file_status&& fs) : m_ft{fs.m_ft}, m_prms{fs.m_prms}{} // = default; 

        ~file_status(){};
        file_status& operator=(const file_status&)  = default;
        file_status& operator=(file_status&&fs)   // = default; 
        { 
            m_ft=fs.m_ft;  m_prms = fs.m_prms; 
            return *this;
        } 
        // observers
        file_type type() const { return m_ft;}
        perms permissions() const { return m_prms;}
        // modifiers
        void type       (file_type ft)  { m_ft=ft ;}
        void permissions(perms prms)    { m_prms = prms; }
    };
    
    /// concerned only with lexical and syntactic aspects and does not necessarily exist in
    /// external storage, and the pathname is not necessarily valid for the current operating system 
    /// or for a particular file system 
    /// A sequence of elements that identify the location of a file within a filesystem. 
    /// The elements are the:
    /// rootname (opt), root-directory (opt), and an optional sequence of filenames. 
    /// The maximum number of elements in the sequence is operating system dependent.
	class path
	{
	public:
		path();
		path(const nana::string&);

		bool empty() const;
		path root() const;
		file_type what() const;

		nana::string filename() const;
#if defined(NANA_WINDOWS)
	public:
		nana::string to_string() const { return text_; }
		operator nana::string() const { return text_; }
	private:
		nana::string text_;
#else
	public:
		std::string to_string() const { return text_; }
		operator std::string() const { return text_; }
	private:
		std::string text_;
#endif
	};

	struct directory_entry
	{
		using path_type = filesystem::path;
		path_type m_path;

		attribute attr{};
		//file_status m_status;

		directory_entry(){}
		directory_entry(const path_type& p, bool is_directory, uintmax_t size)
			:m_path{p}, attr{size, is_directory}
		{}

		void assign          (const path_type& p){ m_path=p;}
		void replace_filename(const path_type& p){ m_path=p;}

		//file_status status() const;

		operator const path_type&() const {return m_path;};
		const path_type& path() const {return m_path;}
	};

    /// an iterator for a sequence of directory_entry elements representing the files in a directory, not an recursive_directory_iterator
	//template<typename FileInfo>
	class directory_iterator 		:public std::iterator<std::input_iterator_tag, directory_entry>
	{
	public:
		using value_type = directory_entry ;
		typedef ptrdiff_t                   difference_type;
		typedef const directory_entry*      pointer;
		typedef const directory_entry&      reference;
		typedef std::input_iterator_tag     iterator_category;

		directory_iterator():end_(true), handle_(nullptr){}

		directory_iterator(const nana::string& file_path) {	_m_prepare(file_path);	}
		directory_iterator(const path& file_path) {	_m_prepare(file_path.filename());	}

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
			return (value_.path().filename() == x.value_.path().filename());
		}
    
		
		// enable directory_iterator range-based for statements
		directory_iterator begin( )    { return *this;  }
		directory_iterator end( )      { return {};     }
	
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

			value_ = value_type(path(wfd_.cFileName), 
                               (FILE_ATTRIBUTE_DIRECTORY & wfd_.dwFileAttributes) == FILE_ATTRIBUTE_DIRECTORY,
                                wfd_.nFileSizeLow);

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
					bool is_directory = false;
					unsigned size = 0;
					if(stat((path_ + dnt->d_name).c_str(), &fst) == 0)
					{
						is_directory = (0 != S_ISDIR(fst.st_mode));
						size = fst.st_size;
					}
					value_ = value_type(static_cast<nana::string>(nana::charset(dnt->d_name)), is_directory, size);
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
					value_ = value_type(path(wfd_.cFileName), 
								(FILE_ATTRIBUTE_DIRECTORY & wfd_.dwFileAttributes) == FILE_ATTRIBUTE_DIRECTORY,
								wfd_.nFileSizeLow);
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

					nana::string d_name = nana::charset(dnt->d_name);
					struct stat fst;
					if(stat((path_ + "/" + dnt->d_name).c_str(), &fst) == 0)
						value_ = value_type(std::move(d_name), (0 != S_ISDIR(fst.st_mode)), fst.st_size);
					else
						value_.m_path = path(std::move(d_name));
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
		bool	end_{false};

#if defined(NANA_WINDOWS)
		WIN32_FIND_DATA		wfd_;
		nana::string	path_;
#elif defined(NANA_LINUX)
		std::string	path_;
#endif
		std::shared_ptr<find_handle_t> find_ptr_;
		find_handle_t	handle_{nullptr};
		value_type	value_;
	};


    //class recursive_directory_iterator;
    //// enable recursive_directory_iterator range-based for statements
    //recursive_directory_iterator begin(recursive_directory_iterator iter) noexcept;
    //recursive_directory_iterator end(const recursive_directory_iterator&) noexcept;

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


    // file_status status(const path& p);
	bool file_attrib(const nana::string& file, attribute&);

	inline bool is_directory(file_status s) { return s.type() == file_type::directory ;}
	inline bool is_directory(const path& p) { return directory_iterator(p)->attr.directory; }//works??
	inline bool is_directory(const directory_entry& d) { return d.attr.directory; }
    //bool is_directory(const path& p, error_code& ec) noexcept;

    //bool is_regular_file(file_status s) noexcept;

	inline bool is_empty(const path& p)
    {
        directory_iterator d(p) ;
        return d->attr.directory ? d == directory_iterator()
                                  : d->attr.size == 0;
    }
    //bool is_empty(const path& p, error_code& ec) noexcept;

           uintmax_t file_size(const nana::string& file);  // deprecate?
	inline uintmax_t file_size(const path& p){return file_size(p.filename());}
    //uintmax_t file_size(const path& p, error_code& ec) noexcept;
	//long long filesize(const nana::string& file);


    bool create_directories(const path& p);
    //bool create_directories(const path& p, error_code& ec) noexcept;
    bool create_directory(const path& p);
    //bool create_directory(const path& p, error_code& ec) noexcept;
    bool create_directory(const path& p, const path& attributes);
    //bool create_directory(const path& p, const path& attributes,     error_code& ec) noexcept;
	bool create_directory(const nana::string& dir, bool & if_exist);
	inline bool create_directory(const path& p, bool & if_exist)
    {
        return create_directory(p.filename(), if_exist);
    };

    
    bool modified_file_time(const nana::string& file, struct tm&);


	nana::string path_user();
	
    
    path current_path();
    //path current_path(error_code& ec);
    void current_path(const path& p);
    //void current_path(const path& p, error_code& ec) noexcept;    
    //nana::string path_current();


    //bool remove(const path& p);
    //bool remove(const path& p, error_code& ec) noexcept;
	bool rmfile(const nana::char_t* file);

    //uintmax_t remove_all(const path& p);
    //uintmax_t remove_all(const path& p, error_code& ec) noexcept;
	bool rmdir(const nana::char_t* dir, bool fails_if_not_empty);
	nana::string root(const nana::string& path);


}//end namespace filesystem
} //end namespace experimental
}//end namespace nana

#endif
