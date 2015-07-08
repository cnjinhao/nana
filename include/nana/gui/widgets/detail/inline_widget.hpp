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
			/// A type to index a item
			using index_type = Index;

			/// A type to the value of the item
			using value_type = Value;

			/// The destructor
			virtual ~inline_widget_indicator() = default;

			/// Returns the host widget of the indicator
			virtual ::nana::widget& host() const = 0;

			/// Modifies the value of a item specified by pos
			virtual void modify(index_type pos, const value_type&) const = 0;

			/// Sends a signal that a specified item is selected
			virtual void selected(index_type) = 0;

			/// Sends a signal that a specified item is hovered
			virtual void hovered(index_type) = 0;
		};

		template<typename Index, typename Value>
		class inline_widget_notifier_interface
		{
		public:
			/// A type to index a item
			using index_type = Index;

			/// A type to the value of the item
			using value_type = Value;

			/// A typedef name of a inline widget indicator
			using inline_indicator = inline_widget_indicator<index_type, value_type>;

			/// A type to the notifier interface that will be refered by the abstract factory pattern
			using factory_interface = inline_widget_notifier_interface;

			/// The destructor
			virtual ~inline_widget_notifier_interface() = default;

			/// A message to create the inline widget
			virtual void create(window) = 0;

			/// A message to activate the inline widget to attach a specified item
			virtual void activate(inline_indicator&, index_type) = 0;

			/// A message to resize the inline widget
			virtual void resize(const size&) = 0;

			/// A message to set the value from the item
			virtual void set(const value_type&) = 0;

			/// Determines whether to draw the background of the widget
			virtual bool whether_to_draw() const = 0;
		};	//end class inline_widget_notifier_interface
	}
}

#endif