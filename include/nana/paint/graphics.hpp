/*
 *	Paint Graphics Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2021 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/paint/graphics.hpp
 */

#ifndef NANA_PAINT_GRAPHICS_HPP
#define NANA_PAINT_GRAPHICS_HPP

#include <memory>

#include "../basic_types.hpp"
#include "../gui/basis.hpp"
#include <filesystem>
#include <string_view>
#include "detail/ptdefs.hpp"

#include <optional>


namespace nana
{
	namespace paint
	{
		class font
		{
			friend class graphics;
		public:
			using path_type = ::std::filesystem::path;

			using font_style = ::nana::detail::font_style;

			font();
			font(drawable_type);
			font(const font&);

			/// creates a font object.
			/// @param info Specifies the font family, size and styles.
			/// @param dpi Specifies the DPI for scaling the font, 0 indicates the system DPI.
			font(const font_info& info, std::size_t dpi = 0);

			/// creates a font object.
			/// @param name The font family.
			/// @param size_pt The font size.
			/// @param fs The font style.
			/// @param dpi Specifies the DPI for scaling the font, 0 indicates the system DPI.
			font(const ::std::string& name, double size_pt, const font_style& fs = {}, std::size_t dpi = 0);

			/// creates a font object with a truetype font file.
			/// @param size_pt The font size.
			/// @param truetype The path to a truetype font file
			/// @param fs The font style.
			/// @param dpi Specifies the DPI for scaling the font, 0 indicates the system DPI.
			font(double size_pt, const path_type& truetype, const font_style& fs = {}, std::size_t dpi = 0);

			~font();
			bool empty() const;

			void set_default() const;
			::std::string name() const;

			/// Returns font size, in point.
			/**
			* @param fixed Indicates whether to return a fixed font size. If this parameter is false, the method may return zero for default system font size. If the parameter is true, the method returns a fixed size of default font size if the font size that assigned by constructor is zero.
			* @return The font size, in point.
			*/
			double size(bool fixed = false) const;

			bool bold() const;
			unsigned weight() const;
			bool italic() const;
			native_font_type handle() const;
			
			void release();
			
			bool strikeout() const;
			bool underline() const;

			std::optional<font_info> info() const;

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
			graphics();
			graphics(const ::nana::size&);                 ///< size in pixel
			graphics(const graphics&);      ///< the resource is not copyed, the two graphics objects refer to the *SAME* resource
			graphics& operator=(const graphics&);

			graphics(graphics&&);
			graphics& operator=(graphics&&);

			~graphics();

			bool changed() const;           ///< Returns true if the graphics object is operated
			bool empty() const;             ///< Returns true if the graphics object does not refer to any resource.
			explicit operator bool() const noexcept;

			drawable_type handle() const;
			const void* pixmap() const;
			const void* context() const;

			void swap(graphics& other) noexcept;

			/// Creates a graphics/drawable resource
			/**
			 * @param sz The dimension of the graphics to be requested. If sz is empty, it performs as release().
			 */
			void make(const ::nana::size& sz);
			void resize(const ::nana::size&);
			void typeface(const font&);						///< Selects a specified font type into the graphics object.
			font typeface() const;

			::nana::size text_extent_size(std::string_view text) const;
			::nana::size text_extent_size(std::wstring_view text) const;
#ifdef __cpp_char8_t
			::nana::size text_extent_size(std::u8string_view text) const;
#endif

			///Only supports the wide string, because it is very hard to specify the begin and end position in a UTF-8 string.
			::nana::size glyph_extent_size(std::wstring_view text, std::size_t begin, std::size_t end) const;

			/// Returns a buffer which stores the pixel of each charater stored in text.
			/**
			 * @param text The text to be requested.
			 * @return A buffer which stores the pixel of each character stored in text, its length is same with text's length. If text is empty, it returns a buffer with a senseless value.
			 */
			std::unique_ptr<unsigned[]> glyph_pixels(std::wstring_view text) const;

			::nana::size	bidi_extent_size(std::string_view utf8_text) const;
			::nana::size	bidi_extent_size(std::wstring_view text) const;
#ifdef __cpp_char8_t
			::nana::size 	bidi_extent_size(std::u8string_view text) const;
#endif

			bool text_metrics(unsigned & ascent, unsigned& descent, unsigned& internal_leading) const;

