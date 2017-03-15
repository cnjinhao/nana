/*
 *	A Generic Abstract Factory Pattern Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/pat/abstract_factory.hpp
 *	@description: A generic easy-to-use abstract factory pattern implementation
 */

#ifndef NANA_PAT_ABSFACTORY_HPP
#define NANA_PAT_ABSFACTORY_HPP
#include "cloneable.hpp"
#include <memory>
#include <tuple>
#include <type_traits>

namespace nana
{
	namespace pat
	{
		namespace detail
		{
			//A Base class for abstract factory, avoids decorated name length exceeding for a class template.
			class abstract_factory_base
			{
			public:
				virtual ~abstract_factory_base() = default;
			};
		}

		template<typename Interface>
		class abstract_factory
			: public detail::abstract_factory_base
		{
		public:
			using interface_type = Interface;

			virtual ~abstract_factory() = default;
			virtual std::unique_ptr<interface_type> create() = 0;
		};

		namespace detail
		{
			template<typename Useless, std::size_t ...Index>
			struct pack{
				using type = pack;
			};

			template<bool Negative, bool Zero, class IntConst, class Pack>
			struct make_pack_helper
			{	// explodes gracefully below 0
				static_assert(!Negative,
					"make_integer_sequence<T, N> requires N to be non-negative.");
			};

			template<typename Useless, std::size_t ... Vals>
			struct make_pack_helper<false, true,
				std::integral_constant<std::size_t, 0>,
				pack<Useless, Vals...> >
				: pack<Useless, Vals...>
			{	// ends recursion at 0
			};

			template<typename Useless, std::size_t X, std::size_t... Vals>
			struct make_pack_helper<false, false,
				std::integral_constant<std::size_t, X>,
				pack<Useless, Vals...> >
				: make_pack_helper<false, X == 1,
				std::integral_constant<std::size_t, X - 1>,
				pack<Useless, X - 1, Vals...> >
			{	// counts down to 0
			};

			template<std::size_t Size>
			using make_pack = typename make_pack_helper<Size < 0, Size == 0, std::integral_constant<std::size_t, Size>, pack<int> >::type;

			template<typename T, typename Interface, typename ...Args>
			class abs_factory
				: public abstract_factory<Interface>
			{
				std::unique_ptr<Interface> create() override
				{
					constexpr auto const Size = std::tuple_size<decltype(args_)>::value;
					return std::unique_ptr<Interface>{ _m_new(make_pack<Size>{}) };
				}

				template<std::size_t ... Index>
				Interface* _m_new(const pack<int, Index...> &)
				{
					return new T(std::get<Index>(args_)...);
				}
			public:
				abs_factory(const Args&... args)
					: args_(args...)
				{
				}
			private:
				std::tuple<Args...> args_;
			};
		}//end namespace detail

		template<typename Type, typename ...Args>
		pat::cloneable<abstract_factory<typename Type::factory_interface>> make_factory(Args &&... args)
		{
			return detail::abs_factory<Type, typename Type::factory_interface, typename std::decay<Args>::type...>(std::forward<Args>(args)...);
		}
	}//end namespace pat
}//end namespace nana
#endif
