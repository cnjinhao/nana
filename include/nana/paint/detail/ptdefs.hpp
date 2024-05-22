/**
 *	Font styles and font information
 *	Nana C++ Library 
 *  Documentation https://nana.acemind.cn/documentation
 *  Sources: https://github.com/cnjinhao/nana
 *	Copyright(C) 2003-2024 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file nana/paint/detail/ptdefs.hpp 
 */

#ifndef NANA_PAINT_PTDEFS_INCLUDED
#define NANA_PAINT_PTDEFS_INCLUDED
#include <string>

namespace nana
{
	namespace detail
	{
		struct native_font_signature;  ///< \todo unused?
		
		struct font_style
		{
			unsigned weight{ 400 };	//normal
			bool italic{ false };
			bool underline{ false };
			bool strike_out{ false };
			bool antialiasing{ true };

			font_style& change_weight(unsigned);
			font_style& change_italic(bool);
			font_style& change_underline(bool);
			font_style& change_strikeout(bool);
			font_style& change_antialiasing(bool);
		};
	}//end namespace detail

	namespace paint
	{
		using native_font_type = ::nana::detail::native_font_signature*;  ///< \todo unused?

		struct font_info
		{
			std::string family;	            ///< Font family
			double size_pt;		            ///< Font Size, in pt., 1 pt = 1 inch/72 (not scaled)
			nana::detail::font_style style;	///< Font Styles
		};
	}
}

#endif