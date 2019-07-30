/*
 *	Platform Specification Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/detail/platform_spec.hpp
 *
 *	This file provides basis class and data structrue that required by nana
 *	This file should not be included by any header files.
 */

#if defined(NANA_POSIX)

#ifndef NANA_DETAIL_PLATFORM_SPEC_HPP
#define NANA_DETAIL_PLATFORM_SPEC_HPP

#include <nana/push_ignore_diagnostic>

#include <atomic>
#include <thread>
#include <mutex>
#include <memory>
#include <condition_variable>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xos.h>
#include <nana/gui/basis.hpp>
#include <nana/paint/image.hpp>
#include <nana/paint/graphics.hpp>
#include <nana/gui/detail/event_code.hpp>

#include <vector>
#include <map>
#include <functional>
#include "msg_packet.hpp"
#include "../platform_abstraction_types.hpp"

#if defined(NANA_USE_XFT)
	#include <X11/Xft/Xft.h>
	#include <fstream>
#endif

namespace nana
{
namespace detail
{
	class msg_dispatcher;

#if defined(NANA_USE_XFT)
	class conf
	{
	public:
		conf(const char * file);
		bool open(const char* file);
		std::string value(const char* key);
	private:
		std::ifstream ifs_;
	};
#endif

	struct drawable_impl_type
	{
		using font_type = ::std::shared_ptr<font_interface>;

		Pixmap	pixmap;
		GC	context;

		font_type font;

		nana::point	line_begin_pos;

		struct string_spec
		{
			unsigned tab_length;
			unsigned tab_pixels;
			unsigned whitespace_pixels;
		}string;

		unsigned fgcolor_rgb{ 0xFFFFFFFF };
		unsigned bgcolor_rgb{ 0xFFFFFFFF };

#if defined(NANA_USE_XFT)
		XftDraw * xftdraw{nullptr};
		XftColor	xft_fgcolor;
#endif
		drawable_impl_type();

		void set_color(const ::nana::color&);
		void set_text_color(const ::nana::color&);

		void update_color();
		void update_text_color();
	private:
		drawable_impl_type(const drawable_impl_type&) = delete;
		drawable_impl_type& operator=(const drawable_impl_type&) = delete;

		unsigned current_color_{ 0xFFFFFF };
	};

	struct atombase_tag
	{
		Atom wm_protocols;
		//window manager support
		Atom wm_change_state;
		Atom wm_delete_window;
		//ext
		Atom net_frame_extents;
		Atom net_wm_state;
		Atom net_wm_state_skip_taskbar;
		Atom net_wm_state_fullscreen;
		Atom net_wm_state_maximized_horz;
		Atom net_wm_state_maximized_vert;
		Atom net_wm_state_modal;
		Atom net_wm_name;
		Atom net_wm_window_type;
		Atom net_wm_window_type_normal;
		Atom net_wm_window_type_utility;
		Atom net_wm_window_type_dialog;
		Atom motif_wm_hints;

		Atom clipboard;
		Atom text;
		Atom text_uri_list;
		Atom utf8_string;
		Atom targets;

		Atom xdnd_aware;
		Atom xdnd_enter;
		Atom xdnd_position;
		Atom xdnd_status;
		Atom xdnd_action_copy;
		Atom xdnd_action_move;
		Atom xdnd_action_link;
		Atom xdnd_drop;
		Atom xdnd_selection;
		Atom xdnd_typelist;
		Atom xdnd_leave;
		Atom xdnd_finished;
	};

	//A forward declaration of caret data
	struct caret_rep;

	/// class timer_core
	/**
	 * Platform-spec only provides the declaration for intrducing a handle type, the definition
	 * of timer_core is given by gui/timer.cpp
	 */
	class timer_core;

	class timer_runner;

	class platform_scope_guard
	{
	public:
		platform_scope_guard();
		~platform_scope_guard();
	};

	class x11_dragdrop_interface
	{
	public:
		virtual ~x11_dragdrop_interface() = default;

		virtual void add_ref() = 0;
		virtual std::size_t release() = 0;
	};

	class platform_spec
	{
		typedef platform_spec self_type;

		struct window_context_t
		{
			native_window_type owner;
			std::vector<native_window_type> * owned;
		};
	public:
		int error_code;
	public:
		typedef void (*timer_proc_type)(thread_t tid);
		typedef void (*event_proc_type)(Display*, msg_packet_tag&);
		typedef ::nana::event_code		event_code;
		typedef ::nana::native_window_type	native_window_type;

