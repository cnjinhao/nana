/*
 *	The fundamental widget class implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2014 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/widget.hpp
 */

#ifndef NANA_GUI_WIDGET_HPP
#define NANA_GUI_WIDGET_HPP
#include <nana/traits.hpp>
#include "../basis.hpp"
#include "../programming_interface.hpp"
#include <nana/internationalization.hpp>
#include <nana/gui/detail/drawer.hpp>
#include <nana/gui/layout_utility.hpp>
#include <functional>

namespace nana
{
	class drawer_trigger;

	        /// Abstract class for defining the capacity interface.
	class widget
		: nana::noncopyable, nana::nonmovable
	{
		typedef void(*dummy_bool_type)(widget* (*)(const widget&));
	public:
		virtual ~widget();
		virtual window handle() const = 0;			///< Returns the handle of window, returns 0 if window is not created.
		bool empty() const;							///< Determines whether the manipulator is handling a window.
		void close();

		window parent() const;

		nana::string caption() const;
		void caption(nana::string);

		template<typename ...Args>
		void i18n(std::string msgid, Args&&... args)
		{
			_m_caption(nana::internationalization().get(msgid, std::forward<Args>(args)...));
		}

		void i18n(i18n_eval);

		void cursor(nana::cursor);
		nana::cursor cursor() const;		///< Retrieves the shape of cursor

		void typeface(const paint::font& font);
		paint::font typeface() const;

		bool enabled() const;				///< Determines whether the window is enabled for mouse and keyboard input.
		void enabled(bool);

		void enable_dropfiles(bool);		///< Enables/Disables a window to accept dropped files.

		void focus();
		bool focused() const;

		void show();						///< Sets the window visible.
		void hide();						///< Sets the window invisible.
		bool visible() const;

		nana::size size() const;
		void size(const nana::size&);
		
		point pos() const;
		void move(int x, int y);
		void move(const rectangle&);

		void foreground(nana::color_t);
		nana::color_t foreground() const;
		void background(nana::color_t);
		nana::color_t background() const;

		general_events& events() const;

		void umake_event(event_handle eh) const;              ///< Deletes an event callback by a handle.
		widget& tooltip(const nana::string&);

		operator dummy_bool_type() const;
		operator window() const;
	protected:
		//protected members, a derived class must call this implementation if it overrides an implementation
		virtual void _m_complete_creation();

		virtual general_events& _m_get_general_events() const = 0;
		virtual nana::string _m_caption() const;
		virtual void _m_caption(nana::string&&);
		virtual nana::cursor _m_cursor() const;
		virtual void _m_cursor(nana::cursor);
		virtual void _m_close();
		virtual bool _m_enabled() const;
		virtual void _m_enabled(bool);
		virtual bool _m_show(bool);
		virtual bool _m_visible() const;
		virtual void _m_size(const nana::size&);
		virtual void _m_move(int x, int y);
		virtual void _m_move(const rectangle&);

		virtual void _m_typeface(const nana::paint::font& font);
		virtual nana::paint::font _m_typeface() const;

		virtual void _m_foreground(nana::color_t);
		virtual nana::color_t _m_foreground() const;
		virtual void _m_background(nana::color_t);
		virtual nana::color_t _m_background() const;
	};

            /// Base class of all the classes defined as a widget window. Defaultly a widget_tag
	template<typename Category, typename DrawerTrigger, typename Events = nana::general_events>
	class widget_object: public widget
	{
	protected:
		typedef DrawerTrigger drawer_trigger_t;
	public:
		widget_object()
			: events_(std::make_shared<Events>())
		{}

		~widget_object()
		{
			if(handle_)
				API::close_window(handle_);
		}

		Events& events() const
		{
			return *events_;
		}

		bool create(window parent_wd, bool visible)   ///< Creates a no-size (zero-size) widget. in a widget/root window specified by parent_wd.
		{
			return create(parent_wd, rectangle(), visible);
		}

