/**
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
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

#include <iomanip>
#include <nana/filesystem/filesystem_selector.hpp>

namespace nana {namespace experimental {namespace filesystem {namespace ext {
#if defined(NANA_WINDOWS)
    constexpr auto def_root = "C:";
    constexpr auto def_rootstr = "C:\\";
    constexpr auto def_rootname = "Local Drive(C:)";
#elif defined(NANA_LINUX)
    constexpr auto def_root = "/";
    constexpr auto def_rootstr = "/";
    constexpr auto def_rootname = "Root/";
#endif

// nana::experimental::filesystem::path_user());    

inline bool is_directory(const std::experimental::filesystem::directory_entry& dir) noexcept
{
    return is_directory(dir.status());
}


//template<class DI> // DI = directory_iterator from std, boost, or nana : return directory_entry
class directory_only_iterator : public std::experimental::filesystem::directory_iterator
{ 
	using DI = std::experimental::filesystem::directory_iterator;
	directory_only_iterator& find_first()
   {
	   auto end = directory_only_iterator{};
	   while (*this != end)
	   {
		   if (is_directory((**this).status())) 
			   return *this;
		   this->DI::operator++();
	   }
       return *this;
   }
public:
    template <class... Arg>
    directory_only_iterator(Arg&&... arg ): DI(std::forward<Arg>(arg)...)
    {
        find_first();
    }
	directory_only_iterator( ) {}
    directory_only_iterator& operator++()
    {
        this->DI::operator++();
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
class regular_file_only_iterator : public std::experimental::filesystem::directory_iterator
{
	using DI = std::experimental::filesystem::directory_iterator;
	regular_file_only_iterator& find_first()
    {
        while(( (*this) != DI{}) && !is_regular_file((**this).status()))
            this->DI::operator++();
        return (*this);
    }
public:
    template <class... Arg>
    regular_file_only_iterator(Arg&&... arg ): DI(std::forward<Arg>(arg)...)
    {
            find_first();
    }
	regular_file_only_iterator() : DI() {}
    regular_file_only_iterator& operator++()
    {
        this->DI::operator++();
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

inline std::string pretty_file_size(const std::experimental::filesystem::path& path) // todo: move to .cpp
{
    try {
        std::size_t bytes = std::experimental::filesystem::file_size ( path );
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
    return {};
}

inline std::string pretty_file_date(const std::experimental::filesystem::path& path) // todo: move to .cpp
{
    try {
        auto ftime = std::experimental::filesystem::last_write_time(path);
        std::time_t cftime = decltype(ftime)::clock::to_time_t(ftime);
        std::stringstream tm;
        tm << std::put_time(std::localtime(&cftime), "%Y-%m-%d, %H:%M:%S");
        return tm.str();
    }
    catch (...) {
        return {};
    }
}

}}}}
#endif //NANA_FILESYSTEM_EXT_HPP
