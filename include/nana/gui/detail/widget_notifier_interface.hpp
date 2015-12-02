/*
 *	Widget Notifier Interface
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/detail/widget_notifier_interface.hpp
 */

#ifndef NANA_GUI_DETAIL_WIDGET_NOTIFIER_INTERFACE_HEADER
#define NANA_GUI_DETAIL_WIDGET_NOTIFIER_INTERFACE_HEADER
#include <memory>
#include <string>

namespace nana
{
	class widget;	//forward declaration

	namespace detail
	{
		class widget_notifier_interface
		{
		public:
			virtual ~widget_notifier_interface() = default;

			static std::unique_ptr<widget_notifier_interface> get_notifier(widget*);	//defined in nana/gui/widgets/widget.cpp

			virtual widget* widget_ptr() const = 0;
			virtual void destroy() = 0;
			virtual ::nana::detail::native_string_type caption() = 0;
			virtual void caption(::nana::detail::native_string_type) = 0;
		};
	}
}

#endif