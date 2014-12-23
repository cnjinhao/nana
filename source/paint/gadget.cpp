/*
 *	Graphics Gadget Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2014 Jinhao(cnjinhao@hotmail.com)
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

		void hollow_triangle(graph_reference graph, int x, int y, uint32_t direction)
		{
			x += 3;
			y += 3;
			switch (direction)
			{
			case directions::to_east:
				graph.line(point{ x + 3, y + 1 }, point{ x + 3, y + 9 });
				graph.line(point{ x + 4, y + 2 }, point{ x + 7, y + 5 });
				graph.line(point{ x + 6, y + 6 }, point{ x + 4, y + 8 });
				break;
			case directions::to_southeast:
				graph.line(point{ x + 2, y + 7 }, point{ x + 7, y + 7 });
				graph.line(point{ x + 7, y + 2 }, point{ x + 7, y + 6 });
				graph.line(point{ x + 3, y + 6 }, point{ x + 6, y + 3 });
				break;
			case directions::to_south:
				y += 3;
				graph.line(point{ x, y }, point{ x + 8, y });
				graph.line(point{ x + 1, y + 1 }, point{ x + 4, y + 4 });
				graph.line(point{ x + 7, y + 1 }, point{ x + 5, y + 3 });
				break;
			case directions::to_west:
				x += 5;
				y += 1;
				graph.line(point{ x, y }, point{ x, y + 8 });
				graph.line(point{ x - 4, y + 4 }, point{ x - 1, y + 1 });
				graph.line(point{ x - 3, y + 5 }, point{ x - 1, y + 7 });
				break;
			case directions::to_north:
				y += 7;
				graph.line(point{ x, y }, point{x + 8, y});
				graph.line(point{x + 1, y - 1}, point{x + 4, y - 4});
				graph.line(point{x + 5, y - 3}, point{x + 7, y - 1});
				break;
			}		
		}

		void solid_triangle(graph_reference graph, int x, int y, uint32_t dir)
		{
			x += 3;
			y += 3;
			switch(dir)
			{
			case directions::to_east:
				for(int i = 0; i < 5; ++i)
					graph.line(point{ x + 3 + i, y + 1 + i }, point{ x + 3 + i, y + 9 - i });
				break;
			case directions::to_southeast:
				for(int i = 0; i < 6; ++i)
					graph.line(point{ x + 2 + i, y + 7 - i }, point{ x + 7, y + 7 - i });
				break;
			case directions::to_south:
				y += 3;
				for(int i = 0; i < 5; ++i)
					graph.line(point{ x + i, y + i }, point{ x + 8 - i, y + i });
				break;
			case directions::to_west:
				x += 5;
				y += 1;
				for(int i = 0; i < 5; ++i)
					graph.line(point{ x - i, y + i }, point{ x - i, y + 8 - i });
				break;
			case directions::to_north:
				y += 7;
				for(int i = 0; i < 5; ++i)
					graph.line(point{ x + i, y - i }, point{ x + 8 - i, y - i });
				break;
			}		
		}

		void direction_arrow(graph_reference graph, int x, int y, uint32_t dir)
		{
			graph.set_color({ 0x0, 0x2, 0x62 });
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
								graph.set_pixel(x + i, y);
						}

						x--;
						y++;
						pixels += 2;
					}

					graph.set_pixel(x + 1, y);
					graph.set_pixel(x + 2, y);
					graph.set_pixel(x + 6, y);
					graph.set_pixel(x + 7, y);
				}
				break;
			case directions::to_south:
				{

					graph.set_pixel(x, y);
					graph.set_pixel(x + 1, y);
					graph.set_pixel(x + 5, y);
					graph.set_pixel(x + 6, y);

					++y;
					int pixels = 7;
					for (int l = 0; l < 4; ++l)
					{
						for (int i = 0; i < pixels; ++i)
						{
							if (l != 0 || i != 3)
								graph.set_pixel(x + i, y);
						}

						x++;
						y++;
						pixels -= 2;
					}
				}
				break;
			}
		}

		void double_arrow_line(nana::paint::graphics & graph, int x, int y, bool horizontal)
		{
			graph.set_pixel(x, y);
			if(horizontal)
			{
				graph.set_pixel(x + 1, y);
				graph.set_pixel(x + 4, y);
				graph.set_pixel(x + 5, y);
			}
			else
			{
				graph.set_pixel(x, y + 1);
				graph.set_pixel(x, y + 4);
				graph.set_pixel(x, y + 5);
			}
		}

		void double_arrow(nana::paint::graphics& graph, int x, int y, directions::t dir)
		{
			switch(dir)
			{
			case directions::to_east:
				double_arrow_line(graph, x + 4, y + 6, true);
				double_arrow_line(graph, x + 5, y + 7, true);
				double_arrow_line(graph, x + 6, y + 8, true);
				double_arrow_line(graph, x + 5, y + 9, true);
				double_arrow_line(graph, x + 4, y + 10, true);
				break;
			case directions::to_west:
				double_arrow_line(graph, x + 5, y + 6, true);
				double_arrow_line(graph, x + 4, y + 7, true);
				double_arrow_line(graph, x + 3, y + 8, true);
				double_arrow_line(graph, x + 4, y + 9, true);
				double_arrow_line(graph, x + 5, y + 10, true);
				break;
			case directions::to_south:
				double_arrow_line(graph, x + 5, y + 4, false);
				double_arrow_line(graph, x + 6, y + 5, false);
				double_arrow_line(graph, x + 7, y + 6, false);
				double_arrow_line(graph, x + 8, y + 5, false);
				double_arrow_line(graph, x + 9, y + 4, false);
				break;
			case directions::to_north:
				double_arrow_line(graph, x + 5, y + 6, false);
				double_arrow_line(graph, x + 6, y + 5, false);
				double_arrow_line(graph, x + 7, y + 4, false);
				double_arrow_line(graph, x + 8, y + 5, false);
				double_arrow_line(graph, x + 9, y + 6, false);
				break;
			default:
				break;
			}
		}
	}//end namespace detail

	//arrow_16_pixels
	//param@style: 0 = hollow, 1 = solid
	void arrow_16_pixels(nana::paint::graphics& graph, int x, int y, const nana::color& clr, uint32_t style, directions::t dir)
	{
		graph.set_color(clr);
		switch(style)
		{
		case 1:
			detail::solid_triangle(graph, x, y, dir);
			break;
		case 2:
			detail::direction_arrow(graph, x, y, dir);
			break;
		case 3:
			detail::double_arrow(graph, x, y, dir);
			break;
		case 0:
		default:
			detail::hollow_triangle(graph, x, y, dir);
			break;
		}
	}

	void close_16_pixels(::nana::paint::graphics& graph, int x, int y, uint32_t style, const ::nana::color& clr)
	{
		graph.set_color(clr);
		if(0 == style)
		{
			x += 3;
			y += 3;

			graph.line({ x, y }, { x + 9, y + 9 });
			graph.line({ x + 1, y }, { x + 9, y + 8 });
			graph.line({ x, y + 1 }, { x + 8, y + 9 });

			graph.line({ x + 9, y }, { x, y + 9 });
			graph.line({ x + 8, y }, { x, y + 8 });
			graph.line({ x + 9, y + 1 }, { x + 1, y + 9 });
		}
		else
		{
			x += 4;
			y += 4;

			graph.line({ x, y }, { x + 7, y + 7 });
			graph.line({ x + 1, y }, { x + 7, y + 6 });
			graph.line({ x, y + 1 }, { x + 6, y + 7 });

			graph.line({ x + 7, y }, { x, y + 7 });
			graph.line({ x + 6, y }, { x, y + 6 });
			graph.line({ x + 7, y + 1 }, { x + 1, y + 7 });
		}
	}

	void cross(graphics& graph, int x, int y, uint32_t size, uint32_t thickness, const ::nana::color& color)
	{
			if (thickness + 2 <= size)
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

				::nana::color darker(0, 0, 0);
				darker.blend(color, true);
				graph.set_color(darker);

				for (int i = 0; i < 11; ++i)
					graph.line(ps[i], ps[i + 1]);
				graph.line(ps[11], ps[0]);

				graph.set_color(color);
				graph.rectangle(rectangle{ ps[10].x + 1, ps[10].y + 1, (gap << 1) + thickness - 2, thickness - 2 }, true);
				graph.rectangle(rectangle{ ps[0].x + 1, ps[0].y + 1, thickness - 2, (gap << 1) + thickness - 2 }, true);

			}
	}
}//end namespace gadget
	
}//end namespace paint
}//end namespace nana
