/*
 *	A CheckBox Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/checkbox.cpp
 */

#include <nana/gui/widgets/checkbox.hpp>
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
				_m_draw_background(graph);
				_m_draw_title(graph);
				_m_draw_checkbox(graph, graph.text_extent_size(L"jN", 2).height + 2);
			}

			void drawer::mouse_down(graph_reference graph, const arg_mouse&)
			{
				refresh(graph);
				API::lazy_refresh();
			}

			void drawer::mouse_up(graph_reference graph, const arg_mouse&)
			{
				if(impl_->react)
					impl_->crook.reverse();
				refresh(graph);
				API::lazy_refresh();
			}

			void drawer::mouse_enter(graph_reference graph, const arg_mouse&)
			{
				refresh(graph);
				API::lazy_refresh();
			}

			void drawer::mouse_leave(graph_reference graph, const arg_mouse&)
			{
				refresh(graph);
				API::lazy_refresh();
			}

			drawer::implement * drawer::impl() const
			{
				return impl_;
			}

			void drawer::_m_draw_background(graph_reference graph)
			{
				if(bground_mode::basic != API::effects_bground_mode(*widget_))
					graph.rectangle(true, API::bgcolor(*widget_));
			}

			void drawer::_m_draw_checkbox(graph_reference graph, unsigned first_line_height)
			{
				impl_->crook.draw(graph, widget_->bgcolor(), widget_->fgcolor(), rectangle(0, first_line_height > 16 ? (first_line_height - 16) / 2 : 0, 16, 16), API::element_state(*widget_));
			}

			void drawer::_m_draw_title(graph_reference graph)
			{
				if (graph.width() > 16 + interval)
				{
					std::wstring title = ::nana::charset(widget_->caption(), ::nana::unicode::utf8);

					unsigned pixels = graph.width() - (16 + interval);

					nana::paint::text_renderer tr(graph);
					if (API::window_enabled(widget_->handle()) == false)
					{
						graph.set_text_color(colors::white);
						tr.render({ 17 + interval, 2 }, title.c_str(), title.length(), pixels);
						graph.set_text_color({ 0x80, 0x80, 0x80 });
					}
					else
						graph.set_text_color(widget_->fgcolor());

					tr.render({ 16 + interval, 1 }, title.c_str(), title.length(), pixels);
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
            bgcolor(API::bgcolor(wd));
		}

		checkbox::checkbox(window wd, const std::string& text, bool visible)
		{
			create(wd, rectangle(), visible);
            bgcolor(API::bgcolor(wd));
			caption(text);
		}

		checkbox::checkbox(window wd, const char* text, bool visible)
			: checkbox(wd, std::string(text), visible)
		{
		}

		checkbox::checkbox(window wd, const nana::rectangle& r, bool visible)
		{
            bgcolor(API::bgcolor(wd));
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
			API::refresh_window(handle());
		}

		void checkbox::transparent(bool enabled)
		{
			if(enabled)
				API::effects_bground(*this, effects::bground_transparent(0), 0.0);
			else
				API::effects_bground_remove(*this);
			API::refresh_window(handle());
		}

		bool checkbox::transparent() const
		{
			return (bground_mode::basic == API::effects_bground_mode(*this));
		}
	//end class checkbox

	//class radio_group
		radio_group::~radio_group()
		{
			for(auto & e : ui_container_)
			{
				e.uiobj->radio(false);
				e.uiobj->react(true);
				API::umake_event(e.eh_checked);
				API::umake_event(e.eh_destroy);
			}
		}

		void radio_group::add(checkbox& uiobj)
		{
			uiobj.radio(true);
			uiobj.check(false);
			uiobj.react(false);

			element_tag el;

			el.uiobj = &uiobj;
			el.eh_checked = uiobj.events().click.connect_unignorable([this](const arg_click& arg)
			{
				for (auto & i : ui_container_)
					i.uiobj->check(arg.window_handle == i.uiobj->handle());
			}, true);

			el.eh_destroy = uiobj.events().destroy.connect_unignorable([this](const arg_destroy& arg)
			{
				for (auto i = ui_container_.begin(); i != ui_container_.end(); ++i)
				{
					if (arg.window_handle == i->uiobj->handle())
					{
						ui_container_.erase(i);
						return;
					}
				}
			});

			ui_container_.push_back(el);
		}

		std::size_t radio_group::checked() const
		{
			for (auto i = ui_container_.cbegin(); i != ui_container_.cend(); ++i)
			{
				if (i->uiobj->checked())
					return static_cast<std::size_t>(i - ui_container_.cbegin());
			}

			return ui_container_.size();
		}

		std::size_t radio_group::size() const
		{
			return ui_container_.size();
		}
	//end class radio_group
}//end namespace nana
