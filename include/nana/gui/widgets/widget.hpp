/**
 *	The fundamental widget class implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/widget.hpp
 */

#ifndef NANA_GUI_WIDGET_HPP
#define NANA_GUI_WIDGET_HPP
#include "../basis.hpp"
#include "../programming_interface.hpp"
#include <nana/internationalization.hpp>
#include <nana/gui/detail/drawer.hpp>
#include <functional>

namespace nana
{
	namespace detail
	{
		//Forward declaration of widget_notifier_interface
		class widget_notifier_interface;
	}

	/// Abstract class for defining the capacity interface.
	class widget
		: nana::noncopyable, nana::nonmovable
	{
		friend class detail::widget_notifier_interface;
		class inner_widget_notifier;
		typedef void(*dummy_bool_type)(widget* (*)(const widget&));
	public:
		using native_string_type = detail::native_string_type;

		virtual ~widget() = default;
		virtual window handle() const = 0;			///< Returns the handle of window, returns 0 if window is not created.
		bool empty() const;							///< Determines whether the manipulator is handling a window.
		void close();

		window parent() const;

		::std::string caption() const throw();
		::std::wstring caption_wstring() const throw();
		native_string_type caption_native() const throw();

		widget& caption(std::string utf8);
		widget& caption(std::wstring);

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
		void move(const point&);
		void move(const rectangle&);

		void fgcolor(const nana::color&);
		nana::color fgcolor() const;
		void bgcolor(const nana::color&);
		nana::color bgcolor() const;

		general_events& events() const;

		void umake_event(event_handle eh) const;              ///< Deletes an event callback by a handle.

		widget& register_shortkey(wchar_t);	///< Registers a shortkey. To remove a registered key, pass 0.

		widget& take_active(bool activated, window take_if_not_activated);
		widget& tooltip(const ::std::string&);

		operator dummy_bool_type() const;
		operator window() const;
	protected:
		std::unique_ptr<::nana::detail::widget_notifier_interface> _m_wdg_notifier();
	private:
		virtual void _m_notify_destroy() = 0;

	protected:
		//protected members, a derived class must call this implementation if it overrides an implementation
		virtual void _m_complete_creation();

		virtual general_events& _m_get_general_events() const = 0;
		virtual native_string_type _m_caption() const throw();
		virtual void _m_caption(native_string_type&&);
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

		virtual void _m_fgcolor(const nana::color&);
		virtual nana::color _m_fgcolor() const;
		virtual void _m_bgcolor(const nana::color&);
		virtual nana::color _m_bgcolor() const;
	};

            /// Base class of all the classes defined as a widget window. Defaultly a widget_tag
	template<typename Category, typename DrawerTrigger, typename Events = ::nana::general_events, typename Scheme = ::nana::widget_colors>
	class widget_object: public widget
	{
	protected:
		typedef DrawerTrigger drawer_trigger_t;
	public:
		using scheme_type	= Scheme;
		using event_type	= Events;

		widget_object()
			:	events_{ std::make_shared<Events>() },
				scheme_{ API::dev::make_scheme<Scheme>() }
		{}

		~widget_object()
		{
			if(handle_)
				API::close_window(handle_);
		}

		event_type& events() const
		{
			return *events_;
		}

		bool create(window parent_wd, bool visible)   ///< Creates a no-size (zero-size) widget. in a widget/root window specified by parent_wd.
		{
			return create(parent_wd, rectangle(), visible);
		}

