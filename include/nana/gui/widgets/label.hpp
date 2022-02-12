/**
 *	A Label Control Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2022 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/label.hpp
 */

#ifndef NANA_GUI_WIDGET_LABEL_HPP
#define NANA_GUI_WIDGET_LABEL_HPP

#include "widget.hpp"
#include <nana/push_ignore_diagnostic>

namespace nana
{
	namespace drawerbase::label
	{
			enum class command  /// Defines the event type for format listener.
			{
				enter, leave, click
			};

			struct border
			{
				enum flag : unsigned int /// Defines the types of borders the label can have
				{
					left = 0x1,
					top = 0x2,
					right = 0x4,
					bottom = 0x8,
					all = 0xf
				};

				unsigned int borders = 0;
				unsigned int width = 1;
				::nana::color border_color{ 0, 0, 0 };
			};

			/// draw the label
			class trigger: public drawer_trigger
			{
			public:
				struct implement;

				trigger();
				~trigger();
				implement * impl() const;
			private:
				void attached(widget_reference, graph_reference)	override;
				void refresh(graph_reference)	override;
				void mouse_move(graph_reference, const arg_mouse&)	override;
				void mouse_leave(graph_reference, const arg_mouse&)	override;
				void click(graph_reference, const arg_click&)	override;
			private:
				implement * impl_;
			};
	}//end namespace drawerbase::label

	class label
		: public widget_object<category::widget_tag, drawerbase::label::trigger>
	{
		label(const label&) = delete;
		label(label&&) = delete;
	public:
		using command = drawerbase::label::command;
		using border = drawerbase::label::border;
		
		label();
		label(window parent, std::string_view text, bool visible = true);
		label(window parent, std::wstring_view text, bool visible = true);
#ifdef __cpp_char8_t
		label(window parent, std::u8string_view text, bool visible = true);
#endif

		label(window, const rectangle& = {}, bool visible = true);
		label& transparent(bool);		///< Switchs the label widget to the transparent background mode.
		bool transparent() const noexcept;
		label& format(bool);		///< Switches the format mode of the widget.
		label& add_format_listener(std::function<void(command, const std::string&)>);

		/// as same as the HTML "for" attribute of a label
		label& click_for(window associated_window) noexcept;

		label& set_border(border); // Sets the border object for the widget
		label& add_border(border::flag); // Adds a border to the widget
		label& remove_border(border::flag); // Removes a border from the widget
		bool has_border(border::flag) const noexcept; // Checks wether the widget has the current border set

		label& border_width(unsigned int);
		unsigned int border_width() const noexcept;

		label& border_color(nana::color);
		nana::color border_color() const noexcept;

		/// Returns the size of the text. If *allowed_width_in_pixel* is not zero, returns a 
		/// "corrected" size that changes lines to fit the text into the specified width
		nana::size measure(unsigned allowed_width_in_pixel) const;

		static ::nana::size measure(::nana::paint::graphics&, const ::std::string&, unsigned allowed_width_in_pixel, bool format_enabled, align h_align, align_v v_align);
#ifdef __cpp_char8_t
		static ::nana::size measure(::nana::paint::graphics&, std::u8string_view, unsigned allowed_width_in_pixel, bool format_enabled, align h_align, align_v v_align);
#endif
		label& text_align(align horizontal_align, align_v vertical_align= align_v::top);
	private:
		//Overrides widget's virtual function
		void _m_complete_creation() override;
		void _m_caption(native_string_type&&) override;
		void _m_update_place(); // updates the parents place
	};
}//end namespace nana
#include <nana/pop_ignore_diagnostic>

#endif
