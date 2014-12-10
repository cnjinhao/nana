/*
 *	Image Processor Algorithm Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2014 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/paint/detail/image_processor.hpp
 *	@brief: This header file implements the algorithms of image processor
 *
 *	DON'T INCLUDE THIS HEADER FILE DIRECTLY TO YOUR SOURCE FILE.
 */

#ifndef NANA_PAINT_DETAIL_IMAGE_PROCESSOR_HPP
#define NANA_PAINT_DETAIL_IMAGE_PROCESSOR_HPP
#include "../image_process_interface.hpp"
#include <nana/paint/pixel_buffer.hpp>
#include <nana/paint/detail/native_paint_interface.hpp>
#include <algorithm>

namespace nana
{
namespace paint
{
namespace detail
{
	namespace algorithms
	{
		class proximal_interoplation
			: public image_process::stretch_interface
		{
			void process(const paint::pixel_buffer& s_pixbuf, const nana::rectangle& r_src, paint::pixel_buffer & pixbuf, const nana::rectangle& r_dst) const
			{
				const auto bytes_per_line = s_pixbuf.bytes_per_line();

				double rate_x = double(r_src.width) / r_dst.width;
				double rate_y = double(r_src.height) / r_dst.height;

				pixel_rgb_t * s_raw_pixbuf = s_pixbuf.raw_ptr(0);

				if(s_pixbuf.alpha_channel())
				{
					for(std::size_t row = 0; row < r_dst.height; ++row)
					{
						const pixel_rgb_t * s_line = pixel_at(s_raw_pixbuf, (static_cast<int>(row * rate_y) + r_src.y) * bytes_per_line);
						pixel_rgb_t * i = pixbuf.raw_ptr(r_dst.y + row);

						for(std::size_t x = 0; x < r_dst.width; ++x, ++i)
						{
							const pixel_rgb_t * s = s_line + static_cast<int>(x * rate_x) + r_src.x;
							if(0 == s->u.element.alpha_channel)
								continue;
							
							if(s->u.element.alpha_channel != 255)
							{
								i->u.element.red = unsigned(i->u.element.red * (255 - s->u.element.alpha_channel) + s->u.element.red * s->u.element.alpha_channel) / 255;
								i->u.element.green = unsigned(i->u.element.green * (255 - s->u.element.alpha_channel) + s->u.element.green * s->u.element.alpha_channel) / 255;
								i->u.element.blue = unsigned(i->u.element.blue * (255 - s->u.element.alpha_channel) + s->u.element.blue * s->u.element.alpha_channel) / 255;
							}
							else
							{
								unsigned alpha_chn = i->u.element.alpha_channel;
								*i = *s;
								i->u.element.alpha_channel = alpha_chn;
							}
						}
					}				
				}
				else
				{
					for(std::size_t row = 0; row < r_dst.height; ++row)
					{
						const pixel_rgb_t * s_line = pixel_at(s_raw_pixbuf, (static_cast<int>(row * rate_y) + r_src.y) * bytes_per_line);
						pixel_rgb_t * i = pixbuf.raw_ptr(r_dst.y + row);

						for(std::size_t x = 0; x < r_dst.width; ++x, ++i)
							*i = s_line[static_cast<int>(x * rate_x) + r_src.x];
					}
				}
			}
		};

