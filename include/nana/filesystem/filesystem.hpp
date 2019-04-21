/**
 *	A ISO C++ filesystem Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file nana/filesystem/filesystem.hpp
 *  @author Ariel Vina-Rodriguez, Jinhao
 *  @brief Mimic std::experimental::filesystem::v1   (boost v3)
 *  and need VC2015 or a C++11 compiler. With a few correction can be compiler by VC2013
 */

// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n4100.pdf      --- pdf of std draft N4100 <filesystem> 2014-07-04
// http://en.cppreference.com/w/cpp/experimental/fs
// http://cpprocks.com/introduction-to-tr2-filesystem-library-in-vs2012/  --- TR2 filesystem in VS2012
// https://msdn.microsoft.com/en-us/library/hh874694%28v=vs.140%29.aspx   ---  C++ 14, the <filesystem> header VS2015
// https://msdn.microsoft.com/en-us/library/hh874694%28v=vs.120%29.aspx   --- <filesystem> header VS2013
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
#include <nana/push_ignore_diagnostic>

//Filesystem Selection
#include <nana/config.hpp>

#if defined(NANA_USING_NANA_FILESYSTEM) || defined(NANA_USING_STD_FILESYSTEM) || defined(NANA_USING_BOOST_FILESYSTEM)
#undef NANA_USING_NANA_FILESYSTEM
#undef NANA_USING_STD_FILESYSTEM
#undef NANA_USING_BOOST_FILESYSTEM
#endif

#define NANA_USING_NANA_FILESYSTEM  0
#define NANA_USING_STD_FILESYSTEM   0
#define NANA_USING_BOOST_FILESYSTEM 0

#if (defined(NANA_FILESYSTEM_FORCE) || ( (defined(STD_FILESYSTEM_NOT_SUPPORTED) && !defined(BOOST_FILESYSTEM_AVAILABLE)) && !(defined(BOOST_FILESYSTEM_FORCE) || defined(STD_FILESYSTEM_FORCE)) ) )

#undef  NANA_USING_NANA_FILESYSTEM
#define NANA_USING_NANA_FILESYSTEM  1

#elif (defined(BOOST_FILESYSTEM_AVAILABLE) && ( defined(BOOST_FILESYSTEM_FORCE) || ( defined(STD_FILESYSTEM_NOT_SUPPORTED) && !defined(STD_FILESYSTEM_FORCE) ) ))

#undef  NANA_USING_BOOST_FILESYSTEM
#define NANA_USING_BOOST_FILESYSTEM 1
#   include <chrono>
#   include <boost/filesystem.hpp>

// add boost::filesystem into std::experimental::filesystem
namespace std {
	namespace experimental {
		namespace filesystem {
			using namespace boost::filesystem;
			using file_time_type = std::chrono::time_point<std::chrono::system_clock>;

			enum class file_type {
				none = boost::filesystem::file_type::status_unknown,
				not_found = boost::filesystem::file_type::file_not_found,
				regular = boost::filesystem::file_type::regular_file,
				directory = boost::filesystem::file_type::directory_file,
				symlink = boost::filesystem::file_type::symlink_file,
				block = boost::filesystem::file_type::block_file,
				character = boost::filesystem::file_type::character_file,
				fifo = boost::filesystem::file_type::fifo_file,
				socket = boost::filesystem::file_type::socket_file,
				unknown = boost::filesystem::file_type::type_unknown,
			};
// Boost dont include generic_u8string
// http://www.boost.org/doc/libs/1_66_0/boost/filesystem/path.hpp
//
// Boost versions: 1.67.0, 1.66.0, ... 1.56.0 enable directory_iterator C++11 range-base for
// http://www.boost.org/doc/libs/1_66_0/boost/filesystem/operations.hpp
// but travis come with an oooold version of boost
// 1.55.0 NOT enable directory_iterator C++11 range-base for
// http://www.boost.org/doc/libs/1_54_0/boost/filesystem/operations.hpp
#if BOOST_VERSION < 105600
            namespace boost
        //  enable directory_iterator C++11 range-base for statement use  --------------------//

