#ifndef NANA_PAINT_DETAIL_IMAGE_PNG_HPP
#define NANA_PAINT_DETAIL_IMAGE_PNG_HPP

#include "image_impl_interface.hpp"

//Separate the libpng from the package that system provides.
#if defined(NANA_LIBPNG)
	#include <nana/extrlib/png.h>
#else
	#include <png.h>
#endif

#include <stdio.h>
#include "../pixel_buffer.hpp"

namespace nana
{
	namespace paint{	namespace detail{

		class image_png
			: public image::image_impl_interface
		{
		public:
			image_png()
			{
			}

			bool open(const nana::char_t* png_file)
			{
#ifdef NANA_UNICODE
				FILE * fp = ::fopen(static_cast<std::string>(nana::charset(png_file)).c_str(), "rb");
#else
				FILE* fp = ::fopen(png_file, "rb");
#endif
				if(nullptr == fp) return false;

				bool is_opened = false;

				png_byte png_sig[8];
				::fread(png_sig, 1, 8, fp);

				//Test whether the file is a png.
				if(0 == png_sig_cmp(png_sig, 0, 8))
				{
					png_structp png_ptr = ::png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
					if(png_ptr)
					{
						png_infop info_ptr = ::png_create_info_struct(png_ptr);

						if(info_ptr)
						{
							if(!setjmp(png_jmpbuf(png_ptr)))
							{
								//The following codes may longjmp while init_io error.
								::png_init_io(png_ptr, fp);
								::png_set_sig_bytes(png_ptr, 8);
								::png_read_info(png_ptr, info_ptr);

								const int png_width = ::png_get_image_width(png_ptr, info_ptr);
								const int png_height = ::png_get_image_height(png_ptr, info_ptr);
								png_byte color_type = ::png_get_color_type(png_ptr, info_ptr);

								::png_set_interlace_handling(png_ptr);
								::png_read_update_info(png_ptr, info_ptr);

								//The following codes may longjmp while image_read error.
								png_bytep * row_ptrs = new png_bytep[png_height];
								const std::size_t png_rowbytes = ::png_get_rowbytes(png_ptr, info_ptr);

								pixbuf_.open(png_width, png_height);

								const bool is_alpha_enabled = ((PNG_COLOR_MASK_ALPHA & color_type) != 0);
								pixbuf_.alpha_channel(is_alpha_enabled);

								if(is_alpha_enabled && (png_rowbytes == png_width * sizeof(pixel_rgb_t)))
								{
									for(int i = 0; i < png_height; ++i)
										row_ptrs[i] = reinterpret_cast<png_bytep>(pixbuf_.raw_ptr(i));

									::png_read_image(png_ptr, row_ptrs);
									::png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);

									for (int i = 0; i < png_height; ++i)
									{
										auto p = pixbuf_.raw_ptr(i);
										for (int u = 0; u < png_width; ++u)
										{
											auto t = p[u].u.element.red;
											p[u].u.element.red = p[u].u.element.blue;
											p[u].u.element.blue = t;
										}
									}
								}
								else
								{
									png_byte * png_pixbuf = new png_byte[png_height * png_rowbytes];

									for(int i = 0; i < png_height; ++i)
										row_ptrs[i] = reinterpret_cast<png_bytep>(png_pixbuf + png_rowbytes * i);

									::png_read_image(png_ptr, row_ptrs);
									::png_destroy_read_struct(&png_ptr, &info_ptr, 0);

									std::size_t png_pixel_bytes = png_rowbytes / png_width;

									pixel_rgb_t * rgb_row_ptr = pixbuf_.raw_ptr(0);
									for(int y = 0; y < png_height; ++y)
									{
										png_bytep png_ptr = row_ptrs[y];

										pixel_rgb_t * rgb_end = rgb_row_ptr + png_width;

										if(is_alpha_enabled)
										{
											for(pixel_rgb_t * i = rgb_row_ptr; i < rgb_end; ++i)
											{
												i->u.element.red = png_ptr[0];
												i->u.element.green = png_ptr[1];
												i->u.element.blue = png_ptr[2];
												i->u.element.alpha_channel = png_ptr[3];
												png_ptr += png_pixel_bytes;
											}
										}
										else
										{
											for(pixel_rgb_t * i = rgb_row_ptr; i < rgb_end; ++i)
											{
												i->u.element.red = png_ptr[0];
												i->u.element.green = png_ptr[1];
												i->u.element.blue = png_ptr[2];
												i->u.element.alpha_channel = 255;
												png_ptr += png_pixel_bytes;
											}
										}
										rgb_row_ptr = rgb_end;
									}

									delete [] png_pixbuf;
								}
								delete [] row_ptrs;
								is_opened = true;
							}
						}
					}
				}

				::fclose(fp);
				return is_opened;
			}

			bool alpha_channel() const
			{
				return pixbuf_.alpha_channel();
			}

			virtual bool empty() const
			{
				return pixbuf_.empty();
			}

			virtual void close()
			{
				pixbuf_.close();
			}

			virtual nana::size size() const
			{
				return pixbuf_.size();
			}

			void paste(const nana::rectangle& src_r, graph_reference graph, int x, int y) const
			{
				pixbuf_.paste(src_r, graph.handle(), x, y);
			}

			void stretch(const nana::rectangle& src_r, graph_reference dst, const nana::rectangle& r) const
			{
				pixbuf_.stretch(src_r, dst.handle(), r);
			}
		private:
			nana::paint::pixel_buffer pixbuf_;

		};
	}//end namespace detail
	}//end namespace paint
}//end namespace nana

#endif
