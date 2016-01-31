/**
 *	A Toolbar Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/toolbar.hpp
 */

#ifndef NANA_GUI_WIDGET_TOOLBAR_HPP
#define NANA_GUI_WIDGET_TOOLBAR_HPP

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
			struct toolbar_events
				: public general_events
			{
				basic_event<arg_toolbar> selected;	///< A mouse click on a control button.
				basic_event<arg_toolbar> enter;		///< The mouse enters a control button.
				basic_event<arg_toolbar> leave;		///< The mouse leaves a control button.
			};

			struct item_type;
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

		toolbar() = default;
		toolbar(window, bool visible, bool detached=false);
		toolbar(window, const rectangle& = rectangle(), bool visible = true, bool detached = false);

		void separate();                      ///< Adds a separator.
		void append(const ::std::string& text, const nana::paint::image& img);   ///< Adds a control button.
		void append(const ::std::string& text);   ///< Adds a control button.
		bool enable(size_type index) const;
		void enable(size_type index, bool enable_state);
		void scale(unsigned s);   ///< Sets the scale of control button.

		void go_right();

		bool detached() { return detached_; };

	private:
		bool   detached_;
	};
}//end namespace nana
#endif
