/*
 *	Platform Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2018 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/detail/native_window_interface.hpp
 */

#ifndef NANA_GUI_DETAIL_NATIVE_WINDOW_INTERFACE_HPP
#define NANA_GUI_DETAIL_NATIVE_WINDOW_INTERFACE_HPP

#include "../basis.hpp"
#include <nana/paint/image.hpp>

#include <functional>

namespace nana
{
namespace detail
{

	struct native_interface
	{
		struct window_result
		{
			native_window_type native_handle;

			unsigned width;		//client size
			unsigned height;	//client size

			unsigned extra_width;	//extra border size, it is useful in Windows, ignore in X11 always 0
			unsigned extra_height;	//extra border size, it is useful in Windows, ignore in X11 always 0
		};

		struct frame_extents
		{
			int left;
			int right;
			int top;
			int bottom;
		};

		using native_string_type = ::nana::detail::native_string_type;

		//Execute a function in a thread which is associated with the specified native window.
		static void affinity_execute(native_window_type, const std::function<void()>&);

		static nana::size	primary_monitor_size();
		static rectangle screen_area_from_point(const point&);
		static window_result create_window(native_window_type, bool nested, const rectangle&, const appearance&);
		static native_window_type create_child_window(native_window_type, const rectangle&);

#if defined(NANA_X11)
		static void set_modal(native_window_type);
#endif
		static void enable_dropfiles(native_window_type, bool);
		static void enable_window(native_window_type, bool);
		// (On Windows) The system displays the large icon in the ALT+TAB dialog box, and the small icon in the window caption.
		static bool window_icon(native_window_type, const paint::image& big_icon, const paint::image& small_icon);
		static void activate_owner(native_window_type);
		static void activate_window(native_window_type);
		static void close_window(native_window_type);
		static void show_window(native_window_type, bool show, bool active);
		static void restore_window(native_window_type);
		static void zoom_window(native_window_type, bool ask_for_max);
		static void	refresh_window(native_window_type);
		static bool is_window(native_window_type);
		static bool	is_window_visible(native_window_type);
		static bool is_window_zoomed(native_window_type, bool ask_for_max);

		static nana::point	window_position(native_window_type);
		static void	move_window(native_window_type, int x, int y);
		static bool	move_window(native_window_type, const rectangle&);
		static void bring_top(native_window_type, bool activated);
		static void	set_window_z_order(native_window_type, native_window_type wd_after, z_order_action action_if_no_wd_after);

		static frame_extents window_frame_extents(native_window_type);
		static bool	window_size(native_window_type, const size&);
		static void	get_window_rect(native_window_type, rectangle&);
		static void	window_caption(native_window_type, const native_string_type&);
		static native_string_type	window_caption(native_window_type);
		static void	capture_window(native_window_type, bool);
		static nana::point	cursor_position();

		static native_window_type get_window(native_window_type wd, window_relationship);
		static native_window_type parent_window(native_window_type child, native_window_type new_parent, bool returns_previous);
		//For Caret
		static void caret_create(native_window_type, const ::nana::size&);
		static void caret_destroy(native_window_type);
		static void caret_pos(native_window_type, const ::nana::point&);
		static void caret_visible(native_window_type, bool);

		static void	set_focus(native_window_type);
		static native_window_type get_focus_window();
		static bool calc_screen_point(native_window_type, nana::point&);
		static bool calc_window_point(native_window_type, nana::point&);

		static native_window_type find_window(int x, int y);
		static nana::size check_track_size(nana::size sz, unsigned extra_width, unsigned extra_height, bool true_for_max);
	};


}//end namespace detail
}//end namespace nana
#endif
