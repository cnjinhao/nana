/*
 *	Bitmap Format Graphics Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/paint/detail/image_bmp.hpp
 *	@contributors: Ryan Gonzalez
 */
#ifndef NANA_PAINT_DETAIL_IMAGE_BMP_HPP
#define NANA_PAINT_DETAIL_IMAGE_BMP_HPP

#include "image_pixbuf.hpp"
#include <memory>

namespace nana{	namespace paint
{
	namespace detail
	{
#ifndef NANA_WINDOWS
		struct bitmap_file_header
		{
			unsigned short bfType;
			unsigned bfSize;
			unsigned short bfReserved1;
			unsigned short bfReserved2;
			unsigned bfOffBits;
		} __attribute__((packed));

		struct bitmap_info_header {
			unsigned biSize;
			int  biWidth;
			int  biHeight;
			unsigned short  biPlanes;
			unsigned short  biBitCount;
			unsigned		biCompression;
			unsigned		biSizeImage;
			int  biXPelsPerMeter;
			int  biYPelsPerMeter;
			unsigned	biClrUsed;
			unsigned	biClrImportant;
		}__attribute__((packed));

		struct rgb_quad
		{
			unsigned char rgbBlue;
			unsigned char rgbGreen;
			unsigned char rgbRed;
			unsigned char rgbReserved;
		};

		struct bitmap_info
		{
			bitmap_info_header bmiHeader;
			rgb_quad	bmiColors[1];
		}__attribute__((packed));
#else
		typedef BITMAPFILEHEADER bitmap_file_header;
		typedef BITMAPINFO	bitmap_info;
		typedef RGBQUAD		rgb_quad;
#endif

