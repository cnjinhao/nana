/**
 *	Platform Specification Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file nana/detail/platform_spec.hpp
 *
 *	This file provides basis class and data structrue that required by nana
 *	This file should not be included by any header files.
 */
#if defined(NANA_WINDOWS)

#ifndef NANA_DETAIL_PLATFORM_SPEC_HPP
#define NANA_DETAIL_PLATFORM_SPEC_HPP

#include <nana/gui/basis.hpp>
#include <nana/paint/image.hpp>
#include <nana/gui/detail/event_code.hpp>

#include <windows.h>
#include <memory>
#include <functional>

#include "../platform_abstraction_types.hpp"

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

		struct map_thread
		{
			rectangle update_area;
			bool ignore_update_area;
			bool forced;
		};

		struct arg_affinity_execute
		{
			const std::function<void()> * function_ptr;
		};

		enum
		{
			tray = 0x501,

			async_activate,
			async_set_focus,
			remote_flush_surface,
			remote_thread_destroy_window,
			operate_caret,	//wParam: 1=Destroy, 2=SetPos
			remote_thread_set_window_pos,
			remote_thread_set_window_text,

			//Execute a function in a thread with is associated with a specified native window
			affinity_execute,

			user,
		};
	};

	struct drawable_impl_type
	{
		using font_type = ::std::shared_ptr<font_interface>;

		HDC		context;
		HBITMAP	pixmap;
		pixel_argb_t*	pixbuf_ptr{nullptr};
		std::size_t		bytes_per_line{0};

		font_type font;

		struct string_spec
		{
			unsigned tab_length;
			unsigned tab_pixels;
			unsigned whitespace_pixels;
		}string;

		unsigned fgcolor_rgb{ 0xFFFFFFFF };
		unsigned bgcolor_rgb{ 0xFFFFFFFF };
		unsigned fgcolor_native{ 0xFFFFFFFF };	//Windows RGB format: 0xBBGGRR
		unsigned bgcolor_native{ 0xFFFFFFFF };	//Windows RGB format

		drawable_impl_type(const drawable_impl_type&) = delete;
		drawable_impl_type& operator=(const drawable_impl_type&) = delete;

		drawable_impl_type();
		~drawable_impl_type();

		void set_color(const ::nana::color&);
		void set_text_color(const ::nana::color&);
	};

	class platform_spec
	{
		platform_spec();
		platform_spec(const platform_spec&) = delete;
		platform_spec& operator=(const platform_spec&) = delete;

		platform_spec(platform_spec&&) = delete;
		platform_spec& operator=(platform_spec&&) = delete;
	public:
		typedef ::nana::event_code event_code;
		typedef ::nana::native_window_type	native_window_type;

		class co_initializer
		{
		public:
			co_initializer();
			~co_initializer();

			void task_mem_free(void* p);
		private:
			HMODULE ole32_;
		};

		struct window_icons
		{
			::nana::paint::image sml_icon;
			::nana::paint::image big_icon;
		};

		~platform_spec();

		static platform_spec& instance();

		void keep_window_icon(native_window_type, const paint::image&sml_icon, const paint::image& big_icon);
		void release_window_icon(native_window_type);
	private:
		struct implementation;
		implementation * const impl_;
	};

}//end namespace detail
}//end namespace nana

// .h ward
#endif

//#if defined(NANA_WINDOWS)
#endif
