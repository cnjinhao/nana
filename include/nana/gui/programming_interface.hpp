/*
 *	Nana GUI Programming Interface Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2014 Jinhao(cnjinhao@hotmail.com)
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
#include GUI_BEDROCK_HPP
#include "effects.hpp"
#include "detail/general_events.hpp"
#include <nana/paint/image.hpp>
#include <memory>

namespace nana
{
	class drawer_trigger;
	class widget;

namespace API
{
	void effects_edge_nimbus(window, effects::edge_nimbus);
	effects::edge_nimbus effects_edge_nimbus(window);

	void effects_bground(window, const effects::bground_factory_interface&, double fade_rate);
	bground_mode effects_bground_mode(window);
	void effects_bground_remove(window);

	//namespace dev
	//@brief: The interfaces defined in namespace dev are used for developing the nana.gui
	namespace dev
	{
		template<typename Object, typename Concept>
		void attach_signal(window wd, Object& object, void (Concept::*f)(::nana::detail::signals::code, const ::nana::detail::signals&))
		{
			using namespace ::nana::detail;
			bedrock::instance().wd_manager.attach_signal(reinterpret_cast<bedrock::core_window_t*>(wd), object, f);
		}

		bool set_events(window, const std::shared_ptr<general_events>&);

		void attach_drawer(widget&, drawer_trigger&);
		nana::string window_caption(window);
		void window_caption(window, nana::string);

		window create_window(window, bool nested, const rectangle&, const appearance&, widget* attached);
		window create_widget(window, const rectangle&, widget* attached);
		window create_lite_widget(window, const rectangle&, widget* attached);
		window create_frame(window, const rectangle&, widget* attached);

		paint::graphics* window_graphics(window);
	}//end namespace dev


	namespace detail
	{
		general_events* get_general_events(window);
	}//end namespace detail

	void exit();

	nana::string transform_shortkey_text(nana::string text, nana::string::value_type &shortkey, nana::string::size_type *skpos);
	bool register_shortkey(window, unsigned long);
	void unregister_shortkey(window);

	nana::size	screen_size();
	rectangle	screen_area_from_point(const point&);
	nana::point	cursor_position();
	rectangle make_center(unsigned width, unsigned height);           ///< Retrieves a rectangle which is in the center of the screen.
	rectangle make_center(window, unsigned width, unsigned height);   ///< Retrieves a rectangle which is in the center of the window

	template<typename Widget=::nana::widget, typename EnumFunction>
	void enum_widgets(window wd, bool recursive, EnumFunction && ef)
	{
		static_assert(std::is_convertible<Widget, ::nana::widget>::value, "enum_widgets<Widget>: The specified Widget is not a widget type.");

		typedef ::nana::detail::basic_window core_window_t;
		auto & brock = ::nana::detail::bedrock::instance();
		internal_scope_guard lock;

		auto children = brock.wd_manager.get_children(reinterpret_cast<core_window_t*>(wd));
		for (auto child : children)
		{
			auto wgt = dynamic_cast<Widget*>(brock.wd_manager.get_widget(child));
			if (nullptr == wgt)
				continue;

			ef(*wgt);
			if (recursive)
				enum_widgets<Widget>(wd, recursive, std::forward<EnumFunction>(ef));
		}
	}

	void window_icon_default(const paint::image&);
	void window_icon(window, const paint::image&);
	bool empty_window(window);                            ///< Determines whether a window is existing.
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
	typename ::nana::dev::event_mapping<Widget>::type & events(window wd)
	{
		typedef typename ::nana::dev::event_mapping<Widget>::type event_type;

		internal_scope_guard lock;
		auto * general_evt = detail::get_general_events(wd);
		if (nullptr == general_evt)
			throw std::invalid_argument("API::events(): bad parameter window handle, no events object or invalid window handle.");

		if (std::is_same<decltype(*general_evt), event_type>::value)
			return *static_cast<event_type*>(general_evt);

		auto * widget_evt = dynamic_cast<event_type*>(general_evt);
		if (nullptr == widget_evt)
			throw std::invalid_argument("API::events(): bad template parameter Widget, the widget type and window handle do not match.");
		return *widget_evt;
	}

	template<typename EventArg, typename std::enable_if<std::is_base_of< ::nana::detail::event_arg_interface, EventArg>::value>::type* = nullptr>
	bool emit_event(event_code evt_code, window wd, const EventArg& arg)
	{
		auto & brock = ::nana::detail::bedrock::instance();
		return brock.emit(evt_code, reinterpret_cast< ::nana::detail::bedrock::core_window_t*>(wd), arg, true, brock.get_thread_context());
	}

	void umake_event(event_handle);

	nana::point window_position(window);
	void move_window(window, int x, int y);
	void move_window(window wd, const rectangle&);

	void bring_to_top(window);
	bool set_window_z_order(window wd, window wd_after, z_order_action action_if_no_wd_after);

	nana::size window_size(window);
	void window_size(window, const size&);
	bool window_rectangle(window, rectangle&);
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

	void window_caption(window, const nana::string& title);
	nana::string window_caption(window);

	void window_cursor(window, cursor);
	cursor window_cursor(window);

	void activate_window(window);
	bool is_focus_window(window);
	window focus_window();
	void focus_window(window);

	window	capture_window();
	window	capture_window(window, bool);         ///< Enables or disables the window to grab the mouse input
	void	capture_ignore_children(bool ignore); ///< Enables or disables the captured window whether redirects the mouse input to its children if the mouse is over its children.
	void modal_window(window);                    ///< Blocks the routine til the specified window is closed.
	void wait_for(window);
	color_t foreground(window);
	color_t foreground(window, color_t);
	color_t background(window);
	color_t background(window, color_t);
	color_t	active(window);
	color_t	active(window, color_t);

	void create_caret(window, unsigned width, unsigned height);
	void destroy_caret(window);
	void caret_effective_range(window, const rectangle&);
	void caret_pos(window, int x, int y);
	nana::point caret_pos(window);
	nana::size caret_size(window);
	void caret_size(window, const size&);
	void caret_visible(window, bool is_show);
	bool caret_visible(window);

	void tabstop(window);                       ///< Sets the window that owns the tabstop.
            /// treu: The focus is not to be changed when Tab key is pressed, and a key_char event with tab will be generated.
	void eat_tabstop(window, bool);
	window move_tabstop(window, bool next);     ///< Sets the focus to the window which tabstop is near to the specified window.

	bool glass_window(window);			/// \deprecated
	bool glass_window(window, bool);	/// \deprecated

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

	void register_menu_window(window, bool has_keyboard);
	bool attach_menubar(window menubar);
	void detach_menubar(window menubar);
	void restore_menubar_taken_window();

	bool is_window_zoomed(window, bool ask_for_max);  ///<Tests a window whether it is maximized or minimized.

	void widget_borderless(window, bool);	///<Enables or disables a borderless widget.
	bool widget_borderless(window);			///<Tests a widget whether it is borderless.

	nana::mouse_action mouse_action(window);
	nana::element_state element_state(window);
}//end namespace API
}//end namespace nana

#endif

