/*
 *	A Button Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/button.cpp
 */

#include <nana/gui/widgets/button.hpp>
#include <nana/paint/text_renderer.hpp>

namespace nana{	namespace drawerbase
{
	namespace button
	{

		//trigger
		//@brief: draw the button
		trigger::trigger()
		{
			attr_.e_state = element_state::normal;
			attr_.omitted = attr_.focused = attr_.pushed = attr_.enable_pushed = attr_.keep_pressed =  false;
			attr_.focus_color = true;
			attr_.icon = nullptr;
		}

		trigger::~trigger()
		{
			delete attr_.icon;
		}

		void trigger::attached(widget_reference widget, graph_reference graph)
		{
			graph_ = &graph;

			wdg_ = &widget;
			window wd = widget;

			API::dev::enable_space_click(widget, true);
			API::tabstop(wd);
			API::effects_edge_nimbus(wd, effects::edge_nimbus::active);
			API::effects_edge_nimbus(wd, effects::edge_nimbus::over);
		}

		bool trigger::enable_pushed(bool eb)
		{
			attr_.enable_pushed = eb;
			return((eb == false) && pushed(false));
		}

		bool trigger::pushed(bool pshd)
		{
			if(pshd != attr_.pushed)
			{
				attr_.pushed = pshd;
				if(false == pshd)
				{
					if (API::find_window(API::cursor_position()) == wdg_->handle())
						attr_.e_state = element_state::hovered;
					else
						attr_.e_state = element_state::normal;
				}
				else
					attr_.e_state = element_state::pressed;

				return true;
			}
			return false;
		}

		bool trigger::pushed() const
		{
			return attr_.pushed;
		}

		void trigger::omitted(bool om)
		{
			attr_.omitted = om;
		}

		bool trigger::focus_color(bool eb)
		{
			if(eb != attr_.focus_color)
			{
				attr_.focus_color = eb;
				return true;
			}
			return false;
		}

		element::cite_bground & trigger::cite()
		{
			return cite_;
		}

		void trigger::refresh(graph_reference graph)
		{
			bool eb = wdg_->enabled();;

			attr_.bgcolor = wdg_->bgcolor();
			attr_.fgcolor = wdg_->fgcolor();

			element_state e_state = attr_.e_state;
			if (eb)
			{
				if (attr_.focused)
				{
					if (element_state::normal == e_state)
						e_state = element_state::focus_normal;
					else if (element_state::hovered == e_state)
						e_state = element_state::focus_hovered;
				}
			}
			else
				e_state = element_state::disabled;

			if (false == cite_.draw(graph, attr_.bgcolor, attr_.fgcolor, ::nana::rectangle{ graph.size() }, e_state))
			{
				if (bground_mode::basic != API::effects_bground_mode(wdg_->handle()))
				{
					_m_draw_background(graph);
					_m_draw_border(graph);
				}
			}
			_m_draw_title(graph, eb);
		}


		void trigger::mouse_enter(graph_reference graph, const arg_mouse&)
		{
			attr_.e_state = (attr_.pushed || attr_.keep_pressed ? element_state::pressed : element_state::hovered);
			refresh(graph);
			API::lazy_refresh();
		}

		void trigger::mouse_leave(graph_reference graph, const arg_mouse&)
		{
			if(attr_.enable_pushed && attr_.pushed)
				return;

			attr_.e_state = element_state::normal;
			refresh(graph);
			API::lazy_refresh();
		}

		void trigger::mouse_down(graph_reference graph, const arg_mouse& arg)
		{
			if (::nana::mouse::left_button != arg.button)
				return;

			_m_press(graph, true);
		}

		void trigger::mouse_up(graph_reference graph, const arg_mouse& arg)
		{
			if (::nana::mouse::left_button != arg.button)
				return;

			_m_press(graph, false);
		}

		void trigger::key_press(graph_reference graph, const arg_keyboard& arg)
		{
			bool ch_tabstop_next;
			switch(arg.key)
			{
			case keyboard::os_arrow_left: case keyboard::os_arrow_up:
				ch_tabstop_next = false;
				break;
			case keyboard::os_arrow_right: case keyboard::os_arrow_down:
				ch_tabstop_next = true;
				break;
			default:
				return;
			}

			API::move_tabstop(*wdg_, ch_tabstop_next);
		}

		void trigger::focus(graph_reference graph, const arg_focus& arg)
		{
			attr_.focused = arg.getting;
			refresh(graph);
			API::lazy_refresh();
		}