		class bilinear_interoplation
			: public image_process::stretch_interface
		{
			struct x_u_table_tag
			{
				int x;
				int iu;
				int iu_minus_coef;
			};

			void process(const paint::pixel_buffer & s_pixbuf, const nana::rectangle& r_src, paint::pixel_buffer & pixbuf, const nana::rectangle& r_dst) const
			{
				const auto s_bytes_per_line = s_pixbuf.bytes_per_line();

				const int shift_size = 8;
				const std::size_t coef = 1 << shift_size;
				const int double_shift_size = shift_size << 1;

				double rate_x = double(r_src.width) / r_dst.width;
				double rate_y = double(r_src.height) / r_dst.height;
				
				const int right_bound = static_cast<int>(r_src.width) - 1 + r_src.x;

				const nana::pixel_rgb_t * s_raw_pixel_buffer = s_pixbuf.raw_ptr(0);

				const int bottom = r_src.y + static_cast<int>(r_src.height - 1);

				x_u_table_tag * x_u_table = new x_u_table_tag[r_dst.width];

				for(std::size_t x = 0; x < r_dst.width; ++x)
				{
					double u = (int(x) + 0.5) * rate_x - 0.5;
					x_u_table_tag el;
					el.x = r_src.x;
					if(u < 0)
					{
						u = 0;
					}
					else
					{
						int ipart = static_cast<int>(u);
						el.x += ipart;
						u -= ipart;
					}
					el.iu = static_cast<int>(u * coef);
					el.iu_minus_coef = coef - el.iu;
					x_u_table[x] = el;
				}

				const bool is_alpha_channel = s_pixbuf.alpha_channel();
				
				for(std::size_t row = 0; row < r_dst.height; ++row)
				{
					double v = (int(row) + 0.5) * rate_y - 0.5;
					int sy = r_src.y;
					if(v < 0)
					{
						v = 0;
					}
					else
					{
						int ipart = static_cast<int>(v);
						sy += ipart;
						v -= ipart;
					}

					std::size_t iv = static_cast<size_t>(v * coef);
					const std::size_t iv_minus_coef = coef - iv;

					const nana::pixel_rgb_t * s_line = pixel_at(s_raw_pixel_buffer,  sy * s_bytes_per_line);
					const nana::pixel_rgb_t * next_s_line = pixel_at(s_line, (sy < bottom ? s_bytes_per_line : 0));

					nana::pixel_rgb_t col0;
					nana::pixel_rgb_t col1;
					nana::pixel_rgb_t col2;
					nana::pixel_rgb_t col3;
					
					pixel_rgb_t * i = pixbuf.raw_ptr(row + r_dst.y) + r_dst.x;
					
					if(is_alpha_channel)
					{
						for(std::size_t x = 0; x < r_dst.width; ++x, ++i)
						{
							x_u_table_tag el = x_u_table[x];
						
							col0 = s_line[el.x];
							col1 = next_s_line[el.x];

							if(el.x < right_bound)
							{
								col2 = s_line[el.x + 1];
								col3 = next_s_line[el.x + 1];
							}
							else
							{
								col2 = col0;
								col3 = col1;
							}
						
							std::size_t coef0 = el.iu_minus_coef * iv_minus_coef;
							std::size_t coef1 = el.iu_minus_coef * iv;
							std::size_t coef2 = el.iu * iv_minus_coef;
							std::size_t coef3 = el.iu * iv;			

							unsigned alpha_chn = static_cast<unsigned>((coef0 * col0.u.element.alpha_channel + coef1 * col1.u.element.alpha_channel + (coef2 * col2.u.element.alpha_channel + coef3 * col3.u.element.alpha_channel)) >> double_shift_size);
							unsigned s_red = static_cast<unsigned>((coef0 * col0.u.element.red + coef1 * col1.u.element.red + (coef2 * col2.u.element.red + coef3 * col3.u.element.red)) >> double_shift_size);
							unsigned s_green = static_cast<unsigned>((coef0 * col0.u.element.green + coef1 * col1.u.element.green + (coef2 * col2.u.element.green + coef3 * col3.u.element.green)) >> double_shift_size);
							unsigned s_blue = static_cast<unsigned>((coef0 * col0.u.element.blue + coef1 * col1.u.element.blue + (coef2 * col2.u.element.blue + coef3 * col3.u.element.blue)) >> double_shift_size);

							if(alpha_chn)
							{
								if(alpha_chn != 255)
								{
									i->u.element.red	= unsigned(i->u.element.red * (255 - alpha_chn) + s_red * alpha_chn) / 255;
									i->u.element.green	= unsigned(i->u.element.green * (255 - alpha_chn) + s_green * alpha_chn) / 255;
									i->u.element.blue	= unsigned(i->u.element.blue * (255 - alpha_chn) + s_blue * alpha_chn) / 255;
								}
								else
								{
									i->u.element.red = s_red;
									i->u.element.green = s_green;
									i->u.element.blue = s_blue;
								}
							}
						}						
					}
					else
					{
						for(std::size_t x = 0; x < r_dst.width; ++x, ++i)
						{
							x_u_table_tag el = x_u_table[x];
						
							col0 = s_line[el.x];
							col1 = next_s_line[el.x];

							if(el.x < right_bound)
							{
								col2 = s_line[el.x + 1];
								col3 = next_s_line[el.x + 1];
							}
							else
							{
								col2 = col0;
								col3 = col1;
							}
						
							std::size_t coef0 = el.iu_minus_coef * iv_minus_coef;
							std::size_t coef1 = el.iu_minus_coef * iv;
							std::size_t coef2 = el.iu * iv_minus_coef;
							std::size_t coef3 = el.iu * iv;			

							i->u.element.red = static_cast<unsigned>((coef0 * col0.u.element.red + coef1 * col1.u.element.red + (coef2 * col2.u.element.red + coef3 * col3.u.element.red)) >> double_shift_size);
							i->u.element.green = static_cast<unsigned>((coef0 * col0.u.element.green + coef1 * col1.u.element.green + (coef2 * col2.u.element.green + coef3 * col3.u.element.green)) >> double_shift_size);
							i->u.element.blue = static_cast<unsigned>((coef0 * col0.u.element.blue + coef1 * col1.u.element.blue + (coef2 * col2.u.element.blue + coef3 * col3.u.element.blue)) >> double_shift_size);
						}
					}
				}
				delete [] x_u_table;
			}
		};

