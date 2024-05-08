/**
 *	Platform Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2024 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file nana/gui/detail/native_window_interface.hpp
 *  @contributors  Jinhao, Ariel Vina-Rodriguez
 */

#ifndef NANA_GUI_DETAIL_NATIVE_WINDOW_INTERFACE_HPP
#define NANA_GUI_DETAIL_NATIVE_WINDOW_INTERFACE_HPP

#include <functional>
#include <iostream>

#include "../basis.hpp"
#include <nana/paint/image.hpp>

namespace nana
{
namespace detail
{
  #if defined(NANA_WINDOWS)
    nana::point scale_to_dpi(int x, int y, int dpi);
	nana::point scale_to_dpi(native_window_type wd, int x, int y);
	nana::point unscale_dpi(native_window_type wd, int x, int y);
    // create helper function to scale nana::rectangle to dpi
    nana::rectangle scale_to_dpi(const nana::rectangle& r, int dpi);
	nana::rectangle scale_to_dpi(native_window_type wd, const nana::rectangle& r);
	nana::rectangle unscale_dpi(const nana::rectangle& r, int dpi);
	// create helper function to scale ::RECT to dpi
	//::RECT scale_to_dpi(const ::RECT& r, int dpi);
	//::RECT scale_to_dpi(native_window_type wd, const ::RECT& r);
	//::RECT unscale_dpi(const ::RECT& r, int dpi);

	nana::size scale_to_dpi(const nana::size& sz, int dpi);
	nana::size scale_to_dpi(native_window_type wd, const nana::size& sz);
	nana::size unscale_dpi(const nana::size& sz, int dpi);
	nana::size unscale_dpi(native_window_type wd, const nana::size& sz);
#endif

	struct native_interface
	{
		struct window_result
		{
			native_window_type native_handle;

			unsigned width;		///< client size
			unsigned height;	///< client size

			unsigned extra_width;	///< extra border size, it is useful in Windows, ignore in X11 always 0
			unsigned extra_height;	///< extra border size, it is useful in Windows, ignore in X11 always 0
		};

		struct frame_extents
		{
			int left;
			int right;
			int top;
			int bottom;
		};

		using native_string_type = ::nana::detail::native_string_type;

		/// Invokes a function in the thread of the specified window.
		static void affinity_execute(native_window_type native_handle, bool post, std::function<void()>&& fn);

		static nana::size	primary_monitor_size();                                 ///< return 'DPI' unscaled size (user-side)
		static rectangle screen_area_from_system_point(const point& system_point);  ///< unused ?

		static window_result create_window(native_window_type owner, bool nested, const rectangle& r, const appearance& ap);
		static native_window_type create_child_window(native_window_type owner, const rectangle& r);

#if defined(NANA_X11)
		static void set_modal(native_window_type);
#endif
		static void enable_dropfiles(native_window_type wd, bool enable);
		static void enable_window   (native_window_type wd, bool enable);

		/// (On Windows) The system displays the large icon in the ALT+TAB dialog box, and the small icon in the window caption.
		static bool window_icon(native_window_type wd, const paint::image& big_icon, const paint::image& small_icon);
		static void activate_owner (native_window_type wd);
		static void activate_window(native_window_type wd);
		static void close_window   (native_window_type wd);                        ///< close the window, destroy the window
		static void show_window    (native_window_type wd, bool show, bool active);
		static void restore_window (native_window_type wd);
		static void zoom_window    (native_window_type wd, bool ask_for_max);
		static void	refresh_window (native_window_type wd);
		static bool is_window      (native_window_type wd);
		static bool	is_window_visible(native_window_type wd);
		static bool is_window_zoomed(native_window_type wd, bool ask_for_max);

		static nana::point	window_position(native_window_type wd);                ///< return 'DPI' unscaled size (user-side)
		static void	move_window   (native_window_type wd, int x, int y);
		static bool	move_window   (native_window_type wd, const rectangle&);
		static void bring_top     (native_window_type wd, bool activated);
		static void	set_window_z_order(native_window_type wd, native_window_type wd_after, z_order_action action_if_no_wd_after);

		static frame_extents window_frame_extents(native_window_type wd);
		static bool	window_size    (native_window_type wd, const size& new_size); ///< change to new_size if possible
		static void	get_window_rect(native_window_type wd, rectangle& r);         ///< unused ?
		static void	window_caption (native_window_type wd, const native_string_type& title);
		static native_string_type	window_caption(native_window_type wd);
		static void	capture_window (native_window_type wd, bool capture);

		static native_window_type get_window   (native_window_type    wd, window_relationship);
		static native_window_type parent_window(native_window_type child, native_window_type new_parent, bool returns_previous);
		
		//For Caret
		static void caret_create  (native_window_type wd, const ::nana::size&);
		static void caret_destroy (native_window_type wd);
		static void caret_pos     (native_window_type wd, const ::nana::point&);
		static void caret_visible (native_window_type wd, bool);

		static void	set_focus     (native_window_type wd);
		static native_window_type get_focus_window();
		static bool calc_screen_point(native_window_type wd, nana::point& window_point);
		static bool calc_window_point(native_window_type wd, nana::point& screen_point);

		static native_window_type find_window(int x, int y);
		static nana::size check_track_size(nana::size sz, unsigned extra_width, unsigned extra_height, bool true_for_max);

		static void start_dpi_awareness(bool aware = false);
		static int window_dpi(native_window_type);  ///< if the window is not DPI aware return the system DPI 
		static int system_dpi();					///< get the DPI of the main monitor
	};


}//end namespace detail
}//end namespace nana
#endif
