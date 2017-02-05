/*
*	Widget Content Measurer Interface
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
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

namespace nana
{
	namespace dev
	{
		/// An interface for measuring content of the widget
		class widget_content_measurer_interface
		{
		public:
			virtual ~widget_content_measurer_interface() = default;

			/// Measures content
			/**
			* @param limit_width true if limits the width, false if limits the height.
			* @param limit_pixels the number of pixels of the limited edge. If this parameter is zero, it is ignored
			* @return the size of content
			*/
			virtual optional<size> measure(bool limit_width, unsigned limit_pixels) const = 0;
		};
	}
}
#endif
