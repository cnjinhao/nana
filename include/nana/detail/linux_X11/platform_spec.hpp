/*
 *	Platform Specification Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
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

#if defined(NANA_LINUX) || defined(NANA_MACOS)

#ifndef NANA_DETAIL_PLATFORM_SPEC_HPP
#define NANA_DETAIL_PLATFORM_SPEC_HPP

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
#include "msg_packet.hpp"

#define NANA_USE_XFT
#if defined(NANA_USE_XFT)
	#include <X11/Xft/Xft.h>
	#include <iconv.h>
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

	class charset_conv
	{
	public:
		charset_conv(const char* tocode, const char* fromcode);
		~charset_conv();
		std::string charset(const std::string& str) const;
		std::string charset(const char * buf, std::size_t len) const;
	private:
		iconv_t handle_;
	};
#endif

	struct font_tag
	{
		std::string name;
		unsigned height;
		unsigned weight;
		bool italic;
		bool underline;
		bool strikeout;
#if defined(NANA_USE_XFT)
		XftFont * handle;
#else
		XFontSet handle;
#endif
	};

	struct drawable_impl_type
	{
		typedef std::shared_ptr<font_tag> font_ptr_t;

		Pixmap	pixmap;
		GC	context;
		font_ptr_t font;

		nana::point	line_begin_pos;

		struct string_spec
		{
			unsigned tab_length;
			unsigned tab_pixels;
			unsigned whitespace_pixels;
		}string;
#if defined(NANA_USE_XFT)
		XftDraw * xftdraw{nullptr};
		XftColor	xft_fgcolor;
		const std::string charset(const std::wstring& str, const std::string& strcode);
#endif
		drawable_impl_type();
		~drawable_impl_type();

		void fgcolor(const ::nana::color&); //deprecated
		unsigned get_color() const;
		unsigned get_text_color() const;
		void set_color(const ::nana::color&);
		void set_text_color(const ::nana::color&);

		void update_color();
		void update_text_color();
	private:
		unsigned current_color_{ 0xFFFFFF };
		unsigned color_{ 0xFFFFFFFF };
		unsigned text_color_{ 0xFFFFFFFF };

#if defined(NANA_USE_XFT)
		struct conv_tag
		{
			iconv_t handle;
			std::string code;
		}conv_;
#endif
	};

	struct atombase_tag
	{
		Atom wm_protocols;
		//window manager support
		Atom wm_change_state;
		Atom wm_delete_window;
		//ext
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
		Atom xdnd_drop;
		Atom xdnd_selection;
		Atom xdnd_typelist;
		Atom xdnd_finished;
	};

	struct caret_tag;

	class timer_runner;

	class platform_scope_guard
	{
	public:
		platform_scope_guard();
		~platform_scope_guard();
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
		typedef drawable_impl_type::font_ptr_t font_ptr_t;
		typedef void (*timer_proc_type)(unsigned tid);
		typedef void (*event_proc_type)(Display*, msg_packet_tag&);
		typedef ::nana::event_code		event_code;
		typedef ::nana::native_window_type	native_window_type;


		platform_spec();
		~platform_spec();

		const font_ptr_t& default_native_font() const;
		void default_native_font(const font_ptr_t&);
		unsigned font_size_to_height(unsigned) const;
		unsigned font_height_to_size(unsigned) const;
		font_ptr_t make_native_font(const char* name, unsigned height, unsigned weight, bool italic, bool underline, bool strick_out);

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
		native_window_type get_owner(native_window_type) const;
		void remove(native_window_type);

		void write_keystate(const XKeyEvent&);
		void read_keystate(XKeyEvent&);

		XIC	caret_input_context(native_window_type) const;
		void caret_open(native_window_type, const ::nana::size&);
		void caret_close(native_window_type);
		void caret_pos(native_window_type, const ::nana::point&);
		void caret_visible(native_window_type, bool);
		void caret_flash(caret_tag&);
		bool caret_update(native_window_type, nana::paint::graphics& root_graph, bool is_erase_caret_from_root_graph);
		static bool caret_reinstate(caret_tag&);
		void set_error_handler();
		int rev_error_handler();

		//grab
		//register a grab window while capturing it if it is unviewable.
		//when native_interface::show a window that is registered as a grab
		//window, the native_interface grabs the window.
		Window grab(Window);
		void set_timer(std::size_t id, std::size_t interval, void (*timer_proc)(std::size_t id));
		void kill_timer(std::size_t id);
		void timer_proc(unsigned tid);

		//Message dispatcher
		void msg_insert(native_window_type);
		void msg_set(timer_proc_type, event_proc_type);
		void msg_dispatch(native_window_type modal);

		//X Selections
		void* request_selection(native_window_type requester, Atom type, size_t & bufsize);
		void write_selection(native_window_type owner, Atom type, const void* buf, size_t bufsize);

		//Icon storage
		//@biref: The image object should be kept for a long time till the window is closed,
		//			the image object is release in remove() method.
		const nana::paint::graphics& keep_window_icon(native_window_type, const nana::paint::image&);
	private:
		static int _m_msg_filter(XEvent&, msg_packet_tag&);
		void _m_caret_routine();
	private:
		Display*	display_;
		Colormap	colormap_;
		atombase_tag atombase_;
		font_ptr_t def_font_ptr_;
		XKeyEvent	key_state_;
		int (*def_X11_error_handler_)(Display*, XErrorEvent*);
		Window grab_;
		std::recursive_mutex xlib_locker_;
		struct caret_holder_tag
		{
			volatile bool exit_thread;
			std::unique_ptr<std::thread> thr;
			std::map<native_window_type, caret_tag*> carets;
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
		}xdnd_;

		msg_dispatcher * msg_dispatcher_;
	};//end class platform_X11

}//end namespace detail

}//end namespace nana

// .h ward
#endif

//#if defined(NANA_LINUX) || defined(NANA_MACOS)
#endif

