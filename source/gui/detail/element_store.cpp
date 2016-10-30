/*
*	The Store for the Storage Of Elements
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file: nana/gui/detail/element_store.cpp
*/
#include <nana/gui/detail/element_store.hpp>

#include <map>

namespace nana
{
namespace detail
{
	//class element_store

	struct element_store::data
	{
		cloneable_element entity;
		::nana::element::element_interface * fast_ptr{ nullptr };
	};

	struct element_store::implementation
	{
		std::map<std::string, data> table;
	};

	element_store::element_store()
		: impl_(new implementation)
	{}

	//Empty destructor for instance of impl
	element_store::~element_store()
	{}

	nana::element::element_interface * const * element_store::bground(const std::string& name)
	{
		element_interface * const * addr = &(impl_->table[name].fast_ptr);
		return addr;
	}

	void element_store::bground(const std::string& name, const pat::cloneable<element_interface>& rhs)
	{
		auto & store = impl_->table[name];

		store.entity = rhs;
		store.fast_ptr = store.entity.get();
	}

	void element_store::bground(const std::string& name, pat::cloneable<element_interface>&& rv)
	{
		auto & store = impl_->table[name];

		store.entity = std::move(rv);
		store.fast_ptr = store.entity.get();
	}
}//end namespace detail
}
