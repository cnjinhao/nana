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
#if 1 //deprecated
			enum class tool_type
			{
				button,
				toggle
			};
#endif
			enum class tools
			{
				separator,
				button,
				toggle,
				dropdown
			};

			struct toolbar_item;
			class item_container;
			class item_proxy;

			/// A callback functor type.
			typedef std::function<void(item_proxy&)> event_fn_t;

		    class item_proxy
		    {
			public:
				item_proxy() = default;
				item_proxy(toolbar_item* item, item_container* cont, nana::toolbar* tb);
#if 1 //deprecated
				item_proxy& tooltype(tool_type type); ///< Deprecated. Use type instead.
				bool istoggle() const; ///< Deprecated. Use type instead.
#endif
				bool empty() const;  ///< Returns the item_proxy state.

				bool enable() const; ///< Gets the tool enable state.
				item_proxy& enable(bool enable_state); ///< Sets the tool enable state.

				tools type() const; ///< Gets the tool type.
				item_proxy& type(tools type); ///< Sets the tool type.
				
				item_proxy& textout(bool show); ///< Show/Hide the text inside the tool.

				item_proxy& answerer(const event_fn_t& handler);  ///< Sets answerer of the tool.

				// tools::toggle
				bool toggle() const; ///< Gets the tool toggle state (only if tool style is toggle).
				item_proxy& toggle(bool toggle_state); ///< Sets the tool toggle state (only if tool style is toggle).
				std::string toggle_group() const;	///< Returns the toggle group associated with the tool (only if tool style is toggle).
				item_proxy& toggle_group(const std::string& group);	///< Adds the tool to a toggle group (only if tool style is toggle).

				// tools::dropdown
				item_proxy& dropdown_append(const std::string& text, const nana::paint::image& img, const event_fn_t& handler = {});   ///< Adds an item to the dropdown menu.
				item_proxy& dropdown_append(const std::string& text, const event_fn_t& handler = {});   ///< Adds an item to the dropdown menu.

				bool dropdown_enable(std::size_t index) const; ///< Gets the dropdown menu item enable state.
				item_proxy& dropdown_enable(std::size_t index, bool enable_state); ///< Sets the dropdown menu item enable state.

				item_proxy& dropdown_answerer(std::size_t index, const event_fn_t& handler);  ///< Sets answerer of the dropdown menu item.

			private:
				toolbar_item* item_{ nullptr };
				item_container* cont_{ nullptr };
				nana::toolbar* const tb_{ nullptr };
		    };

			struct toolbar_events
				: public general_events
			{
				basic_event<arg_toolbar> selected;	///< A mouse click on a tool.
				basic_event<arg_toolbar> enter;		///< The mouse enters a tool.
				basic_event<arg_toolbar> leave;		///< The mouse leaves a tool.
			};

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
				void _m_calc_pixels(toolbar_item*, bool force);
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
		///< A type to count the number of elements.
		using size_type = std::size_t;

#if 1 //deprecated
		using tool_type = drawerbase::toolbar::tool_type;
#endif
		/// Iterator to access item
		using item_proxy = drawerbase::toolbar::item_proxy;

		/// Toolbar tools type enum
		using tools = drawerbase::toolbar::tools;

		/// A callback functor type. Prototype: `void(item_proxy&)`
		using event_fn_t = drawerbase::toolbar::event_fn_t;


		toolbar() = default;
		toolbar(window, bool visible, bool detached=false);
		toolbar(window, const rectangle& = rectangle(), bool visible = true, bool detached = false);

#if 1 //deprecated
		void separate(); ///< Deprecated. Use append_separator() instead.
		item_proxy append(const ::std::string& text, const nana::paint::image& img);   ///< Deprecated. Use append(tool, text, img, fn) instead.
		item_proxy append(const ::std::string& text);   ///< Deprecated. Use append(tool, text, fn) instead.
#endif

		item_proxy append(tools t, const std::string& text, const nana::paint::image& img, const event_fn_t& handler = {});   ///< Adds a tool.
		item_proxy append(tools t, const std::string& text, const event_fn_t& handler = {});   ///< Adds a tool.
		void append_separator();	///< Adds a separator.

		size_type count() const noexcept; ///< Returns tools and separators count.
		item_proxy at(size_type pos);	///< Returns tool at specified position.

		void clear();   ///< Removes all tools and separators.
		
		bool enable(size_type index) const; ///< Gets the tool enable state.
		void enable(size_type index, bool enable_state); ///< Sets the tool enable state.

#if 1 //deprecated
		void tooltype(size_type index, tool_type type); ///< Deprecated. Use at[index].type instead.
		bool istoggle(size_type index) const;  ///< Deprecated. Use at[index].type instead.
		bool toggle(size_type index) const;  ///< Deprecated. Use at[index].toggle instead.
		void toggle(size_type index, bool toggle_state);  ///< Deprecated. Use at[index].toggle instead.
		std::string toggle_group(size_type index) const;  ///< Deprecated. Use at[index].toggle_group instead.
		void toggle_group(size_type index, const ::std::string& group);  ///< Deprecated. Use at[index].toggle_group instead.
		void textout(size_type index, bool show);  ///< Deprecated. Use at[index].textout instead.
		void scale(unsigned s);  ///< Deprecated. Use tools_height instead.
#endif

		void tools_height(unsigned h);   ///< Sets the height of tools.
		void go_right(); ///< Enable to place tools at right part. After calling it, every new tool is right aligned.

		bool detached() { return detached_; };

	private:
		bool   detached_;
	};
}//end namespace nana
#include <nana/pop_ignore_diagnostic>

#endif
