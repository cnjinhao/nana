/*
 *	Graphics Gadget Implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/paint/gadget.cpp
 */

#include <nana/paint/graphics.hpp>
#include <nana/paint/gadget.hpp>

namespace nana
{
namespace paint
{
namespace gadget
{
	namespace detail
	{
		typedef nana::paint::graphics& graph_reference;

		void hollow_triangle(graph_reference graph, int x, int y, nana::color_t color, uint32_t direction)
		{
			x += 3;
			y += 3;
			switch(direction)
			{
			case directions::to_east:
				graph.line(x + 3, y + 1, x + 3, y + 9, color);
				graph.line(x + 4, y + 2 , x + 7, y + 5, color);
				graph.line(x + 6, y + 6, x + 4, y + 8, color);
				break;
			case directions::to_southeast:
				graph.line(x + 2, y + 7, x + 7, y + 7, color);
				graph.line(x + 7, y + 2, x + 7, y + 6, color);
				graph.line(x + 3, y + 6, x + 6, y + 3, color);
				break;
			case directions::to_south:
				y += 3;
				graph.line(x, y, x + 8, y, color);
				graph.line(x + 1, y + 1, x + 4, y + 4, color);
				graph.line(x + 7, y + 1, x + 5, y + 3, color);
				break;
			case directions::to_west:
				x += 5;
				y += 1;
				graph.line(x, y, x, y + 8, color);
				graph.line(x - 4, y + 4, x - 1, y + 1, color);
				graph.line(x - 3, y + 5, x - 1, y + 7, color);
				break;
			case directions::to_north:
				y += 7;
				graph.line(x, y, x + 8, y, color);
				graph.line(x + 1, y - 1, x + 4, y - 4, color);
				graph.line(x + 5, y - 3, x + 7, y - 1, color);
				break;
			}		
		}

		void solid_triangle(graph_reference graph, int x, int y, nana::color_t color, uint32_t dir)
		{
			x += 3;
			y += 3;
			switch(dir)
			{
			case directions::to_east:
				for(int i = 0; i < 5; ++i)
					graph.line(x + 3 + i, y + 1 + i, x + 3 + i, y + 9 - i, color);
				break;
			case directions::to_southeast:
				for(int i = 0; i < 6; ++i)
					graph.line(x + 2 + i, y + 7 - i, x + 7, y + 7 - i, color);
				break;
			case directions::to_south:
				y += 3;
				for(int i = 0; i < 5; ++i)
					graph.line(x + i, y + i, x + 8 - i, y + i, color);
				break;
			case directions::to_west:
				x += 5;
				y += 1;
				for(int i = 0; i < 5; ++i)
					graph.line(x - i, y + i, x - i, y + 8 - i, color);
				break;
			case directions::to_north:
				y += 7;
				for(int i = 0; i < 5; ++i)
					graph.line(x + i, y - i, x + 8 - i, y - i, color);
				break;
			}		
		}

		void direction_arrow(graph_reference graph, int x, int y, nana::color_t color, uint32_t dir)
		{
			y += 5;
			switch(dir)
			{
			case directions::to_north:
				{
					x += 3;

					int pixels = 1;
					for(int l = 0; l < 4; ++l)
					{
						for(int i = 0; i < pixels; ++i)
						{
							if(l ==3 && i == 3)
							{}
							else
								graph.set_pixel(x + i, y, 0x262);
						}

						x--;
						y++;
						pixels += 2;
					}

					graph.set_pixel(x + 1, y, 0x262);
					graph.set_pixel(x + 2, y, 0x262);
					graph.set_pixel(x + 6, y, 0x262);
					graph.set_pixel(x + 7, y, 0x262);
				}
				break;
			case directions::to_south:
				{

					graph.set_pixel(x, y, 0x262);
					graph.set_pixel(x + 1, y, 0x262);
					graph.set_pixel(x + 5, y, 0x262);
					graph.set_pixel(x + 6, y, 0x262);

					++y;
					int pixels = 7;
					for(int l = 0; l < 4; ++l)
					{
						for(int i = 0; i < pixels; ++i)
						{
							if(l == 0 && i == 3){}
							else
							graph.set_pixel(x + i, y, 0x262);
						}

						x++;
						y++;
						pixels -= 2;
					}
				}
				break;
			}
		}

		void double_arrow_line(nana::paint::graphics & graph, int x, int y, color_t color, bool horizontal)
		{
			graph.set_pixel(x, y, color);
			if(horizontal)
			{
				graph.set_pixel(x + 1, y, color);
				graph.set_pixel(x + 4, y, color);
				graph.set_pixel(x + 5, y, color);
			}
			else
			{
				graph.set_pixel(x, y + 1, color);
				graph.set_pixel(x, y + 4, color);
				graph.set_pixel(x, y + 5, color);
			}
		}

