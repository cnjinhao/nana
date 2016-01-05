/*
 *	Traits Implementation
 *	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
 *
 *	@file: nana/traits.hpp
 */

#ifndef NANA_TRAITS_HPP
#define NANA_TRAITS_HPP
#include <type_traits>

namespace nana
{
	class null_type{};

	/// Prevent a class to be copyable
	class noncopyable
	{
		noncopyable(const noncopyable&) = delete;
		noncopyable& operator=(const noncopyable&) = delete;
	protected:
		noncopyable() = default;
	};

	/// Prevent a class to be movable
	class nonmovable
	{
		nonmovable(nonmovable&&) = delete;
		nonmovable& operator=(nonmovable&&) = delete;
	protected:
		nonmovable() = default;
	};

	namespace meta
	{
		template<	typename Param0 = null_type, typename Param1 = null_type,
					typename Param2 = null_type, typename Param3 = null_type,
					typename Param4 = null_type, typename Param5 = null_type,
					typename Param6 = null_type, typename Param7 = null_type,
					typename Param8 = null_type, typename Param9 = null_type>
		struct fixed_type_set
		{
			template<typename T>
			struct count
			{
				enum{value =	std::is_same<Param0, T>::value +
								std::is_same<Param1, T>::value +
								std::is_same<Param2, T>::value +
								std::is_same<Param3, T>::value +
								std::is_same<Param4, T>::value +
								std::is_same<Param5, T>::value +
								std::is_same<Param6, T>::value +
								std::is_same<Param7, T>::value +
								std::is_same<Param8, T>::value +
								std::is_same<Param9, T>::value};
			};
		};
	}//end namespace meta
}//end namespace nana

#endif
