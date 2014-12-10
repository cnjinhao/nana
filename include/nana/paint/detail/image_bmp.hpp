#ifndef NANA_PAINT_DETAIL_IMAGE_BMP_HPP
#define NANA_PAINT_DETAIL_IMAGE_BMP_HPP

#include "image_impl_interface.hpp"
#include <memory>

namespace nana{	namespace paint
{
	namespace detail
	{

#ifndef NANA_WINDOWS
		struct bitmap_file_header
		{
			unsigned short bfType;
			unsigned long bfSize;
			unsigned short bfReserved1;
			unsigned short bfReserved2;
			unsigned long bfOffBits;
		} __attribute__((packed));

		struct bitmap_info_header {
			unsigned long biSize;
			long  biWidth;
			long  biHeight;
			unsigned short  biPlanes;
			unsigned short  biBitCount;
			unsigned long biCompression;
			unsigned long biSizeImage;
			long  biXPelsPerMeter;
			long  biYPelsPerMeter;
			unsigned long biClrUsed;
			unsigned long biClrImportant;
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
			:public image::image_impl_interface
		{
		public:
			image_bmp(){}

			~image_bmp()
			{
				this->close();
			}

			bool open(const nana::char_t* filename)
			{
				if(nullptr == filename) return false;
				std::ifstream ifs;
#if defined(NANA_UNICODE)
				ifs.open(static_cast<std::string>(nana::charset(filename)).c_str(), std::ios::binary);
#else
				ifs.open(filename, std::ios::binary);
#endif
				if(ifs)
				{
					ifs.seekg(0, std::ios::end);
					auto size = ifs.tellg();
					ifs.seekg(0, std::ios::beg);

					if(size <= static_cast<int>(sizeof(bitmap_file_header)))
						return false;

					std::unique_ptr<char[]> buffer(new char[static_cast<int>(size)]);
					
					ifs.read(buffer.get(), size);
					if(size == ifs.gcount())
					{
						bitmap_file_header * header = reinterpret_cast<bitmap_file_header*>(buffer.get());
						if((header->bfType == 0x4D42) && (static_cast<std::streamsize>(header->bfSize) == size))
						{
							unsigned char* bits = reinterpret_cast<unsigned char*>(buffer.get() + header->bfOffBits);
							bitmap_info * info = reinterpret_cast<bitmap_info *>(header + 1);

							//Bitmap file is 4byte-aligned for each line.
							std::size_t bytes_per_line;
							const std::size_t height_pixels = abs(info->bmiHeader.biHeight);
							if(0 == info->bmiHeader.biSizeImage)
								bytes_per_line = (((info->bmiHeader.biWidth * info->bmiHeader.biBitCount + 31) & ~31) >> 3);
							else
								bytes_per_line = info->bmiHeader.biSizeImage / height_pixels;

							pixbuf_.open(info->bmiHeader.biWidth, height_pixels);

							pixel_rgb_t * d = pixbuf_.raw_ptr(0);

							if(16 <= info->bmiHeader.biBitCount)
							{
								pixbuf_.put(bits, info->bmiHeader.biWidth, height_pixels, info->bmiHeader.biBitCount, bytes_per_line, (info->bmiHeader.biHeight < 0));
							}
							else if(8 == info->bmiHeader.biBitCount)
							{
								const pixel_rgb_t * const lend = d + info->bmiHeader.biWidth * height_pixels;

								if(info->bmiHeader.biHeight < 0)
								{
									const unsigned char* s = bits;
									while(d < lend)
									{
										pixel_rgb_t * d_p = d;
										pixel_rgb_t * const dpend = d_p + info->bmiHeader.biWidth;
										const unsigned char * s_p = s;
										while(d_p != dpend)
										{
											rgb_quad & rgb = info->bmiColors[*s_p++];
											d_p->u.element.red = rgb.rgbRed;
											d_p->u.element.green = rgb.rgbGreen;
											d_p->u.element.blue = rgb.rgbBlue;
											d_p->u.element.alpha_channel = rgb.rgbReserved;
											++d_p;
										}
										d = dpend;
										s += bytes_per_line;
									}
								}
								else
								{
									const unsigned char* s = bits + bytes_per_line * (height_pixels - 1);
									while(d < lend)
									{
										pixel_rgb_t * d_p = d;
										pixel_rgb_t * const dpend = d_p + info->bmiHeader.biWidth;
										const unsigned char * s_p = s;
										while(d_p != dpend)
										{
											rgb_quad & rgb = info->bmiColors[*s_p++];
											d_p->u.element.red = rgb.rgbRed;
											d_p->u.element.green = rgb.rgbGreen;
											d_p->u.element.blue = rgb.rgbBlue;
											d_p->u.element.alpha_channel = rgb.rgbReserved;
											++d_p;
										}
										d = dpend;
										s -= bytes_per_line;
									}
								}
							}
							else if(4 == info->bmiHeader.biBitCount)
							{
								const pixel_rgb_t * const lend = d + info->bmiHeader.biWidth * height_pixels;
								if(info->bmiHeader.biHeight < 0)
								{
									const unsigned char* s = bits;
									while(d < lend)
									{
										pixel_rgb_t * d_p = d;
										pixel_rgb_t * const dpend = d_p + info->bmiHeader.biWidth;
										unsigned index = 0;
										while(d_p != dpend)
										{
											rgb_quad & rgb = info->bmiColors[(index & 1) ? (s[index >> 1] & 0xF) : (s[index >> 1] & 0xF0) >> 4];

											d_p->u.element.red = rgb.rgbRed;
											d_p->u.element.green = rgb.rgbGreen;
											d_p->u.element.blue = rgb.rgbBlue;
											d_p->u.element.alpha_channel = rgb.rgbReserved;
											++d_p;
											++index;
										}
										d = dpend;
										s += bytes_per_line;
									}
								}
								else
								{
									const unsigned char* s = bits + bytes_per_line * (height_pixels - 1);
									while(d < lend)
									{
										pixel_rgb_t * d_p = d;
										pixel_rgb_t * const dpend = d_p + info->bmiHeader.biWidth;

										unsigned index = 0;
										while(d_p != dpend)
										{
											rgb_quad & rgb = info->bmiColors[(index & 1) ? (s[index >> 1] & 0xF) : (s[index >> 1] & 0xF0) >> 4];

											d_p->u.element.red = rgb.rgbRed;
											d_p->u.element.green = rgb.rgbGreen;
											d_p->u.element.blue = rgb.rgbBlue;
											d_p->u.element.alpha_channel = rgb.rgbReserved;
											++d_p;
											++index;
										}
										d = dpend;
										s -= bytes_per_line;
									}
								}							
							}
							else if(2 == info->bmiHeader.biBitCount)
							{
								const pixel_rgb_t * const lend = d + info->bmiHeader.biWidth * height_pixels;
								if(info->bmiHeader.biHeight < 0)
								{
									const unsigned char* s = bits;
									while(d < lend)
									{
										pixel_rgb_t * d_p = d;
										pixel_rgb_t * const dpend = d_p + info->bmiHeader.biWidth;
										unsigned index = 0;
										while(d_p != dpend)
										{
											unsigned shift = (3 - (index & 0x3)) << 1; // (index % 4) * 2
											rgb_quad& rgb = info->bmiColors[(s[index >> 2] & (0x3 << shift))>>shift];

											d_p->u.element.red = rgb.rgbRed;
											d_p->u.element.green = rgb.rgbGreen;
											d_p->u.element.blue = rgb.rgbBlue;
											d_p->u.element.alpha_channel = rgb.rgbReserved;
											++d_p;
											++index;
										}
										d = dpend;
										s += bytes_per_line;
									}
								}
								else
								{
									const unsigned char* s = bits + bytes_per_line * (height_pixels - 1);
									while(d < lend)
									{
										pixel_rgb_t * d_p = d;
										pixel_rgb_t * const dpend = d_p + info->bmiHeader.biWidth;

										unsigned index = 0;
										while(d_p != dpend)
										{
											unsigned shift = (3 - (index & 0x3)) << 1; // (index % 4) * 2
											rgb_quad& rgb = info->bmiColors[(s[index >> 2] & (0x3 << shift))>>shift];

											d_p->u.element.red = rgb.rgbRed;
											d_p->u.element.green = rgb.rgbGreen;
											d_p->u.element.blue = rgb.rgbBlue;
											d_p->u.element.alpha_channel = rgb.rgbReserved;
											++d_p;
											++index;
										}
										d = dpend;
										s -= bytes_per_line;
									}
								}							
							}
							else if(1 == info->bmiHeader.biBitCount)
							{
								const pixel_rgb_t * const lend = d + info->bmiHeader.biWidth * height_pixels;
								if(info->bmiHeader.biHeight < 0)
								{
									const unsigned char* s = bits;
									while(d < lend)
									{
										pixel_rgb_t * d_p = d;
										pixel_rgb_t * const dpend = d_p + info->bmiHeader.biWidth;
										unsigned index = 0;
										while(d_p != dpend)
										{
											unsigned bi = (7 - (index & 7));	//(index % 8)
											rgb_quad & rgb = info->bmiColors[(s[index >> 3] & (1 << bi)) >> bi];

											d_p->u.element.red = rgb.rgbRed;
											d_p->u.element.green = rgb.rgbGreen;
											d_p->u.element.blue = rgb.rgbBlue;
											d_p->u.element.alpha_channel = rgb.rgbReserved;
											++d_p;
											++index;
										}
										d = dpend;
										s += bytes_per_line;
									}
								}
								else
								{
									const unsigned char* s = bits + bytes_per_line * (height_pixels - 1);
									while(d < lend)
									{
										pixel_rgb_t * d_p = d;
										pixel_rgb_t * const dpend = d_p + info->bmiHeader.biWidth;

										unsigned index = 0;
										while(d_p != dpend)
										{
											unsigned bi = (7 - (index & 7));
											rgb_quad & rgb = info->bmiColors[(s[index >> 3] & (1 << bi)) >> bi];

											d_p->u.element.red = rgb.rgbRed;
											d_p->u.element.green = rgb.rgbGreen;
											d_p->u.element.blue = rgb.rgbBlue;
											d_p->u.element.alpha_channel = rgb.rgbReserved;
											++d_p;
											++index;
										}
										d = dpend;
										s -= bytes_per_line;
									}
								}							
							}
						}
					}
				}
				return (false == pixbuf_.empty());
			}

			bool alpha_channel() const
			{
				return false;
			}

			bool empty() const
			{
				return pixbuf_.empty();
			}

			void close()
			{
				pixbuf_.close();
			}

			nana::size size() const
			{
				return pixbuf_.size();
			}

			void paste(const nana::rectangle& src_r, graph_reference graph, int x, int y) const
			{
				if(graph && pixbuf_)
					pixbuf_.paste(src_r, graph.handle(), x, y);
			}

			void stretch(const nana::rectangle& src_r, graph_reference graph, const nana::rectangle& r) const
			{
				if(graph && pixbuf_)
					pixbuf_.stretch(src_r, graph.handle(), r);
			}
		private:
			nana::paint::pixel_buffer pixbuf_;
		};//end class bmpfile
	}//end namespace detail
}//end namespace paint
}//end namespace nana

#endif
