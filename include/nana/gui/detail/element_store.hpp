/*
*	The Store for the Storage Of Elements
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file: nana/gui/detail/element_store.hpp
*/

#ifndef NANA_GUI_DETAIL_ELEMENT_STORE_HPP
#define NANA_GUI_DETAIL_ELEMENT_STORE_HPP

#include <nana/gui/element.hpp>
#include <nana/pat/cloneable.hpp>

#include <string>
#include <nana/push_ignore_diagnostic>


namespace nana
{
namespace detail
{
	class element_store
	{
		using element_interface = ::nana::element::element_interface;
		using cloneable_element = pat::cloneable< ::nana::element::element_interface>;

		struct data;
	public:
		element_store();
		~element_store();

		element_interface * const * bground(const std::string&);
		void bground(const std::string&, const pat::cloneable<element_interface>&);
		void bground(const std::string&, pat::cloneable<element_interface>&&);
	private:
		struct implementation;
		std::unique_ptr<implementation> impl_;
	};
}//end namespace detail
}
#include <nana/pop_ignore_diagnostic>

#endif
