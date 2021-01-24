/*
 *	A Button Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2021 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/button.cpp
 */

#include <nana/gui/widgets/button.hpp>
#include <nana/gui/detail/widget_content_measurer_interface.hpp>

#include <nana/paint/text_renderer.hpp>

namespace nana
{
	namespace drawerbase::button
	{
		class trigger::measurer
			: public dev::widget_content_measurer_interface
		{
		public:
			measurer(trigger* t)
				: trigger_{ t }
			{}

			std::optional<size> measure(graph_reference graph, unsigned limit_pixels, bool /*limit_width*/) const override
			{
				//Button doesn't provide a support of vfit and hfit
				if (limit_pixels)
					return{};

				wchar_t shortkey;
				return graph.text_extent_size(api::transform_shortkey_text(trigger_->wdg_->caption(), shortkey, nullptr));
			}

			size extension() const override
			{
				return { 14, 10};
			}
		private:
			trigger * trigger_;
		};

		//trigger
		//@brief: draw the button
		struct trigger::attributes {
			element_state e_state{element_state::normal};
			bool omitted{ false };
			bool focused{ false };
			bool pushed{ false };
			bool keep_pressed{ false };
			bool enable_pushed{ false };
			bool focus_color{ true };
			bool gradual_background{ true };
			paint::image* icon{ nullptr };
			::nana::color bgcolor;
			::nana::color fgcolor;
		};

		trigger::trigger():
			attr_(new attributes)
		{
			measurer_.reset(new measurer{this});
		}

		trigger::~trigger()
		{
			delete attr_->icon;
		}

		void trigger::attached(widget_reference widget, graph_reference graph)
		{
			graph_ = &graph;

			wdg_ = &widget;
			window wd = widget;

			api::dev::enable_space_click(widget, true);
			api::tabstop(wd);
			api::effects_edge_nimbus(wd, effects::edge_nimbus::active);
			api::effects_edge_nimbus(wd, effects::edge_nimbus::over);
			api::dev::set_measurer(widget, measurer_.get());
		}

		bool trigger::enable_pushed(bool eb)
		{
			attr_->enable_pushed = eb;
			return((eb == false) && pushed(false));
		}

		bool trigger::pushed(bool pshd)
		{
			if(pshd != attr_->pushed)
			{
				attr_->pushed = pshd;
				if(false == pshd)
				{
					if (api::find_window(api::cursor_position()) == wdg_->handle())
						attr_->e_state = element_state::hovered;
					else
						attr_->e_state = element_state::normal;
				}
				else
					attr_->e_state = element_state::pressed;

				return true;
			}
			return false;
		}

		const trigger::attributes& trigger::attr() const
		{
			return *attr_;
		}

		trigger::attributes& trigger::attr()
		{
			return *attr_;
		}

		element::cite_bground & trigger::cite()
		{
			return cite_;
		}

		void trigger::refresh(graph_reference graph)
		{
			bool eb = wdg_->enabled();;

			attr_->bgcolor = wdg_->bgcolor();
			attr_->fgcolor = wdg_->fgcolor();

			element_state e_state = attr_->e_state;
			if (eb)
			{
				if (attr_->focused)
				{
					if (element_state::normal == e_state)
						e_state = element_state::focus_normal;
					else if (element_state::hovered == e_state)
						e_state = element_state::focus_hovered;
				}
			}
			else
				e_state = element_state::disabled;

			if (false == cite_.draw(graph, attr_->bgcolor, attr_->fgcolor, ::nana::rectangle{ graph.size() }, e_state))
			{
				if (api::is_transparent_background(*wdg_))
					api::dev::copy_transparent_background(*wdg_, graph);
				else
					_m_draw_background(graph);

				_m_draw_border(graph);
			}
			_m_draw_title(graph, eb);
		}


		void trigger::mouse_enter(graph_reference graph, const arg_mouse&)
		{
			attr_->e_state = (attr_->pushed || attr_->keep_pressed ? element_state::pressed : element_state::hovered);
			refresh(graph);
			api::dev::lazy_refresh();
		}

		void trigger::mouse_leave(graph_reference graph, const arg_mouse&)
		{
			if(attr_->enable_pushed && attr_->pushed)
				return;

			attr_->e_state = element_state::normal;
			refresh(graph);
			api::dev::lazy_refresh();
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

		void trigger::key_press(graph_reference, const arg_keyboard& arg)
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

			api::move_tabstop(*wdg_, ch_tabstop_next);
		}

