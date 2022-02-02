/**
 *	A date chooser Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2022 Jinhao(cnjinhao@hotmail.com)
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

	namespace drawerbase::date_chooser
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
	}//end namespace drawerbase::date_chooser

	/// class date_chooser represents a calendar.
	/**
	 * i18n texts used by date_chooser:
	 *	"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December",
	 *	"SUNDAY_SHORT", "MONDAY_SHORT", "TUESDAY_SHORT", "WEDNESDAY_SHORT", "THURSDAY_SHORT","FRIDAY_SHORT", "SATURDAY_SHORT"
	 */
	class date_chooser
		: public widget_object<category::widget_tag, drawerbase::date_chooser::trigger, drawerbase::date_chooser::date_chooser_events>
	{
	public:
		date_chooser();
		date_chooser(window, bool visible);
		date_chooser(window, const nana::rectangle& r = rectangle(), bool visible = true);

		bool chose() const;

		/// Sets transforming animation frames and time
		/**
		 * @param frame_count The number of frames
		 * @param duration The time of a frame.
		 */
		void transform_duration(std::size_t frame_count, std::int64_t duration);
		nana::date read() const;
		void weekstr(unsigned index, std::string);///<Set the week strings which will be displayed for day, index is in the range of [0, 6]
#ifdef __cpp_char8_t
		void weekstr(unsigned index, std::u8string_view);
#endif
	};
}//end namespace nana
#include <nana/pop_ignore_diagnostic>

#endif
