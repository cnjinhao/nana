/**
 *	A date chooser Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/date_chooser.hpp
 */

#ifndef NANA_GUI_WIDGETS_DATE_CHOOSER_HPP
#define NANA_GUI_WIDGETS_DATE_CHOOSER_HPP
#include <nana/push_ignore_diagnostic>
#include "widget.hpp"
#include <nana/datetime.hpp>

namespace nana
{
	class date_chooser;

	struct arg_datechooser
		: public event_arg
	{
		date_chooser * const widget;

		arg_datechooser(date_chooser* wdg)
			: widget(wdg)
		{}
	};

	namespace drawerbase
	{
		namespace date_chooser
		{
			struct date_chooser_events
				: public general_events
			{

				basic_event<arg_datechooser> date_changed;
			};
			
			class trigger : public drawer_trigger
			{
				class model;
			public:
				static const int topbar_height = 34;
				static const int border_size = 3;

				trigger();
				~trigger();
				model* get_model() const;
			private:
				void refresh(graph_reference) override;
				void mouse_move(graph_reference, const arg_mouse&)	override;
				void mouse_leave(graph_reference, const arg_mouse&)	override;
				void mouse_up(graph_reference, const arg_mouse&)	override;
				void key_press(graph_reference, const arg_keyboard&) override;
			private:
				model * model_;
			};
		
		}//end namespace date_chooser
	
	}//end namespace drawerbase

	/// \see nana::date
	class date_chooser
		: public widget_object<category::widget_tag, drawerbase::date_chooser::trigger, drawerbase::date_chooser::date_chooser_events>
	{
	public:
		date_chooser();
		date_chooser(window, bool visible);
		date_chooser(window, const nana::rectangle& r = rectangle(), bool visible = true);

		bool chose() const;
		nana::date read() const;
		void weekstr(unsigned index, ::std::string);///<Set the week strings which will be displayed for day, index is in the range of [0, 6]
	};
}//end namespace nana
#include <nana/pop_ignore_diagnostic>

#endif