		void trigger::focus(graph_reference graph, const arg_focus& arg)
		{
			attr_->focused = arg.getting;
			refresh(graph);
			api::dev::lazy_refresh();
		}

		void trigger::_m_draw_title(graph_reference graph, bool enabled)
		{
			wchar_t shortkey;
			std::string::size_type shortkey_pos;
			std::string mbstr = api::transform_shortkey_text(wdg_->caption(), shortkey, &shortkey_pos);
			std::wstring str = to_wstring(mbstr);

			nana::size ts = graph.text_extent_size(str);
			nana::size gsize = graph.size();

			nana::size icon_sz;
			if(attr_->icon)
			{
				icon_sz = attr_->icon->size();
				icon_sz.width += 5;
			}

			nana::point pos{
				static_cast<int>(gsize.width - 1 - ts.width) >> 1, static_cast<int>(gsize.height - 1 - ts.height) >> 1
			};

			if(pos.x < static_cast<int>(icon_sz.width))
				pos.x = static_cast<int>(icon_sz.width);
			if(attr_->icon) pos.x += icon_sz.width / 2;

			unsigned omitted_pixels = gsize.width - icon_sz.width - icon_sz.width / 2;
			std::size_t txtlen = str.size();
			const auto txtptr = str.c_str();
			if(ts.width)
			{
				nana::paint::text_renderer tr(graph);
				if(enabled)
				{
					if (element_state::pressed == attr_->e_state)
					{
						++pos.x;
						++pos.y;
					}

					auto text_color = (attr_->focus_color && attr_->focused ? ::nana::color(colors::blue) : attr_->fgcolor);
					graph.palette(true, text_color);

					if (attr_->omitted)
						tr.render(pos, txtptr, txtlen, omitted_pixels, paint::text_renderer::mode::truncate_with_ellipsis);
					else
						graph.bidi_string(pos, { txtptr, txtlen });

					api::dev::draw_shortkey_underline(graph, mbstr, shortkey, shortkey_pos, pos, text_color);
				}
				else
				{
					graph.palette(true, color{ colors::white });
					if(attr_->omitted)
					{
						tr.render(point{ pos.x + 1, pos.y + 1 }, txtptr, txtlen, omitted_pixels, paint::text_renderer::mode::truncate_with_ellipsis);
						graph.palette(true, color{ colors::gray });
						tr.render(pos, txtptr, txtlen, omitted_pixels, paint::text_renderer::mode::truncate_with_ellipsis);
					}
					else
					{
						graph.bidi_string(point{ pos.x + 1, pos.y + 1 }, { txtptr, txtlen });
						graph.palette(true, color{ colors::gray });
						graph.bidi_string(pos, { txtptr, txtlen });
					}
				}
			}

			if(attr_->icon)
				attr_->icon->paste(graph, point{ pos.x - static_cast<int>(icon_sz.width) - 1,
					static_cast<int>(gsize.height - icon_sz.height) / 2 + (element_state::pressed == attr_->e_state) });
		}

		void trigger::_m_draw_background(graph_reference graph)
		{
			nana::rectangle r(graph.size());
			r.pare_off(1);

			if (attr_->gradual_background)
			{
				auto from = attr_->bgcolor.blend(colors::white, 0.8);
				auto to = attr_->bgcolor.blend(colors::black, 0.05);

				if (element_state::pressed == attr_->e_state)
				{
					r.x = r.y = 2;
					std::swap(from, to);
				}
				graph.gradual_rectangle(r, from, to, true);
				return;
			}
			
			graph.rectangle(true, attr_->bgcolor);
		}

		void trigger::_m_draw_border(graph_reference graph)
		{
			nana::rectangle r(graph.size());

			::nana::color lt(static_cast<color_rgb>(0x7f7f7f)), rb(static_cast<color_rgb>(0x707070));
			graph.frame_rectangle(r, lt, lt, rb, rb);

			//Render the cornors of border using the color that is blended with parent's background color.
			graph.palette(false, api::bgcolor(wdg_->parent()).blend(colors::button_face, 0.5));

			paint::draw draw(graph);
			draw.corner(r, 1);

			graph.palette(false, static_cast<color_rgb>(0x919191));

			draw.corner(r.pare_off(1), 1);

			if (element_state::pressed == attr_->e_state)
				graph.rectangle(r, false, static_cast<color_rgb>(0xc3c3c3));
		}

