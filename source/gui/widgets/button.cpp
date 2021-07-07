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
		class trigger::content_measurer
			: public dev::widget_content_measurer_interface
		{
		public:
			content_measurer(impl* imp):
				impl_{ imp }
			{}

			//Move the definition after impl
			std::optional<size> measure(graph_reference graph, unsigned limit_pixels, bool /*limit_width*/) const override;

			size extension() const override
			{
				return { 14, 10};
			}
		private:
			impl * const impl_;
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
			paint::image	icon;
			::nana::color	bgcolor;
			::nana::color	fgcolor;
		};

		struct trigger::impl {
			widget* wdg{nullptr};
			element::cite_bground cite{ "button" };
			content_measurer measurer;
			attributes attr;

			impl() :
				measurer{ this }
			{}
		};

		std::optional<size> trigger::content_measurer::measure(graph_reference graph, unsigned limit_pixels, bool /*limit_width*/) const
		{
			//Button doesn't provide a support of vfit and hfit
			if (limit_pixels)
				return{};

			//This method is according to trigger::_m_draw_title

			wchar_t shortkey;
			auto content_size = graph.text_extent_size(api::transform_shortkey_text(impl_->wdg->caption(), shortkey, nullptr));

			if (impl_->attr.icon)
			{
				auto icon_sz = impl_->attr.icon.size();
				content_size.width += icon_sz.width + 5;
				content_size.height = std::max(content_size.height, icon_sz.height);
			}

			return content_size;
		}

		trigger::trigger():
			impl_(new impl)
		{
		}

		trigger::~trigger()
		{
		}

		void trigger::attached(widget_reference wdg, graph_reference)
		{
			impl_->wdg = &wdg;
			window wd = wdg;

			api::dev::enable_space_click(wd, true);
			api::tabstop(wd, true);
			api::effects_edge_nimbus(wd, effects::edge_nimbus::active);
			api::effects_edge_nimbus(wd, effects::edge_nimbus::over);
			api::dev::set_measurer(wd, &impl_->measurer);
		}

		bool trigger::enable_pushed(bool eb)
		{
			impl_->attr.enable_pushed = eb;
			return((eb == false) && pushed(false));
		}

		bool trigger::pushed(bool pshd)
		{
			if(pshd != impl_->attr.pushed)
			{
				impl_->attr.pushed = pshd;
				if(false == pshd)
				{
					if (api::find_window(api::cursor_position()) == impl_->wdg->handle())
						impl_->attr.e_state = element_state::hovered;
					else
						impl_->attr.e_state = element_state::normal;
				}
				else
					impl_->attr.e_state = element_state::pressed;

				return true;
			}
			return false;
		}

		const trigger::impl* trigger::get_impl() const
		{
			return impl_.get();
		}

		trigger::impl* trigger::get_impl()
		{
			return impl_.get();
		}

		void trigger::refresh(graph_reference graph)
		{
			bool eb = impl_->wdg->enabled();;

			impl_->attr.bgcolor = impl_->wdg->bgcolor();
			impl_->attr.fgcolor = impl_->wdg->fgcolor();

			element_state e_state = impl_->attr.e_state;
			if (eb)
			{
				if (impl_->attr.focused)
				{
					if (element_state::normal == e_state)
						e_state = element_state::focus_normal;
					else if (element_state::hovered == e_state)
						e_state = element_state::focus_hovered;
				}
			}
			else
				e_state = element_state::disabled;

			if (false == impl_->cite.draw(graph, impl_->attr.bgcolor, impl_->attr.fgcolor, ::nana::rectangle{ graph.size() }, e_state))
			{
				if (api::is_transparent_background(*impl_->wdg))
					api::dev::copy_transparent_background(*impl_->wdg, graph);
				else
					_m_draw_background(graph);

				if (false == API::widget_borderless(*impl_->wdg))
					_m_draw_border(graph);
			}
			_m_draw_title(graph, eb);
		}


		void trigger::mouse_enter(graph_reference graph, const arg_mouse&)
		{
			impl_->attr.e_state = (impl_->attr.pushed || impl_->attr.keep_pressed ? element_state::pressed : element_state::hovered);
			refresh(graph);
			api::dev::lazy_refresh();
		}

		void trigger::mouse_leave(graph_reference graph, const arg_mouse&)
		{
			if(impl_->attr.enable_pushed && impl_->attr.pushed)
				return;

			impl_->attr.e_state = element_state::normal;
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

			api::move_tabstop(*impl_->wdg, ch_tabstop_next);
		}

		void trigger::focus(graph_reference graph, const arg_focus& arg)
		{
			impl_->attr.focused = arg.getting;
			refresh(graph);
			api::dev::lazy_refresh();
		}

		void trigger::_m_draw_title(graph_reference graph, bool enabled)
		{
			wchar_t shortkey;
			std::string::size_type shortkey_pos;
			std::string mbstr = api::transform_shortkey_text(impl_->wdg->caption(), shortkey, &shortkey_pos);
			std::wstring str = to_wstring(mbstr);

			nana::size ts = graph.text_extent_size(str);
			nana::size gsize = graph.size();

			//Text position
			auto tmp_sz = gsize - 1 - ts;
			point pos{
				static_cast<int>(tmp_sz.width) / 2, static_cast<int>(tmp_sz.height) / 2
			};

			auto icon_sz = impl_->attr.icon.size();
			if (impl_->attr.icon)
			{
				icon_sz.width += 5;
				pos.x += icon_sz.width / 2;
			}

			if (pos.x - static_cast<int>(icon_sz.width) < 4)
				pos.x += 4 - pos.x + icon_sz.width;
			

			if(ts.width)
			{
				auto const omitted_pixels = gsize.width - icon_sz.width - icon_sz.width / 2;

				std::wstring_view text_sv{ str.data(), str.size() };
				nana::paint::text_renderer tr(graph);
				if(enabled)
				{
					if (element_state::pressed == impl_->attr.e_state)
						++pos;

					auto text_color = (impl_->attr.focus_color && impl_->attr.focused ? ::nana::color(colors::blue) : impl_->attr.fgcolor);
					graph.palette(true, text_color);

					if (impl_->attr.omitted)
						tr.render(pos, text_sv, omitted_pixels, paint::text_renderer::mode::truncate_with_ellipsis);
					else
						graph.bidi_string(pos, text_sv);

					api::dev::draw_shortkey_underline(graph, mbstr, shortkey, shortkey_pos, pos, text_color);
				}
				else
				{
					graph.palette(true, color{ colors::white });
					if(impl_->attr.omitted)
					{
						tr.render(point{ pos.x + 1, pos.y + 1 }, text_sv, omitted_pixels, paint::text_renderer::mode::truncate_with_ellipsis);
						graph.palette(true, color{ colors::gray });
						tr.render(pos, text_sv, omitted_pixels, paint::text_renderer::mode::truncate_with_ellipsis);
					}
					else
					{
						graph.bidi_string(point{ pos.x + 1, pos.y + 1 }, text_sv);
						graph.palette(true, color{ colors::gray });
						graph.bidi_string(pos, text_sv);
					}
				}
			}

			if(impl_->attr.icon)
				impl_->attr.icon.paste(graph, point{ pos.x - static_cast<int>(icon_sz.width) - 1,
					static_cast<int>(gsize.height - icon_sz.height) / 2 + (element_state::pressed == impl_->attr.e_state) });
		}

		void trigger::_m_draw_background(graph_reference graph)
		{
			nana::rectangle r(graph.size());
			r.pare_off(1);

			if (impl_->attr.gradual_background)
			{
				auto from = impl_->attr.bgcolor.blend(colors::white, 0.8);
				auto to = impl_->attr.bgcolor.blend(colors::black, 0.05);

				if (element_state::pressed == impl_->attr.e_state)
				{
					r.x = r.y = 2;
					std::swap(from, to);
				}
				graph.gradual_rectangle(r, from, to, true);
				return;
			}
			
			graph.rectangle(true, impl_->attr.bgcolor);
		}

		void trigger::_m_draw_border(graph_reference graph)
		{
			nana::rectangle r(graph.size());

			::nana::color lt(static_cast<color_rgb>(0x7f7f7f)), rb(static_cast<color_rgb>(0x707070));
			graph.frame_rectangle(r, lt, lt, rb, rb);

			//Render the cornors of border using the color that is blended with parent's background color.
			graph.palette(false, api::bgcolor(impl_->wdg->parent()).blend(colors::button_face, 0.5));

			paint::draw draw(graph);
			draw.corner(r, 1);

			graph.palette(false, static_cast<color_rgb>(0x919191));

			draw.corner(r.pare_off(1), 1);

			if (element_state::pressed == impl_->attr.e_state)
				graph.rectangle(r, false, static_cast<color_rgb>(0xc3c3c3));
		}

		void trigger::_m_press(graph_reference graph, bool is_pressed)
		{
			if (is_pressed)
			{
				if (impl_->attr.e_state == element_state::pressed)
					return;

				impl_->attr.e_state = element_state::pressed;
				impl_->attr.keep_pressed = true;
				impl_->wdg->set_capture(true);
			}
			else
			{
				impl_->wdg->release_capture();

				impl_->attr.keep_pressed = false;
				if (impl_->attr.enable_pushed && (false == impl_->attr.pushed))
				{
					impl_->attr.pushed = true;
					return;
				}

				if (element_state::pressed == impl_->attr.e_state)
					impl_->attr.e_state = element_state::hovered;
				else
					impl_->attr.e_state = element_state::normal;

				impl_->attr.pushed = false;
			}

			refresh(graph);
			api::dev::lazy_refresh();
		}

		void trigger::emit_click()
		{
			arg_click arg;
			arg.window_handle = impl_->wdg->handle();
			arg.mouse_args = nullptr;
			api::emit_event(event_code::click, arg.window_handle, arg);
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
		get_drawer_trigger().get_impl()->attr.icon = img;
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
		return get_drawer_trigger().get_impl()->attr.pushed;
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
		auto& attr = get_drawer_trigger().get_impl()->attr;
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
		auto& attr = get_drawer_trigger().get_impl()->attr;
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
		auto& attr = get_drawer_trigger().get_impl()->attr;
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
		get_drawer_trigger().get_impl()->cite.set(rv);
		return *this;
	}

	button& button::set_bground(const std::string& name)
	{
		internal_scope_guard lock;
		get_drawer_trigger().get_impl()->cite.set(name.data());
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

