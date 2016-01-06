/*
 *	A Menubar implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2009-2014 Jinhao(cnjinhao@hotmail.com)
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

namespace nana
{
	namespace drawerbase
	{
		namespace menubar
		{
			using native_string_type = ::nana::detail::native_string_type;

			class item_renderer
			{
			public:
				enum class state
				{
					normal, highlighted, selected
				};

				using graph_reference = paint::graphics&;

				item_renderer(window, graph_reference);
				virtual void background(const point&, const ::nana::size&, state);
				virtual void caption(const point&, const native_string_type&);
			private:
				window	handle_;
				graph_reference graph_;
			};

			class trigger
				: public drawer_trigger
			{
				class itembase;
			public:
				trigger();
				~trigger();
				nana::menu* push_back(const std::string&);
				nana::menu* at(size_t) const;
				std::size_t size() const;
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
				void _m_move(bool to_left);
				bool _m_popup_menu();
				void _m_total_close();
				bool _m_close_menu();
				std::size_t _m_item_by_pos(const ::nana::point&);
				bool _m_track_mouse(const ::nana::point&);
			private:
				widget *widget_;
				nana::paint::graphics	*graph_;
				
				itembase*	items_;

				struct state_type
				{
					enum behavior_t
					{
						behavior_none, behavior_focus, behavior_menu,
					};

					state_type();

					std::size_t active;
					behavior_t behavior;

					bool menu_active;
					bool passive_close;

					bool nullify_mouse;

					nana::menu *menu;
					nana::point mouse_pos;
				}state_;
			};
		}//end namespace menubar
	}//end namespace drawerbase

	  /// \brief A toolbar at the top of window for popuping menus.
	  ///
	  /// The widget sets as shortkey the character behind the first of & in the text, for the item. e.g. "File(&F)" or "&File".
	class menubar
		:	public widget_object<category::widget_tag, drawerbase::menubar::trigger>
	{
	public:
		menubar() = default;					///< The default constructor delay creation.
		menubar(window);						///< Create a menubar at the top of the specified window.
		~menubar();
		void create(window);					///< Create a menubar at the top of the specified window.
		menu& push_back(const std::string&);	///< Appends a new (empty) menu.
		menu& at(size_t index) const;		    ///< Gets the menu specified by index.
		std::size_t length() const;		        ///< Number of menus.
	private:
		::nana::event_handle evt_resized_{nullptr};
	};//end class menubar
}//end namespace nana
#endif