		platform_spec(const platform_spec&) = delete;
		platform_spec& operator=(const platform_spec&) = delete;

		platform_spec();
		~platform_spec();

		Display* open_display();
		void close_display();

		void lock_xlib();
		void unlock_xlib();

		Window root_window();
		int screen_depth();
		Visual* screen_visual();

		Colormap& colormap();

		static self_type& instance();
		const atombase_tag & atombase() const;

		void make_owner(native_window_type owner, native_window_type wd);

		// Cancel the ownership
		bool umake_owner(native_window_type child);
		native_window_type get_owner(native_window_type) const;
		void remove(native_window_type);

		void write_keystate(const XKeyEvent&);
		void read_keystate(XKeyEvent&);

		XIC	caret_input_context(native_window_type) const;
		void caret_open(native_window_type, const ::nana::size&);
		void caret_close(native_window_type);
		void caret_pos(native_window_type, const ::nana::point&);
		void caret_visible(native_window_type, bool);
		bool caret_update(native_window_type, nana::paint::graphics& root_graph, bool is_erase_caret_from_root_graph);
		void set_error_handler();
		int rev_error_handler();

		//grab
		//register a grab window while capturing it if it is unviewable.
		//when native_interface::show a window that is registered as a grab
		//window, the native_interface grabs the window.
		Window grab(Window);
		void set_timer(const timer_core*, std::size_t interval, void (*timer_proc)(const timer_core* tm));
		void kill_timer(const timer_core*);
		void timer_proc(thread_t tid);

		//Message dispatcher
		void msg_insert(native_window_type);
		void msg_set(timer_proc_type, event_proc_type);
		void msg_dispatch(native_window_type modal);
		void msg_dispatch(std::function<propagation_chain(const msg_packet_tag&)>);

		//X Selections
		void* request_selection(native_window_type requester, Atom type, size_t & bufsize);
		void write_selection(native_window_type owner, Atom type, const void* buf, size_t bufsize);

		//Icon storage
		//@biref: The image object should be kept for a long time till the window is closed,
		//			the image object is release in remove() method.
		const nana::paint::graphics& keep_window_icon(native_window_type, const nana::paint::image&);

		bool register_dragdrop(native_window_type, x11_dragdrop_interface*);
		std::size_t dragdrop_target(native_window_type, bool insert, std::size_t count);
		x11_dragdrop_interface* remove_dragdrop(native_window_type);
	private:
		static int _m_msg_filter(XEvent&, msg_packet_tag&);
		void _m_caret_routine();
	private:
		Display*	display_;
		Colormap	colormap_;
		atombase_tag atombase_;

		XKeyEvent	key_state_;
		int (*def_X11_error_handler_)(Display*, XErrorEvent*);
		Window grab_;
		std::recursive_mutex xlib_locker_;
		struct caret_holder_tag
		{
			std::atomic<bool> exit_thread;
			std::unique_ptr<std::thread> thr;
			std::map<native_window_type, caret_rep*> carets;
		}caret_holder_;

		std::map<native_window_type, window_context_t> wincontext_;
		std::map<native_window_type, nana::paint::graphics> iconbase_;

		struct timer_runner_tag
		{
			timer_runner * runner;
			std::recursive_mutex mutex;
			bool delete_declared;
			timer_runner_tag();
		}timer_;

		struct selection_tag
		{
			struct item_t
			{
				Atom	type;
				Window	requestor;
				void*	buffer;
				size_t	bufsize;
				std::mutex cond_mutex;
				std::condition_variable cond;
			};

			std::vector<item_t*> items;

			struct content_tag
			{
				std::string * utf8_string;
			}content;
		}selection_;

		struct xdnd_tag
		{
			Atom good_type;
			int timestamp;
			Window wd_src;
			nana::point pos;

			std::map<native_window_type, x11_dragdrop_interface*> dragdrop;
			std::map<native_window_type, std::size_t> targets;
		}xdnd_;

		msg_dispatcher * msg_dispatcher_;
	};//end class platform_X11

}//end namespace detail

}//end namespace nana

#include <nana/pop_ignore_diagnostic>
// .h ward
#endif

#endif

