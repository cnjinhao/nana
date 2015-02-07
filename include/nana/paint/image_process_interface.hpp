/*
 *	Image Processing Interfaces
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/paint/image_process_interface.hpp
 */

#ifndef NANA_PAINT_IMAGE_PROCESS_INTERFACE_HPP
#define NANA_PAINT_IMAGE_PROCESS_INTERFACE_HPP

#include <nana/basic_types.hpp>
#include <nana/paint/pixel_buffer.hpp>

namespace nana
{
	namespace paint
	{                       /// Image Processing Algorithm Interfaces
		namespace image_process
		{           /// The interface of stretch algorithm.
			class stretch_interface
			{
			public:
				virtual ~stretch_interface() = default;
                    /// Copies the image from a source rectangle into a destination rectangle, stretching or compressing the image to fit the dimensions of the destination rectangle in destination(d_pixbuf). 
				virtual void process(const paint::pixel_buffer & s_pixbuf, 
                                      const nana::rectangle& source_rectangle, 
                                      paint::pixel_buffer & d_pixbuf, 
                                      const nana::rectangle& destination_rectangle
                                      ) const = 0;
			};

			class alpha_blend_interface
			{
			public:
				virtual ~alpha_blend_interface() = default;
				virtual void process(const paint::pixel_buffer& s_pixbuf, const nana::rectangle& s_r, paint::pixel_buffer& d_pixbuf, const point& d_pos) const = 0;
			};
                    /// The interface of a blend algorithm.
			class blend_interface
			{
			public:
				virtual ~blend_interface() = default;
                    /// \brief  Blends two images with specified area and blend rate.
                    ///
                    /// Semantics: \code  dest_pixbuf = dest_pixbuf * fade_rate + scr_pixbuf * (1 - fade_rate); \endcode 
                    /// The area is always valid, it is calculated by Nana before passing to the algorithm. So the area could be applied without a check.
				virtual void process( const paint::pixel_buffer& src_pixbuf, 
                                      const nana::rectangle& src_area, 
                                      paint::pixel_buffer& dest_pixbuf, 
                                      const nana::point& dest_pos, 
                                      double fade_rate              ///< blend rate in the range of [0, 1]
                                      ) const = 0;
			};
                    /// The interface of line algorithm. 
			class line_interface
			{
			public:
				virtual ~line_interface() = default;

				    /// \brief   Draws a line 
                    ///
                    /// Semantics: \code    pixbuf = pixbuf * (1 - fade_rate) + color * fade_rate    \endcode 
                    /// The two points are calculated by Nana, they are always valid, and pos_beg.x <= pos_end.x
				virtual void process(paint::pixel_buffer & pixbuf, 
                                      const point& pos_beg,    ///<  left point
                                      const point& pos_end,    ///<  right point 
                                      const ::nana::color&, 
                                      double fade_rate              ///< blend rate in the range of [0, 1] If not 0, the line is blended to the pixbuf
                                      ) const = 0;
			};

			class blur_interface
			{
			public:
				virtual ~blur_interface() = default;
				virtual void process(paint::pixel_buffer&, const nana::rectangle& r, std::size_t radius) const = 0;
			};
		}
	}//end namespace paint
}//end namespace nana
#endif
