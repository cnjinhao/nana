/**
 *	A CheckBox Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *  @file: nana/gui/widgets/checkbox.hpp
 */

#ifndef NANA_GUI_WIDGET_CHECKBOX_HPP
#define NANA_GUI_WIDGET_CHECKBOX_HPP
#include <nana/push_ignore_diagnostic>

#include "widget.hpp"
#include <vector>
#include <memory>

namespace nana {

	//forward-declaration
	class checkbox;

	struct arg_checkbox
		: public event_arg
	{
		checkbox * const widget;

		arg_checkbox(checkbox* wdg)
			: widget(wdg)
		{}
	};

namespace drawerbase
{
	namespace checkbox
	{
		struct scheme
			: public widget_geometrics
		{
			color_proxy square_bgcolor{ static_cast<color_argb>(0x0) };
			color_proxy square_border_color{ colors::black };
		};

		struct events_type
			: public general_events
		{
			basic_event<arg_checkbox> checked;
		};

		class drawer
			: public drawer_trigger
		{
			struct implement;
		public:
			drawer();
			~drawer();	//To instance imptr_;
			void attached(widget_reference, graph_reference)	override;
			void refresh(graph_reference)	override;
			void mouse_enter(graph_reference, const arg_mouse&)	override;
			void mouse_leave(graph_reference, const arg_mouse&)	override;
			void mouse_down(graph_reference, const arg_mouse&)	override;
			void mouse_up(graph_reference, const arg_mouse&)	override;
		public:
			implement * impl() const;
		private:
			static const int interval = 4;
			implement * impl_;
		};
	}//end namespace checkbox
}//end namespace drawerbase

	
    class checkbox
		: public widget_object<category::widget_tag, drawerbase::checkbox::drawer, drawerbase::checkbox::events_type, drawerbase::checkbox::scheme>
	{
	public:
		checkbox();
		checkbox(window, bool visible);
		checkbox(window, const std::string& text, bool visible = true);
		checkbox(window, const char* text, bool visible = true);
		checkbox(window, const rectangle& = rectangle(), bool visible = true);

		void element_set(const char* name);
		void react(bool want);		///< Enables the reverse check while clicking on the checkbox.
		bool checked() const;

		/// Checks/unchecks the checkbox
		void check(bool state);

		/// \brief With the radio mode, users make a choice among a set of mutually exclusive, 
		/// related options. Users can choose one and only one option. 
		/// There is a helper class manages checkboxs for radio mode, 
		/// \see radio_group.
		void radio(bool);
		void transparent(bool value);
		bool transparent() const;
	};//end class checkbox

    /// for managing checkboxs in radio mode
	class radio_group
	{
		struct element_tag
		{
			checkbox * uiobj;
			event_handle eh_clicked;
			event_handle eh_checked;
			event_handle eh_destroy;
			event_handle eh_keyboard;
		};
	public:
		constexpr static const std::size_t npos = static_cast<std::size_t>(-1);
		~radio_group();
		void add(checkbox&);

		///< Retrieves the index of checked option. It returns radio_group::npos if no checkbox is checked.
		std::size_t checked() const;
		std::size_t size() const;

		template<typename Function>
		void on_clicked(Function&& click_fn)
		{
			for (auto & e : ui_container_)
			{
				e.uiobj->events().click(std::move(click_fn));
			}
		}

		template<typename Function>
		void on_checked(Function&& check_fn)
		{
			for (auto & e : ui_container_)
			{
				e.uiobj->events().checked(std::move(check_fn));
			}
		}
	private:
		std::vector<element_tag> ui_container_;
	};
}//end namespace nana
#include <nana/pop_ignore_diagnostic>

#endif
