/**
 *	A Panel Implementation
 *	Nana C++ Library(http://www.nanaro.org)
 *	Copyright(C) 2003-2020 Jinhao(cnjinhao@hotmail.com)
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
	namespace drawerbase::panel
	{
			class drawer: public drawer_trigger
			{
				void attached(widget_reference, graph_reference)	override;
				void refresh(graph_reference)	override;
			private:
				window window_{nullptr};
			};
	}//end namespace drawerbase::panel
	
    /// For placing other widgets, where the bool template parameter determines if it is widget or lite_widget, which in actual use makes no difference.
	template<bool HasBackground>
	class panel
		: public widget_object<typename std::conditional<HasBackground, category::widget_tag, category::lite_widget_tag>::type,
								drawerbase::panel::drawer>
	{
	public:
		panel() = default;

		panel(window wd, const nana::rectangle& r = rectangle(), bool visible = true)
		{
			this->create(wd, r, visible);
		}

		bool transparent() const
		{
			return api::is_transparent_background(*this);
		}

		void transparent(bool tr)
		{
			if(tr)
				api::effects_bground(*this, effects::bground_transparent(0), 0);
			else
				api::effects_bground_remove(*this);
		}
	private:
		void _m_complete_creation() override
		{
			if(HasBackground)
				this->bgcolor(api::bgcolor(this->parent()));
		}
	};
}//end namespace nana
#endif
