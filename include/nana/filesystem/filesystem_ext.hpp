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

// nana::experimental::filesystem::path_user());   //  REPLACE !!!!!!!!!! to filesystem_ext.hhp

inline bool is_directory(const directory_entry& d) noexcept
{
    return is_directory(d.status());
}


template<class DI> // DI = directory_iterator from std, boost, or nana : return directory_entry
class directory_only_iterator : public DI
{
   DI& find_first()
   {
       while(( (*this) != DI{}) || !is_directory((*this)->status()) )
           this->DI::operator++();
       return (*this);
   }
public:
    template <class... Arg>
    directory_only_iterator(Arg&&... arg ): DI(std::forward<Arg>(arg)...)
    {
        find_first();
    }
    directory_only_iterator& operator++()
    {
        this->DI::operator++();
        return find_first();
    }
};

template<class DI> // DI = directory_iterator from std, boost, or nana : value_type directory_entry
class regular_file_only_iterator : public DI
{
    DI& find_first()
    {
        while(( (*this) != DI{}) || !is_regular_file((*this)->status()) )
            this->DI::operator++();
        return (*this);
    }
public:
    template <class... Arg>
    regular_file_only_iterator(Arg&&... arg ): DI(std::forward<Arg>(arg)...)
    {
            find_first();
    }
    regular_file_only_iterator& operator++()
    {
        this->DI::operator++();
        return find_first();
    }
};

    }}}}
#endif //NANA_FILESYSTEM_EXT_HPP
