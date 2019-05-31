/*
 *	Nana GUI Programming Interface Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/programming_interface.hpp
 */

#ifndef NANA_GUI_PROGRAMMING_INTERFACE_HPP
#define NANA_GUI_PROGRAMMING_INTERFACE_HPP
#include <nana/config.hpp>
#include "effects.hpp"
#include "detail/general_events.hpp"
#include "detail/color_schemes.hpp"
#include "detail/widget_content_measurer_interface.hpp"
#include <nana/paint/image.hpp>
#include <memory>

namespace nana
{
	class drawer_trigger;
	class widget;

	namespace dev
	{
		/// Traits for widget classes
		template<typename Widget>
		struct widget_traits
		{
			using event_type = typename Widget::event_type;
			using scheme_type = typename Widget::scheme_type;
		};

		template<>
		struct widget_traits<widget>
		{
			using event_type = ::nana::general_events;
			using scheme_type = ::nana::widget_geometrics;
		};
	}

namespace API
{
#ifdef NANA_X11
	//Some platform specific functions for X11
	namespace x11
	{
		/// Returns the connection to the X server
		const void* get_display();
	}
#endif

	namespace detail
	{
		::nana::widget_geometrics* make_scheme(::nana::detail::scheme_factory_interface&&);
	}

	void effects_edge_nimbus(window, effects::edge_nimbus);
	effects::edge_nimbus effects_edge_nimbus(window);

	void effects_bground(window, const effects::bground_factory_interface&, double fade_rate);
	void effects_bground(std::initializer_list<window> wdgs, const effects::bground_factory_interface&, double fade_rate);

	bground_mode effects_bground_mode(window);
	void effects_bground_remove(window);

	//namespace dev
	//@brief: The interfaces defined in namespace dev are used for developing the nana.gui
	namespace dev
	{
		void affinity_execute(window window_handle, const std::function<void()>&);

		bool set_events(window, const std::shared_ptr<general_events>&);

		template<typename Scheme>
		std::unique_ptr<Scheme> make_scheme()
		{
			return std::unique_ptr<Scheme>{static_cast<Scheme*>(API::detail::make_scheme(::nana::detail::scheme_factory<Scheme>()))};
		}

		void set_scheme(window, widget_geometrics*);
		widget_geometrics* get_scheme(window);

		/// Sets a content measurer
		void set_measurer(window, ::nana::dev::widget_content_measurer_interface*);

		void attach_drawer(widget&, drawer_trigger&);
		::nana::detail::native_string_type window_caption(window) noexcept;
		void window_caption(window, ::nana::detail::native_string_type);

		window create_window(window, bool nested, const rectangle&, const appearance&, widget* attached);
		window create_widget(window, const rectangle&, widget* attached);
		window create_lite_widget(window, const rectangle&, widget* attached);

		paint::graphics* window_graphics(window);

		void delay_restore(bool);

		void register_menu_window(window, bool has_keyboard);
		void set_menubar(window wd, bool attach);

		void enable_space_click(window, bool enable);

		bool copy_transparent_background(window, paint::graphics&);
		bool copy_transparent_background(window, const rectangle& src_r, paint::graphics&, const point& dst_pt);

		/// Refreshes a widget surface
		/*
		 * This function will copy the drawer surface into system window after the event process finished.
		 */
		void lazy_refresh();

		void draw_shortkey_underline(paint::graphics&, const std::string& text, wchar_t shortkey, std::size_t shortkey_position, const point& text_pos, const color&);

		void window_draggable(window, bool enabled);
		bool window_draggable(window);
	}//end namespace dev


	/// Returns the widget pointer of the specified window.
	/*
	 * @param window_handle A handle to a window owning the widget.
	 * @return A widget pointer.
	 */
	widget* get_widget(window window_handle);

	namespace detail
	{
		general_events* get_general_events(window);

		// emits both internal and external event (internal event can be filtered)
		bool emit_event(event_code, window, const ::nana::event_arg&);

		// explicitly emits internal event (internal event not to be filtered)
		bool emit_internal_event(event_code, window, const ::nana::event_arg&);

		class enum_widgets_function_base
		{
		public:
			virtual ~enum_widgets_function_base() = default;
			void enum_widgets(window, bool recursive);
		private:
			virtual bool _m_enum_fn(::nana::widget*) = 0;
		};

