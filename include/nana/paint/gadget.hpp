/*
 *	Graphics Gadget Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
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
	void close_16_pixels(::nana::paint::graphics&, int x, int y, unsigned style, const color&);
	void cross(::nana::paint::graphics&, int x, int y, unsigned size, unsigned thickness, const nana::color&);

}//end namespace gadget
	
}//end namespace paint
}//end namespace nana

#endif
