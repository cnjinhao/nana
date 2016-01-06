/*
 *	The fundamental widget class implementation
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/widget.cpp
 */

#include <nana/gui/widgets/widget.hpp>
#include <nana/gui/tooltip.hpp>
#include <nana/gui/detail/widget_notifier_interface.hpp>

namespace nana
{
	namespace internationalization_parts
	{
		void set_eval(window, i18n_eval&&);
	}

	//class widget
	//@brief:The definition of class widget
		class widget::inner_widget_notifier : public detail::widget_notifier_interface
		{
		public:
			inner_widget_notifier(widget& wdg)
				: wdg_(wdg)
			{}

		private:
			//implementation of widget_notifier_interface
			widget* widget_ptr() const override
			{
				return &wdg_;
			}

			void destroy() override
			{
				wdg_._m_notify_destroy();
			}

			native_string_type caption() override
			{
				return wdg_._m_caption();
			}

			void caption(native_string_type text) override
			{
				wdg_._m_caption(std::move(text));
			}
		private:
			widget& wdg_;
		};

		std::string widget::caption() const throw()
		{
			return to_utf8(_m_caption());
		}

		std::wstring widget::caption_wstring() const throw()
		{
#if defined(NANA_WINDOWS)
			return _m_caption();
#else
			return to_wstring(_m_caption());
#endif
		}

		auto widget::caption_native() const throw() -> native_string_type
		{
			return _m_caption();
		}

		widget& widget::caption(std::string utf8)
		{
			::nana::throw_not_utf8(utf8);
			native_string_type str = to_nstring(utf8);
			_m_caption(std::move(str));
			return *this;
		}

		widget& widget::caption(std::wstring text)
		{
			native_string_type str = to_nstring(text);
			_m_caption(std::move(str));
			return *this;
		}

		void widget::i18n(i18n_eval eval)
		{
			if (handle())
			{
				native_string_type str = to_nstring(eval());
				_m_caption(std::move(str));
				internationalization_parts::set_eval(handle(), std::move(eval));
			}
		}

		nana::cursor widget::cursor() const
		{
			return _m_cursor();
		}

		void widget::cursor(nana::cursor cur)
		{
			_m_cursor(cur);
		}

		void widget::typeface(const nana::paint::font& font)
		{
			_m_typeface(font);
		}

		nana::paint::font widget::typeface() const
		{
			return _m_typeface();
		}

		void widget::close()
		{
			_m_close();
		}

		window widget::parent() const
		{
			return API::get_parent_window(handle());
		}

		bool widget::enabled() const
		{
			return API::window_enabled(handle());
		}

		void widget::enabled(bool value)
		{
			_m_enabled(value);
		}

		void widget::enable_dropfiles(bool enb)
		{
			API::enable_dropfiles(handle(), enb);
		}

		bool widget::empty() const
		{
			return (nullptr == handle());
		}

		void widget::focus()
		{
			API::focus_window(handle());
		}

		bool widget::focused() const
		{
			return (API::focus_window() == handle());
		}

		void widget::show()
		{
			_m_show(true);
		}

		void widget::hide()
		{
			_m_show(false);
		}

		bool widget::visible() const
		{
			return _m_visible();
		}

		nana::size widget::size() const
		{
			return API::window_size(handle());
		}

		void widget::size(const nana::size& sz)
		{
			_m_size(sz);
		}

		nana::point widget::pos() const
		{
			return API::window_position(handle());
		}

		void widget::move(int x, int y)
		{
			_m_move(x, y);
		}

		void widget::move(const point& pos)
		{
			_m_move(pos.x, pos.y);
		}

		void widget::move(const rectangle& r)
		{
			_m_move(r);
		}

		void widget::fgcolor(const nana::color& col)
		{
			_m_fgcolor(col);
		}

		nana::color widget::fgcolor() const
		{
			return _m_fgcolor();
		}

		void widget::bgcolor(const nana::color& col)
		{
			_m_bgcolor(col);
		}

		nana::color widget::bgcolor() const
		{
			return _m_bgcolor();
		}

		general_events& widget::events() const
		{
			return _m_get_general_events();
		}

		void widget::umake_event(event_handle eh) const
		{
			API::umake_event(eh);
		}

		widget& widget::register_shortkey(wchar_t key)
		{
			if (key)
				API::register_shortkey(handle(), static_cast<unsigned long>(key));
			else
				API::unregister_shortkey(handle());
			return *this;
		}

		widget& widget::take_active(bool activated, window take_if_not_activated)
		{
			API::take_active(handle(), activated, take_if_not_activated);
			return *this;
		}

		widget& widget::tooltip(const ::std::string& text)
		{
			nana::tooltip::set(*this, text);
			return *this;
		}

		widget::operator widget::dummy_bool_type() const
		{
			return (handle()? dummy_bool_type(1):0);
		}

		widget::operator window() const
		{
			return handle();
		}

		std::unique_ptr<::nana::detail::widget_notifier_interface> widget::_m_wdg_notifier()
		{
			return std::unique_ptr<::nana::detail::widget_notifier_interface>(new inner_widget_notifier(*this));
		}

		void widget::_m_complete_creation()
		{}

		auto widget::_m_caption() const throw() -> native_string_type
		{
			return API::dev::window_caption(handle());
		}

		void widget::_m_caption(native_string_type&& str)
		{
			API::dev::window_caption(handle(), std::move(str));
		}

		nana::cursor widget::_m_cursor() const
		{
			return API::window_cursor(handle());
		}

		void widget::_m_cursor(nana::cursor cur)
		{
			API::window_cursor(handle(), cur);
		}

		void widget::_m_close()
		{
			API::close_window(handle());
		}

		bool widget::_m_enabled() const
		{
			return API::window_enabled(handle());
		}

		void widget::_m_enabled(bool value)
		{
			API::window_enabled(handle(), value);
		}

		bool widget::_m_show(bool visible)
		{
			API::show_window(handle(), visible);
			return visible;
		}

		bool widget::_m_visible() const
		{
			return API::visible(handle());
		}

		void widget::_m_size(const nana::size& sz)
		{
			API::window_size(handle(), sz);
		}

		void widget::_m_move(int x, int y)
		{
			API::move_window(handle(), { x, y });
		}

		void widget::_m_move(const rectangle& r)
		{
			API::move_window(handle(), r);
		}

		void widget::_m_typeface(const paint::font& font)
		{
			API::typeface(handle(), font);
		}

		nana::paint::font widget::_m_typeface() const
		{
			return API::typeface(handle());
		}

		void widget::_m_fgcolor(const nana::color& col)
		{
			API::fgcolor(handle(), col);
		}

		nana::color widget::_m_fgcolor() const
		{
			return API::fgcolor(handle());
		}

		void widget::_m_bgcolor(const nana::color& col)
		{
			API::bgcolor(handle(), col);
		}

		nana::color widget::_m_bgcolor() const
		{
			return API::bgcolor(handle());
		}
	//end class widget

	namespace detail
	{
		std::unique_ptr<widget_notifier_interface> widget_notifier_interface::get_notifier(widget* wdg)
		{
			return std::unique_ptr<widget_notifier_interface>(new widget::inner_widget_notifier(*wdg));
		}
	}
}//end namespace nana