		//alpha_blend
		class alpha_blend
			: public image_process::alpha_blend_interface
		{
			//process
			virtual void process(const paint::pixel_buffer& s_pixbuf, const nana::rectangle& s_r, paint::pixel_buffer& d_pixbuf, const nana::point& d_pos) const
			{
				nana::pixel_rgb_t * d_rgb = d_pixbuf.at(d_pos);
				nana::pixel_rgb_t * s_rgb = s_pixbuf.raw_ptr(s_r.y) + s_r.x;
				if(d_rgb && s_rgb)
				{
					const unsigned rest = s_r.width & 0x3;
					const unsigned length_align4 = s_r.width - rest;

					std::size_t d_step_bytes = d_pixbuf.bytes_per_line() - (s_r.width - rest) * sizeof(pixel_rgb_t);
					std::size_t s_step_bytes = s_pixbuf.bytes_per_line() - (s_r.width - rest) * sizeof(pixel_rgb_t);
					for(unsigned line = 0; line < s_r.height; ++line)
					{
						pixel_rgb_t* end = d_rgb + length_align4;
						for(; d_rgb < end; d_rgb += 4, s_rgb += 4)
						{
							//0
							if(s_rgb->u.element.alpha_channel)
							{
								if(s_rgb->u.element.alpha_channel != 255)
								{
									d_rgb->u.element.red = unsigned(d_rgb->u.element.red * (255 - s_rgb[0].u.element.alpha_channel) + s_rgb[0].u.element.red * s_rgb[0].u.element.alpha_channel) / 255;
									d_rgb->u.element.green = unsigned(d_rgb->u.element.green * (255 - s_rgb[0].u.element.alpha_channel) + s_rgb[0].u.element.green * s_rgb[0].u.element.alpha_channel) / 255;
									d_rgb->u.element.blue = unsigned(d_rgb->u.element.blue * (255 - s_rgb[0].u.element.alpha_channel) + s_rgb[0].u.element.blue * s_rgb[0].u.element.alpha_channel) / 255;
								}
								else
									*d_rgb = *s_rgb;
							}

							//1
							if(s_rgb[1].u.element.alpha_channel)
							{
								if(s_rgb[1].u.element.alpha_channel != 255)
								{
									d_rgb[1].u.element.red = unsigned(d_rgb[1].u.element.red * (255 - s_rgb[1].u.element.alpha_channel) + s_rgb[1].u.element.red * s_rgb[1].u.element.alpha_channel) / 255;
									d_rgb[1].u.element.green = unsigned(d_rgb[1].u.element.green * (255 - s_rgb[1].u.element.alpha_channel) + s_rgb[1].u.element.green * s_rgb[1].u.element.alpha_channel) / 255;
									d_rgb[1].u.element.blue = unsigned(d_rgb[1].u.element.blue * (255 - s_rgb[1].u.element.alpha_channel) + s_rgb[1].u.element.blue * s_rgb[1].u.element.alpha_channel) / 255;
								}
								else
									d_rgb[1] = s_rgb[1];
							}

							//2
							if(s_rgb[2].u.element.alpha_channel)
							{
								if(s_rgb[2].u.element.alpha_channel != 255)
								{
									d_rgb[2].u.element.red = unsigned(d_rgb[2].u.element.red * (255 - s_rgb[2].u.element.alpha_channel) + s_rgb[2].u.element.red * s_rgb[2].u.element.alpha_channel) / 255;
									d_rgb[2].u.element.green = unsigned(d_rgb[2].u.element.green * (255 - s_rgb[2].u.element.alpha_channel) + s_rgb[2].u.element.green * s_rgb[2].u.element.alpha_channel) / 255;
									d_rgb[2].u.element.blue = unsigned(d_rgb[2].u.element.blue * (255 - s_rgb[2].u.element.alpha_channel) + s_rgb[2].u.element.blue * s_rgb[2].u.element.alpha_channel) / 255;
								}
								else
									d_rgb[2] = s_rgb[2];
							}

							//3
							if(s_rgb[3].u.element.alpha_channel)
							{
								if(s_rgb[3].u.element.alpha_channel != 255)
								{
									d_rgb[3].u.element.red = unsigned(d_rgb[3].u.element.red * (255 - s_rgb[3].u.element.alpha_channel) + s_rgb[3].u.element.red * s_rgb[3].u.element.alpha_channel) / 255;
									d_rgb[3].u.element.green = unsigned(d_rgb[3].u.element.green * (255 - s_rgb[3].u.element.alpha_channel) + s_rgb[3].u.element.green * s_rgb[3].u.element.alpha_channel) / 255;
									d_rgb[3].u.element.blue = unsigned(d_rgb[3].u.element.blue * (255 - s_rgb[3].u.element.alpha_channel) + s_rgb[3].u.element.blue * s_rgb[3].u.element.alpha_channel) / 255;
								}
								else
									d_rgb[3] = s_rgb[3];
							}
						}

						const pixel_rgb_t * s_end = s_rgb + rest;
						for(auto i = s_rgb; i != s_end; ++i)
						{
							if(i->u.element.alpha_channel)
							{
								if(i->u.element.alpha_channel != 255)
								{
									d_rgb[3].u.element.red = unsigned(d_rgb[3].u.element.red * (255 - i->u.element.alpha_channel) + i->u.element.red * i->u.element.alpha_channel) / 255;
									d_rgb[3].u.element.green = unsigned(d_rgb[3].u.element.green * (255 - i->u.element.alpha_channel) + i->u.element.green * i->u.element.alpha_channel) / 255;
									d_rgb[3].u.element.blue = unsigned(d_rgb[3].u.element.blue * (255 - i->u.element.alpha_channel) + i->u.element.blue * i->u.element.alpha_channel) / 255;
								}
								else
									d_rgb[3] = *i;
							}
						}
						d_rgb = pixel_at(d_rgb, d_step_bytes);
						s_rgb = pixel_at(s_rgb, s_step_bytes);
					}
				}
			}
		
		};

