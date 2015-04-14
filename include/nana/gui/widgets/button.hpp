/**
 *	A Button Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *  @file: nana/gui/widgets/button.hpp
 */

#ifndef NANA_GUI_WIDGET_BUTTON_HPP
#define NANA_GUI_WIDGET_BUTTON_HPP
#include "widget.hpp"
#include <nana/gui/element.hpp>


namespace nana{
	namespace drawerbase
	{
		namespace button
		{
			///	Draw the button
			class trigger: public drawer_trigger
			{
			public:
				trigger();
				~trigger();

				void emit_click();
				void icon(const nana::paint::image&);
				bool enable_pushed(bool);
				bool pushed(bool);
				bool pushed() const;
				void omitted(bool);
				bool focus_color(bool);

				element::cite_bground & cite();
			private:
				void attached(widget_reference, graph_reference) override;
				void refresh(graph_reference)	override;
				void mouse_enter(graph_reference, const arg_mouse&) override;
				void mouse_leave(graph_reference, const arg_mouse&) override;
				void mouse_down(graph_reference, const arg_mouse&)	override;
				void mouse_up(graph_reference, const arg_mouse&)	override;
				void key_char(graph_reference, const arg_keyboard&)	override;
				void key_press(graph_reference, const arg_keyboard&) override;
				void focus(graph_reference, const arg_focus&) override;
			private:
				void _m_draw(graph_reference);
				void _m_draw_title(graph_reference, bool enabled);
				void _m_draw_background(graph_reference);
				void _m_draw_border(graph_reference);
			private:
				widget* wdg_{nullptr};
				paint::graphics* graph_{nullptr};

				element::cite_bground cite_{"button"};

				struct attr_tag
				{
					element_state e_state;
					bool omitted;
					bool focused;
					bool pushed;
					bool keep_pressed;
					bool enable_pushed;
					bool focus_color;
					paint::image * icon;
					::nana::color bgcolor;
					::nana::color fgcolor;
				}attr_;
			};
		}//end namespace button
	}//end namespace drawerbase

		/// Define a button widget and provides the interfaces to be operational
		class button
			: public widget_object<category::widget_tag, drawerbase::button::trigger>
		{
			typedef widget_object<category::widget_tag, drawerbase::button::trigger> base_type;
		public:
			button();
			button(window, bool visible);
			button(window, const nana::string& caption, bool visible = true);
			button(window, const nana::char_t* caption, bool visible = true);
			button(window, const nana::rectangle& = rectangle(), bool visible = true);

			button& icon(const nana::paint::image&);
			button& enable_pushed(bool);
			bool pushed() const;
			button& pushed(bool);
			button& omitted(bool);	    	               ///< Enables/Disables omitting displaying the caption if the text is too long.
			button& enable_focus_color(bool);              ///< Enables/Disables showing the caption with a special color to indicate the button is focused.

			button& set_bground(const pat::cloneable<element::element_interface>&);	///< Sets a user-defined background element.
			button& set_bground(const std::string&);	///< Sets a pre-defined background element by a name.

			button& transparent(bool enable);
			bool transparent() const;

			button& edge_effects(bool enable);
		private:
			void _m_shortkey();
		private:
			//Overrides widget virtual functions
			void _m_complete_creation() override;
			void _m_caption(nana::string&&) override;
		};
}//end namespace nana
#endif

