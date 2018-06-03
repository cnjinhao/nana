/*
*	Widget Content Measurer Interface
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2017 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file: nana/gui/detail/widget_content_measurer_interface.hpp
*/

#ifndef NANA_WIDGET_CONTENT_MEASURER_INTERFACE_HEADER_INCLUDED
#define NANA_WIDGET_CONTENT_MEASURER_INTERFACE_HEADER_INCLUDED

#include <nana/basic_types.hpp>
#include <nana/optional.hpp>
#include <nana/paint/graphics.hpp>

namespace nana
{
	namespace dev
	{
		/// An interface for measuring content of the widget
		class widget_content_measurer_interface
		{
		public:
			using graph_reference = paint::graphics&;
			virtual ~widget_content_measurer_interface() = default;

			/// Measures content
			/**
			 * @param graph The graphics for the operation.
			 * @param limit_pixels The number of pixels of the limited edge. If this parameter is zero, it is ignored.
			 * @param limit_width True if limits the width, false if limits the height.
			 * @return the size of content.
			 */
			virtual ::std::optional<size> measure(graph_reference graph, unsigned limit_pixels, bool limit_width) const = 0;

			/// Returns the extension to the size of widget from content extent
			/**
			 * @return the width and height of extension to the widget size.
			 */
			virtual size extension() const = 0;
		};
	}
}
#endif
