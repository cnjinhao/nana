/*
 *	A Toolbar Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2014 Jinhao(cnjinhao@hotmail.com)
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
#include <vector>

namespace nana
{
	class toolbar;

	struct arg_toolbar
	{
		toolbar& widget;
		std::size_t button;
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

			class drawer
				: public drawer_trigger
			{
				struct drawer_impl_type;

			public:
				typedef std::size_t size_type;

				drawer();
				~drawer();

				void append(const nana::string&, const nana::paint::image&);
				void append();
				bool enable(size_type) const;
				bool enable(size_type, bool);
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
				size_type _m_which(int x, int y, bool want_if_disabled) const;
				void _m_draw_background(nana::color_t);
				void _m_draw();
				void _m_owner_sized(const arg_resized&);
			private:
				void _m_fill_pixels(item_type*, bool force);
			private:
				::nana::toolbar*	widget_;
				::nana::paint::graphics*	graph_;
				drawer_impl_type*	impl_;
			};
		
		}//end namespace toolbar
	}//end namespace drawerbase
    /// Control bar that contains buttons for controlling
	class toolbar
		: public widget_object<category::widget_tag, drawerbase::toolbar::drawer, drawerbase::toolbar::toolbar_events>
	{
	public:
		typedef std::size_t size_type;      ///< A type to count the number of elements.

		toolbar();
		toolbar(window, bool visible);
		toolbar(window, const rectangle& = rectangle(), bool visible = true);

		void append();                      ///< Adds a separator.
		void append(const nana::string& text, const nana::paint::image& img);   ///< Adds a control button.
		void append(const nana::string& text);   ///< Adds a control button.
		bool enable(size_type index) const;
		void enable(size_type index, bool enable_state);
		void scale(unsigned s);   ///< Sets the scale of control button.
	};
}//end namespace nana
#endif
