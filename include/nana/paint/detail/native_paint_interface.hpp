/*
 *	Platform Implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/paint/detail/native_paint_interface.hpp
 */

#ifndef NANA_PAINT_DETAIL_PLATFORM_HPP
#define NANA_PAINT_DETAIL_PLATFORM_HPP
#include <nana/basic_types.hpp>

namespace nana
{
namespace paint
{
namespace detail
{
	nana::size drawable_size(drawable_type);

	std::unique_ptr<unsigned char[]> alloc_fade_table(double fade_rate);
	void free_fade_table(const unsigned char*);

	//color = bgcolor * fade_rate + fgcolor * (1 - fade_rate);
	//nana::pixel_color_t fade_color(nana::pixel_color_t bgcolor, nana::pixel_color_t fgcolor, double fade_rate);	//deprecated
	nana::pixel_color_t fade_color(nana::pixel_color_t bgcolor, nana::pixel_color_t fgcolor, const unsigned char* const fade_table);
	nana::pixel_color_t fade_color_intermedia(pixel_color_t fgcolor, const unsigned char* fade_table);
	nana::pixel_color_t fade_color_by_intermedia(pixel_color_t bgcolor, nana::pixel_color_t fgcolor_intermedia, const unsigned char* const fade_table);

	void blend(drawable_type dw, const nana::rectangle& r, pixel_color_t, double fade_rate);

	nana::size raw_text_extent_size(drawable_type, const wchar_t*, std::size_t len);
	nana::size text_extent_size(drawable_type, const wchar_t*, std::size_t len);
	void draw_string(drawable_type, const nana::point&, const wchar_t *, std::size_t len);
}//end namespace detail
}//end namespace paint
}//end namespace nana

#endif