			void line_begin(int x, int y);

			void bitblt(int x, int y, const graphics& source);     ///<   Transfers the source to the specified point.
			void bitblt(const ::nana::rectangle& r_dst, native_window_type src);  ///< Transfers the color data corresponding to r_dst from the src window to this graphics.
			void bitblt(const ::nana::rectangle& r_dst, native_window_type src, const point& p_src);  ///< Transfers the color data corresponding to r_dst from the src window at point p_src to this graphics.
			void bitblt(const ::nana::rectangle& r_dst, const graphics& src);     ///< Transfers the color data corresponding to r_dst from the src graphics to this graphics.
			void bitblt(const ::nana::rectangle& r_dst, const graphics& src, const point& p_src);///< Transfers the color data corresponding to r_dst from the src graphics at point p_src to this graphics.

			void blend(const ::nana::rectangle& r, const ::nana::color&, double fade_rate);
			void blend(const ::nana::rectangle& blend_r, const graphics& blend_graph, const point& blend_graph_point, double fade_rate);///< blends with the blend_graph.

			void blur(const ::nana::rectangle& r, std::size_t radius);      ///< Blur process.

			void paste(graphics& dst, int x, int y) const;    ///< Paste the graphics object into the dest at (x, y)
			void paste(native_window_type dst, const ::nana::rectangle&, int sx, int sy) const;  ///< Paste the graphics object into a platform-dependent window at (x, y)
			void paste(native_window_type dst, int dx, int dy, unsigned width, unsigned height, int sx, int sy) const;
			void paste(drawable_type dst, int x, int y) const;
			void paste(const ::nana::rectangle& r_src, graphics& dst, int x, int y) const;
			void rgb_to_wb(bool skip_transparent_pixels = false);   ///< Transform a color graphics into black&white.

			void stretch(const ::nana::rectangle& src_r, graphics& dst, const ::nana::rectangle& r) const;
			void stretch(graphics& dst, const ::nana::rectangle&) const;

			void flush();

			unsigned width() const;		///< Returns the width of the off-screen buffer.
			unsigned height() const;	///< Returns the height of the off-screen buffer.
			::nana::size size() const;
			void setsta();				///< Clears the status if the graphics object had been changed
			void set_changed();
			void release();

			/// Saves images as a windows bitmap file
			/// @param file_utf8 A UTF-8 string to a filename
			void save_as_file(const char* file_utf8) const noexcept;

			::nana::color	palette(bool for_text) const;
			graphics&		palette(bool for_text, const ::nana::color&);

			void set_pixel(int x, int y, const ::nana::color&);
			void set_pixel(int x, int y);

			unsigned bidi_string(const point&, std::string_view utf8str);
			unsigned bidi_string(const point& pos, std::wstring_view str);

			void string(const point&, std::string_view utf8str);
			void string(const point&, std::string_view utf8str, const nana::color&);

			void string(const point&, std::wstring_view str);
			void string(const point&, std::wstring_view str, const nana::color&);

#ifdef __cpp_char8_t
			void string(const point&, std::u8string_view str);
			void string(const point&, std::u8string_view str, const nana::color&);
#endif

			void line(const point&, const point&);
			void line(const point&, const point&, const color&);
			void line_to(const point&, const color&);
			void line_to(const point&);

			void rectangle(bool solid);
			void rectangle(bool solid, const color&);
			void rectangle(const ::nana::rectangle&, bool solid);
			void rectangle(const ::nana::rectangle&, bool solid, const color&);
			void frame_rectangle(const ::nana::rectangle&, const color& left, const color& top, const color& right, const color& bottom);
			void frame_rectangle(const ::nana::rectangle&, const color&, unsigned gap);

			void gradual_rectangle(const ::nana::rectangle&, const color& from, const color& to, bool vertical);
			void round_rectangle(const ::nana::rectangle&, unsigned radius_x, unsigned radius_y, const color&, bool solid, const color& color_if_solid);
		private:
			struct implementation;
			std::unique_ptr<implementation> impl_;
		};

		class draw
		{
		public:
			draw(graphics& graph);

			void corner(const rectangle& r, unsigned pixels);
		private:
			graphics& graph_;
		};
	}//end namespace paint
}	//end namespace nana
#endif