		void trigger::_m_draw_title(graph_reference graph, bool enabled)
		{
			wchar_t shortkey;
			std::string::size_type shortkey_pos;
			std::string mbstr = API::transform_shortkey_text(wdg_->caption(), shortkey, &shortkey_pos);
			std::wstring str = to_wstring(mbstr);

			nana::size ts = graph.text_extent_size(str);
			nana::size gsize = graph.size();

			nana::size icon_sz;
			if(attr_.icon)
			{
				icon_sz = attr_.icon->size();
				icon_sz.width += 5;
			}

			nana::point pos{
				static_cast<int>(gsize.width - 1 - ts.width) >> 1, static_cast<int>(gsize.height - 1 - ts.height) >> 1
			};

			if(pos.x < static_cast<int>(icon_sz.width))
				pos.x = static_cast<int>(icon_sz.width);

			unsigned omitted_pixels = gsize.width - icon_sz.width;
			std::size_t txtlen = str.size();
			const auto txtptr = str.c_str();
			if(ts.width)
			{
				nana::paint::text_renderer tr(graph);
				if(enabled)
				{
					if (element_state::pressed == attr_.e_state)
					{
						++pos.x;
						++pos.y;
					}

					graph.palette(true, attr_.focus_color && attr_.focused ? ::nana::color(colors::blue) : attr_.fgcolor);

					if (attr_.omitted)
						tr.render(pos, txtptr, txtlen, omitted_pixels, true);
					else
						graph.bidi_string(pos, txtptr, txtlen);

					if(shortkey)
					{
						unsigned off_w = (shortkey_pos ? graph.text_extent_size(mbstr.c_str(), static_cast<unsigned>(shortkey_pos)).width : 0);

						wchar_t keystr[2] = {nana::utf::char_at(mbstr.c_str() + shortkey_pos, 0, 0), 0};
						auto shortkey_size = graph.text_extent_size(keystr, 1);

						unsigned ascent, descent, inleading;
						graph.text_metrics(ascent, descent, inleading);

						pos.x += off_w;
						pos.y += static_cast<int>(ascent + 2);

						graph.line(pos, point{ pos.x + static_cast<int>(shortkey_size.width) - 1, pos.y }, colors::black);
					}
				}
				else
				{
					graph.palette(true, ::nana::color(colors::white));
					if(attr_.omitted)
					{
						tr.render(point{ pos.x + 1, pos.y + 1 }, txtptr, txtlen, omitted_pixels, true);
						graph.palette(true, ::nana::color(colors::gray));
						tr.render(pos, txtptr, txtlen, omitted_pixels, true);
					}
					else
					{
						graph.bidi_string(point{ pos.x + 1, pos.y + 1 }, txtptr, txtlen);
						graph.palette(true, ::nana::color(colors::gray));
						graph.bidi_string(pos, txtptr, txtlen);
					}
				}
			}

			if(attr_.icon)
				attr_.icon->paste(graph, point{ 3, static_cast<int>(gsize.height - icon_sz.height) / 2 });
		}

		void trigger::_m_draw_background(graph_reference graph)
		{
			nana::rectangle r(graph.size());
			r.pare_off(1);

			auto from = attr_.bgcolor.blend(colors::white, 0.2);
			auto to = attr_.bgcolor.blend(colors::black, 0.95);

			if (element_state::pressed == attr_.e_state)
			{
				r.x = r.y = 2;
				std::swap(from, to);
			}
			graph.gradual_rectangle(r, from, to, true);
		}

		void draw_corner_point(::nana::paint::graphics& graph, const rectangle& r)
		{
			graph.set_pixel(r.x, r.y);
			graph.set_pixel(r.right() - 1, r.y);
			graph.set_pixel(r.right() - 1, r.bottom() - 1);
			graph.set_pixel(r.x, r.bottom() - 1);
		}

		void trigger::_m_draw_border(graph_reference graph)
		{
			nana::rectangle r(graph.size());

			::nana::color lt(static_cast<color_rgb>(0x7f7f7f)), rb(static_cast<color_rgb>(0x707070));
			graph.frame_rectangle(r, lt, lt, rb, rb);

			graph.palette(false, colors::button_face);
			draw_corner_point(graph, r);

			graph.palette(false, static_cast<color_rgb>(0x919191));
			draw_corner_point(graph, r.pare_off(1));

			if (element_state::pressed == attr_.e_state)
				graph.rectangle(r, false, static_cast<color_rgb>(0xc3c3c3));
		}

