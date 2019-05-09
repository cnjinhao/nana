/**
 *	A Toolbar Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2018 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/toolbar.hpp
 */

#ifndef NANA_GUI_WIDGET_TOOLBAR_HPP
#define NANA_GUI_WIDGET_TOOLBAR_HPP
#include <nana/push_ignore_diagnostic>

#include "widget.hpp"

namespace nana
{
	class toolbar;

	struct arg_toolbar
		: public event_arg
	{
		toolbar& widget;
		std::size_t button;

		arg_toolbar(toolbar&, std::size_t);
	};

	namespace drawerbase
	{
		namespace toolbar
		{
			enum class tool_type
			{
				button,
				toggle
			};

		    class item_proxy
		    {
			public:
				item_proxy(::nana::toolbar*, std::size_t pos);

				bool enable() const;
				item_proxy& enable(bool enable_state);

				item_proxy& tooltype(tool_type type); ///< Sets the tool style.

				bool istoggle() const; ///< Returns true if the tool style is toggle.
				bool toggle() const; ///< Gets the tool toggle state (only if tool style is toggle).
				item_proxy& toggle(bool toggle_state); ///< Sets the tool toggle state (only if tool style is toggle).
				std::string toggle_group() const;	///< Returns the toggle group associated with the tool (only if tool style is toggle).
				item_proxy& toggle_group(const ::std::string& group);	///< Adds the tool to a toggle group (only if tool style is toggle).

				item_proxy& textout(bool show); ///< Show/Hide the text inside the button

			private:
				nana::toolbar* const tb_;
				std::size_t const pos_;
		    };

			struct item_type
			{
				std::string text;
				nana::paint::image image;
				unsigned	pixels{ 0 };
				unsigned	position{ 0 }; // last item position.
				nana::size	textsize;
				bool		enable{ true };

				tool_type	type{ tool_type::button };
				bool		toggle{ false };
				std::string group;

				bool		textout{ false };

				item_type(const std::string& text, const nana::paint::image& img, tool_type type)
					:text(text), image(img), type(type)
				{}
			};

			struct toolbar_events
				: public general_events
			{
				basic_event<arg_toolbar> selected;	///< A mouse click on a control button.
				basic_event<arg_toolbar> enter;		///< The mouse enters a control button.
				basic_event<arg_toolbar> leave;		///< The mouse leaves a control button.
			};

			class item_container;

			class drawer
				: public drawer_trigger
			{
				struct drawer_impl_type;

			public:
				using size_type = std::size_t;

				drawer();
				~drawer();

				item_container& items() const;
				void scale(unsigned);
			private:
				void refresh(graph_reference)	override;
				void attached(widget_reference, graph_reference)	override;
				void detached()	override;
				void mouse_move(graph_reference, const arg_mouse&)	override;
				void mouse_leave(graph_reference, const arg_mouse&)	override;
				void mouse_down(graph_reference, const arg_mouse&)	override;
				void mouse_up(graph_reference, const arg_mouse&)	override;
			private:
				size_type _m_which(point, bool want_if_disabled) const;
				void _m_calc_pixels(item_type*, bool force);
			private:
				::nana::toolbar*	widget_;
				drawer_impl_type*	impl_;
			};

		}//end namespace toolbar
	}//end namespace drawerbase

    /// Control bar that contains buttons for controlling
	class toolbar
		: public widget_object<category::widget_tag, drawerbase::toolbar::drawer, drawerbase::toolbar::toolbar_events>
	{
	public:
		using size_type = std::size_t;      ///< A type to count the number of elements.
		using tool_type = drawerbase::toolbar::tool_type;

		toolbar() = default;
		toolbar(window, bool visible, bool detached=false);
		toolbar(window, const rectangle& = rectangle(), bool visible = true, bool detached = false);

		void separate();                      ///< Adds a separator.
		drawerbase::toolbar::item_proxy append(const ::std::string& text, const nana::paint::image& img);   ///< Adds a control button.
		drawerbase::toolbar::item_proxy append(const ::std::string& text);   ///< Adds a control button.
		void clear();   ///< Removes all control buttons and separators.
		
		bool enable(size_type index) const;
		void enable(size_type index, bool enable_state);

		void tooltype(size_type index, tool_type type); ///< Sets the tool style.

		bool istoggle(size_type index) const; ///< Returns true if the tool style is toggle.
		bool toggle(size_type index) const; ///< Gets the tool toggle state (only if tool style is toggle).
		void toggle(size_type index, bool toggle_state); ///< Sets the tool toggle state (only if tool style is toggle).
		std::string toggle_group(size_type index) const;	///< Returns the toggle group associated with the tool (only if tool style is toggle).
		void toggle_group(size_type index, const ::std::string& group);	///< Adds the tool to a toggle group (only if tool style is toggle).

		void textout(size_type index, bool show); ///< Show/Hide the text inside the button

		void scale(unsigned s);   ///< Sets the scale of control button.

		/// Enable to place buttons at right part. After calling it, every new button is right aligned.
		void go_right();

		bool detached() { return detached_; };

	private:
		bool   detached_;
	};
}//end namespace nana
#include <nana/pop_ignore_diagnostic>

#endif