		//  begin() and end() are only used by a range-based for statement in the context of
		//  auto - thus the top-level const is stripped - so returning const is harmless and
		//  emphasizes begin() is just a pass through.
		inline const directory_iterator& begin(const directory_iterator& iter) BOOST_NOEXCEPT
		{
			return iter;
		}

		inline directory_iterator end(const directory_iterator&) BOOST_NOEXCEPT
		{
			return directory_iterator();
		}
#endif

		} // filesystem
	} // experimental

	namespace filesystem
	{
		using namespace experimental::filesystem;
	}

#ifndef __cpp_lib_experimental_filesystem
#   define __cpp_lib_experimental_filesystem 201406
#endif
} // std

#else
#   undef NANA_USING_STD_FILESYSTEM
#   define NANA_USING_STD_FILESYSTEM 1
	//Detects whether the compiler supports std::filesystem under current options
#	if ((defined(_MSC_VER) && (_MSC_VER >= 1912) && defined(_MSVC_LANG) && _MSVC_LANG >= 201703)) ||				\
		((__cplusplus >= 201703L) && \
			(defined(__clang__) && (__clang_major__ >= 7) ||		\
			(!defined(__clang__) && defined(__GNUC__) && (__GNUC__ >= 8))) )
#   	include <filesystem>
#	else
#   	include <experimental/filesystem>
		namespace std{
			namespace filesystem{
				using namespace std::experimental::filesystem;
			}
		}
#		undef NANA_USING_STD_EXPERIMENTAL_FILESYSTEM
#		define NANA_USING_STD_EXPERIMENTAL_FILESYSTEM
#	endif
#endif

#if NANA_USING_NANA_FILESYSTEM

#include <string>
#include <system_error>
#include <iterator>
#include <memory>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <algorithm>

#include <nana/deploy.hpp>

namespace nana  { namespace experimental { namespace filesystem
{
#ifndef CXX_NO_INLINE_NAMESPACE
			inline namespace v1
			{
#endif

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
		none = 0,		///< There are no permissions set for the file.
		all = 0x1FF,		///< owner_all | group_all | others_all
		mask = 0xFFF,		///< all | set_uid | set_gid | sticky_bit.
		unknown = 0xFFFF	///<  not known, such as when a file_status object is created without specifying the permissions
	};
    //enum class copy_options;

    enum class directory_options
    {
    	none,
    	follow_directory_symlink,
    	skip_permission_denied
    };

    struct space_info
    {
        uintmax_t capacity;
        uintmax_t free;
        uintmax_t available;
    };

	using file_time_type = std::chrono::time_point<std::chrono::system_clock>; ///< trivial-clock> ;

	class file_status
	{
		file_type m_ft = file_type::none;
		perms     m_prms = perms::unknown;

	public:
		explicit file_status(file_type ft = file_type::none, perms prms = perms::unknown);

		// observers
		file_type type() const;
		perms permissions() const;

		// modifiers
		void type(file_type ft);
		void permissions(perms prms);
	private:
		file_type	value_;
		perms		perms_;
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
#if defined(NANA_WINDOWS)
		using value_type = wchar_t;
		const static value_type preferred_separator = L'\\';
#else
		using value_type = char;
		const static value_type preferred_separator = '/';
#endif
		using string_type = std::basic_string<value_type>;

		path() = default;

		template<typename Source>
		path(const Source& source)
		{
			_m_assign(source);
		}

		// modifiers
		void clear() noexcept;
		path& make_preferred();
		path& remove_filename();
		//path& replace_filename(const path& replacement);
		//path& replace_extension(const path& replacement = path());
		//void swap(path& rhs) noexcept;

		// decomposition
		path root_name() const;
		path root_directory() const;
		path root_path() const;
		path relative_path() const;
		path parent_path() const;
		path filename() const;
		path stem() const;
		path extension() const;

		// query
		bool empty() const noexcept;
		bool has_root_name() const { return !root_name().empty();  }
		bool has_root_directory() const { return !root_directory().empty();  }
		bool has_root_path() const { return !root_path().empty();  }
		bool has_relative_path() const { return !relative_path().empty(); }
		bool has_parent_path() const { return !parent_path().empty(); };   // temp;;
		bool has_filename() const    { return !filename().empty(); };   // temp;
		//bool has_stem() const;
		bool has_extension() const   { return !extension().empty(); };   // temp
		bool is_absolute() const;
		bool is_relative() const;

