/*
 *	Paint Graphics Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2014 Jinhao(cnjinhao@hotmail.com)
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
			font(const ::std::string& name, unsigned size, bool bold = false, bool italic = false, bool underline = false, bool strike_out = false);
			~font();
			bool empty() const;
			void make(const ::std::string& name, unsigned size, bool bold = false, bool italic = false, bool underline = false, bool strike_out = false);
			void make_raw(const ::std::string& name, unsigned height, unsigned weight, bool italic, bool underline, bool strike_out);

			void set_default() const;
			::std::string name() const;
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
			graphics(const ::nana::size&);                 ///< size in pixel
			graphics(const graphics&);      ///< the resource is not copyed, the two graphics objects refer to the *SAME* resource
			graphics& operator=(const graphics&);
			bool changed() const;           ///< Returns true if the graphics object is operated
			bool empty() const;             ///< Returns true if the graphics object does not refer to any resource.
			operator const void*() const;

			drawable_type handle() const;
			const void* pixmap() const;
			const void* context() const;

			void make(const ::nana::size&);					///< Creates a bitmap resource that size is width by height in pixel
			void resize(const ::nana::size&);
			void typeface(const font&);						///< Selects a specified font type into the graphics object.
			font typeface() const;
			::nana::size	text_extent_size(const ::std::string&) const;
			::nana::size	text_extent_size(const char*, std::size_t len) const;
			::nana::size	text_extent_size(const wchar_t*) const;    ///< Computes the width and height of the specified string of text.
			::nana::size	text_extent_size(const ::std::wstring&) const;    ///< Computes the width and height of the specified string of text.
			::nana::size	text_extent_size(const wchar_t*, std::size_t length) const;    ///< Computes the width and height of the specified string of text with the specified length.
			::nana::size	text_extent_size(const ::std::wstring&, std::size_t length) const;    ///< Computes the width and height of the specified string of text with the specified length.
			::nana::size	glyph_extent_size(const wchar_t*, std::size_t length, std::size_t begin, std::size_t end) const;
			::nana::size	glyph_extent_size(const ::std::wstring&, std::size_t length, std::size_t begin, std::size_t end) const;
			bool glyph_pixels(const wchar_t *, std::size_t length, unsigned* pxbuf) const;
			::nana::size	bidi_extent_size(const std::wstring&) const;
			::nana::size	bidi_extent_size(const std::string&) const;

			bool text_metrics(unsigned & ascent, unsigned& descent, unsigned& internal_leading) const;

			void line_begin(int x, int y);

			void bitblt(int x, int y, const graphics& source);     ///<   Transfers the source to the specified point.
			void bitblt(const ::nana::rectangle& r_dst, native_window_type src);  ///< Transfers the color data corresponding to r_dst from the src window to this graphics.
			void bitblt(const ::nana::rectangle& r_dst, native_window_type src, const point& p_src);  ///< Transfers the color data corresponding to r_dst from the src window at point p_src to this graphics.
			void bitblt(const ::nana::rectangle& r_dst, const graphics& src);     ///< Transfers the color data corresponding to r_dst from the src graphics to this graphics.
			void bitblt(const ::nana::rectangle& r_dst, const graphics& src, const point& p_src);///< Transfers the color data corresponding to r_dst from the src graphics at point p_src to this graphics.

			void blend(const ::nana::rectangle& s_r, graphics& dst, const point& d_pos, double fade_rate) const;///< blends with the dst object.

			void blur(const ::nana::rectangle& r, std::size_t radius);      ///< Blur process.

			void paste(graphics& dst, int x, int y) const;    ///< Paste the graphics object into the dest at (x, y)
			void paste(native_window_type dst, const ::nana::rectangle&, int sx, int sy) const;  ///< Paste the graphics object into a platform-dependent window at (x, y)
			void paste(native_window_type dst, int dx, int dy, unsigned width, unsigned height, int sx, int sy) const;
			void paste(drawable_type dst, int x, int y) const;
			void paste(const ::nana::rectangle& r_src, graphics& dst, int x, int y) const;
			void rgb_to_wb();   ///< Transform a color graphics into black&white.

			void stretch(const ::nana::rectangle& src_r, graphics& dst, const ::nana::rectangle& r) const;
			void stretch(graphics& dst, const ::nana::rectangle&) const;

			void flush();

			unsigned width() const;
			unsigned height() const;      ///< Returns the height of the off-screen buffer.
			::nana::size size() const;
			void setsta();      ///<  	Clears the status if the graphics object had been changed
			void set_changed();
			void release();

			/// Saves images as a windows bitmap file
			/// @param file_utf8 A UTF-8 string to a filename
			void save_as_file(const char* file_utf8) const throw();

			::nana::color	palette(bool for_text) const;
			graphics&		palette(bool for_text, const ::nana::color&);

			unsigned bidi_string(const nana::point&, const wchar_t *, std::size_t len);
			unsigned bidi_string(const point& pos, const char*, std::size_t len);

			void blend(const ::nana::rectangle& r, const ::nana::color&, double fade_rate);

			void set_pixel(int x, int y, const ::nana::color&);
			void set_pixel(int x, int y);

			void string(const point&, const std::string& text_utf8);
			void string(const point&, const std::string& text_utf8, const color&);

			void string(point, const wchar_t*, std::size_t len);
			void string(const point&, const wchar_t*);
			void string(const point&, const ::std::wstring&);
			void string(const point&, const ::std::wstring&, const color&);

			void line(const point&, const point&);
			void line(const point&, const point&, const color&);
			void line_to(const point&, const color&);
			void line_to(const point&);

			void rectangle(bool solid);
			void rectangle(bool solid, const color&);
			void rectangle(const ::nana::rectangle&, bool solid);
			void rectangle(const ::nana::rectangle&, bool solid, const color&);
			void frame_rectangle(const ::nana::rectangle&, const color& left, const color& top, const color& right, const color& bottom);

			void gradual_rectangle(const ::nana::rectangle&, const color& from, const color& to, bool vertical);
			void round_rectangle(const ::nana::rectangle&, unsigned radius_x, unsigned radius_y, const color&, bool solid, const color& color_if_solid);
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

