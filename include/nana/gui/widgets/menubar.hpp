/*
 *	A Menubar implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2009-2018 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/menubar.hpp
 */
 
#ifndef NANA_GUI_WIDGETS_MENUBAR_HPP
#define NANA_GUI_WIDGETS_MENUBAR_HPP
#include "widget.hpp"
#include "menu.hpp"
#include <nana/push_ignore_diagnostic>

namespace nana
{
	namespace drawerbase
	{
		namespace menubar
		{
			using native_string_type = ::nana::detail::native_string_type;

			struct scheme
				: public widget_geometrics
			{
				color_proxy text_fgcolor{ colors::black };
				color_proxy body_highlight{ static_cast<color_rgb>(0xc0ddfc) };
				color_proxy body_selected{ colors::white };
				color_proxy border_selected{ colors::dark_border };
				color_proxy border_highlight{ colors::highlight };
			};

			class trigger
				: public drawer_trigger
			{
				struct essence;
			public:
				trigger();
				~trigger();
				essence& ess() const;
			private:
				void attached(widget_reference, graph_reference)	override;
				void refresh(graph_reference)	override;
				void mouse_move(graph_reference, const arg_mouse&)	override;
				void mouse_leave(graph_reference, const arg_mouse&)	override;
				void mouse_down(graph_reference, const arg_mouse&)	override;
				void mouse_up(graph_reference, const arg_mouse&)	override;
				void focus(graph_reference, const arg_focus&)		override;
				void key_press(graph_reference, const arg_keyboard&)	override;
				void key_release(graph_reference, const arg_keyboard&)	override;
				void shortkey(graph_reference, const arg_keyboard&)	override;
			private:
				void _m_move(graph_reference, bool to_left);
			private:
				essence * const ess_;
			};
		}//end namespace menubar
	}//end namespace drawerbase

	  /// \brief A toolbar at the top of window for pop-upping menus.
	  ///
	  /// The widget sets as shortkey the character behind the first of & in the text, for the item. e.g. "File(&F)" or "&File".
	class menubar
		:	public widget_object<category::widget_tag, drawerbase::menubar::trigger, ::nana::general_events, drawerbase::menubar::scheme>
	{
	public:
		menubar() = default;					///< The default constructor delay creation.
		menubar(window);						///< Create a menubar at the top of the specified window.
		~menubar();
		void create(window);					///< Create a menubar at the top of the specified window.
		menu& push_back(const std::string&);	///< Appends a new (empty) menu.
		menu& at(size_t index) const;		    ///< Gets the menu specified by index.
		std::size_t length() const;		        ///< Number of menus.
		void clear();							///< Removes all the menus.
		
		/// Deselects the menu
		/**
		 * If a menu is popped up, the menu deselects the item and close the pop-upped menu.
		 * @return true if an item is deselected, false otherwise.
		 */
		bool cancel();

		/// Determines the mouse is hovered on the menubar or its popped menu.
		bool hovered() const;
	private:
		::nana::event_handle evt_resized_{nullptr};
	};//end class menubar
}//end namespace nana
#include <nana/pop_ignore_diagnostic>

#endif
