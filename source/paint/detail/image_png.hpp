#ifndef NANA_PAINT_DETAIL_IMAGE_PNG_HPP
#define NANA_PAINT_DETAIL_IMAGE_PNG_HPP

#include "image_pixbuf.hpp"
#include <cstring>

//Separate the libpng from the package that system provides.
#if defined(NANA_LIBPNG)
	#include <nana_extrlib/png.h>
#else
	#include <png.h>
#endif

#include <stdio.h>


namespace nana
{
	namespace paint{	namespace detail{

		class image_png
			: public basic_image_pixbuf
		{
			void _m_read_png(png_structp png_ptr, png_infop info_ptr)
			{
				::png_read_info(png_ptr, info_ptr);

				const int png_width = ::png_get_image_width(png_ptr, info_ptr);
				const int png_height = ::png_get_image_height(png_ptr, info_ptr);
				png_byte color_type = ::png_get_color_type(png_ptr, info_ptr);
				const auto bit_depth = ::png_get_bit_depth(png_ptr, info_ptr);

				pixbuf_.open(png_width, png_height);

				//do some extra work for palette/gray color type
				if (PNG_COLOR_TYPE_PALETTE == color_type)
					::png_set_palette_to_rgb(png_ptr);
				else if ((PNG_COLOR_TYPE_GRAY == color_type) || (PNG_COLOR_TYPE_GRAY_ALPHA == color_type))
					::png_set_gray_to_rgb(png_ptr);

				auto is_alpha_enabled = (::png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS) != 0);
				if (is_alpha_enabled)
					::png_set_tRNS_to_alpha(png_ptr);

				is_alpha_enabled |= ((PNG_COLOR_MASK_ALPHA & color_type) != 0);
				pixbuf_.alpha_channel(is_alpha_enabled);

				//make sure 8-bit per channel
				if (16 == bit_depth)
					::png_set_strip_16(png_ptr);

				::png_set_interlace_handling(png_ptr);
				::png_read_update_info(png_ptr, info_ptr);

				//The following codes may longjmp while image_read error.
				png_bytep * row_ptrs = new png_bytep[png_height];
				const std::size_t png_rowbytes = ::png_get_rowbytes(png_ptr, info_ptr);

				if (is_alpha_enabled && (png_rowbytes == png_width * sizeof(pixel_argb_t)))
				{
					for (int i = 0; i < png_height; ++i)
						row_ptrs[i] = reinterpret_cast<png_bytep>(pixbuf_.raw_ptr(i));

					::png_read_image(png_ptr, row_ptrs);

					if (std::is_same<pixel_argb_t, pixel_color_t>::value)
					{
						for (int i = 0; i < png_height; ++i)
						{
							auto p = pixbuf_.raw_ptr(i);
							for (int u = 0; u < png_width; ++u)
							{
								auto t = p[u].element.red;
								p[u].element.red = p[u].element.blue;
								p[u].element.blue = t;
							}
						}
					}
				}
				else
				{
					png_byte * png_pixbuf = new png_byte[png_height * png_rowbytes];

					for (int i = 0; i < png_height; ++i)
						row_ptrs[i] = reinterpret_cast<png_bytep>(png_pixbuf + png_rowbytes * i);

					::png_read_image(png_ptr, row_ptrs);

					std::size_t png_pixel_bytes = png_rowbytes / png_width;

					pixel_argb_t * rgb_row_ptr = pixbuf_.raw_ptr(0);
					for (int y = 0; y < png_height; ++y)
					{
						png_bytep png_ptr = row_ptrs[y];

						pixel_argb_t * rgb_end = rgb_row_ptr + png_width;

						if (is_alpha_enabled)
						{
							for (pixel_argb_t * i = rgb_row_ptr; i < rgb_end; ++i)
							{
								i->element.red = png_ptr[0];
								i->element.green = png_ptr[1];
								i->element.blue = png_ptr[2];
								i->element.alpha_channel = png_ptr[3];
								png_ptr += png_pixel_bytes;
							}
						}
						else
						{
							for (pixel_argb_t * i = rgb_row_ptr; i < rgb_end; ++i)
							{
								i->element.red = png_ptr[0];
								i->element.green = png_ptr[1];
								i->element.blue = png_ptr[2];
								i->element.alpha_channel = 255;
								png_ptr += png_pixel_bytes;
							}
						}
						rgb_row_ptr = rgb_end;
					}

					delete[] png_pixbuf;
				}
				delete[] row_ptrs;
			}
		public:
			bool open(const std::filesystem::path& png_file) override
			{
				auto fp = ::fopen(to_osmbstr(to_utf8(png_file.native())).c_str(), "rb");
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

								//8-byte of sig has been read, tell the libpng there are some bytes missing from start of file
								::png_set_sig_bytes(png_ptr, 8);

								_m_read_png(png_ptr, info_ptr);

								is_opened = true;
							}
						}

						::png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
					}
				}

				::fclose(fp);
				return is_opened;
			}

			class png_reader
			{
			public:
				png_reader(const void* data, std::size_t bytes) noexcept
					: data_ptr_(reinterpret_cast<const char*>(data)), bytes_(bytes)
				{
				}

				static void PNGCAPI read(png_structp png_ptr, png_bytep buf, png_size_t bytes)
				{
					auto self = reinterpret_cast<png_reader*>(::png_get_io_ptr(png_ptr));

					auto read_bytes = self->bytes_ < bytes ? self->bytes_ : bytes;

					if (read_bytes)
						std::memcpy(buf, self->data_ptr_, read_bytes);

					self->bytes_ -= read_bytes;
					self->data_ptr_ += read_bytes;
				}
			private:
				const char* data_ptr_;
				std::size_t bytes_;
			};

			bool open(const void* data, std::size_t bytes) override
			{
				if (bytes < 8 || 0 != ::png_sig_cmp(reinterpret_cast<png_const_bytep>(data), 0, 8))
					return false;

				auto png_ptr = ::png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
				if (!png_ptr)
					return false;
				
				bool is_opened = false;

				png_infop info_ptr = ::png_create_info_struct(png_ptr);

				if (info_ptr)
				{
					png_reader reader{ data, bytes };

					if (!setjmp(png_jmpbuf(png_ptr)))
					{
						::png_set_read_fn(png_ptr, &reader, &png_reader::read);

						_m_read_png(png_ptr, info_ptr);
						is_opened = true;
					}
				}

				::png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
				
				return is_opened;
			}
		};
	}//end namespace detail
	}//end namespace paint
}//end namespace nana

#endif