		bool create(window parent_wd, const rectangle & r = rectangle(), bool visible = true)  ///< in a widget/root window specified by parent_wd.
		{
			if(parent_wd && this->empty())
			{
				handle_ = API::dev::create_widget(parent_wd, r, this);
				API::dev::set_events(handle_, events_);
				API::dev::attach_signal(handle_, *this, &widget_object::signal);
				API::dev::attach_drawer(*this, trigger_);
				if(visible)
					API::show_window(handle_, true);
				
				this->_m_complete_creation();
			}
			return (this->empty() == false);
		}

		window handle() const override
		{
			return handle_;
		}

		widget_object& borderless(bool enable)
		{
			API::widget_borderless(handle_, enable);
			return *this;
		}

		bool borderless() const
		{
			return API::widget_borderless(handle_);
		}
	protected:
		DrawerTrigger& get_drawer_trigger()
		{
			return trigger_;
		}

		const DrawerTrigger& get_drawer_trigger() const
		{
			return trigger_;
		}
	private:
		void signal(detail::signals::code code, const detail::signals& sig)
		{
			typedef detail::signals::code codes;
			switch(code)
			{
			case codes::caption:
				this->_m_caption(sig.info.caption);
				break;
			case codes::read_caption:
				*sig.info.str = this->_m_caption();
				break;
			case codes::destroy:
				handle_ = nullptr;
				break;
			default:
				break;
			}
		}

		general_events& _m_get_general_events() const
		{
			return *events_;
		}
	private:
		window handle_{nullptr};
		DrawerTrigger trigger_;
		std::shared_ptr<Events> events_;
	};//end class widget_object

	        /// Base class of all the classes defined as a non-graphics-buffer widget window. The second template parameter DrawerTrigger is always ignored.\see nana::panel
	template<typename DrawerTrigger, typename Events>
	class widget_object<category::lite_widget_tag, DrawerTrigger, Events>: public widget
	{
	protected:
		typedef DrawerTrigger drawer_trigger_t;
	public:

		widget_object()
			:	events_(std::make_shared<Events>())
		{}

		~widget_object()
		{
			if(handle_)
				API::close_window(handle_);
		}

		Events& events() const
		{
			return *events_;
		}

		bool create(window parent_wd, bool visible)    ///< Creates a no-size (zero-size) widget. in a widget/root window specified by parent_wd.
		{
			return create(parent_wd, rectangle(), visible);
		}

		bool create(window parent_wd, const rectangle& r = rectangle(), bool visible = true)  ///< in a widget/root window specified by parent_wd.
		{
			if(parent_wd && this->empty())
			{
				handle_ = API::dev::create_lite_widget(parent_wd, r, this);
				API::dev::set_events(handle_, events_);
				if(visible)
					API::show_window(handle_, true);
				this->_m_complete_creation();
			}
			return (this->empty() == false);
		}

		window handle() const
		{
			return handle_;
		}
	private:
		void signal(detail::signals::code code, const detail::signals& sig)
		{
			typedef detail::signals::code codes;
			switch(code)
			{
			case codes::caption:
				this->_m_caption(sig.info.caption);
				break;
			case codes::read_caption:
				*sig.info.str = this->_m_caption();
				break;
			case codes::destroy:
				handle_ = nullptr;
				break;
			default:
				break;
			}
		}

		general_events& _m_get_general_events() const
		{
			return *events_;
		}
	private:
		window handle_{nullptr};
		std::shared_ptr<Events> events_;
	};//end class widget_object


	        /// Base class of all the classes defined as a root window. \see nana::form
	template<typename DrawerTrigger, typename Events>
	class widget_object<category::root_tag, DrawerTrigger, Events>: public widget
	{
	protected:
		typedef DrawerTrigger drawer_trigger_t;
	public:

		widget_object()
		{
			handle_ = API::dev::create_window(nullptr, false, API::make_center(300, 150), appearance(), this);
			_m_bind_and_attach();
		}

		widget_object(const rectangle& r, const appearance& apr = appearance())
		{
			handle_ = API::dev::create_window(nullptr, false, r, apr, this);
			_m_bind_and_attach();
		}

		widget_object(window owner, bool nested, const rectangle& r = rectangle(), const appearance& apr = appearance())
		{
			handle_ = API::dev::create_window(owner, nested, r, apr, this);
			_m_bind_and_attach();
		}