		//blend
		class blend
			: public image_process::blend_interface
		{
			//process
			virtual void process(const paint::pixel_buffer& s_pixbuf, const nana::rectangle& s_r, paint::pixel_buffer& d_pixbuf, const nana::point& d_pos, double fade_rate) const
			{
				nana::pixel_rgb_t * d_rgb = d_pixbuf.raw_ptr(d_pos.y) + d_pos.x;
				nana::pixel_rgb_t * s_rgb = s_pixbuf.raw_ptr(s_r.y) + s_r.x;

				if(d_rgb && s_rgb)
				{
					unsigned char* tablebuf = detail::alloc_fade_table(fade_rate);//new unsigned char[0x100 * 2];
					unsigned char* d_table = tablebuf;
					unsigned char* s_table = d_table + 0x100;

					const unsigned rest = s_r.width & 0x3;
					const unsigned length_align4 = s_r.width - rest;

					std::size_t d_step_bytes = d_pixbuf.bytes_per_line() - (s_r.width - rest) * sizeof(pixel_rgb_t);
					std::size_t s_step_bytes = s_pixbuf.bytes_per_line() - (s_r.width - rest) * sizeof(pixel_rgb_t);
					for(unsigned line = 0; line < s_r.height; ++line)
					{
						pixel_rgb_t* end = d_rgb + length_align4;
						for(; d_rgb < end; d_rgb += 4, s_rgb += 4)
						{
							//0
							d_rgb[0].u.element.red = unsigned(d_table[d_rgb[0].u.element.red] + s_table[s_rgb[0].u.element.red]);
							d_rgb[0].u.element.green = unsigned(d_table[d_rgb[0].u.element.green] + s_table[s_rgb[0].u.element.green]);
							d_rgb[0].u.element.blue = unsigned(d_table[d_rgb[0].u.element.blue] + s_table[s_rgb[0].u.element.blue]);

							//1
							d_rgb[1].u.element.red = unsigned(d_table[d_rgb[1].u.element.red] + s_table[s_rgb[1].u.element.red]);
							d_rgb[1].u.element.green = unsigned(d_table[d_rgb[1].u.element.green] + s_table[s_rgb[1].u.element.green]);
							d_rgb[1].u.element.blue = unsigned(d_table[d_rgb[1].u.element.blue] + s_table[s_rgb[1].u.element.blue]);

							//2
							d_rgb[2].u.element.red = unsigned(d_table[d_rgb[2].u.element.red] + s_table[s_rgb[2].u.element.red]);
							d_rgb[2].u.element.green = unsigned(d_table[d_rgb[2].u.element.green] + s_table[s_rgb[2].u.element.green]);
							d_rgb[2].u.element.blue = unsigned(d_table[d_rgb[2].u.element.blue] + s_table[s_rgb[2].u.element.blue]);

							//3
							d_rgb[3].u.element.red = unsigned(d_table[d_rgb[3].u.element.red] + s_table[s_rgb[3].u.element.red]);
							d_rgb[3].u.element.green = unsigned(d_table[d_rgb[3].u.element.green] + s_table[s_rgb[3].u.element.green]);
							d_rgb[3].u.element.blue = unsigned(d_table[d_rgb[3].u.element.blue] + s_table[s_rgb[3].u.element.blue]);
						}

						for(unsigned i = 0; i < rest; ++i)
						{
							d_rgb[i].u.element.red = unsigned(d_table[d_rgb[i].u.element.red] + s_table[s_rgb[i].u.element.red]);
							d_rgb[i].u.element.green = unsigned(d_table[d_rgb[i].u.element.green] + s_table[s_rgb[i].u.element.green]);
							d_rgb[i].u.element.blue = unsigned(d_table[d_rgb[i].u.element.blue] + s_table[s_rgb[i].u.element.blue]);
						}
						d_rgb = pixel_at(d_rgb, d_step_bytes);
						s_rgb = pixel_at(s_rgb, s_step_bytes);
					}
					detail::free_fade_table(tablebuf);
				}
			}
		};

