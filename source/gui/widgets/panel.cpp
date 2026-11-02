/*
 *	A Panel Implementation
 *	Copyright(C) 2003-2020 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: source/gui/widgets/panel.cpp
 *
 *	@brief: panel is a widget used for placing some widgets.
 */

#include <nana/gui/widgets/panel.hpp>

namespace nana
{
	namespace drawerbase::panel
	{
		//class drawer
		void drawer::attached(widget_reference wdg, graph_reference)
		{
			wdg.caption("panel widget");
			window_ = wdg.handle();

			api::ignore_mouse_focus(wdg, true);

		}

		void drawer::refresh(graph_reference graph)
		{
			if (!api::dev::copy_transparent_background(window_, graph))
				graph.rectangle(true, api::bgcolor(window_));
		}
		//end class drawer
	}//end namespace drawerbase::panel
}//end namespace nana
