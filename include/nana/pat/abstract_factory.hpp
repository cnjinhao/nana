/*
 *	A Generic Abstract Factory Pattern Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2014 Jinhao(cnjinhao@hotmail.com)
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
			template<typename T, typename Interface>
			class abs_factory
				: public abstract_factory<Interface>
			{
				std::unique_ptr<Interface> create() override
				{
					return std::unique_ptr<Interface>{ new T };
				}
			};
		}//end namespace detail

		template<typename Type>
		pat::cloneable<abstract_factory<typename Type::factory_interface>> make_factory()
		{
			return detail::abs_factory<Type, typename Type::factory_interface>();
		}
	}//end namespace pat
}//end namespace nana
#endif
