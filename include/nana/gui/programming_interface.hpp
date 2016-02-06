/*
 *	Nana GUI Programming Interface Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
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
			using scheme_type = ::nana::widget_colors;
		};
	}

namespace API
{
	namespace detail
	{
		::nana::widget_colors* make_scheme(::nana::detail::scheme_factory_base&&);
	}

	void effects_edge_nimbus(window, effects::edge_nimbus);
	effects::edge_nimbus effects_edge_nimbus(window);

	void effects_bground(window, const effects::bground_factory_interface&, double fade_rate);
	bground_mode effects_bground_mode(window);
	void effects_bground_remove(window);

	//namespace dev
	//@brief: The interfaces defined in namespace dev are used for developing the nana.gui
	namespace dev
	{
		bool set_events(window, const std::shared_ptr<general_events>&);
		
		template<typename Scheme>
		std::unique_ptr<Scheme> make_scheme()
		{
			return std::unique_ptr<Scheme>{static_cast<Scheme*>(API::detail::make_scheme(::nana::detail::scheme_factory<Scheme>()))};
		}

		void set_scheme(window, widget_colors*);
		widget_colors* get_scheme(window);

		void attach_drawer(widget&, drawer_trigger&);
		::nana::detail::native_string_type window_caption(window) throw();
		void window_caption(window, ::nana::detail::native_string_type);

		window create_window(window, bool nested, const rectangle&, const appearance&, widget* attached);
		window create_widget(window, const rectangle&, widget* attached);
		window create_lite_widget(window, const rectangle&, widget* attached);
		window create_frame(window, const rectangle&, widget* attached);

		paint::graphics* window_graphics(window);

		void delay_restore(bool);

		void register_menu_window(window, bool has_keyboard);
		void set_menubar(window wd, bool attach);

		void enable_space_click(window, bool enable);
	}//end namespace dev


	widget* get_widget(window);

	namespace detail
	{
		general_events* get_general_events(window);
		bool emit_event(event_code, window, const ::nana::event_arg&);

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

	void exit();

	std::string transform_shortkey_text(std::string text, wchar_t &shortkey, std::string::size_type *skpos);
	bool register_shortkey(window, unsigned long);
	void unregister_shortkey(window);

	nana::point	cursor_position();
	rectangle make_center(unsigned width, unsigned height);           ///< Retrieves a rectangle which is in the center of the screen.
	rectangle make_center(window, unsigned width, unsigned height);   ///< Retrieves a rectangle which is in the center of the window

	template<typename Widget=::nana::widget, typename EnumFunction>
	void enum_widgets(window wd, bool recursive, EnumFunction && fn)
	{
		static_assert(std::is_convertible<Widget, ::nana::widget>::value, "enum_widgets<Widget>: The specified Widget is not a widget type.");

		detail::enum_widgets_function<Widget, EnumFunction> enum_fn(static_cast<EnumFunction&&>(fn));
		enum_fn.enum_widgets(wd, recursive);
	}

	void window_icon_default(const paint::image& small_icon, const paint::image& big_icon = {});
	void window_icon(window, const paint::image& small_icon, const paint::image& big_icon = {});
	
	bool empty_window(window);		///< Determines whether a window is existing.
	bool is_window(window);			///< Determines whether a window is existing, equal to !empty_window.
	bool is_destroying(window);		///< Determines whether a window is destroying
	void enable_dropfiles(window, bool);

    /// \brief Retrieves the native window of a Nana.GUI window.
    ///
    /// The native window type is platform-dependent. Under Microsoft Windows, a conversion can be employed between
    /// nana::native_window_type and HWND through reinterpret_cast operator. Under X System, a conversion can
    /// be employed between nana::native_window_type and Window through reinterpret_cast operator.
    /// \return If the function succeeds, the return value is the native window handle to the Nana.GUI window. If fails return zero.
	native_window_type root(window);
	window	root(native_window_type);                     ///< Retrieves the native window of a Nana.GUI window.

	void fullscreen(window, bool);
	bool enabled_double_click(window, bool);
	bool insert_frame(window frame, native_window_type);
	native_window_type frame_container(window frame);
	native_window_type frame_element(window frame, unsigned index);
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

		if (std::is_same<::nana::general_events, event_type>::value)
			return *static_cast<event_type*>(general_evt);

		auto * widget_evt = dynamic_cast<event_type*>(general_evt);
		if (nullptr == widget_evt)
			throw std::invalid_argument("API::events(): bad template parameter Widget, the widget type and window handle do not match.");
		return *widget_evt;
	}

	template<typename EventArg, typename std::enable_if<std::is_base_of< ::nana::event_arg, EventArg>::value>::type* = nullptr>
	bool emit_event(event_code evt_code, window wd, const EventArg& arg)
	{
		return detail::emit_event(evt_code, wd, arg);
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

		if (std::is_same<::nana::widget_colors, scheme_type>::value)
			return *static_cast<scheme_type*>(wdg_colors);

		auto * comp_wdg_colors = dynamic_cast<scheme_type*>(wdg_colors);
		if (nullptr == comp_wdg_colors)
			throw std::invalid_argument("API::scheme(): bad template parameter Widget, the widget type and window handle do not match.");
		return *comp_wdg_colors;
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
	bool get_window_rectangle(window, rectangle&);
	bool track_window_size(window, const size&, bool true_for_max);   ///< Sets the minimum or maximum tracking size of a window.
	void window_enabled(window, bool);
	bool window_enabled(window);

	/**	@brief	A widget drawer draws the widget surface in answering an event.
     *
     *          This function will tell the drawer to copy the graphics into window after event answering.
     *          Tells Nana.GUI to copy the buffer of event window to screen after the event is processed.
     *          This function only works for a drawer_trigger, when a drawer_trigger receives an event,
     *          after drawing, a drawer_trigger should call lazy_refresh to tell the Nana.GUI to refresh
     *          the window to the screen after the event process finished.
	 */
	void lazy_refresh();

	/**	@brief:	calls refresh() of a widget's drawer. if currently state is lazy_refresh, Nana.GUI may paste the drawing on the window after an event processing.
	 *	@param window: specify a window to be refreshed.
	 */
	void refresh_window(window);           ///< Refreshs the window and display it immediately calling the refresh method of its drawer_trigger..
	void refresh_window_tree(window);      ///< Refreshs the specified window and all it’s children windows, then display it immediately
	void update_window(window);            ///< Copies the off-screen buffer to the screen for immediate display.

	void window_caption(window, const std::string& title_utf8);
	void window_caption(window, const std::wstring& title);
	::std::string window_caption(window);

	void window_cursor(window, cursor);
	cursor window_cursor(window);

	void activate_window(window);
	bool is_focus_ready(window);
	window focus_window();
	void focus_window(window);

	window	capture_window();
	window	capture_window(window, bool);         ///< Enables or disables the window to grab the mouse input
	void	capture_ignore_children(bool ignore); ///< Enables or disables the captured window whether redirects the mouse input to its children if the mouse is over its children.
	void modal_window(window);                    ///< Blocks the routine til the specified window is closed.
	void wait_for(window);

	color fgcolor(window);
	color fgcolor(window, const color&);
	color bgcolor(window);
	color bgcolor(window, const color&);
	color activated_color(window);
	color activated_color(window, const color&);

	void create_caret(window, unsigned width, unsigned height);
	void destroy_caret(window);
	void caret_effective_range(window, const rectangle&);
	void caret_pos(window, const ::nana::point&);
	nana::point caret_pos(window);
	nana::size caret_size(window);
	void caret_size(window, const size&);
	void caret_visible(window, bool is_show);
	bool caret_visible(window);

	void tabstop(window);                       ///< Sets the window that owns the tabstop.
            /// treu: The focus is not to be changed when Tab key is pressed, and a key_char event with tab will be generated.
	void eat_tabstop(window, bool);
	window move_tabstop(window, bool next);     ///< Sets the focus to the window which tabstop is near to the specified window.

	/// Sets the window active state. If a window active state is false, the window will not obtain the focus when a mouse clicks on it wich will be obteined by take_if_has_active_false.
	void take_active(window, bool has_active, window take_if_has_active_false);

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

	void at_safe_place(window, std::function<void()>);
}//end namespace API
}//end namespace nana

#endif

