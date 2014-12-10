/*
 *	Graphics Gadget Implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/paint/gadget.hpp
 */

#ifndef NANA_PAINT_GADGET_HPP
#define NANA_PAINT_GADGET_HPP
#include "graphics.hpp"
#include "image.hpp"
#include <nana/basic_types.hpp>

namespace nana
{
namespace paint
{
namespace gadget
{
	struct directions
	{
		enum t{to_east, to_southeast, to_south, to_southwest, to_west, to_northwest, to_north, to_northeast};
	};

	void arrow_16_pixels(nana::paint::graphics&, int x, int y, unsigned color, uint32_t style, directions::t direction);
	void close_16_pixels(nana::paint::graphics&, int x, int y, uint32_t style, uint32_t color);
	void cross(nana::paint::graphics&, int x, int y, uint32_t size, uint32_t thickness, nana::color_t color);

}//end namespace gadget
	
}//end namespace paint
}//end namespace nana

#endif
