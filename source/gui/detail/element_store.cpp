/*
*	The Store for the Storage Of Elements
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2014 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file: nana/gui/detail/element_store.cpp
*/
#include <nana/gui/detail/element_store.hpp>

namespace nana
{
namespace detail
{
	//class element_store
	element_store::data::data()
		: fast_ptr(nullptr)
	{}

	nana::element::element_interface * const * element_store::bground(const std::string& name)
	{
		element_interface * const * addr = &(bground_.table[name].fast_ptr);
		return addr;
	}

	void element_store::bground(const std::string& name, const pat::cloneable<element_interface>& rhs)
	{
		auto & store = bground_.table[name];

		store.object = rhs;
		store.fast_ptr = store.object.get();
	}

	void element_store::bground(const std::string& name, pat::cloneable<element_interface>&& rv)
	{
		auto & store = bground_.table[name];

		store.object = std::move(rv);
		store.fast_ptr = store.object.get();
	}
}//end namespace detail
}
