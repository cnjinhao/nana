/**
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2018 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file nana\filesystem\filesystem_ext.hpp
*   @autor by Ariel Vina-Rodriguez:
*	@brief Some convenient extensions to the filesystem library.
*
*/

#ifndef NANA_FILESYSTEM_EXT_HPP
#define NANA_FILESYSTEM_EXT_HPP

#include <nana/filesystem/filesystem.hpp>
#include <nana/deploy.hpp>

namespace nana 
{
namespace filesystem_ext
{

#if defined(NANA_WINDOWS)
    constexpr auto const def_root = "C:";
    constexpr auto const def_rootstr = "C:\\";
    constexpr auto const def_rootname = "Local Drive(C:)";
#elif defined(NANA_POSIX)
    constexpr auto const def_root = "/";
    constexpr auto const def_rootstr = "/";
    constexpr auto const def_rootname = "Root/";
#endif

std::filesystem::path path_user();    ///< extention ?

													/// workaround Boost not having path.generic_u8string() - a good point for http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0251r0.pdf
inline std::string generic_u8string(const std::filesystem::path& p)
{
#if NANA_USING_BOOST_FILESYSTEM
	return nana::to_utf8(p.generic_wstring());
#else
	return p.generic_u8string();
#endif
}

inline bool is_directory(const std::filesystem::directory_entry& dir) noexcept
{
    return is_directory(dir.status());
}

//template<class DI> // DI = directory_iterator from std, boost, or nana : return directory_entry
class directory_only_iterator : public std::filesystem::directory_iterator
{
	using directory_iterator = std::filesystem::directory_iterator;

	directory_only_iterator& find_first()
	{
		auto end = directory_only_iterator{};
		while (*this != end)
		{
			if (is_directory((**this).status()))
				return *this;
			this->directory_iterator::operator++();
		}
		return *this;
	}
public:
	directory_only_iterator() = default;

	template <typename Arg, typename... Args>
	directory_only_iterator(Arg&& arg, Args&&... args) : directory_iterator(arg, std::forward<Args>(args)...)
	{
		find_first();
	}

    directory_only_iterator& operator++()
    {
		this->directory_iterator::operator++();
        return find_first();
    }
};

inline directory_only_iterator begin(directory_only_iterator iter) noexcept
{
	return iter;
}

inline directory_only_iterator end(const directory_only_iterator&) noexcept
{
	return{};
}

//template<class DI> // DI = directory_iterator from std, boost, or nana : value_type directory_entry
class regular_file_only_iterator : public std::filesystem::directory_iterator
{
	using directory_iterator = std::filesystem::directory_iterator;
	regular_file_only_iterator& find_first()
	{
		while (((*this) != directory_iterator{}) && !is_regular_file((**this).status()))
			this->directory_iterator::operator++();
		return (*this);
	}
public:
	regular_file_only_iterator() = default;

	template <typename Arg, typename... Args>
	regular_file_only_iterator(Arg&& arg, Args&&... args) : directory_iterator(std::forward<Arg>(arg), std::forward<Args>(args)...)
	{
		find_first();
	}

	regular_file_only_iterator& operator++()
	{
		this->directory_iterator::operator++();
		return find_first();
	}
};

inline regular_file_only_iterator begin(regular_file_only_iterator iter) noexcept
{
	return iter;
}

inline regular_file_only_iterator end(const regular_file_only_iterator&) noexcept
{
	return{};
}

std::string pretty_file_size(const std::filesystem::path& path);

std::string pretty_file_date(const std::filesystem::path& path);

bool modified_file_time(const std::filesystem::path& p, struct tm&);    ///< extention ?

}  // filesystem_ext
}  // nana

#endif //NANA_FILESYSTEM_EXT_HPP
