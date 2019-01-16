/*
 *	Image Processor Algorithm Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2018 Jinhao(cnjinhao@hotmail.com)
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
#include <nana/paint/pixel_buffer.hpp>
#include <nana/paint/image_process_interface.hpp>
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
		///@brief	Seek a pixel address by using offset bytes
		///@return	the specified pixel address
		inline pixel_color_t * pixel_at(pixel_color_t * p, std::size_t bytes)
		{
			return reinterpret_cast<pixel_color_t*>(reinterpret_cast<char*>(p) + bytes);
		}

		inline const pixel_color_t * pixel_at(const pixel_color_t * p, std::size_t bytes)
		{
			return reinterpret_cast<const pixel_color_t*>(reinterpret_cast<const char*>(p) + bytes);
		}

		class proximal_interoplation
			: public image_process::stretch_interface
		{
			void process(const paint::pixel_buffer& s_pixbuf, const nana::rectangle& r_src, paint::pixel_buffer & pixbuf, const nana::rectangle& r_dst) const
			{
				const auto bytes_per_line = s_pixbuf.bytes_per_line();

				double rate_x = double(r_src.width) / r_dst.width;
				double rate_y = double(r_src.height) / r_dst.height;

				pixel_argb_t * s_raw_pixbuf = s_pixbuf.raw_ptr(0);

				if(s_pixbuf.alpha_channel())
				{
					for(std::size_t row = 0; row < r_dst.height; ++row)
					{
						const pixel_argb_t * s_line = pixel_at(s_raw_pixbuf, (static_cast<int>(row * rate_y) + r_src.y) * bytes_per_line);
						pixel_argb_t * i = pixbuf.raw_ptr(r_dst.y + row);

						for(std::size_t x = 0; x < r_dst.width; ++x, ++i)
						{
							const pixel_argb_t * s = s_line + static_cast<int>(x * rate_x) + r_src.x;
							if(0 == s->element.alpha_channel)
								continue;
							
							if(s->element.alpha_channel != 255)
							{
								i->element.red = unsigned(i->element.red * (255 - s->element.alpha_channel) + s->element.red * s->element.alpha_channel) / 255;
								i->element.green = unsigned(i->element.green * (255 - s->element.alpha_channel) + s->element.green * s->element.alpha_channel) / 255;
								i->element.blue = unsigned(i->element.blue * (255 - s->element.alpha_channel) + s->element.blue * s->element.alpha_channel) / 255;
							}
							else
							{
								unsigned alpha_chn = i->element.alpha_channel;
								*i = *s;
								i->element.alpha_channel = alpha_chn;
							}
						}
					}				
				}
				else
				{
					for(std::size_t row = 0; row < r_dst.height; ++row)
					{
						const pixel_argb_t * s_line = pixel_at(s_raw_pixbuf, (static_cast<int>(row * rate_y) + r_src.y) * bytes_per_line);
						pixel_argb_t * i = pixbuf.raw_ptr(r_dst.y + row);

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

				const nana::pixel_argb_t * s_raw_pixel_buffer = s_pixbuf.raw_ptr(0);

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

					const nana::pixel_argb_t * s_line = pixel_at(s_raw_pixel_buffer,  sy * s_bytes_per_line);
					const nana::pixel_argb_t * next_s_line = pixel_at(s_line, (sy < bottom ? s_bytes_per_line : 0));

					nana::pixel_argb_t col0;
					nana::pixel_argb_t col1;
					nana::pixel_argb_t col2;
					nana::pixel_argb_t col3;
					
					pixel_argb_t * i = pixbuf.raw_ptr(row + r_dst.y) + r_dst.x;
					
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

							unsigned alpha_chn = static_cast<unsigned>((coef0 * col0.element.alpha_channel + coef1 * col1.element.alpha_channel + (coef2 * col2.element.alpha_channel + coef3 * col3.element.alpha_channel)) >> double_shift_size);
							unsigned s_red = static_cast<unsigned>((coef0 * col0.element.red + coef1 * col1.element.red + (coef2 * col2.element.red + coef3 * col3.element.red)) >> double_shift_size);
							unsigned s_green = static_cast<unsigned>((coef0 * col0.element.green + coef1 * col1.element.green + (coef2 * col2.element.green + coef3 * col3.element.green)) >> double_shift_size);
							unsigned s_blue = static_cast<unsigned>((coef0 * col0.element.blue + coef1 * col1.element.blue + (coef2 * col2.element.blue + coef3 * col3.element.blue)) >> double_shift_size);

							if(alpha_chn)
							{
								if(alpha_chn != 255)
								{
									i->element.red	= unsigned(i->element.red * (255 - alpha_chn) + s_red * alpha_chn) / 255;
									i->element.green	= unsigned(i->element.green * (255 - alpha_chn) + s_green * alpha_chn) / 255;
									i->element.blue	= unsigned(i->element.blue * (255 - alpha_chn) + s_blue * alpha_chn) / 255;
								}
								else
								{
									i->element.red = s_red;
									i->element.green = s_green;
									i->element.blue = s_blue;
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

							i->element.red = static_cast<unsigned char>((coef0 * col0.element.red + coef1 * col1.element.red + (coef2 * col2.element.red + coef3 * col3.element.red)) >> double_shift_size);
							i->element.green = static_cast<unsigned char>((coef0 * col0.element.green + coef1 * col1.element.green + (coef2 * col2.element.green + coef3 * col3.element.green)) >> double_shift_size);
							i->element.blue = static_cast<unsigned char>((coef0 * col0.element.blue + coef1 * col1.element.blue + (coef2 * col2.element.blue + coef3 * col3.element.blue)) >> double_shift_size);
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
				auto d_rgb = d_pixbuf.at(d_pos);
				auto s_rgb = s_pixbuf.raw_ptr(s_r.y) + s_r.x;
				if(d_rgb && s_rgb)
				{
					const unsigned rest = s_r.width & 0x3;
					const unsigned length_align4 = s_r.width - rest;

					std::size_t d_step_bytes = d_pixbuf.bytes_per_line() - (s_r.width - rest) * sizeof(pixel_argb_t);
					std::size_t s_step_bytes = s_pixbuf.bytes_per_line() - (s_r.width - rest) * sizeof(pixel_argb_t);
					for(unsigned line = 0; line < s_r.height; ++line)
					{
						const auto end = d_rgb + length_align4;
						for(; d_rgb < end; d_rgb += 4, s_rgb += 4)
						{
							//0
							if(s_rgb->element.alpha_channel)
							{
								if(s_rgb->element.alpha_channel != 255)
								{
									d_rgb->element.red = unsigned(d_rgb->element.red * (255 - s_rgb[0].element.alpha_channel) + s_rgb[0].element.red * s_rgb[0].element.alpha_channel) / 255;
									d_rgb->element.green = unsigned(d_rgb->element.green * (255 - s_rgb[0].element.alpha_channel) + s_rgb[0].element.green * s_rgb[0].element.alpha_channel) / 255;
									d_rgb->element.blue = unsigned(d_rgb->element.blue * (255 - s_rgb[0].element.alpha_channel) + s_rgb[0].element.blue * s_rgb[0].element.alpha_channel) / 255;
								}
								else
									*d_rgb = *s_rgb;
							}

							//1
							if(s_rgb[1].element.alpha_channel)
							{
								if(s_rgb[1].element.alpha_channel != 255)
								{
									d_rgb[1].element.red = unsigned(d_rgb[1].element.red * (255 - s_rgb[1].element.alpha_channel) + s_rgb[1].element.red * s_rgb[1].element.alpha_channel) / 255;
									d_rgb[1].element.green = unsigned(d_rgb[1].element.green * (255 - s_rgb[1].element.alpha_channel) + s_rgb[1].element.green * s_rgb[1].element.alpha_channel) / 255;
									d_rgb[1].element.blue = unsigned(d_rgb[1].element.blue * (255 - s_rgb[1].element.alpha_channel) + s_rgb[1].element.blue * s_rgb[1].element.alpha_channel) / 255;
								}
								else
									d_rgb[1] = s_rgb[1];
							}

							//2
							if(s_rgb[2].element.alpha_channel)
							{
								if(s_rgb[2].element.alpha_channel != 255)
								{
									d_rgb[2].element.red = unsigned(d_rgb[2].element.red * (255 - s_rgb[2].element.alpha_channel) + s_rgb[2].element.red * s_rgb[2].element.alpha_channel) / 255;
									d_rgb[2].element.green = unsigned(d_rgb[2].element.green * (255 - s_rgb[2].element.alpha_channel) + s_rgb[2].element.green * s_rgb[2].element.alpha_channel) / 255;
									d_rgb[2].element.blue = unsigned(d_rgb[2].element.blue * (255 - s_rgb[2].element.alpha_channel) + s_rgb[2].element.blue * s_rgb[2].element.alpha_channel) / 255;
								}
								else
									d_rgb[2] = s_rgb[2];
							}

							//3
							if(s_rgb[3].element.alpha_channel)
							{
								if(s_rgb[3].element.alpha_channel != 255)
								{
									d_rgb[3].element.red = unsigned(d_rgb[3].element.red * (255 - s_rgb[3].element.alpha_channel) + s_rgb[3].element.red * s_rgb[3].element.alpha_channel) / 255;
									d_rgb[3].element.green = unsigned(d_rgb[3].element.green * (255 - s_rgb[3].element.alpha_channel) + s_rgb[3].element.green * s_rgb[3].element.alpha_channel) / 255;
									d_rgb[3].element.blue = unsigned(d_rgb[3].element.blue * (255 - s_rgb[3].element.alpha_channel) + s_rgb[3].element.blue * s_rgb[3].element.alpha_channel) / 255;
								}
								else
									d_rgb[3] = s_rgb[3];
							}
						}

						const pixel_argb_t * s_end = s_rgb + rest;
						auto rest_d_rgb = d_rgb;
						for(auto i = s_rgb; i != s_end; ++i)
						{
							if(i->element.alpha_channel)
							{
								if(i->element.alpha_channel != 255)
								{
									rest_d_rgb->element.red = unsigned(rest_d_rgb->element.red * (255 - i->element.alpha_channel) + i->element.red * i->element.alpha_channel) / 255;
									rest_d_rgb->element.green = unsigned(rest_d_rgb->element.green * (255 - i->element.alpha_channel) + i->element.green * i->element.alpha_channel) / 255;
									rest_d_rgb->element.blue = unsigned(rest_d_rgb->element.blue * (255 - i->element.alpha_channel) + i->element.blue * i->element.alpha_channel) / 255;
								}
								else
									*rest_d_rgb = *i;
							}
							++rest_d_rgb;
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
				auto d_rgb = d_pixbuf.raw_ptr(d_pos.y) + d_pos.x;
				auto s_rgb = s_pixbuf.raw_ptr(s_r.y) + s_r.x;

				if(d_rgb && s_rgb)
				{
					auto ptr = detail::alloc_fade_table(fade_rate);//new unsigned char[0x100 * 2];

					unsigned char* d_table = ptr.get();
					unsigned char* s_table = d_table + 0x100;

					const unsigned rest = s_r.width & 0x3;
					const unsigned length_align4 = s_r.width - rest;

					std::size_t d_step_bytes = d_pixbuf.bytes_per_line() - (s_r.width - rest) * sizeof(pixel_argb_t);
					std::size_t s_step_bytes = s_pixbuf.bytes_per_line() - (s_r.width - rest) * sizeof(pixel_argb_t);
					for(unsigned line = 0; line < s_r.height; ++line)
					{
						const auto end = d_rgb + length_align4;
						for(; d_rgb < end; d_rgb += 4, s_rgb += 4)
						{
							//0
							d_rgb[0].element.red = unsigned(d_table[d_rgb[0].element.red] + s_table[s_rgb[0].element.red]);
							d_rgb[0].element.green = unsigned(d_table[d_rgb[0].element.green] + s_table[s_rgb[0].element.green]);
							d_rgb[0].element.blue = unsigned(d_table[d_rgb[0].element.blue] + s_table[s_rgb[0].element.blue]);

							//1
							d_rgb[1].element.red = unsigned(d_table[d_rgb[1].element.red] + s_table[s_rgb[1].element.red]);
							d_rgb[1].element.green = unsigned(d_table[d_rgb[1].element.green] + s_table[s_rgb[1].element.green]);
							d_rgb[1].element.blue = unsigned(d_table[d_rgb[1].element.blue] + s_table[s_rgb[1].element.blue]);

							//2
							d_rgb[2].element.red = unsigned(d_table[d_rgb[2].element.red] + s_table[s_rgb[2].element.red]);
							d_rgb[2].element.green = unsigned(d_table[d_rgb[2].element.green] + s_table[s_rgb[2].element.green]);
							d_rgb[2].element.blue = unsigned(d_table[d_rgb[2].element.blue] + s_table[s_rgb[2].element.blue]);

							//3
							d_rgb[3].element.red = unsigned(d_table[d_rgb[3].element.red] + s_table[s_rgb[3].element.red]);
							d_rgb[3].element.green = unsigned(d_table[d_rgb[3].element.green] + s_table[s_rgb[3].element.green]);
							d_rgb[3].element.blue = unsigned(d_table[d_rgb[3].element.blue] + s_table[s_rgb[3].element.blue]);
						}

						for(unsigned i = 0; i < rest; ++i)
						{
							d_rgb[i].element.red = unsigned(d_table[d_rgb[i].element.red] + s_table[s_rgb[i].element.red]);
							d_rgb[i].element.green = unsigned(d_table[d_rgb[i].element.green] + s_table[s_rgb[i].element.green]);
							d_rgb[i].element.blue = unsigned(d_table[d_rgb[i].element.blue] + s_table[s_rgb[i].element.blue]);
						}
						d_rgb = pixel_at(d_rgb, d_step_bytes);
						s_rgb = pixel_at(s_rgb, s_step_bytes);
					}
				}
			}
		};

		//class line
		class bresenham_line
			: public image_process::line_interface
		{
			virtual void process(paint::pixel_buffer & pixbuf, const nana::point& pos_beg, const nana::point& pos_end, const ::nana::color& clr, double fade_rate) const
			{
				//Return if it is completely transparent
				if (fade_rate <= 0)
					return;

				auto rgb_color = clr.px_color().value;
				const std::size_t bytes_pl = pixbuf.bytes_per_line();
				
				unsigned char * fade_table = nullptr;
				std::unique_ptr<unsigned char[]> autoptr;
				nana::pixel_argb_t rgb_imd = {};
				if(fade_rate < 1)
				{
					autoptr = detail::alloc_fade_table(1.0 - fade_rate);
					fade_table = autoptr.get();
					rgb_imd.value = rgb_color;
					rgb_imd = detail::fade_color_intermedia(rgb_imd, fade_table);
				}

				auto i = pixel_at(pixbuf.raw_ptr(0), pos_beg.y * bytes_pl) + pos_beg.x;

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
					step_bytes += sizeof(pixel_argb_t);
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
							i->value = rgb_color;
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
								i->value = rgb_color;
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
								i->value = rgb_color;
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
			}
		};

		class superfast_blur
			: public image_process::blur_interface
		{
			void process(pixel_buffer& pixbuf, const nana::rectangle& area, std::size_t u_radius) const
			{
				const int radius = static_cast<int>(u_radius);
				const int safe_radius = std::min(radius, static_cast<int>(area.height) - 2);
				const int radius_plus_one = radius + 1;

				const int width_3times = static_cast<int>(area.width * 3);
				const int wm = area.width - 1;
				const int hm = area.height - 1;
				const int wh = area.width * area.height;
				const int div = (radius << 1) + 1;

				const int large_edge = std::max(area.width, area.height);

				std::unique_ptr<int[]> table_rgb(new int[wh * 3 + (large_edge << 1) + div * 256]);

				int * tbl_rgb = table_rgb.get();
				int * const vmin = tbl_rgb + 3 * wh;

				int * const vmax = vmin + large_edge;
				int * dv = vmax + large_edge;

				for (int i = 0; i < 256; ++i)
				{
					dv[0] = i;
					for (int u = 1; u < div; u += 2)
					{
						dv[u] = i;
						dv[u + 1] = i;
					}
					dv += div;
				}

				dv = vmax + large_edge;

				auto linepix = pixbuf.raw_ptr(area.y) + area.x;

				for(int x = 0; x < static_cast<int>(area.width); ++x)
				{
					vmin[x] = std::min(x + radius_plus_one, wm);
					vmax[x] = std::max(x - radius, 0);
				}

				for(int y = 0; y < static_cast<int>(area.height); ++y)
				{
					int sum_r = 0, sum_g = 0, sum_b = 0;
					if(radius <= wm)
					{
						auto px = linepix;

						sum_r = int(px->element.red) * radius_plus_one;
						sum_g = int(px->element.blue) * radius_plus_one;
						sum_b = int(px->element.blue) * radius_plus_one;

						auto radius_px_end = px + radius_plus_one;
						for (++px; px < radius_px_end; ++px)
						{
							sum_r += px->element.red;
							sum_g += px->element.green;
							sum_b += px->element.blue;
						}
					}
					else
					{
						for(int i = - radius; i <= radius; ++i)
						{
							auto px = linepix[std::min(wm, (i > 0 ? i : 0))];
							sum_r += px.element.red;
							sum_g += px.element.green;
							sum_b += px.element.blue;
						}
					}

					for(int x = 0; x < static_cast<int>(area.width); ++x)
					{
						tbl_rgb[0] = dv[sum_r];
						tbl_rgb[1] = dv[sum_g];
						tbl_rgb[2] = dv[sum_b];
						tbl_rgb += 3;

						auto& p1 = linepix[vmin[x]];
						auto& p2 = linepix[vmax[x]];

						sum_r += p1.element.red - p2.element.red;
						sum_g += p1.element.green - p2.element.green;
						sum_b += p1.element.blue - p2.element.blue;
					}
					linepix = pixbuf.raw_ptr(area.y + y) + area.x;
				}

				const std::size_t bytes_pl = pixbuf.bytes_per_line();

				tbl_rgb = table_rgb.get();

				for (int y = 0; y < static_cast<int>(area.height); ++y)
				{
					vmin[y] = std::min(y + radius_plus_one, hm) * width_3times;
					vmax[y] = std::max(y - radius, 0) * width_3times;
				}

				for(int x = 0; x < static_cast<int>(area.width); ++x)
				{
					int sum_r = int(tbl_rgb[0]) * radius_plus_one;
					int sum_g = int(tbl_rgb[1]) * radius_plus_one;
					int sum_b = int(tbl_rgb[2]) * radius_plus_one;

					int nextln = width_3times;
					for (int i = 0; i < safe_radius; ++i)
					{
						sum_r += tbl_rgb[nextln];
						sum_g += tbl_rgb[nextln + 1];
						sum_b += tbl_rgb[nextln + 2];
						nextln += width_3times;
					}

					linepix = pixbuf.raw_ptr(area.y) + x + area.x;
					for(int y = 0; y < static_cast<int>(area.height); ++y)
					{
						linepix->value = 0xFF000000 | (dv[sum_r] << 16) | (dv[sum_g] << 8) | dv[sum_b];

						int pt1 = vmin[y];
						int pt2 = vmax[y];

						sum_r += tbl_rgb[pt1] - tbl_rgb[pt2];
						sum_g += tbl_rgb[pt1 + 1] - tbl_rgb[pt2 + 1];
						sum_b += tbl_rgb[pt1 + 2] - tbl_rgb[pt2 + 2];
						linepix = pixel_at(linepix, bytes_pl);
					}
					tbl_rgb += 3;
				}
			}
		};//end class superfast_blur
	}
}
}
}

#endif