		template<typename Widget, typename EnumFunction>
		class enum_widgets_function
			: public enum_widgets_function_base
		{
		public:
			enum_widgets_function(EnumFunction && enum_fn)
				: enum_fn_(static_cast<EnumFunction&&>(enum_fn))
			{}
		private:
			bool _m_enum_fn(::nana::widget* wd) override
			{
				return _m_enum_call<Widget>(wd);
			}

			template<typename T, typename std::enable_if<std::is_same<::nana::widget, T>::value>::type* = nullptr>
			bool _m_enum_call(::nana::widget* wd)
			{
				enum_fn_(*wd);
				return true;
			}

			template<typename T, typename std::enable_if<!std::is_same<::nana::widget, T>::value>::type* = nullptr>
			bool _m_enum_call(::nana::widget* wd)
			{
				auto ptr = dynamic_cast<Widget*>(wd);
				if (nullptr == ptr)
					return false;

				enum_fn_(*ptr);
				return true;
			}
		private:
			EnumFunction && enum_fn_;
		};
	}//end namespace detail

	///Sets languages
	/**
	 * Specifies the languages in order to make the program display multi-languages correctly
	 * Under Windows, the pragram can display multi-languages correctly, so this function is useless for Windows.
	 */
	void font_languages(const std::string& langs);

	void exit();	    ///< close all windows in current thread
	void exit_all();	///< close all windows

	/// @brief	Searches whether the text contains a '&' and removes the character for transforming.
	///			If the text contains more than one '&' characters, the others are ignored. e.g
	///			text = "&&a&bcd&ef", the result should be "&abcdef", shortkey = 'b', and pos = 2.
	std::string transform_shortkey_text
					( std::string text,      ///< the text is transformed
					  wchar_t &shortkey,     ///<  the character which indicates a short key.
					  std::string::size_type *skpos ///< retrieves the shortkey position if it is not a null_ptr;
					);
	bool register_shortkey(window, unsigned long);
	void unregister_shortkey(window);

	nana::point	cursor_position();
	rectangle make_center(unsigned width, unsigned height);           ///< Retrieves a rectangle which is in the center of the screen.
	rectangle make_center(window, unsigned width, unsigned height);   ///< Retrieves a rectangle which is in the center of the window

	template<typename Widget=::nana::widget, typename EnumFunction>
	void enum_widgets(window wd, bool recursive, EnumFunction && fn)
	{
		static_assert(std::is_convertible<typename std::decay<Widget>::type*, ::nana::widget*>::value, "enum_widgets<Widget>: The specified Widget is not a widget type.");

		detail::enum_widgets_function<Widget, EnumFunction> enum_fn(static_cast<EnumFunction&&>(fn));
		enum_fn.enum_widgets(wd, recursive);
	}

	void window_icon_default(const paint::image& small_icon, const paint::image& big_icon = {});
	void window_icon(window, const paint::image& small_icon, const paint::image& big_icon = {});

	bool empty_window(window);		///< Determines whether a window is existing.
	bool is_window(window);			///< Determines whether a window is existing, equal to !empty_window.
	bool is_destroying(window);		///< Determines whether a window is destroying
	void enable_dropfiles(window, bool);

	bool is_transparent_background(window);

    /// \brief Retrieves the native window of a Nana.GUI window.
    ///
    /// The native window type is platform-dependent. Under Microsoft Windows, a conversion can be employed between
    /// nana::native_window_type and HWND through reinterpret_cast operator. Under X System, a conversion can
    /// be employed between nana::native_window_type and Window through reinterpret_cast operator.
    /// \return If the function succeeds, the return value is the native window handle to the Nana.GUI window. If fails return zero.
	native_window_type root(window);
	window	root(native_window_type);                     ///< Retrieves the native window of a Nana.GUI window.

	void fullscreen(window, bool);

	void close_window(window);
	void show_window(window, bool show);                  ///< Sets a window visible state.
	void restore_window(window);
	void zoom_window(window, bool ask_for_max);
	bool visible(window);
	window	get_parent_window(window);
	window	get_owner_window(window);

	bool	set_parent_window(window, window new_parent);

