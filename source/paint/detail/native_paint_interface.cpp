/*
 *	Platform Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/paint/detail/native_paint_interface.cpp
 *	@contributors:	dareg
 */

#include <nana/detail/platform_spec_selector.hpp>
#include <nana/paint/detail/native_paint_interface.hpp>
#include <nana/paint/pixel_buffer.hpp>
#include <nana/gui/layout_utility.hpp>

#if defined(NANA_WINDOWS)
	#include <windows.h>
#elif defined(NANA_X11)
	#include <X11/Xlib.h>
#endif

namespace nana
{
namespace paint
{
namespace detail
{

	nana::size drawable_size(drawable_type dw)
	{
		if(0 == dw) return nana::size();
#if defined(NANA_WINDOWS)
		BITMAP bmp;
		::GetObject(dw->pixmap, sizeof bmp, &bmp);
		return nana::size(bmp.bmWidth, bmp.bmHeight);
#elif defined(NANA_X11)
        nana::detail::platform_spec & spec = nana::detail::platform_spec::instance();
        Window root;
        int x, y;
        unsigned width, height;
        unsigned border, depth;
        nana::detail::platform_scope_guard psg;
        ::XGetGeometry(spec.open_display(), dw->pixmap, &root, &x, &y, &width, &height, &border, &depth);
		return nana::size(width, height);
#endif
	}


	std::unique_ptr<unsigned char[]> alloc_fade_table(double fade_rate)
	{
		std::unique_ptr<unsigned char[]> ptr(new unsigned char[0x100 * 2]);
		unsigned char* tablebuf = ptr.get();
		unsigned char* d_table = tablebuf;
		unsigned char* s_table = d_table + 0x100;

		double fade_rate_mul_to_add = 0;
		double fade_rate_2 = fade_rate + fade_rate;
		double fade_rate_3 = fade_rate_2 + fade_rate;
		double fade_rate_4 = fade_rate_3 + fade_rate;

		for(int i = 0; i < 0x100; i += 4, fade_rate_mul_to_add += fade_rate_4)
		{
			d_table[0] = static_cast<unsigned char>(fade_rate_mul_to_add);
			s_table[0] = i - d_table[0];

			d_table[1] = static_cast<unsigned char>(fade_rate_mul_to_add + fade_rate);
			s_table[1] = i + 1 - d_table[1];

			d_table[2] = static_cast<unsigned char>(fade_rate_mul_to_add + fade_rate_2);
			s_table[2] = i + 2 - d_table[2];

			d_table[3] = static_cast<unsigned char>(fade_rate_mul_to_add + fade_rate_3);
			s_table[3] = i + 3 - d_table[3];

			d_table += 4;
			s_table += 4;
		}
		return ptr;
	}

	nana::pixel_color_t fade_color_intermedia(nana::pixel_color_t fgcolor, const unsigned char* fade_table)
	{
		fade_table += 0x100;
		fgcolor.element.red = fade_table[fgcolor.element.red];
		fgcolor.element.green = fade_table[fgcolor.element.green];
		fgcolor.element.blue = fade_table[fgcolor.element.blue];
		return fgcolor;
	}

	nana::pixel_color_t fade_color_by_intermedia(nana::pixel_color_t bgcolor, nana::pixel_color_t fgcolor_intermedia, const unsigned char* const fade_table)
	{
		bgcolor.element.red = fade_table[bgcolor.element.red] + fgcolor_intermedia.element.red;
		bgcolor.element.green = fade_table[bgcolor.element.green] + fgcolor_intermedia.element.green;
		bgcolor.element.blue = fade_table[bgcolor.element.blue] + fgcolor_intermedia.element.blue;
		return bgcolor;
	}

	void blend(drawable_type dw, const rectangle& area, pixel_color_t color, double fade_rate)
	{
		if (fade_rate <= 0) return;
		if (fade_rate > 1) fade_rate = 1;

		rectangle r;
		if (false == ::nana::overlap(rectangle{ drawable_size(dw) }, area, r))
			return;

		unsigned red = static_cast<unsigned>((color.value & 0xFF0000) * fade_rate);
		unsigned green = static_cast<unsigned>((color.value & 0xFF00) * fade_rate);
		unsigned blue = static_cast<unsigned>((color.value & 0xFF) * fade_rate);

		double lrate = 1 - fade_rate;
		pixel_buffer pixbuf(dw, r.y, r.height);

		for (std::size_t row = 0; row < r.height; ++row)
		{
			auto i = pixbuf.raw_ptr(row) + r.x;
			const auto end = i + r.width;
			for (; i < end; ++i)
			{
				unsigned px_r = ((static_cast<unsigned>((i->value & 0xFF0000) * lrate) + red) & 0xFF0000);
				unsigned px_g = ((static_cast<unsigned>((i->value & 0xFF00) * lrate) + green) & 0xFF00);
				unsigned px_b = ((static_cast<unsigned>((i->value & 0xFF) * lrate) + blue) & 0xFF);
				i->value = (px_r | px_g | px_b);
			}
		}
		pixbuf.paste(nana::rectangle(r.x, 0, r.width, r.height), dw, point{r.x, r.y});
	}

