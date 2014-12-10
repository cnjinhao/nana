/*
 *	Paint Graphics Implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/paint/graphics.hpp
 */

#ifndef NANA_PAINT_GRAPHICS_HPP
#define NANA_PAINT_GRAPHICS_HPP

#include "../basic_types.hpp"
#include "../gui/basis.hpp"
#include "pixel_buffer.hpp"
#include <memory>

namespace nana
{
	namespace paint
	{
		namespace detail
		{
			struct native_font_signature;
		}// end namespace detail

		typedef detail::native_font_signature*		native_font_type;

		class font
		{
			friend class graphics;
		public:
			font();
			font(drawable_type);
			font(const font&);
			font(const char_t* name, unsigned size, bool bold = false, bool italic = false, bool underline = false, bool strike_out = false);
			~font();
			bool empty() const;
			void make(const char_t* name, unsigned size, bool bold = false, bool italic = false, bool underline = false, bool strike_out = false);
			void make_raw(const char_t*, unsigned height, unsigned weight, bool italic, bool underline, bool strike_out);

			void set_default() const;
			::nana::string name() const;
			unsigned size() const;
			bool bold() const;
			unsigned height() const;
			unsigned weight() const;
			bool italic() const;
			native_font_type handle() const;
			void release();

			font& operator=(const font&);
			bool operator==(const font&) const;
			bool operator!=(const font&) const;
		private:
			struct impl_type;
			impl_type * impl_;
		};

		/// \brief off-screen resource defined as ref-counting, can refer one resource
        ///
        /// Load a bitmap into a graphics:
        /// \code
        /// nana::paint::graphics graph;
        /// nana::paint::image img("C:\\ABitmap.bmp");
        /// img.paste(graph, 0, 0); //graph may create if it is empty
        /// \endcode
		class graphics
		{
		public:
			typedef ::nana::native_window_type native_window_type;

			graphics();
			graphics(unsigned width, unsigned height);   ///< size in pixel
			graphics(const ::nana::size&);                 ///< size in pixel
			graphics(const graphics&);      ///< the resource is not copyed, the two graphics objects refer to the *SAME* resource
			graphics& operator=(const graphics&);
			bool changed() const;           ///< Returns true if the graphics object is operated
			bool empty() const;             ///< Returns true if the graphics object does not refer to any resource.
			operator const void*() const;

			drawable_type handle() const;
			const void* pixmap() const;
			const void* context() const;
			void make(unsigned width, unsigned height);       ///< Creates a bitmap resource that size is width by height in pixel
			void resize(unsigned width, unsigned height);
			void typeface(const font&);                       ///< Selects a specified font type into the graphics object.
			font typeface() const;
			::nana::size	text_extent_size(const char_t*) const;    ///< Computes the width and height of the specified string of text.
			::nana::size	text_extent_size(const string&) const;    ///< Computes the width and height of the specified string of text.
			::nana::size	text_extent_size(const char_t*, std::size_t length) const;    ///< Computes the width and height of the specified string of text with the specified length.
			::nana::size	text_extent_size(const string&, std::size_t length) const;    ///< Computes the width and height of the specified string of text with the specified length.
			::nana::size	glyph_extent_size(const char_t*, std::size_t length, std::size_t begin, std::size_t end) const;
			::nana::size	glyph_extent_size(const string&, std::size_t length, std::size_t begin, std::size_t end) const;
			bool glyph_pixels(const char_t *, std::size_t length, unsigned* pxbuf) const;
			::nana::size	bidi_extent_size(const string&) const;

			bool text_metrics(unsigned & ascent, unsigned& descent, unsigned& internal_leading) const;

			unsigned bidi_string(int x, int y, color_t, const char_t *, std::size_t len);
			void string(int x, int y, color_t, const ::nana::string&, std::size_t len);
			void string(int x, int y, color_t, const ::nana::string&);
			void string(int x, int y, color_t, const char_t*, std::size_t len);
			void string(int x, int y, color_t, const char_t*);