		void double_arrow(nana::paint::graphics& graph, int x, int y, color_t color, directions::t dir)
		{
			switch(dir)
			{
			case directions::to_east:
				double_arrow_line(graph, x + 4, y + 6, color, true);
				double_arrow_line(graph, x + 5, y + 7, color, true);
				double_arrow_line(graph, x + 6, y + 8, color, true);
				double_arrow_line(graph, x + 5, y + 9, color, true);
				double_arrow_line(graph, x + 4, y + 10, color, true);
				break;
			case directions::to_west:
				double_arrow_line(graph, x + 5, y + 6, color, true);
				double_arrow_line(graph, x + 4, y + 7, color, true);
				double_arrow_line(graph, x + 3, y + 8, color, true);
				double_arrow_line(graph, x + 4, y + 9, color, true);
				double_arrow_line(graph, x + 5, y + 10, color, true);
				break;
			case directions::to_south:
				double_arrow_line(graph, x + 5, y + 4, color, false);
				double_arrow_line(graph, x + 6, y + 5, color, false);
				double_arrow_line(graph, x + 7, y + 6, color, false);
				double_arrow_line(graph, x + 8, y + 5, color, false);
				double_arrow_line(graph, x + 9, y + 4, color, false);
				break;
			case directions::to_north:
				double_arrow_line(graph, x + 5, y + 6, color, false);
				double_arrow_line(graph, x + 6, y + 5, color, false);
				double_arrow_line(graph, x + 7, y + 4, color, false);
				double_arrow_line(graph, x + 8, y + 5, color, false);
				double_arrow_line(graph, x + 9, y + 6, color, false);
				break;
			default:
				break;
			}
		}
	}//end namespace detail

	//arrow_16_pixels
	//param@style: 0 = hollow, 1 = solid
	void arrow_16_pixels(nana::paint::graphics& graph, int x, int y, unsigned color, uint32_t style, directions::t dir)
	{
		switch(style)
		{
		case 1:
			detail::solid_triangle(graph, x, y, color, dir);
			break;
		case 2:
			detail::direction_arrow(graph, x, y, color, dir);
			break;
		case 3:
			detail::double_arrow(graph, x, y, color, dir);
			break;
		case 0:
		default:
			detail::hollow_triangle(graph, x, y, color, dir);
			break;
		}
	}

	void close_16_pixels(nana::paint::graphics& graph, int x, int y, uint32_t style, uint32_t color)
	{
		if(0 == style)
		{
			x += 3;
			y += 3;

			graph.line(x, y, x + 9, y + 9, color);
			graph.line(x + 1, y, x + 9, y + 8, color);
			graph.line(x, y + 1, x + 8, y + 9, color);

			graph.line(x + 9, y, x , y + 9, color);
			graph.line(x + 8, y, x, y + 8, color);
			graph.line(x + 9, y + 1, x + 1, y + 9, color);
		}
		else
		{
			x += 4;
			y += 4;

			graph.line(x, y, x + 7, y + 7, color);
			graph.line(x + 1, y, x + 7, y + 6, color);
			graph.line(x, y + 1, x + 6, y + 7, color);

			graph.line(x + 7, y, x , y + 7, color);
			graph.line(x + 6, y, x, y + 6, color);
			graph.line(x + 7, y + 1, x + 1, y + 7, color);		
		}
	}

	void cross(graphics& graph, int x, int y, uint32_t size, uint32_t thickness, nana::color_t color)
	{
		if(thickness + 2 <= size)
		{
			int gap = (size - thickness) / 2;

			nana::point ps[12];
			ps[0].x = x + gap;
			ps[1].x = ps[0].x + thickness - 1;
			ps[1].y = ps[0].y = y;

			ps[2].x = ps[1].x;
			ps[2].y = y + gap;

			ps[3].x = ps[2].x + gap;
			ps[3].y = ps[2].y;

			ps[4].x = ps[3].x;
			ps[4].y = ps[3].y + thickness - 1;

			ps[5].x = ps[1].x;
			ps[5].y = ps[4].y;

			ps[6].x = ps[5].x;
			ps[6].y = ps[5].y + gap;

			ps[7].x = x + gap;
			ps[7].y = ps[6].y;

			ps[8].x = ps[7].x;
			ps[8].y = ps[4].y;

			ps[9].x = x;
			ps[9].y = ps[4].y;

			ps[10].x = x;
			ps[10].y = y + gap;

			ps[11].x = x + gap;
			ps[11].y = y + gap;

			nana::color_t dkcolor = graph.mix(color, 0x0, 0.5);
			for(int i = 0; i < 11; ++i)
				graph.line(ps[i], ps[i + 1], dkcolor);
			graph.line(ps[11], ps[0], dkcolor);

			graph.rectangle(ps[10].x + 1, ps[10].y + 1, (gap << 1) + thickness - 2, thickness - 2, color, true);
			graph.rectangle(ps[0].x + 1, ps[0].y + 1, thickness - 2, (gap << 1) + thickness - 2, color, true);

		}
	}
}//end namespace gadget
	
}//end namespace paint
}//end namespace nana
