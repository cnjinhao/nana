/**
 *	A Panel Implementation
 *	Nana C++ Library(http://www.nanaro.org)
 *	Copyright(C) 2003-2017 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/panel.hpp
 *	@author: Jinhao
 *	@contributors: Ariel Vina-Rodriguez
 *
 *	@brief panel is a widget used for placing some widgets.
 */

#ifndef NANA_GUI_WIDGETS_PANEL_HPP
#define NANA_GUI_WIDGETS_PANEL_HPP
#include "widget.hpp"
#include <type_traits>

namespace nana
{
	namespace drawerbase
	{
		namespace panel
		{
			class drawer: public drawer_trigger
			{
				void attached(widget_reference, graph_reference)	override;
				void refresh(graph_reference)	override;
			private:
				window window_{nullptr};
			};
		}// end namespace panel
	}//end namespace drawerbase
    /// For placing other widgets, where the bool template parameter determines if it is widget or lite_widget, which in actual use makes no difference.
	template<bool HasBackground>
	class panel
		: public widget_object<typename std::conditional<HasBackground, category::widget_tag, category::lite_widget_tag>::type,
								drawerbase::panel::drawer>
	{
	public:
		panel(){}

		panel(window wd, bool visible)
		{
			this->create(wd, rectangle(), visible);
			this->bgcolor(API::bgcolor(wd));
		}

		panel(window wd, const nana::rectangle& r = rectangle(), bool visible = true)
		{
			this->create(wd, r, visible);
			this->bgcolor(API::bgcolor(wd));
		}

		bool transparent() const
		{
			return API::is_transparent_background(*this);
		}

		void transparent(bool tr)
		{
			if(tr)
				API::effects_bground(*this, effects::bground_transparent(0), 0);
			else
				API::effects_bground_remove(*this);
		}
	};
}//end namespace nana
#endif
