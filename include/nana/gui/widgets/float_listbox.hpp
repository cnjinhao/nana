/**
 *	A float_listbox Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/float_listbox.hpp
 */

#ifndef NANA_GUI_WIDGETS_FLOAT_LISTBOX_HPP
#define NANA_GUI_WIDGETS_FLOAT_LISTBOX_HPP

#include "widget.hpp"
#include <vector>
#include <memory>

namespace nana
{
	namespace drawerbase{
		namespace float_listbox
		{
			class item_interface
			{
			public:
				virtual ~item_interface(){}

				virtual const nana::paint::image & image() const = 0;
				virtual const char*	text() const = 0;
			};

			//struct module_def
			//@brief: This defines a data structure used for float_listbox
			struct module_def
			{
				std::vector<std::shared_ptr<item_interface>> items;

				std::size_t max_items{10};			// the number of items display.
				mutable std::size_t index{::nana::npos};		// the result of the selection.
				mutable bool have_selected;
			};

			class item_renderer
			{
			public:
				typedef widget&	widget_reference;
				typedef paint::graphics& graph_reference;
				enum state_t{StateNone, StateHighlighted};

				virtual ~item_renderer() = default;
				virtual void image(bool enabled, unsigned pixels) = 0;
				virtual void render(widget_reference, graph_reference, const nana::rectangle&, const item_interface*, state_t) = 0;
				virtual unsigned item_pixels(graph_reference) const = 0;
			};

			class drawer_impl;

			class trigger
				: public drawer_trigger
			{
			public:
				trigger();
				~trigger();
				drawer_impl& get_drawer_impl();
				const drawer_impl& get_drawer_impl() const;
			private:
				void attached(widget_reference, graph_reference)	override;
				void detached()	override;
				void refresh(graph_reference)	override;
				void mouse_move(graph_reference, const arg_mouse&)	override;
				void mouse_up(graph_reference, const arg_mouse&)	override;
			private:
				class drawer_impl *drawer_;
			};
		}
	}//end namespace drawerbase

	class float_listbox
		: public widget_object<category::root_tag, drawerbase::float_listbox::trigger>
	{
		typedef widget_object<category::root_tag, drawerbase::float_listbox::trigger> base_type;
	public:
		typedef drawerbase::float_listbox::item_renderer item_renderer;
		typedef drawerbase::float_listbox::module_def module_type;
		typedef drawerbase::float_listbox::item_interface item_interface;

		/** @brief Constructor
		 *	@param window	A handle to a window which is a owner of float_listbox
		 *	@param rectangle	A position and a size of float_listbox
		 *	@param is_ignore_first_mouse_up	The flost_listbox will be closed when a mouse_up is emitted, this parameter is specified for ignoring the first mouse_up emitting.
		 *	@remark	The float_listbox can be popupped in a mouse_down event, the next mouse_up may be ignored, otherwise the float_listbox will be closed when user releases the button.
		 */
		float_listbox(window, const rectangle&, bool is_ignore_first_mouse_up);

		void set_module(const module_type&, unsigned image_pixels);
		void scroll_items(bool upwards);
		void move_items(bool upwards, bool circle);
		void renderer(item_renderer*);
		std::size_t index() const;
	};
}

#endif