		//class line
		class bresenham_line
			: public image_process::line_interface
		{
			virtual void process(paint::pixel_buffer & pixbuf, const nana::point& pos_beg, const nana::point& pos_end, nana::color_t color, double fade_rate) const
			{
				const std::size_t bytes_pl = pixbuf.bytes_per_line();
				unsigned char * fade_table = nullptr;
				nana::pixel_rgb_t rgb_imd;
				if(fade_rate != 0.0)
				{
					fade_table = detail::alloc_fade_table(1 - fade_rate);
					rgb_imd.u.color = color;
					rgb_imd = detail::fade_color_intermedia(rgb_imd, fade_table);
				}

				nana::pixel_rgb_t * i = pixel_at(pixbuf.raw_ptr(0), pos_beg.y * bytes_pl) + pos_beg.x;

				auto delta = pos_end - pos_beg;
				
				int step_bytes;
				if(delta.y < 0)
				{
					delta.y = -delta.y;
					step_bytes = -static_cast<int>(bytes_pl);
				}
				else
					step_bytes = static_cast<int>(bytes_pl);

				if(delta.x == delta.y)
				{
					step_bytes += sizeof(pixel_rgb_t);
					++delta.x;

					if(fade_table)
					{
						for(int x = 0; x < delta.x; ++x)
						{
							*i = detail::fade_color_by_intermedia(*i, rgb_imd, fade_table);
							i = pixel_at(i, step_bytes);
						}
					}
					else
					{
						for(int x = 0; x < delta.x; ++x)
						{
							i->u.color = color;
							i = pixel_at(i, step_bytes);
						}			
					}
				}
				else
				{
					int dx_2 = delta.x << 1;
					int dy_2 = delta.y << 1;
					if(delta.x > delta.y)
					{
						int error = dy_2 - delta.x;
						++delta.x;						//Include the end poing

						if(fade_table)
						{
							for(int x = 0; x < delta.x; ++x)
							{
								*i = detail::fade_color_by_intermedia(*i, rgb_imd, fade_table);
								if(error >= 0)
								{
									error -= dx_2;
									i = pixel_at(i, step_bytes);
								}
								error += dy_2;
								++i;
							}		
						}
						else
						{
							for(int x = 0; x < delta.x; ++x)
							{
								i->u.color = color;
								if(error >= 0)
								{
									error -= dx_2;
									i = pixel_at(i, step_bytes);
								}
								error += dy_2;
								++i;
							}
						}
					}
					else
					{
						int error = dx_2 - delta.y;
						++delta.y;						//Include the end point

						if(fade_table)
						{
							for (int y = 0; y < delta.y; ++y)
							{
								*i = detail::fade_color_by_intermedia(*i, rgb_imd, fade_table);
								if(error >= 0)
								{
									error -= dy_2;
									++i;
								}
								error += dx_2;
								i = pixel_at(i, step_bytes);
							}					
						}
						else
						{
							for (int y = 0; y < delta.y; ++y)
							{
								i->u.color = color;
								if(error >= 0)
								{
									error -= dy_2;
									++i;
								}
								error += dx_2;
								i = pixel_at(i, step_bytes);
							}
						}
					}
				}

				detail::free_fade_table(fade_table);
			}
		};

