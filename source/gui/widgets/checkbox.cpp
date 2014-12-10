/*
 *	A CheckBox Implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/checkbox.cpp
 */

#include <nana/gui/widgets/checkbox.hpp>
#include <nana/paint/gadget.hpp>
#include <nana/paint/text_renderer.hpp>
#include <nana/gui/element.hpp>
#include <algorithm>

namespace nana{ namespace drawerbase
{
namespace checkbox
{
	typedef element::crook_interface::state crook_state;

	struct drawer::implement
	{
		bool react;
		bool radio;
		facade<element::crook> crook;
	};

			drawer::drawer()
				: widget_(nullptr),
					imptr_(new drawer::implement),
					impl_(imptr_.get())
			{
				impl_->react = true;
				impl_->radio = false;
			}

			drawer::~drawer()
			{}

			void drawer::attached(widget_reference widget, graph_reference)
			{
				widget_ = &widget;
			}

			void drawer::refresh(graph_reference graph)
			{
				_m_draw(graph);
			}

			void drawer::mouse_down(graph_reference graph, const arg_mouse&)
			{
				_m_draw(graph);
			}

			void drawer::mouse_up(graph_reference graph, const arg_mouse&)
			{
				if(impl_->react)
					impl_->crook.reverse();
				_m_draw(graph);
			}

			void drawer::mouse_enter(graph_reference graph, const arg_mouse&)
			{
				_m_draw(graph);
			}

			void drawer::mouse_leave(graph_reference graph, const arg_mouse&)
			{
				_m_draw(graph);
			}

			drawer::implement * drawer::impl() const
			{
				return impl_;
			}

			void drawer::_m_draw(graph_reference graph)
			{
				_m_draw_background(graph);
				_m_draw_title(graph);
				_m_draw_checkbox(graph, graph.text_extent_size(STR("jN"), 2).height + 2);
				API::lazy_refresh();
			}

			void drawer::_m_draw_background(graph_reference graph)
			{
				if(bground_mode::basic != API::effects_bground_mode(*widget_))
					graph.rectangle(API::background(*widget_), true);
			}

			void drawer::_m_draw_checkbox(graph_reference graph, unsigned first_line_height)
			{
				impl_->crook.draw(graph, widget_->background(), widget_->foreground(), rectangle(0, first_line_height > 16 ? (first_line_height - 16) / 2 : 0, 16, 16), API::element_state(*widget_));
			}

			void drawer::_m_draw_title(graph_reference graph)
			{
				if(graph.width() > 16 + interval)
				{
					nana::string title = widget_->caption();

					unsigned fgcolor = widget_->foreground();
					unsigned pixels = graph.width() - (16 + interval);

					nana::paint::text_renderer tr(graph);
					if(API::window_enabled(widget_->handle()) == false)
					{
						tr.render(17 + interval, 2, 0xFFFFFF, title.c_str(), title.length(), pixels);
						fgcolor = 0x808080;
					}

					tr.render(16 + interval, 1, fgcolor, title.c_str(), title.length(), pixels);
				}
			}
		//end class drawer
	} //end namespace checkbox
}//end namespace drawerbase

	//class checkbox

		checkbox::checkbox(){}

		checkbox::checkbox(window wd, bool visible)
		{
			create(wd, rectangle(), visible);
		}

		checkbox::checkbox(window wd, const nana::string& text, bool visible)
		{
			create(wd, rectangle(), visible);
			caption(text);
		}

		checkbox::checkbox(window wd, const nana::char_t* text, bool visible)
		{
			create(wd, rectangle(), visible);
			caption(text);
		}

		checkbox::checkbox(window wd, const nana::rectangle& r, bool visible)
		{
			create(wd, r, visible);
		}

		void checkbox::element_set(const char* name)
		{
			get_drawer_trigger().impl()->crook.switch_to(name);
		}

		void checkbox::react(bool want)
		{
			get_drawer_trigger().impl()->react = want;
		}

		bool checkbox::checked() const
		{
			return (get_drawer_trigger().impl()->crook.checked() != drawerbase::checkbox::crook_state::unchecked);
		}

		void checkbox::check(bool chk)
		{
			typedef drawerbase::checkbox::crook_state crook_state;
			get_drawer_trigger().impl()->crook.check(chk ? crook_state::checked : crook_state::unchecked);
			API::refresh_window(handle());
		}

		void checkbox::radio(bool is_radio)
		{
			get_drawer_trigger().impl()->crook.radio(is_radio);
		}

		void checkbox::transparent(bool enabled)
		{
			if(enabled)
				API::effects_bground(*this, effects::bground_transparent(0), 0.0);
			else
				API::effects_bground_remove(*this);
		}

		bool checkbox::transparent() const
		{
			return (bground_mode::basic == API::effects_bground_mode(*this));
		}
	//end class checkbox

	//class radio_group
		radio_group::~radio_group()
		{
			for(auto & i : ui_container_)
			{
				API::umake_event(i.eh_checked);
				API::umake_event(i.eh_destroy);
			}
		}

		void radio_group::add(checkbox& uiobj)
		{
			uiobj.radio(true);
			uiobj.check(false);
			uiobj.react(false);

			element_tag el;

			el.uiobj = &uiobj;
			el.eh_checked = uiobj.events().click.connect_front(std::bind(&radio_group::_m_checked, this, std::placeholders::_1));
			el.eh_destroy = uiobj.events().destroy.connect(std::bind(&radio_group::_m_destroy, this, std::placeholders::_1));
			ui_container_.push_back(el);
		}

		std::size_t radio_group::checked() const
		{
			auto i = std::find_if(ui_container_.cbegin(), ui_container_.cend(), [](decltype(*ui_container_.cbegin())& x)
				{
					return (x.uiobj->checked());
				});
			return static_cast<std::size_t>(i - ui_container_.cbegin());
		}

		std::size_t radio_group::size() const
		{
			return ui_container_.size();
		}

		void radio_group::_m_checked(const arg_mouse& arg)
		{
			for (auto & i : ui_container_)
				i.uiobj->check(arg.window_handle == i.uiobj->handle());
		}

		void radio_group::_m_destroy(const arg_destroy& arg)
		{
			auto i = std::find_if(ui_container_.begin(), ui_container_.end(), [&arg](decltype(*ui_container_.begin()) & x)
					{
						return (arg.window_handle == x.uiobj->handle());
					});
			if(i != ui_container_.end())
				ui_container_.erase(i);
		}
	//end class radio_group
}//end namespace nana