			void set_pixel(int x, int y, color_t);
			void rectangle(int x, int y, unsigned width, unsigned height, color_t, bool solid);
			void rectangle(color_t, bool solid);
			void rectangle(const ::nana::rectangle&, color_t, bool solid);
			void rectangle_line(const ::nana::rectangle&, color_t left, color_t top, color_t right, color_t bottom);
			void round_rectangle(int x, int y, unsigned width, unsigned height, unsigned radius_x, unsigned radius_y, color_t, bool solid, color_t color_if_solid);
			void round_rectangle(const ::nana::rectangle&, unsigned radius_x, unsigned radius_y, color_t, bool solid, color_t color_if_solid);
			void shadow_rectangle(const ::nana::rectangle&, color_t beg_color, color_t end_color, bool vertical);
			void shadow_rectangle(int x, int y, unsigned width, unsigned height, color_t beg_color, color_t end_color, bool vertical); ///< Draws a width and height rectangle at (x, y) and the color in range of [begin, end]

			void line(int x1, int y1, int x2, int y2, color_t);     ///<  Draws a line from point (x1, y1) to point (x2, y2) in the specified color.
			void line(const point& beg, const point& end, color_t);
			void lines(const point* points, std::size_t n_of_points, color_t);
			void line_begin(int x, int y);
			void line_to(int x, int y, color_t);

			void bitblt(int x, int y, const graphics& source);     ///<   Transfers the source to the specified point.
			void bitblt(const ::nana::rectangle& r_dst, native_window_type src);  ///< Transfers the color data corresponding to r_dst from the src window to this graphics.
			void bitblt(const ::nana::rectangle& r_dst, native_window_type src, const point& p_src);  ///< Transfers the color data corresponding to r_dst from the src window at point p_src to this graphics.
			void bitblt(const ::nana::rectangle& r_dst, const graphics& src);     ///< Transfers the color data corresponding to r_dst from the src graphics to this graphics.
			void bitblt(const ::nana::rectangle& r_dst, const graphics& src, const point& p_src);///< Transfers the color data corresponding to r_dst from the src graphics at point p_src to this graphics.

			void blend(const ::nana::rectangle& s_r, graphics& dst, const point& d_pos, double fade_rate) const;///< blends with the dst object.
			void blend(const ::nana::rectangle& r, color_t, double fade_rate);      ///< blends the specifed block width the specified color.

			void blur(const ::nana::rectangle& r, std::size_t radius);      ///< Blur process.

			void paste(graphics& dst, int x, int y) const;    ///< Paste the graphics object into the dest at (x, y)
			void paste(native_window_type dst, const ::nana::rectangle&, int sx, int sy) const;  ///< Paste the graphics object into a platform-dependent window at (x, y)
			void paste(native_window_type dst, int dx, int dy, unsigned width, unsigned height, int sx, int sy) const;
			void paste(drawable_type dst, int x, int y) const;
			void paste(const ::nana::rectangle& r_src, graphics& dst, int x, int y) const;
			void rgb_to_wb();   ///< Transform a color graphics into black&white.

			void stretch(const ::nana::rectangle& src_r, graphics& dst, const ::nana::rectangle& r) const;
			void stretch(graphics& dst, const ::nana::rectangle& r) const;

			void flush();

			unsigned width() const;
			unsigned height() const;      ///< Returns the height of the off-screen buffer.
			::nana::size size() const;
			void setsta();      ///<  	Clears the status if the graphics object had been changed
			void release();
			void save_as_file(const char*);

			static color_t mix(color_t colorX, color_t colorY, double persent);
		private:
			std::shared_ptr< ::nana::detail::drawable_impl_type> dwptr_;
			font			font_shadow_;
            drawable_type	handle_;
			::nana::size	size_;
			pixel_buffer	pxbuf_;
			bool changed_;
		};
	}//end namespace paint
}	//end namespace nana
#endif