	template<typename Widget=::nana::widget>
	typename ::nana::dev::widget_traits<Widget>::event_type & events(window wd)
	{
		using event_type = typename ::nana::dev::widget_traits<Widget>::event_type;

		internal_scope_guard lock;
		auto * general_evt = detail::get_general_events(wd);
		if (nullptr == general_evt)
			throw std::invalid_argument("API::events(): bad parameter window handle, no events object or invalid window handle.");

#ifdef __cpp_if_constexpr
		if constexpr(std::is_same_v<event_type, ::nana::general_events>)
		{
			return *general_evt;
		}
		else
		{
			auto * widget_evt = dynamic_cast<event_type*>(general_evt);
			if (nullptr == widget_evt)
				throw std::invalid_argument("API::events(): bad template parameter Widget, the widget type and window handle do not match.");
			return *widget_evt;
		}
#else
		if (std::is_same<::nana::general_events, event_type>::value)
			return *static_cast<event_type*>(general_evt);

		auto * widget_evt = dynamic_cast<event_type*>(general_evt);
		if (nullptr == widget_evt)
			throw std::invalid_argument("API::events(): bad template parameter Widget, the widget type and window handle do not match.");
		return *widget_evt;
#endif
	}

	template<typename EventArg, typename std::enable_if<std::is_base_of< ::nana::event_arg, EventArg>::value>::type* = nullptr>
	bool emit_event(event_code evt_code, window wd, const EventArg& arg)
	{
		return detail::emit_event(evt_code, wd, arg);
	}

	template<typename EventArg, typename std::enable_if<std::is_base_of< ::nana::event_arg, EventArg>::value>::type* = nullptr>
	bool emit_internal_event(event_code evt_code, window wd, const EventArg& arg)
	{
		return detail::emit_internal_event(evt_code, wd, arg);
	}

	void umake_event(event_handle);

	template<typename Widget = ::nana::widget>
	typename ::nana::dev::widget_traits<Widget>::scheme_type & scheme(window wd)
	{
		using scheme_type = typename ::nana::dev::widget_traits<Widget>::scheme_type;

		internal_scope_guard lock;
		auto * wdg_colors = dev::get_scheme(wd);
		if (nullptr == wdg_colors)
			throw std::invalid_argument("API::scheme(): bad parameter window handle, no events object or invalid window handle.");

#ifdef __cpp_if_constexpr
		if constexpr(std::is_same<::nana::widget_geometrics, scheme_type>::value)
		{
			return *static_cast<scheme_type*>(wdg_colors);
		}
		else
		{
			auto * comp_wdg_colors = dynamic_cast<scheme_type*>(wdg_colors);
			if (nullptr == comp_wdg_colors)
				throw std::invalid_argument("API::scheme(): bad template parameter Widget, the widget type and window handle do not match.");
			return *comp_wdg_colors;
		}
#else
		if (std::is_same<::nana::widget_geometrics, scheme_type>::value)
			return *static_cast<scheme_type*>(wdg_colors);

		auto * comp_wdg_colors = dynamic_cast<scheme_type*>(wdg_colors);
		if (nullptr == comp_wdg_colors)
			throw std::invalid_argument("API::scheme(): bad template parameter Widget, the widget type and window handle do not match.");
		return *comp_wdg_colors;
#endif
	}

	point window_position(window);
	void move_window(window, const point&);
	void move_window(window wd, const rectangle&);

	void bring_top(window, bool activated);
	bool set_window_z_order(window wd, window wd_after, z_order_action action_if_no_wd_after);

	void draw_through(window, std::function<void()>);
	void map_through_widgets(window, native_drawable_type);

	size window_size(window);
	void window_size(window, const size&);
	size window_outline_size(window);
	void window_outline_size(window, const size&);

	::std::optional<rectangle> window_rectangle(window);
	bool get_window_rectangle(window, rectangle&);
	bool track_window_size(window, const size&, bool true_for_max);   ///< Sets the minimum or maximum tracking size of a window.
	void window_enabled(window, bool);
	bool window_enabled(window);

	/// Refresh the window and display it immediately calling the refresh function of its drawer_trigger.
	/*
	 * The drawer::refresh() will be called. If the current state is lazy_refrsh, the window is delayed to update the graphics until an event is finished.
	 * @param window_handle A handle to the window to be refreshed.
	 */
	void refresh_window(window window_handle);
	void refresh_window_tree(window);      ///< Refreshes the specified window and all its children windows, then displays it immediately
	void update_window(window);            ///< Copies the off-screen buffer to the screen for immediate display.

	void window_caption(window, const std::string& title_utf8);
	void window_caption(window, const std::wstring& title);
	::std::string window_caption(window);

	void window_cursor(window, cursor);
	cursor window_cursor(window);

	void activate_window(window);

	/// Determines whether the specified window will get the keyboard focus when its root window gets native system focus.
	bool is_focus_ready(window);

	/// Returns the current keyboard focus window.
	window focus_window();

	/// Sets the keyboard focus for a specified window.
	void focus_window(window);

