/**
 *	A Spin box widget
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2017 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/spinbox.hpp
 */

#ifndef NANA_GUI_WIDGET_SPINBOX_HPP
#define NANA_GUI_WIDGET_SPINBOX_HPP

#include <nana/push_ignore_diagnostic>
#include "widget.hpp"
#include "skeletons/text_editor_part.hpp"

namespace nana
{
	class spinbox;

	struct arg_spinbox
		: public event_arg
	{
		spinbox & widget;
		arg_spinbox(spinbox&);
	};

	namespace drawerbase
	{
		namespace spinbox
		{
			struct spinbox_events
				: public general_events
			{
				basic_event<arg_spinbox> text_changed;
			};

			/// Declaration of internal spinbox implementation
			class implementation;

			/// Drawer of spinbox
			class drawer
				: public ::nana::drawer_trigger
			{
				drawer(const drawer&) = delete;
				drawer(drawer&&) = delete;
				drawer& operator=(const drawer&) = delete;
				drawer& operator=(drawer&&) = delete;
			public:
				drawer();
				~drawer();
				implementation * impl() const;
			private:
				//Overrides drawer_trigger
				void attached(widget_reference, graph_reference) override;
				void detached() override;
				void refresh(graph_reference)	override;

				void focus(graph_reference, const arg_focus&)	override;
				void mouse_wheel(graph_reference, const arg_wheel&) override;
				void dbl_click(graph_reference, const arg_mouse&) override;
				void mouse_down(graph_reference, const arg_mouse&)	override;
				void mouse_move(graph_reference, const arg_mouse&)	override;
				void mouse_up(graph_reference, const arg_mouse& arg)	override;
				void mouse_leave(graph_reference, const arg_mouse&)	override;
				void key_ime(graph_reference, const arg_ime& arg) override;
				void key_press(graph_reference, const arg_keyboard&) override;
				void key_char(graph_reference, const arg_keyboard&) override;
				void resized(graph_reference, const arg_resized&) override;
			private:
				implementation * const impl_;
			};
		}
	}//end namespace drawerbase

	/// Spinbox Widget
	class spinbox
		: public widget_object <category::widget_tag,
		                        drawerbase::spinbox::drawer,
		                        drawerbase::spinbox::spinbox_events,
		                        ::nana::widgets::skeletons::text_editor_scheme>
	{
	public:
		/// Constructs a spinbox.
		spinbox();
		spinbox(window, bool visible);
		spinbox(window, const nana::rectangle& = {}, bool visible = true);

		/// Sets the widget whether it accepts user keyboard input.
		/// @param accept Set to indicate whether it accepts user keyboard input.   
		void editable(bool accept);

		/// Determines whether the widget accepts user keyboard input.
		bool editable() const;

		/// Sets the numeric spin values and step.
		void range(int begin, int last, int step);
		void range(double begin, double last, double step);

		/// Sets the string spin values.
		void range(std::vector<std::string> values_utf8);

		std::vector<std::string> range_string() const;
		std::pair<int, int> range_int() const;
		std::pair<double, double> range_double() const;

		/// Selects/Deselects the text
		void select(bool);

		/// Gets the spun value
		::std::string value() const;
		void value(const ::std::string&);
		int to_int() const;
		double to_double() const;

		/// Sets the modifiers
		void modifier(std::string prefix_utf8, std::string suffix_utf8);
		void modifier(const std::wstring & prefix, const std::wstring& suffix);
	private:
		native_string_type _m_caption() const noexcept;
		void _m_caption(native_string_type&&);
	}; //end class spinbox
}//end namespace nana
#include <nana/pop_ignore_diagnostic>
#endif //NANA_GUI_WIDGET_SPINBOX_HPP
