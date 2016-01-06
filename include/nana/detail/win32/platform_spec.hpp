/*
 *	Platform Specification Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
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
#if defined(NANA_WINDOWS)

#ifndef NANA_DETAIL_PLATFORM_SPEC_HPP
#define NANA_DETAIL_PLATFORM_SPEC_HPP

#include <nana/deploy.hpp>
#include <nana/gui/basis.hpp>
#include <nana/paint/image.hpp>
#include <nana/gui/detail/event_code.hpp>

#include <windows.h>
#include <map>
#include <memory>

namespace nana
{

namespace detail
{
	//struct messages
	//@brief:	This defines some messages that are used for remote thread invocation.
	//			Some Windows APIs are window-thread-dependent, the operation in other thread
	//			must be posted to its own thread.
	struct messages
	{
		struct caret
		{
			int x;
			int y;
			unsigned width;
			unsigned height;
			bool visible;
		};

		struct move_window
		{
			enum { Pos = 1, Size = 2};
			int x;
			int y;
			unsigned width;
			unsigned height;
			unsigned ignore; //determinate that pos or size would be ignored.
		};

		struct map_thread
		{
			rectangle update_area;
			bool ignore_update_area;
			bool forced;
		};

		enum
		{
			tray = 0x501,
			async_activate,
			async_set_focus,
			map_thread_root_buffer,
			remote_thread_destroy_window,
			remote_thread_move_window,
			operate_caret,	//wParam: 1=Destroy, 2=SetPos
			remote_thread_set_window_pos,
			remote_thread_set_window_text,
			user,
		};
	};

	struct font_tag
	{
		native_string_type name;
		unsigned height;
		unsigned weight;
		bool italic;
		bool underline;
		bool strikeout;
		HFONT handle;

		struct deleter
		{
			void operator()(const font_tag*) const;
		};
	};

	struct drawable_impl_type
	{
		typedef std::shared_ptr<font_tag> font_ptr_t;

		HDC		context;
		HBITMAP	pixmap;
		pixel_argb_t*	pixbuf_ptr{nullptr};
		std::size_t		bytes_per_line{0};
		font_ptr_t font;

		struct pen_spec
		{
			HPEN	handle;
			unsigned color;
			int style;
			int width;

			void set(HDC context, int style, int width,unsigned color);
		}pen;

		struct brush_spec
		{
			enum t{Solid, HatchBDiagonal};

			HBRUSH handle;
			t style;
			unsigned color;

			void set(HDC context, t style, unsigned color);
		}brush;

		struct round_region_spec
		{
			HRGN handle;
			nana::rectangle r;
			unsigned radius_x;
			unsigned radius_y;

			void set(const nana::rectangle& r, unsigned radius_x, unsigned radius_y);
		}round_region;

		struct string_spec
		{
			unsigned tab_length;
			unsigned tab_pixels;
			unsigned whitespace_pixels;
		}string;

		drawable_impl_type();
		~drawable_impl_type();

		void fgcolor(const ::nana::color&);	//deprecated
		unsigned get_color() const;
		unsigned get_text_color() const;
		void set_color(const ::nana::color&);
		void set_text_color(const ::nana::color&);

		void update_pen();
		void update_brush();
	private:
		unsigned color_{ 0xffffffff };
		unsigned text_color_{0xffffffff};
	};

	class platform_spec
	{
	public:
		typedef drawable_impl_type::font_ptr_t	font_ptr_t;
		typedef ::nana::event_code event_code;
		typedef ::nana::native_window_type	native_window_type;

		class co_initializer
		{
		public:
			co_initializer();
			~co_initializer();
		private:
			HMODULE ole32_;
		};

		struct window_icons
		{
			::nana::paint::image sml_icon;
			::nana::paint::image big_icon;
		};

		platform_spec();

		const font_ptr_t& default_native_font() const;
		void default_native_font(const font_ptr_t&);
		unsigned font_size_to_height(unsigned) const;
		unsigned font_height_to_size(unsigned) const;
		font_ptr_t make_native_font(const char* name, unsigned height, unsigned weight, bool italic, bool underline, bool strike_out);

		static platform_spec& instance();

		void keep_window_icon(native_window_type, const paint::image&sml_icon, const paint::image& big_icon);
		void release_window_icon(native_window_type);
	private:
		font_ptr_t	def_font_ptr_;
		std::map<native_window_type, window_icons> iconbase_;
	};

}//end namespace detail
}//end namespace nana

// .h ward
#endif

//#if defined(NANA_WINDOWS)
#endif