		void trigger::_m_press(graph_reference graph, bool is_pressed)
		{
			if (is_pressed)
			{
				if (attr_->e_state == element_state::pressed)
					return;

				attr_->e_state = element_state::pressed;
				attr_->keep_pressed = true;
				wdg_->set_capture(true);
			}
			else
			{
				wdg_->release_capture();

				attr_->keep_pressed = false;
				if (attr_->enable_pushed && (false == attr_->pushed))
				{
					attr_->pushed = true;
					return;
				}

				if (element_state::pressed == attr_->e_state)
					attr_->e_state = element_state::hovered;
				else
					attr_->e_state = element_state::normal;

				attr_->pushed = false;
			}

			refresh(graph);
			api::dev::lazy_refresh();
		}

		void trigger::emit_click()
		{
			arg_click arg;
			arg.window_handle = wdg_->handle();
			arg.mouse_args = nullptr;
			api::emit_event(event_code::click, arg.window_handle, arg);
		}

		void trigger::icon(const nana::paint::image& img)
		{
			if(img.empty())
			{
				delete attr_->icon;
				attr_->icon = nullptr;
				return;
			}

			if(nullptr == attr_->icon)
				attr_->icon = new paint::image;
			*attr_->icon = img;
		}
		//end class trigger
	}//end namespace drawerbase::button

	//button
	//@brief: Define a button widget and it provides the interfaces to be operational
	button::button(){}

	button::button(window parent, std::string_view title, bool visible)
	{
		create(parent, rectangle(), visible);
		caption(title);
	}

	button::button(window parent, std::wstring_view title, bool visible)
	{
		create(parent, rectangle(), visible);
		caption(title);
	}

#ifdef __cpp_char8_t
	button::button(window parent, std::u8string_view title, bool visible)
	{
		create(parent, rectangle(), visible);
		caption(title);
	}
#endif

	button::button(window wd, const rectangle& r, bool visible)
	{
		create(wd, r, visible);
	}

	button& button::icon(const nana::paint::image& img)
	{
		internal_scope_guard lock;
		get_drawer_trigger().icon(img);
		api::refresh_window(handle());
		return *this;
	}

	button& button::enable_pushed(bool eb)
	{
		internal_scope_guard lock;
		if(get_drawer_trigger().enable_pushed(eb))
			api::refresh_window(handle());
		return *this;
	}

	bool button::pushed() const
	{
		internal_scope_guard lock;
		return get_drawer_trigger().attr().pushed;
	}

	button& button::pushed(bool psd)
	{
		internal_scope_guard isg;
		if(get_drawer_trigger().pushed(psd))
			api::refresh_window(handle());
		return *this;
	}

	button& button::omitted(bool enabled)
	{
		internal_scope_guard lock;
		auto& attr = get_drawer_trigger().attr();
		if (attr.omitted != enabled)
		{
			attr.omitted = enabled;
			api::refresh_window(handle());
		}
		return *this;
	}

	button& button::enable_focus_color(bool enabled)
	{
		internal_scope_guard lock;
		auto& attr = get_drawer_trigger().attr();
		if (attr.focus_color != enabled)
		{
			attr.focus_color = enabled;
			api::refresh_window(handle());
		}
		return *this;
	}

	button& button::enable_gradual_background(bool enabled)
	{
		internal_scope_guard lock;
		auto& attr = get_drawer_trigger().attr();
		if (attr.gradual_background != enabled)
		{
			attr.gradual_background = enabled;
			api::refresh_window(handle());
		}
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
			api::effects_bground(*this, effects::bground_transparent(0), 0.0);
		else
			api::effects_bground_remove(*this);
		return *this;
	}

	bool button::transparent() const
	{
		return api::is_transparent_background(*this);
	}

	button& button::edge_effects(bool enable)
	{
		if (enable)
		{
			api::effects_edge_nimbus(*this, effects::edge_nimbus::active);
			api::effects_edge_nimbus(*this, effects::edge_nimbus::over);
		}
		else
			api::effects_edge_nimbus(*this, effects::edge_nimbus::none);

		return *this;
	}

	void button::_m_complete_creation()
	{
		events().shortkey.connect_unignorable([this](const arg_keyboard&)
		{
			get_drawer_trigger().emit_click();
		});
	}

	void button::_m_caption(native_string_type&& text)
	{
		api::unregister_shortkey(handle());

		wchar_t shortkey;
		api::transform_shortkey_text(to_utf8(text), shortkey, nullptr);
		if (shortkey)
			api::register_shortkey(handle(), shortkey);

		base_type::_m_caption(std::move(text));
	}
	//end class button
}//end namespace nana

