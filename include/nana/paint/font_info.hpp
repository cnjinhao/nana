/*
 *	Font Info
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2020 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#ifndef NANA_PAINT_FONT_INFO_INCLUDED
#define NANA_PAINT_FONT_INFO_INCLUDED
#include <string>

namespace nana
{
	namespace paint
	{
		struct font_info
		{
			std::string font_family;
			double size_pt;
			unsigned weight{ 400 };	//normal
			bool italic{ false };
			bool underline{ false };
			bool strike_out{ false };
		};
	}
}

#endif //NANA_PAINT_FONT_INFO_INCLUDED