		~widget_object()
		{
			if(handle_)
				API::close_window(handle_);
		}

		Events& events() const
		{
			return *events_;
		}

		void activate()
		{
			API::activate_window(handle_);
		}

		void bring_to_top()
		{
			API::bring_to_top(handle_);
		}

		window handle() const
		{
			return handle_;
		}

		native_window_type native_handle() const
		{
			return API::root(handle_);
		}

		window owner() const
		{
			return API::get_owner_window(handle_);
		}

		void icon(const nana::paint::image& ico)
		{
			API::window_icon(handle_, ico);
		}

		void restore()
		{
			API::restore_window(handle_);
		}

		void zoom(bool ask_for_max)
		{
			API::zoom_window(handle_, ask_for_max);
		}

		bool is_zoomed(bool ask_for_max) const
		{
			return API::is_window_zoomed(handle_, ask_for_max);
		}
	protected:
		DrawerTrigger& get_drawer_trigger()
		{
			return trigger_;
		}

		const DrawerTrigger& get_drawer_trigger() const
		{
			return trigger_;
		}
	private:
		void signal(detail::signals::code code, const detail::signals& sig)
		{
			typedef detail::signals::code codes;
			switch(code)
			{
			case codes::caption:
				this->_m_caption(sig.info.caption);
				break;
			case codes::read_caption:
				*sig.info.str = this->_m_caption();
				break;
			case codes::destroy:
				handle_ = nullptr;
				break;
			default:
				break;
			}
		}

		void _m_bind_and_attach()
		{
			events_ = std::make_shared<Events>();
			API::dev::set_events(handle_, events_);
			API::dev::attach_signal(handle_, *this, &widget_object::signal);
			API::dev::attach_drawer(*this, trigger_);
		}

		general_events& _m_get_general_events() const
		{
			return *events_;
		}
	private:
		window handle_;
		DrawerTrigger trigger_;
		std::shared_ptr<Events> events_;
	};//end class widget_object<root_tag>

	           /// Base class of all the classes defined as a frame window. \see nana::frame
	template<typename Drawer, typename Events>
	class widget_object<category::frame_tag, Drawer, Events>: public widget{};

	           /// Especialization. Base class of all the classes defined as a frame window. \see nana::frame
	template<typename Events>
	class widget_object<category::frame_tag, int, Events>: public widget
	{
	protected:
		typedef int drawer_trigger_t;
	public:
		widget_object()
			:	events_(std::make_shared<Events>())
		{}

		~widget_object()
		{
			if(handle_)
				API::close_window(handle_);
		}

		Events& events() const
		{
			return *events_;
		}

		bool create(window parent_wd, bool visible)    ///< Creates a no-size (zero-size) widget. in a widget/root window specified by parent_wd.
		{
			return create(parent_wd, rectangle(), visible);
		}
                 /// Creates in a widget/root window specified by parent_wd.
		bool create(window parent_wd, const rectangle& r = rectangle(), bool visible = true)
		{
			if(parent_wd && this->empty())
			{
				handle_ = API::dev::create_frame(parent_wd, r, this);
				API::dev::set_events(handle_, events_);
				API::dev::attach_signal(handle_, *this, &widget_object::signal);
				API::show_window(handle_, visible);
				this->_m_complete_creation();
			}
			return (this->empty() == false);
		}

		window handle() const
		{
			return handle_;
		}
	private:
		virtual drawer_trigger* get_drawer_trigger()
		{
			return nullptr;
		}

		void signal(detail::signals::code code, const detail::signals& sig)
		{
			typedef detail::signals::code codes;
			switch(code)
			{
			case codes::caption:
				this->_m_caption(sig.info.caption);
				break;
			case codes::read_caption:
				*sig.info.str = this->_m_caption();
				break;
			case codes::destroy:
				handle_ = nullptr;
				break;
			default:
				break;
			}
		}

		general_events& _m_get_general_events() const
		{
			return *events_;
		}
	private:
		window handle_{nullptr};
		std::shared_ptr<Events> events_;
	};//end class widget_object<category::frame_tag>
}//end namespace nana
#endif