		int compare(const path& other) const;

		file_type what() const;

		const value_type*c_str() const;
		const string_type& native() const;
		operator string_type() const;

		std::string string() const;
		std::wstring wstring() const;
		std::string u8string() const;
		// std::u16string u16string() const;
		// std::u32string u32string() const;

		std::string generic_string() const ;
		std::wstring generic_wstring() const;
		std::string generic_u8string() const;
		// std::u16string generic_u16string() const;
		// std::u32string generic_u32string() const;

		path lexically_normal() const;

		//appends
		path& operator/=(const path& other);

		template<typename Source>
		path& operator/=(const Source& source)
		{
			path other(source);
			return this->operator/=(other);
		}

		template<typename Source>
		path& append(const Source& source)
		{
			path other(source);
			return this->operator/=(other);
		}
	private:
		void _m_assign(const std::string& source_utf8);
		void _m_assign(const std::wstring& source);
	private:
		string_type pathstr_;
	};

	bool operator==(const path& lhs, const path& rhs);
	bool operator!=(const path& lhs, const path& rhs);
	bool operator<(const path& lhs, const path& rhs);
	bool operator>(const path& lhs, const path& rhs);
	path operator/(const path& lhs, const path& rhs);


	class filesystem_error
		: public std::system_error
	{
	public:
		explicit filesystem_error(const std::string& msg, std::error_code);

		filesystem_error(const std::string& msg, const path& path1, std::error_code err);
		filesystem_error(const std::string& msg, const path& path1, const path& path2, std::error_code err);

		const path& path1() const noexcept;
		const path& path2() const noexcept;
		// const char* what() const noexcept;
	private:
		path path1_;
		path path2_;
	};


	class directory_entry
	{
	public:
		directory_entry() = default;
		explicit directory_entry(const ::nana::experimental::filesystem::path&);

		//modifiers
		void assign(const ::nana::experimental::filesystem::path&);
		void replace_filename(const ::nana::experimental::filesystem::path&);

		//observers
		file_status status() const;
		operator const filesystem::path&() const {	return path_;	};
		const filesystem::path& path() const;
	private:
		::nana::experimental::filesystem::path path_;
	};

    /// InputIterator that iterate over the sequence of directory_entry elements representing the files in a directory, not an recursive_directory_iterator
	class directory_iterator 		:public std::iterator<std::input_iterator_tag, directory_entry>
	{
		using find_handle = void*;
	public:

		directory_iterator() noexcept;
		explicit directory_iterator(const path& p);
		directory_iterator(const path& p, directory_options opt);

		const value_type& operator*() const;
		const value_type* operator->() const;

		directory_iterator& operator++();
		directory_iterator operator++(int);  ///< extention

		bool equal(const directory_iterator& x) const;

	private:
		template<typename Char>
		static bool _m_ignore(const Char * p)
		{
			while(*p == '.')
				++p;
			return (*p == 0);
		}

		void _m_prepare(const path& file_path);
		void _m_read();
	private:
		bool	end_{false};
		path::string_type path_;
		directory_options option_{ directory_options::none };

		std::shared_ptr<find_handle> find_ptr_;
		find_handle	handle_{nullptr};
		value_type	value_;
	};
	/// enable directory_iterator range-based for statements
	inline directory_iterator begin( directory_iterator iter) noexcept
	{
		return iter;
	}

	inline directory_iterator end(	const directory_iterator&) noexcept
	{
		return {};
	}


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


	file_status status(const path& p);
	file_status status(const path& p, std::error_code&);

	std::uintmax_t file_size(const path& p);
	std::uintmax_t file_size(const path& p, std::error_code& ec) noexcept;

	inline bool is_directory(file_status s) noexcept
	{ return s.type() == file_type::directory ;}

	bool is_directory(const path& p);
    bool is_directory(const path& p, std::error_code& ec) noexcept;