	/// Returns a window which has grabbed the mouse input.
	window	capture_window();

	/// Enables a window to grab the mouse input.
	/**
	 * @param window_handle A handle to a window to grab the mouse input.
	 * @param ignore_children Indicates whether to redirect the mouse input to its children if the mouse pointer is over its children.
	 */
	void set_capture(window window_handle, bool ignore_children);

	/// Disable a window to grab the mouse input.
	/**
	 * @param window handle A handle to a window to release grab of mouse input.
	 */
	void release_capture(window window_handle);

	/// Blocks the execution and other windows' messages until the specified window is closed.
	void modal_window(window);

	/// Blocks the execution until the specified window is closesd.
	void wait_for(window);

	color fgcolor(window);
	color fgcolor(window, const color&);
	color bgcolor(window);
	color bgcolor(window, const color&);
	color activated_color(window);
	color activated_color(window, const color&);

	void create_caret(window, const size&);
	void destroy_caret(window);

	/// Opens an existing caret of a window.
	/**
	 * This function returns an object to operate caret. The object doesn't create or destroy the caret.
	 * When you are finished with the caret, be sure to reset the pointer.
	 *
	 * @param window_handle A handle to a window whose caret is to be retrieved
	 * @return a pointer to the caret proxy.
	 * @except throws std::runtime if the window doesn't have a caret when disable_throw is false
	 */
	::std::unique_ptr<caret_interface> open_caret(window window_handle, bool disable_throw = false);

	/// Enables that the user can give input focus to the specified window using TAB key.
	void tabstop(window);

	/// Enables or disables a window to receive a key_char event for pressing TAB key.
	/*
	 * @param window_handle A handle to the window to catch TAB key through key_char event.
	 * @param enable Indicates whether to enable or disable catch of TAB key. If this parameter is *true*, the window
	 * receives a key_char event when pressing TAB key, and the input focus is not changed. If this parameter is *false*, the
	 * input focus is changed to the next tabstop window.
	 */
	void eat_tabstop(window window_handle, bool enable);

	/// Sets the input focus to the window which the tabstop is near to the specified window.
	/*
	 * @param window_handle A handle to the window.
	 * @param forward Indicates whether forward or backward window to be given the input focus.
	 * @return A handle to the window which to be given the input focus.
	 */
	window move_tabstop(window window_handle, bool forward);

	/// Sets the window active state. If a window active state is false, the window will not obtain the focus when a mouse clicks on it which will be obtained by take_if_has_active_false.
	void take_active(window, bool has_active, window take_if_has_active_false);

	/// Copies the graphics of a specified to a new graphics object.
	bool window_graphics(window, nana::paint::graphics&);
	bool root_graphics(window, nana::paint::graphics&);
	bool get_visual_rectangle(window, nana::rectangle&);

	void typeface(window, const nana::paint::font&);
	paint::font typeface(window);

	bool calc_screen_point(window, point&);   ///<Converts window coordinates to screen coordinates
	bool calc_window_point(window, point&);   ///<Converts screen coordinates to window coordinates.

	window find_window(const nana::point& mspos);

	bool is_window_zoomed(window, bool ask_for_max);  ///<Tests a window whether it is maximized or minimized.

	void widget_borderless(window, bool);	///<Enables or disables a borderless widget.
	bool widget_borderless(window);			///<Tests a widget whether it is borderless.

	nana::mouse_action mouse_action(window);
	nana::element_state element_state(window);

	bool ignore_mouse_focus(window, bool ignore);	///< Enables/disables the mouse focus, it returns the previous state
	bool ignore_mouse_focus(window);				///< Determines whether the mouse focus is enabled

	void at_safe_place(window, ::std::function<void()>);

	/// Returns a widget content extent size
	/**
	 * @param wd A handle to a window that returns its content extent size.
	 * @param limited_px Specifies the max pixels of width or height. If this parameter is zero, this parameter will be ignored.
	 * @param limit_width Indicates whether the it limits the width or height. If this parameter is *true*, the width is limited.
	 * If the parameter is *false*, the height is limited. This parameter is ignored if limited_px = 0.
	 * @return if optional has a value, the first size indicates the content extent, the second size indicates the size of
	 * widget by the content extent. 
	 */
	::std::optional<std::pair<::nana::size, ::nana::size>> content_extent(window wd, unsigned limited_px, bool limit_width);

	unsigned screen_dpi(bool x_requested);

	dragdrop_status window_dragdrop_status(::nana::window);
}//end namespace API

}//end namespace nana

#endif