		bool create(window parent_wd, const rectangle & r = {}, bool visible = true)  ///< in a widget/root window specified by parent_wd.
		{
			if(parent_wd && this->empty())
			{
				handle_ = API::dev::create_widget(parent_wd, r, this);
				API::dev::set_events(handle_, events_);
				API::dev::set_scheme(handle_, scheme_.get());
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

		scheme_type& scheme() const
		{
			return *scheme_;
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
		general_events& _m_get_general_events() const override
		{
			return *events_;
		}

		void _m_notify_destroy() override final
		{
			handle_ = nullptr;
		}
	private:
		window handle_{nullptr};
		DrawerTrigger trigger_;
		std::shared_ptr<Events> events_;
		std::unique_ptr<scheme_type> scheme_;
	};//end class widget_object

	        /// Base class of all the classes defined as a non-graphics-buffer widget window. The second template parameter DrawerTrigger is always ignored.\see nana::panel
	template<typename DrawerTrigger, typename Events, typename Scheme>
	class widget_object<category::lite_widget_tag, DrawerTrigger, Events, Scheme>: public widget
	{
	protected:
		typedef DrawerTrigger drawer_trigger_t;
	public:
		using scheme_type = Scheme;
		using event_type = Events;

		widget_object()
			: events_{ std::make_shared<Events>() }, scheme_{ API::dev::make_scheme<scheme_type>() }
		{}

		~widget_object()
		{
			if(handle_)
				API::close_window(handle_);
		}

		event_type& events() const
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
				API::dev::set_scheme(handle_, scheme_.get());
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

		scheme_type& scheme() const
		{
			return *scheme_;
		}
	private:
		general_events& _m_get_general_events() const override
		{
			return *events_;
		}

		void _m_notify_destroy() override final
		{
			handle_ = nullptr;
		}
	private:
		window handle_{nullptr};
		std::shared_ptr<Events> events_;
		std::unique_ptr<scheme_type> scheme_;
	};//end class widget_object


	        /// Base class of all the classes defined as a root window. \see nana::form
	template<typename DrawerTrigger, typename Events, typename Scheme>
	class widget_object<category::root_tag, DrawerTrigger, Events, Scheme>: public widget
	{
	protected:
		typedef DrawerTrigger drawer_trigger_t;
	public:
		using scheme_type = Scheme;
		using event_type = Events;

		widget_object()
		{
			handle_ = API::dev::create_window(nullptr, false, API::make_center(300, 150), appearance(), this);
			_m_bind_and_attach();
		}

		widget_object(const rectangle& r, const appearance& apr = {})
		{
			handle_ = API::dev::create_window(nullptr, false, r, apr, this);
			_m_bind_and_attach();
		}

		widget_object(window owner, bool nested, const rectangle& r = {}, const appearance& apr = {})
		{
			handle_ = API::dev::create_window(owner, nested, r, apr, this);
			_m_bind_and_attach();
		}

		~widget_object()
		{
			if(handle_)
				API::close_window(handle_);
		}

		event_type& events() const
		{
			return *events_;
		}

		void activate()
		{
			API::activate_window(handle_);
		}

		window handle() const override
		{
			return handle_;
		}

		native_window_type native_handle() const
		{
			return API::root(handle_);
		}

		void bring_top(bool activated)
		{
			API::bring_top(handle(), activated);
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

		bool is_zoomed(bool check_maximized) const
		{
			return API::is_window_zoomed(handle_, check_maximized);
		}

		widget_object& z_order(window wd_after, z_order_action action_if_no_wd_after)
		{
			API::set_window_z_order(handle_, wd_after, action_if_no_wd_after);
			return *this;
		}

		scheme_type& scheme() const
		{
			return *scheme_;
		}

		void draw_through(std::function<void()> draw_fn)
		{
			API::draw_through(handle(), draw_fn);
		}

		void map_through_widgets(native_drawable_type drawable)
		{
			API::map_through_widgets(handle(), drawable);
		}

		void outline_size(const ::nana::size& sz)
		{
			API::window_outline_size(handle(), sz);
		}

		::nana::size outline_size() const
		{
			return API::window_outline_size(handle());
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
		void _m_bind_and_attach()
		{
			events_ = std::make_shared<Events>();
			API::dev::set_events(handle_, events_);

			scheme_ = API::dev::make_scheme<scheme_type>();
			API::dev::set_scheme(handle_, scheme_.get());
			API::dev::attach_drawer(*this, trigger_);
		}

		general_events& _m_get_general_events() const override
		{
			return *events_;
		}

		void _m_notify_destroy() override final
		{
			handle_ = nullptr;
		}
	private:
		window handle_;
		DrawerTrigger trigger_;
		std::shared_ptr<Events> events_;
		std::unique_ptr<scheme_type> scheme_;
	};//end class widget_object<root_tag>

	           /// Base class of all the classes defined as a frame window. \see nana::frame
	template<typename Drawer, typename Events, typename Scheme>
	class widget_object<category::frame_tag, Drawer, Events, Scheme>: public widget{};

	           /// Especialization. Base class of all the classes defined as a frame window. \see nana::frame
	template<typename Events, typename Scheme>
	class widget_object<category::frame_tag, int, Events, Scheme>: public widget
	{
	protected:
		typedef int drawer_trigger_t;
	public:
		using scheme_type = Scheme;
		using event_type = Events;

		widget_object()
			: events_{ std::make_shared<Events>() }, scheme_{ API::dev::make_scheme<scheme_type>() }
		{}

		~widget_object()
		{
			if(handle_)
				API::close_window(handle_);
		}

		event_type& events() const
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
				API::dev::set_scheme(handle_, scheme_.get());
				API::show_window(handle_, visible);
				this->_m_complete_creation();
			}
			return (this->empty() == false);
		}

		window handle() const override
		{
			return handle_;
		}

		scheme_type& scheme() const
		{
			return *scheme_;
		}
	private:
		virtual drawer_trigger* get_drawer_trigger()
		{
			return nullptr;
		}

		general_events& _m_get_general_events() const override
		{
			return *events_;
		}

		void _m_notify_destroy() override final
		{
			handle_ = nullptr;
		}
	private:
		window handle_{nullptr};
		std::shared_ptr<Events> events_;
		std::unique_ptr<scheme_type> scheme_;
	};//end class widget_object<category::frame_tag>
}//end namespace nana
#endif
