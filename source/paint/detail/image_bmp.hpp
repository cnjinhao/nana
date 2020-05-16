/*
 *	Bitmap Format Graphics Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2020 Jinhao(cnjinhao@hotmail.com)
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

#include <memory>
#include <cstring>
#include "image_pixbuf.hpp"

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

		struct bitmap_core_header
		{
			unsigned biSize;
			unsigned short  biWidth;
			unsigned short  biHeight;
			unsigned short  biPlanes;
			unsigned short  biBitCount;
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
#else
		typedef BITMAPFILEHEADER	bitmap_file_header;
		typedef BITMAPCOREHEADER	bitmap_core_header;
		typedef BITMAPINFOHEADER	bitmap_info_header;
		typedef RGBQUAD		rgb_quad;
#endif

		class image_bmp
			:public basic_image_pixbuf
		{
		public:
			~image_bmp()
			{
				this->close();
			}

			bool open(const void* file_data, std::size_t bytes) override
			{
				auto bmp_file = reinterpret_cast<const bitmap_file_header*>(file_data);
				if ((bmp_file->bfType != 0x4D42) || (bmp_file->bfSize != bytes))
					return false;

				auto const header_bytes = *reinterpret_cast<const unsigned long*>(bmp_file + 1);
				
				//There are two kind of base headers. Determinate it by size of header(The first ulong of header).
				//Only Windows Bitmap(BITMAPINFOHEADER) is supported.
				if (sizeof(bitmap_core_header) == header_bytes)
				{
					//The OS/2 BITMAPCOREHEADER is not supported.
					throw std::invalid_argument("BMP with OS/2 BITMAPCOREHEADER is not supported now.");
				}

				auto header = reinterpret_cast<const bitmap_info_header *>(bmp_file + 1);

				const std::size_t bmp_height = std::abs(header->biHeight);

				//Bitmap file is 4byte-aligned for each line.
				auto bytes_per_line = (((header->biWidth * header->biBitCount + 31) & ~31) >> 3);

				pixbuf_.open(header->biWidth, bmp_height);

				auto bits = reinterpret_cast<const unsigned char*>(reinterpret_cast<const char*>(file_data) + bmp_file->bfOffBits);

				if (16 <= header->biBitCount)
					pixbuf_.put(bits, header->biWidth, bmp_height, header->biBitCount, bytes_per_line, (header->biHeight < 0));
				else
					_m_put_with_palette(header, bits, bmp_file->bfSize - bmp_file->bfOffBits,  bytes_per_line);

				return true;
			}

			bool open(const std::filesystem::path& filename) override
			{
				std::ifstream ifs(filename.string(), std::ios::binary);

				auto const bytes = static_cast<unsigned>(std::filesystem::file_size(filename));
				if (ifs && (bytes > static_cast<int>(sizeof(bitmap_file_header))))
				{
					std::unique_ptr<char[]> buffer{ new char[bytes] };

					ifs.read(buffer.get(), bytes);
					if (bytes == static_cast<std::size_t>(ifs.gcount()))
						return open(buffer.get(), bytes);
				}
				return false;
			}

			bool alpha_channel() const override
			{
				return false;
			}
		private:
			std::unique_ptr<unsigned char[]> _m_decompress_rle8(const bitmap_info_header* header, const unsigned char* data, std::size_t data_size)
			{
				std::size_t const lines = std::abs(header->biHeight);
				unsigned char* const indexes = new unsigned char[header->biWidth * lines];
				auto p = indexes;
				auto p_line = p;
				auto const p_end = indexes + header->biWidth * lines;

				std::size_t line_pos = 0;
				auto end = data + data_size;
				while (data != end && p < p_end)
				{
					if (0 == data[0])
					{
						//escape
						if (0 == data[1])
						{
							//eol
							data += 2;
							++line_pos;
						}
						else if (1 == data[1])
						{
							//eof
							data += 2;
							break;
						}
						else if (2 == data[1])
						{
							//delta
							auto x = data[2];
							auto y = data[3];

							// Check if the delta is available
							if ((p + x < p_line + header->biWidth) && (line_pos + y < lines))
							{
								p += y * header->biWidth + x;
								line_pos += y;
							}
							else
								break;

							data += 4;
						}
						else
						{
							//absolute
							std::memcpy(p, data + 2, data[1]);
							p += data[1];

							data += ((data[1] + 1) & 0xFFE) + 2;
						}
					}
					else
					{
						std::memset(p, data[1], data[0]);
						p += data[0];

						data += 2;
					}
				}

				return std::unique_ptr<unsigned char[]>{ indexes };
			}

			void _m_put_with_palette(const bitmap_info_header* header, const unsigned char* bits, std::size_t length, unsigned line_bytes)
			{
				auto const image_height = std::abs(header->biHeight);
				const std::size_t total_pixels = header->biWidth * static_cast<std::size_t>(image_height);

				auto const color_table = reinterpret_cast<const rgb_quad*>(reinterpret_cast<const unsigned char*>(header) + header->biSize);

				auto dst_px = pixbuf_.raw_ptr(0);
				auto const end_dst_px = dst_px + total_pixels;

				int line_pos = image_height - 1;
				int delta = -1;
				if (header->biHeight < 0)
				{
					line_pos = 0;
					delta = 1;
				}

				if (8 == header->biBitCount)
				{
					//decompressed indexes
					std::unique_ptr<unsigned char[]> indexes;

					if (1 == header->biCompression)
					{
						indexes = _m_decompress_rle8(header, bits, length);
						line_bytes = header->biWidth;
						bits = indexes.get();
					}

					while (dst_px < end_dst_px)
					{	
						auto px_indexes = bits + line_bytes * line_pos;
						auto const line_end_dst_px = dst_px + header->biWidth;
						while (dst_px != line_end_dst_px)
						{
							auto & rgb = color_table[*px_indexes++];
							dst_px->element.red = rgb.rgbRed;
							dst_px->element.green = rgb.rgbGreen;
							dst_px->element.blue = rgb.rgbBlue;
							dst_px->element.alpha_channel = rgb.rgbReserved;

							++dst_px;
						}
						line_pos += delta;
					}
				}
				else
				{
					while (dst_px < end_dst_px)
					{
						auto px_indexes = bits + line_bytes * line_pos;
						auto const line_end_dst_px = dst_px + header->biWidth;
						std::size_t pos = 0;
						switch (header->biBitCount)
						{
						case 4:
							while (dst_px != line_end_dst_px)
							{
								auto & rgb = color_table[((pos & 1) ? px_indexes[pos >> 1] : (px_indexes[pos >> 1] >> 4)) & 0xF];
								dst_px->element.red = rgb.rgbRed;
								dst_px->element.green = rgb.rgbGreen;
								dst_px->element.blue = rgb.rgbBlue;
								dst_px->element.alpha_channel = rgb.rgbReserved;
								++dst_px;
								++pos;
							}
							break;
						case 2:
							while (dst_px != line_end_dst_px)
							{
								//auto const shift = ((3 - (pos & 0x3)) << 1); // (index % 4) * 2
								auto& rgb = color_table[(px_indexes[pos >> 2] >> ((3 - (pos & 0x3)) << 1)) & 0x3];
								dst_px->element.red = rgb.rgbRed;
								dst_px->element.green = rgb.rgbGreen;
								dst_px->element.blue = rgb.rgbBlue;
								dst_px->element.alpha_channel = rgb.rgbReserved;
								++dst_px;
								++pos;
							}
							break;
						case 1:
							while (dst_px != line_end_dst_px)
							{
								auto & rgb = color_table[(px_indexes[pos >> 3] >> (7 - (pos & 7))) & 1];

								dst_px->element.red = rgb.rgbRed;
								dst_px->element.green = rgb.rgbGreen;
								dst_px->element.blue = rgb.rgbBlue;
								dst_px->element.alpha_channel = rgb.rgbReserved;
								++dst_px;
								++pos;
							}
							break;
						}
						line_pos += delta;
					}
				}
			}
		};//end class bmpfile
	}//end namespace detail
}//end namespace paint
}//end namespace nana

#endif