		class image_bmp
			:public basic_image_pixbuf
		{
		public:
			image_bmp(){}

			~image_bmp()
			{
				this->close();
			}

			bool open(const void* data, std::size_t bytes) override
			{
				auto bmp_data = reinterpret_cast<const char*>(data);

				auto header = reinterpret_cast<const bitmap_file_header*>(bmp_data);
				if ((header->bfType != 0x4D42) || (header->bfSize != bytes))
					return false;

				auto bits = reinterpret_cast<const unsigned char*>(bmp_data + header->bfOffBits);
				auto info = reinterpret_cast<const bitmap_info *>(header + 1);

				//Bitmap file is 4byte-aligned for each line.
				std::size_t bytes_per_line;
				const std::size_t height_pixels = std::abs(info->bmiHeader.biHeight);
				if (0 == info->bmiHeader.biSizeImage)
					bytes_per_line = (((info->bmiHeader.biWidth * info->bmiHeader.biBitCount + 31) & ~31) >> 3);
				else
					bytes_per_line = info->bmiHeader.biSizeImage / height_pixels;

				pixbuf_.open(info->bmiHeader.biWidth, height_pixels);

				auto d = pixbuf_.raw_ptr(0);

				if (16 <= info->bmiHeader.biBitCount)
				{
					pixbuf_.put(bits, info->bmiHeader.biWidth, height_pixels, info->bmiHeader.biBitCount, bytes_per_line, (info->bmiHeader.biHeight < 0));
				}
				else if (8 == info->bmiHeader.biBitCount)
				{
					const auto lend = d + info->bmiHeader.biWidth * height_pixels;

					if (info->bmiHeader.biHeight < 0)
					{
						auto s = bits;
						while (d < lend)
						{
							auto d_p = d;
							auto dpend = d_p + info->bmiHeader.biWidth;
							auto s_p = s;
							while (d_p != dpend)
							{
								auto & rgb = info->bmiColors[*s_p++];
								d_p->element.red = rgb.rgbRed;
								d_p->element.green = rgb.rgbGreen;
								d_p->element.blue = rgb.rgbBlue;
								d_p->element.alpha_channel = rgb.rgbReserved;
								++d_p;
							}
							d = dpend;
							s += bytes_per_line;
						}
					}
					else
					{
						const auto* s = bits + bytes_per_line * (height_pixels - 1);
						while (d < lend)
						{
							auto d_p = d;
							auto* const dpend = d_p + info->bmiHeader.biWidth;
							const auto * s_p = s;
							while (d_p != dpend)
							{
								auto & rgb = info->bmiColors[*s_p++];
								d_p->element.red = rgb.rgbRed;
								d_p->element.green = rgb.rgbGreen;
								d_p->element.blue = rgb.rgbBlue;
								d_p->element.alpha_channel = rgb.rgbReserved;
								++d_p;
							}
							d = dpend;
							s -= bytes_per_line;
						}
					}
				}
				else if (4 == info->bmiHeader.biBitCount)
				{
					const auto * const lend = d + info->bmiHeader.biWidth * height_pixels;
					if (info->bmiHeader.biHeight < 0)
					{
						const unsigned char* s = bits;
						while (d < lend)
						{
							auto d_p = d;
							auto * const dpend = d_p + info->bmiHeader.biWidth;
							unsigned index = 0;
							while (d_p != dpend)
							{
								auto & rgb = info->bmiColors[(index & 1) ? (s[index >> 1] & 0xF) : (s[index >> 1] & 0xF0) >> 4];

								d_p->element.red = rgb.rgbRed;
								d_p->element.green = rgb.rgbGreen;
								d_p->element.blue = rgb.rgbBlue;
								d_p->element.alpha_channel = rgb.rgbReserved;
								++d_p;
								++index;
							}
							d = dpend;
							s += bytes_per_line;
						}
					}
					else
					{
						const auto* s = bits + bytes_per_line * (height_pixels - 1);
						while (d < lend)
						{
							auto d_p = d;
							auto * const dpend = d_p + info->bmiHeader.biWidth;

							unsigned index = 0;
							while (d_p != dpend)
							{
								auto & rgb = info->bmiColors[(index & 1) ? (s[index >> 1] & 0xF) : (s[index >> 1] & 0xF0) >> 4];

								d_p->element.red = rgb.rgbRed;
								d_p->element.green = rgb.rgbGreen;
								d_p->element.blue = rgb.rgbBlue;
								d_p->element.alpha_channel = rgb.rgbReserved;
								++d_p;
								++index;
							}
							d = dpend;
							s -= bytes_per_line;
						}
					}
				}
				else if (2 == info->bmiHeader.biBitCount)
				{
					const auto * const lend = d + info->bmiHeader.biWidth * height_pixels;
					if (info->bmiHeader.biHeight < 0)
					{
						const unsigned char* s = bits;
						while (d < lend)
						{
							auto d_p = d;
							auto * const dpend = d_p + info->bmiHeader.biWidth;
							unsigned index = 0;
							while (d_p != dpend)
							{
								unsigned shift = (3 - (index & 0x3)) << 1; // (index % 4) * 2
								auto& rgb = info->bmiColors[(s[index >> 2] & (0x3 << shift)) >> shift];

								d_p->element.red = rgb.rgbRed;
								d_p->element.green = rgb.rgbGreen;
								d_p->element.blue = rgb.rgbBlue;
								d_p->element.alpha_channel = rgb.rgbReserved;
								++d_p;
								++index;
							}
							d = dpend;
							s += bytes_per_line;
						}
					}
					else
					{
						const auto* s = bits + bytes_per_line * (height_pixels - 1);
						while (d < lend)
						{
							auto d_p = d;
							auto * const dpend = d_p + info->bmiHeader.biWidth;

							unsigned index = 0;
							while (d_p != dpend)
							{
								unsigned shift = (3 - (index & 0x3)) << 1; // (index % 4) * 2
								auto& rgb = info->bmiColors[(s[index >> 2] & (0x3 << shift)) >> shift];

								d_p->element.red = rgb.rgbRed;
								d_p->element.green = rgb.rgbGreen;
								d_p->element.blue = rgb.rgbBlue;
								d_p->element.alpha_channel = rgb.rgbReserved;
								++d_p;
								++index;
							}
							d = dpend;
							s -= bytes_per_line;
						}
					}
				}
				else if (1 == info->bmiHeader.biBitCount)
				{
					const auto * const lend = d + info->bmiHeader.biWidth * height_pixels;
					if (info->bmiHeader.biHeight < 0)
					{
						const auto* s = bits;
						while (d < lend)
						{
							auto d_p = d;
							auto * const dpend = d_p + info->bmiHeader.biWidth;
							unsigned index = 0;
							while (d_p != dpend)
							{
								unsigned bi = (7 - (index & 7));	//(index % 8)
								auto & rgb = info->bmiColors[(s[index >> 3] & (1 << bi)) >> bi];

								d_p->element.red = rgb.rgbRed;
								d_p->element.green = rgb.rgbGreen;
								d_p->element.blue = rgb.rgbBlue;
								d_p->element.alpha_channel = rgb.rgbReserved;
								++d_p;
								++index;
							}
							d = dpend;
							s += bytes_per_line;
						}
					}
					else
					{
						const auto* s = bits + bytes_per_line * (height_pixels - 1);
						while (d < lend)
						{
							auto d_p = d;
							auto * const dpend = d_p + info->bmiHeader.biWidth;

							unsigned index = 0;
							while (d_p != dpend)
							{
								unsigned bi = (7 - (index & 7));
								auto & rgb = info->bmiColors[(s[index >> 3] & (1 << bi)) >> bi];

								d_p->element.red = rgb.rgbRed;
								d_p->element.green = rgb.rgbGreen;
								d_p->element.blue = rgb.rgbBlue;
								d_p->element.alpha_channel = rgb.rgbReserved;
								++d_p;
								++index;
							}
							d = dpend;
							s -= bytes_per_line;
						}
					}
				}

				return true;
			}

			bool open(const nana::experimental::filesystem::path& filename) override
			{
				std::ifstream ifs(filename.string(), std::ios::binary);
				if(ifs)
				{
					ifs.seekg(0, std::ios::end);
					auto size = static_cast<std::size_t>(ifs.tellg());
					ifs.seekg(0, std::ios::beg);

					if(size <= static_cast<int>(sizeof(bitmap_file_header)))
						return false;

					std::unique_ptr<char[]> buffer(new char[static_cast<int>(size)]);

					ifs.read(buffer.get(), size);
					if (size == static_cast<std::size_t>(ifs.gcount()))
						return open(buffer.get(), size);
				}
				return false;
			}

			bool alpha_channel() const override
			{
				return false;
			}
		};//end class bmpfile
	}//end namespace detail
}//end namespace paint
}//end namespace nana

#endif
