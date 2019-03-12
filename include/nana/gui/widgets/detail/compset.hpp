/*
 *	Concept of Component Set
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/detail/compset.hpp
 */

#ifndef NANA_GUI_WIDGETS_DETAIL_COMPSET_HPP
#define NANA_GUI_WIDGETS_DETAIL_COMPSET_HPP

#include <nana/basic_types.hpp>

namespace nana{	namespace widgets{ namespace detail
{
	/// A component set used for accessing the components of items of a widget.
	template<typename Component, typename ItemAttribute>
	class compset
	{
	public:
		/// A type of widget-defined components.
		typedef Component component_t;

		/// A type of widget-defined item attribute.
		typedef ItemAttribute item_attribute_t;

		/// A type of a componenet state for rendering.
		struct comp_attribute_t
		{
			nana::rectangle area;
			bool mouse_pointed;
		};

	public:
		/// The destrcutor
		virtual ~compset(){}

		/// Access the widget-defined item attribute.
		virtual const item_attribute_t& item_attribute() const = 0;

		/// Access a specified component of the item.
		virtual bool comp_attribute(component_t, comp_attribute_t &) const = 0;
	};

	/// A component set placer used for specifying component position and size.
	template<typename Component, typename ItemAttribute, typename WidgetScheme>
	class compset_placer
	{
	public:
		typedef ::nana::paint::graphics & graph_reference;
		/// A type of widget-defined components.
		typedef Component component_t;

		/// A type of widget-defined item attribute.
		typedef ItemAttribute item_attribute_t;

		/// Widget scheme.
		typedef WidgetScheme widget_scheme_t;
	public:
		/// The destructor.
		virtual ~compset_placer(){}

		/// Enable/Disable the specified component.
		virtual void enable(component_t, bool) = 0;
		virtual bool enabled(component_t) const = 0;

		/// Height of an item, in pixels.
		//	this method is content-indepented, this feature is easy for implementation.
		virtual unsigned item_height(graph_reference) const = 0;

		/// Width of an item, in pixels
		virtual unsigned item_width(graph_reference, const item_attribute_t&) const = 0;

		/// Locate a component through the specified coordinate.
		/// @param comp the component of the item.
		/// @param attr the attribute of the item.
		/// @param r the pointer refers to a rectangle for receiving the position and size of the component.
		/// @returns the true when the component is located by the locator.
		virtual bool locate(component_t comp, const item_attribute_t& attr, rectangle * r) const = 0;
	};
}//end namespace detail
}//end namespace widgets
}//end namespace nana
#endif