	nana::size raw_text_extent_size(drawable_type dw, const wchar_t* text, std::size_t len)
	{
		if(nullptr == dw || nullptr == text || 0 == len) return nana::size();
#if defined(NANA_WINDOWS)
		::SIZE size;
		if(::GetTextExtentPoint32(dw->context, text, static_cast<int>(len), &size))
			return nana::size(size.cx, size.cy);
#elif defined(NANA_X11)
	#if defined(NANA_USE_XFT)
		std::string utf8str = to_utf8(std::wstring(text, len));
		XGlyphInfo ext;
		XftFont * fs = reinterpret_cast<XftFont*>(dw->font->handle);
		::XftTextExtentsUtf8(nana::detail::platform_spec::instance().open_display(), fs,
								reinterpret_cast<XftChar8*>(const_cast<char*>(utf8str.c_str())), utf8str.size(), &ext);
		return nana::size(ext.xOff, fs->ascent + fs->descent);
	#else
		XRectangle ink;
		XRectangle logic;
		::XmbTextExtents(reinterpret_cast<XFontSet>(dw->font->handle), text, len, &ink, &logic);
		return nana::size(logic.width, logic.height);
	#endif
#endif
		return nana::size();
	}

	nana::size text_extent_size(drawable_type dw, const wchar_t * text, std::size_t len)
	{
		if (nullptr == dw || nullptr == text || 0 == len)
			return{};

		nana::size extents = raw_text_extent_size(dw, text, len);

		const wchar_t* const end = text + len;
		int tabs = 0;
		for(; text != end; ++text)
		{
			if(*text == '\t')
				++tabs;
		}
		if(tabs)
			extents.width = static_cast<int>(extents.width) - tabs * static_cast<int>(dw->string.tab_pixels - dw->string.whitespace_pixels * dw->string.tab_length);
		return extents;
	}

	void draw_string(drawable_type dw, const nana::point& pos, const wchar_t * str, std::size_t len)
	{
#if defined(NANA_WINDOWS)
		::TextOut(dw->context, pos.x, pos.y, str, static_cast<int>(len));
#elif defined(NANA_X11)
		auto disp = ::nana::detail::platform_spec::instance().open_display();
	#if defined(NANA_USE_XFT)
		auto fs = reinterpret_cast<XftFont*>(dw->font->handle);

		//Fixed missing array declaration by dareg
		std::unique_ptr<FT_UInt[]> glyphs_ptr(new FT_UInt[len]);
		auto glyphs = glyphs_ptr.get();
		const auto endstr = str + len;
		for(auto chr = str; chr != endstr; ++chr)
		{
			(*glyphs++) = XftCharIndex(disp, fs, *chr);
		}
		XftDrawGlyphs(dw->xftdraw, &(dw->xft_fgcolor), fs, pos.x, pos.y + fs->ascent, glyphs_ptr.get(), len);
	#else
		XFontSet fs = reinterpret_cast<XFontSet>(dw->font->handle);
		XFontSetExtents * ext = ::XExtentsOfFontSet(fs);
		XFontStruct ** fontstructs;
		char ** font_names;
		int size = ::XFontsOfFontSet(fs, &fontstructs, &font_names);
		unsigned ascent = 0;
		unsigned descent = 0;
		XFontStruct **fontstructs_end = fontstructs + size;
		for(XFontStruct** i = fontstructs; i < fontstructs_end; ++i)
		{
			if(ascent < (*i)->ascent)
				ascent = (*i)->ascent;
			if(descent < (*i)->descent)
				descent = (*i)->descent;
		}
		XmbDrawString(display, dw->pixmap, reinterpret_cast<XFontSet>(dw->font->handle), dw->context, pos.x, pos.y + ascent + descent, buf, len);
	#endif
#endif
	}
}//end namespace detail
}//end namespace paint
}//end namespace nana