		class superfast_blur
			: public image_process::blur_interface
		{
			void process(pixel_buffer& pixbuf, const nana::rectangle& area, std::size_t u_radius) const
			{
				int radius = static_cast<int>(u_radius);
				int w = area.width;
				int h = area.height;
				int wm = w - 1;
				int hm = h - 1;
				int wh = w * h;
				int div = (radius << 1) + 1;

				int large_edge = (w > h ? w : h);
				const int div_256 = div * 256;

				std::unique_ptr<int[]> all_table(new int[(wh << 1) + wh + (large_edge << 1) + div_256]);


				int * r = all_table.get();
				int * g = r + wh;
				int * b = g + wh;

				int * vmin = b + wh;
				int * vmax = vmin + large_edge;

				int * dv = vmax + large_edge;
				int end_div = div - 1;
				for(int i = 0, *dv_block = dv; i < 256; ++i)
				{
					for(int u = 0; u < end_div; u += 2)
					{
						dv_block[u] = i;
						dv_block[u + 1] = i;
					}
					dv_block[div - 1] = i;
					dv_block += div;
				}

				auto linepix = pixbuf.raw_ptr(area.y) + area.x;

				int yi = 0;
				for(int y = 0; y < h; ++y)
				{
					int sum_r = 0, sum_g = 0, sum_b = 0;
					if(radius <= wm)
					{
						for(int i = - radius; i <= radius; ++i)
						{
							nana::pixel_rgb_t px = linepix[(i > 0 ? i : 0)];
							sum_r += px.u.element.red;
							sum_g += px.u.element.green;
							sum_b += px.u.element.blue;
						}					
					}
					else
					{
						for(int i = - radius; i <= radius; ++i)
						{
							nana::pixel_rgb_t px = linepix[std::min(wm, (i > 0 ? i : 0))];
							sum_r += px.u.element.red;
							sum_g += px.u.element.green;
							sum_b += px.u.element.blue;
						}
					}

					for(int x = 0; x < w; ++x)
					{
						r[yi] = dv[sum_r];
						g[yi] = dv[sum_g];
						b[yi] = dv[sum_b];

						if(0 == y)
						{
							vmin[x] = std::min(x + radius + 1, wm);
							vmax[x] = std::max(x - radius, 0);
						}

						nana::pixel_rgb_t p1 = linepix[vmin[x]];
						nana::pixel_rgb_t p2 = linepix[vmax[x]];

						sum_r += p1.u.element.red - p2.u.element.red;
						sum_g += p1.u.element.green - p2.u.element.green;
						sum_b += p1.u.element.blue - p2.u.element.blue;
						++yi;
					}
					linepix = pixbuf.raw_ptr(area.y + y) + area.x;
				}

				const int yp_init = -radius * w;

				const std::size_t bytes_pl = pixbuf.bytes_per_line();
				for(int x = 0; x < w; ++x)
				{
					int sum_r = 0, sum_g = 0, sum_b = 0;

					int yp = yp_init;
					for(int i = -radius; i <= radius; ++i)
					{
						if(yp < 1)
						{
							sum_r += r[x];
							sum_g += g[x];
							sum_b += b[x];
						}
						else
						{
							int yi = yp + x;
							sum_r += r[yi];
							sum_g += g[yi];
							sum_b += b[yi];
						}
						yp += w;
					}

					linepix = pixbuf.raw_ptr(area.y) + x;

					for(int y = 0; y < h; ++y)
					{
						linepix->u.color = 0xFF000000 | (dv[sum_r] << 16) | (dv[sum_g] << 8) | dv[sum_b];
						if(x == 0)
						{
							vmin[y] = std::min(y + radius + 1, hm) * w;
							vmax[y] = std::max(y - radius, 0) * w;
						}

						int pt1 = x + vmin[y];
						int pt2 = x + vmax[y];

						sum_r += r[pt1] - r[pt2];
						sum_g += g[pt1] - g[pt2];
						sum_b += b[pt1] - b[pt2];

						linepix = pixel_at(linepix, bytes_pl);
					}
				}
			}
		};//end class superfast_blur
	}
}
}
}

#endif