		void trigger::_m_press(graph_reference graph, bool is_pressed)
		{
			if (is_pressed)
			{
				if (attr_.e_state == element_state::pressed)
					return;

				attr_.e_state = element_state::pressed;
				attr_.keep_pressed = true;
				API::capture_window(*wdg_, true);
			}
			else
			{
				API::capture_window(*wdg_, false);
				attr_.keep_pressed = false;
				if (attr_.enable_pushed && (false == attr_.pushed))
				{
					attr_.pushed = true;
					return;
				}

				if (element_state::pressed == attr_.e_state)
					attr_.e_state = element_state::hovered;
				else
					attr_.e_state = element_state::normal;

				attr_.pushed = false;
			}

			refresh(graph);
			API::lazy_refresh();
		}

		void trigger::emit_click()
		{
			arg_click arg;
			arg.window_handle = wdg_->handle();
			arg.mouse_args = nullptr;
			API::emit_event(event_code::click, arg.window_handle, arg);
		}

		void trigger::icon(const nana::paint::image& img)
		{
			if(img.empty())	return;

			if(nullptr == attr_.icon)
				attr_.icon = new paint::image;
			*attr_.icon = img;
		}
		//end class trigger
	}//end namespace button
}//end namespace drawerbase

		//button
		//@brief: Defaine a button widget and it provides the interfaces to be operational
			button::button(){}

			button::button(window wd, bool visible)
			{
				create(wd, rectangle(), visible);
			}

			button::button(window wd, const std::string& text, bool visible)
			{
				create(wd, rectangle(), visible);
				caption(text);
			}

			button::button(window wd, const char* text, bool visible)
			{
				create(wd, rectangle(), visible);
				caption(text);
			}

			button::button(window wd, const rectangle& r, bool visible)
			{
				create(wd, r, visible);
			}

			button& button::icon(const nana::paint::image& img)
			{
				internal_scope_guard isg;
				get_drawer_trigger().icon(img);
				API::refresh_window(handle());
				return *this;
			}

			button& button::enable_pushed(bool eb)
			{
				internal_scope_guard isg;
				if(get_drawer_trigger().enable_pushed(eb))
					API::refresh_window(handle());
				return *this;
			}

			bool button::pushed() const
			{
				return get_drawer_trigger().pushed();
			}

			button& button::pushed(bool psd)
			{
				internal_scope_guard isg;
				if(get_drawer_trigger().pushed(psd))
					API::refresh_window(handle());
				return *this;
			}

			button& button::omitted(bool om)
			{
				internal_scope_guard isg;
				get_drawer_trigger().omitted(om);
				API::refresh_window(handle());
				return *this;
			}

			button& button::enable_focus_color(bool eb)
			{
				internal_scope_guard lock;
				if(get_drawer_trigger().focus_color(eb))
					API::refresh_window(handle());
				return *this;
			}

			button& button::set_bground(const pat::cloneable<element::element_interface>& rv)
			{
				internal_scope_guard lock;
				get_drawer_trigger().cite().set(rv);
				return *this;
			}

			button& button::set_bground(const std::string& name)
			{
				internal_scope_guard lock;
				get_drawer_trigger().cite().set(name.data());
				return *this;
			}

			button& button::transparent(bool enabled)
			{
				if (enabled)
					API::effects_bground(*this, effects::bground_transparent(0), 0.0);
				else
					API::effects_bground_remove(*this);
				return *this;
			}

			bool button::transparent() const
			{
				return (bground_mode::basic == API::effects_bground_mode(*this));
			}

			button& button::edge_effects(bool enable)
			{
				if (enable)
				{
					API::effects_edge_nimbus(*this, effects::edge_nimbus::active);
					API::effects_edge_nimbus(*this, effects::edge_nimbus::over);
				}
				else
					API::effects_edge_nimbus(*this, effects::edge_nimbus::none);

				return *this;
			}

			void button::_m_complete_creation()
			{
				events().shortkey.connect_unignorable([this]
				{
					get_drawer_trigger().emit_click();
				});
			}

			void button::_m_caption(native_string_type&& text)
			{
				API::unregister_shortkey(handle());

				wchar_t shortkey;
				API::transform_shortkey_text(to_utf8(text), shortkey, nullptr);
				if (shortkey)
					API::register_shortkey(handle(), shortkey);

				base_type::_m_caption(std::move(text));
			}
		//end class button
}//end namespace nana

