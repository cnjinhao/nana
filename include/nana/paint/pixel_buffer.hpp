/*
 *	Pixel Buffer Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/paint/pixel_buffer.hpp
 */

#ifndef NANA_PAINT_PIXEL_BUFFER_HPP
#define NANA_PAINT_PIXEL_BUFFER_HPP

#include <nana/gui/basis.hpp>
#include <memory>

namespace nana{	namespace paint
{
	class pixel_buffer
	{
		struct pixel_buffer_storage;
		typedef bool (pixel_buffer:: * unspecified_bool_t)() const;
	public:
		pixel_buffer() = default;
		pixel_buffer(drawable_type, const nana::rectangle& want_rectangle);
		pixel_buffer(drawable_type, std::size_t top, std::size_t lines);
		pixel_buffer(std::size_t width, std::size_t height);

		~pixel_buffer();

		void attach(drawable_type, const nana::rectangle& want_rectangle);

		bool open(drawable_type);
		bool open(drawable_type, const nana::rectangle& want_rectangle);
		bool open(std::size_t width, std::size_t height);

		void alpha_channel(bool enabled);
		bool alpha_channel() const;

		void close();

		bool empty() const;

		operator unspecified_bool_t() const;

		std::size_t bytes() const;
		std::size_t bytes_per_line() const;
		nana::size size() const;

		pixel_color_t * at(const point& pos) const;
		pixel_color_t * raw_ptr(std::size_t row) const;
		pixel_color_t * operator[](std::size_t row) const;

		void fill_row(std::size_t row, const unsigned char* buffer, std::size_t bytes, unsigned bits_per_pixel);

		void put(const unsigned char* rawbits, std::size_t width, std::size_t height, std::size_t bits_per_pixel, std::size_t bytes_per_line, bool is_negative);
		
		void line(const std::string& name);
		void line(const ::nana::point& pos_beg, const ::nana::point& pos_end, const ::nana::color&, double fade_rate);

		void rectangle(const nana::rectangle&, const ::nana::color&, double fade_rate, bool solid);
		void gradual_rectangle(const ::nana::rectangle&, const ::nana::color& from, const ::nana::color& to, double fade_rate, bool vertical);
		
		pixel_color_t pixel(int x, int y) const;
		void pixel(int x, int y, pixel_color_t);

		void paste(drawable_type, const point& p_dst) const;
		void paste(const nana::rectangle& s_r, drawable_type, const point& p_dst) const;
		void paste(native_window_type, const point& p_dst) const;
		void stretch(const std::string& name);
		void stretch(const nana::rectangle& s_r, drawable_type, const nana::rectangle& r) const;
		void blend(const std::string& name);
		void blend(const nana::rectangle& s_r, drawable_type dw_dst, const nana::point& d_pos, double fade_rate) const;
		void blur(const nana::rectangle& r, std::size_t radius);

	private:
		std::shared_ptr<pixel_buffer_storage> storage_;
	};
}//end namespace paint
}//end namespace nana

#endif