	inline bool is_regular_file(file_status s) noexcept
	{
		return s.type() == file_type::regular;
	}
	inline bool is_regular_file(const path& p)
	{
		return is_regular_file(status(p));
	}
	// bool is_regular_file(const path& p, error_code& ec) noexcept;
    // Returns: is_regular_file(status(p, ec)).Returns false if an error occurs.

	inline bool is_empty(const path& p)
    {
		auto fs = status(p);

		if (is_directory(fs))
			return (directory_iterator() == directory_iterator(p));

		return (file_size(p) == 0);
    }
    // bool is_empty(const path& p, error_code& ec) noexcept;


	bool create_directories(const path& p);
	//bool create_directories(const path& p, error_code& ec) noexcept;
	bool create_directory(const path& p);
	//bool create_directory(const path& p, error_code& ec) noexcept;
	bool create_directory(const path& p, const path& attributes);
	//bool create_directory(const path& p, const path& attributes,     error_code& ec) noexcept;


	/// The time of last data modification of p, determined as if by the value of the POSIX
    /// stat structure member st_mtime obtained as if by POSIX stat().
	file_time_type last_write_time(const path& p);
	/// returns file_time_type::min() if an error occurs
	//file_time_type last_write_time(const path& p, error_code& ec) noexcept;


	path current_path();
	//path current_path(error_code& ec);
	void current_path(const path& p);   ///< chdir
	//void current_path(const path& p, error_code& ec) noexcept;

	bool remove(const path& p);
	bool remove(const path& p, std::error_code& ec); // noexcept;

	//uintmax_t remove_all(const path& p);
	//uintmax_t remove_all(const path& p, error_code& ec) noexcept;

	template<typename CharType>
	std::basic_string<CharType> parent_path(const std::basic_string<CharType>& path)
	{
		auto index = path.size();

		if (index)
		{
			auto str = path.c_str();

			for (--index; index > 0; --index)
			{
				auto c = str[index];
				if (c != '\\' && c != '/')
					break;
			}

			for (--index; index > 0; --index)
			{
				auto c = str[index];
				if (c == '\\' || c == '/')
					break;
			}
		}

		return index ? path.substr(0, index + 1) : std::basic_string<CharType>();
	}
#ifndef CXX_NO_INLINE_NAMESPACE
} //end namespace v1
#endif
} //end namespace filesystem
} //end namespace experimental

  //namespace filesystem = experimental::filesystem;
} //end namespace nana


namespace std {
	namespace experimental {
		namespace filesystem {

#       ifdef CXX_NO_INLINE_NAMESPACE
			using namespace nana::experimental::filesystem;
#       else
			using namespace nana::experimental::filesystem::v1;
#       endif

		} // filesystem
	} // experimental

	namespace filesystem {
		using namespace std::experimental::filesystem;

#if defined(NANA_FILESYSTEM_FORCE) || \
    (defined(_MSC_VER) && ((!defined(_MSVC_LANG)) || (_MSVC_LANG < 201703)))
		path absolute(const path& p);
		path absolute(const path& p, std::error_code& err);

		path canonical(const path& p);
		path canonical(const path& p, std::error_code& err);

		path weakly_canonical(const path& p);
		path weakly_canonical(const path& p, std::error_code& err);
#endif

#if defined(NANA_FILESYSTEM_FORCE) || defined(NANA_MINGW)
        bool exists( std::filesystem::file_status s ) noexcept;
        bool exists( const std::filesystem::path& p );
        bool exists( const std::filesystem::path& p, std::error_code& ec ) noexcept;
#endif
	}
} // std
#else

//Implements the missing functions for various version of experimental/filesystem
#	if defined(NANA_USING_STD_EXPERIMENTAL_FILESYSTEM)
	namespace std
	{
		namespace filesystem
		{
			//Visual Studio 2017
			#if (defined(_MSC_VER) && (_MSC_VER > 1912)) ||	\
				(!defined(__clang__) && defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__ < 801))
			path weakly_canonical(const path& p);
			path weakly_canonical(const path& p, std::error_code& err);
			#endif
		}
	}
#	endif

#endif	//NANA_USING_NANA_FILESYSTEM

#include <nana/pop_ignore_diagnostic>
#endif	//NANA_FILESYSTEM_HPP
