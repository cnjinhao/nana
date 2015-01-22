/*
 *	A Spin box widget
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/spanbox.hpp
 */

#ifndef NANA_GUI_WIDGET_SPINBOX_HPP
#define NANA_GUI_WIDGET_SPINBOX_HPP
#include "widget.hpp"
#include "skeletons/text_editor_scheme.hpp"

namespace nana
{
	namespace drawerbase
	{
		namespace spinbox
		{
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
				void refresh(graph_reference)	override;

				void focus(graph_reference, const arg_focus&)	override;
				void mouse_wheel(graph_reference, const arg_wheel&) override;
				void mouse_down(graph_reference, const arg_mouse&)	override;
				void mouse_move(graph_reference, const arg_mouse&)	override;
				void mouse_up(graph_reference, const arg_mouse& arg)	override;
				void mouse_leave(graph_reference, const arg_mouse&)	override;
				void key_press(graph_reference, const arg_keyboard&) override;
				void key_char(graph_reference, const arg_keyboard&) override;
			private:
				implementation * const impl_;
			};
		};
	}//end namespace drawerbase

	/// Spinbox Widget
	class spinbox
		: public widget_object <category::widget_tag, drawerbase::spinbox::drawer, ::nana::general_events, ::nana::widgets::skeletons::text_editor_scheme>
	{
	public:
		/// Constructs a spinbox.
		spinbox();
		spinbox(window, bool visible);
		spinbox(window, const nana::rectangle& = {}, bool visible = true);

		/// Sets the widget whether it accepts user keyboard input.
		/// @param accept Set to indicate whether it accepts uesr keyboard input.
		void editable(bool accept);

		/// Determines whether the widget accepts user keyboard input.
		bool editable() const;

		/// Sets the numeric spin values and step.
		void range(int begin, int last, int step);
		void range(double begin, double last, double step);

		/// Sets the string spin values.
		void range(std::initializer_list<std::string> steps_utf8);
		void range(std::initializer_list<std::wstring> steps);

		/// Sets a predicator that determines whether accepts the current user input.
		/// @param pred Predicator to determines the input.
		void set_accept(std::function<bool(::nana::char_t)> pred);

		/// Sets the spinbox that only accepts integer input.
		void set_accept_integer();

		/// Sets the spinbox that only accepts real number input.
		void set_accept_real();

		/// Removes the accept excluding predicate accept.
		void remove_accept();

		/// Sets the qualifications
		void qualify(std::wstring prefix, std::wstring suffix);
		void qualify(const std::string & prefix_utf8, const std::string& suffix_utf8);
	private:
		::nana::string _m_caption() const;
		void _m_caption(::nana::string&&);
	};
}//end namespace nana

#endif //NANA_GUI_WIDGET_SPINBOX_HPP