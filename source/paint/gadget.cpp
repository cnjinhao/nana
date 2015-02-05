/*
 *	Graphics Gadget Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
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
	void close_16_pixels(::nana::paint::graphics& graph, int x, int y, unsigned style, const ::nana::color& clr)
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

	void cross(graphics& graph, int x, int y, unsigned size, unsigned thickness, const ::nana::color& clr)
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

			graph.set_color(clr.blend(colors::black, true));

			for (int i = 0; i < 11; ++i)
				graph.line(ps[i], ps[i + 1]);
			graph.line(ps[11], ps[0]);

			graph.set_color(clr);
			graph.rectangle(rectangle{ ps[10].x + 1, ps[10].y + 1, (gap << 1) + thickness - 2, thickness - 2 }, true);
			graph.rectangle(rectangle{ ps[0].x + 1, ps[0].y + 1, thickness - 2, (gap << 1) + thickness - 2 }, true);
		}
	}
}//end namespace gadget
	
}//end namespace paint
}//end namespace nana
