/*
 *	The fundamental widget class implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/widget.cpp
 */

#include <nana/gui/widgets/widget.hpp>
#include <nana/gui/tooltip.hpp>

namespace nana
{
	namespace internationalization_parts
	{
		void set_eval(window, i18n_eval&&);
	}
	//class widget
	//@brief:The definition of class widget
		widget::~widget(){}

		nana::string widget::caption() const
		{
			return this->_m_caption();
		}

		void widget::caption(nana::string str)
		{
			_m_caption(std::move(str));
		}

		void widget::i18n(i18n_eval eval)
		{
			if (handle())
			{
				_m_caption(eval());
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
			return API::is_focus_window(handle());
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

		void widget::move(const rectangle& r)
		{
			_m_move(r);
		}

		void widget::foreground(nana::color_t value)
		{
			_m_foreground(value);
		}

		nana::color_t widget::foreground() const
		{
			return _m_foreground();
		}

		void widget::background(nana::color_t value)
		{
			_m_background(value);
		}

		nana::color_t widget::background() const
		{
			return _m_background();
		}

		general_events& widget::events() const
		{
			return _m_get_general_events();
		}

		void widget::umake_event(event_handle eh) const
		{
			API::umake_event(eh);
		}

		widget& widget::tooltip(const nana::string& text)
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

		void widget::_m_complete_creation()
		{}

		nana::string widget::_m_caption() const
		{
			return API::dev::window_caption(handle());
		}

		void widget::_m_caption(nana::string&& str)
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
			API::move_window(handle(), x, y);
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

		void widget::_m_foreground(nana::color_t value)
		{
			API::foreground(handle(), value);
		}

		nana::color_t widget::_m_foreground() const
		{
			return API::foreground(handle());
		}

		void widget::_m_background(nana::color_t value)
		{
			API::background(handle(), value);
		}

		nana::color_t widget::_m_background() const
		{
			return API::background(handle());
		}

	//end class widget
}//end namespace nana

