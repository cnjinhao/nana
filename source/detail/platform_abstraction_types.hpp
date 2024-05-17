/**
*	A Bedrock Platform-Independent Implementation
*	Nana C++ Library 
 *  Documentation https://nana.acemind.cn/documentation
 *  Sources: https://github.com/cnjinhao/nana
*	Copyright(C) 2003-2024 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file nana\source\detail\platform_abstraction_types.hpp
*   @contributors  Jinhao 
*/

#ifndef NANA_DETAIL_PLATFORM_ABSTRACTION_TYPES_HEADER_INCLUDED
#define NANA_DETAIL_PLATFORM_ABSTRACTION_TYPES_HEADER_INCLUDED

#include <nana/config.hpp>
#include <nana/paint/detail/ptdefs.hpp>

#ifdef NANA_X11
#	define NANA_USE_XFT
#endif

namespace nana
{
	class font_interface
	{
	public:
		using font_style       = detail::font_style;
		using native_font_type = paint::native_font_type;

		virtual ~font_interface() = default;

		virtual native_font_type native_handle() const = 0;

		virtual const paint::font_info& font_info() const = 0;
	};
}

#endif