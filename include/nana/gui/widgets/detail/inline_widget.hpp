/**
 *	A Inline Widget Interface Definition
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/detail/inline_widget.hpp
 *
 */

#ifndef NANA_GUI_INLINE_WIDGETS_HPP
#define NANA_GUI_INLINE_WIDGETS_HPP
#include "../../basis.hpp"

namespace nana
{
	namespace detail
	{
		template<typename Index, typename Value>
		class inline_widget_indicator
		{
		public:
			using index_type = Index;
			using value_type = Value;
			virtual ~inline_widget_indicator() = default;
		private:
			virtual void modify(index_type, const value_type&) const = 0;
		};

		template<typename Index, typename Value>
		class inline_widget_interface
		{
		public:
			using index_type = Index;
			using value_type = Value;
			using inline_indicator = inline_widget_indicator<index_type, value_type>;
			using factory_interface = inline_widget_interface;

			virtual ~inline_widget_interface() = default;

			virtual void create(window) = 0;
			virtual void activate(inline_indicator&, index_type) = 0;
			virtual void resize(const size&) = 0;
			virtual void set(const value_type&) = 0;
		};
	}
}

#endif