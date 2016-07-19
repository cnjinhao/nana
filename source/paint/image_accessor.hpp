/*
*	Paint Image Accessor
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file nana/paint/image_accessor.hpp
*	@brief A declaration of class image_accessor. It is used to access image private data, internal use.
*/
#ifndef NANA_PAINT_IMAGE_ACCESS_HEADER_INCLUDED
#define NANA_PAINT_IMAGE_ACCESS_HEADER_INCLUDED
namespace nana
{
	namespace paint
	{
		class image_accessor
		{
		public:
#if defined(NANA_WINDOWS)
			static HICON icon(const image&);
#else
			static int icon(const image&);
#endif
		};
	}
}
#endif