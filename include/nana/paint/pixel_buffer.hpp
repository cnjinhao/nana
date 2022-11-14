/*
 *	Pixel Buffer Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2022 Jinhao(cnjinhao@hotmail.com)
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
#include <filesystem>

namespace nana{	namespace paint
{
	/// Alpha blend operations
	enum class alpha_methods
	{
		straight_alpha,	///< DestColor = SrcColor * SrcAlpha + DestColor * (1 - SrcAlpha), DestAlpha = SrcAlpha * SrcAlpha + (1 - SrcAlpha) * DestAlpha
		premultiplied_alpha, ///< DestColor = SrcColor + DestColor * (1 - SrcAlpha), DestAlpha = ScrAlpha + (1 - SrcAlpha) * DestAlpha
		direct_copy,	///< DestColor = SrcColor, DestAlpha = SrcAlpha
	};

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

		/// Sets the alpha channel of all pixels.
		/**
		 * @param alpha Alpha value in range of [0, 1], 0 is full transparent, 1 is full opacity.
		 */
		void make_transparent(double alpha);

		//pixel_color_t * at(const point& pos) const; //deprecated
		pixel_color_t * raw_ptr(std::size_t row) const;
		pixel_color_t * operator[](std::size_t row) const;
		pixel_color_t* operator[](const point& pt) const noexcept;

		void fill_row(std::size_t row, const unsigned char* buffer, std::size_t bytes, unsigned bits_per_pixel);

		void put(const unsigned char* rawbits, std::size_t width, std::size_t height, std::size_t bits_per_pixel, std::size_t bytes_per_line, bool is_negative);
		void put_16bit(const unsigned char* rawbits, std::size_t width, std::size_t height, std::size_t bytes_per_line, bool is_negative, unsigned mask_red, unsigned mask_green, unsigned mask_blue, unsigned mask_alpha);
		
		void line(const std::string& name);
		void line(const ::nana::point& pos_beg, const ::nana::point& pos_end, const ::nana::color&, double fade_rate);

		void rectangle(const nana::rectangle&, const ::nana::color&, double fade_rate, bool solid);
		void gradual_rectangle(const ::nana::rectangle&, const ::nana::color& from, const ::nana::color& to, double fade_rate, bool vertical);
		
		pixel_color_t pixel(int x, int y) const;
		void pixel(int x, int y, pixel_color_t);
		void pixel(const nana::point& pt, pixel_color_t);

		void paste(drawable_type, const point& p_dst) const;
		void paste(const nana::rectangle& s_r, drawable_type, const point& p_dst, alpha_methods = alpha_methods::straight_alpha) const;
		void paste(const nana::rectangle& s_r, pixel_buffer&, const point& p_dst, alpha_methods = alpha_methods::straight_alpha) const;
		void paste(native_window_type, const point& p_dst) const;
		void stretch(const std::string& name);
		void stretch(const nana::rectangle& s_r, drawable_type, const nana::rectangle& r, alpha_methods = alpha_methods::straight_alpha) const;
		void stretch(const nana::rectangle& s_r, pixel_buffer&, const nana::rectangle& r, alpha_methods = alpha_methods::straight_alpha) const;
		void blend(const std::string& name);
		void blend(const nana::rectangle& s_r, drawable_type dw_dst, const nana::point& d_pos, double fade_rate) const;
		void blur(const nana::rectangle& r, std::size_t radius);

		pixel_buffer rotate(double angle, const color& extend_color);

		// Saves pixels as a Windows bitmap file.
		bool save(std::filesystem::path) const;

		void to_grayscale();

		void ellipse(const nana::point&, float a, float b, const color&);

	private:
		std::shared_ptr<pixel_buffer_storage> storage_;
	};
}//end namespace paint
}//end namespace nana

#endif
