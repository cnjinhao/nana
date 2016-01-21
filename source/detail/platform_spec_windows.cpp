/*
 *	Platform Specification Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/detail/platform_spec.cpp
 *
 *	This file provides basis class and data structrue that required by nana
 */

#include <nana/detail/platform_spec_selector.hpp>

#if defined(NANA_WINDOWS)

#include <shellapi.h>
#include <stdexcept>

#if defined(_MSC_VER)
#include <VersionHelpers.h>
#endif // _MSVC

namespace nana
{
namespace detail
{
	drawable_impl_type::drawable_impl_type()
	{
		pen.handle = nullptr;
		pen.color = 0xffffffff;
		pen.style = -1;
		pen.width = -1;

		brush.handle = nullptr;
		brush.style = brush_spec::Solid;
		brush.color = 0xffffffff;

		round_region.handle = nullptr;
		round_region.radius_x = round_region.radius_y = 0;

		string.tab_length = 4;
		string.tab_pixels = 0;
		string.whitespace_pixels = 0;
	}

	drawable_impl_type::~drawable_impl_type()
	{
		::DeleteDC(context);
		::DeleteObject(pixmap);
		::DeleteObject(pen.handle);
		::DeleteObject(brush.handle);
		::DeleteObject(round_region.handle);
	}

	void drawable_impl_type::fgcolor(const ::nana::color& clr)
	{
		set_text_color(clr);
	}

	unsigned drawable_impl_type::get_color() const
	{
		return color_;
	}

	unsigned drawable_impl_type::get_text_color() const
	{
		return text_color_;
	}

	void drawable_impl_type::set_color(const ::nana::color& clr)
	{
		color_ = (clr.px_color().value & 0xFFFFFF);
	}

	void drawable_impl_type::set_text_color(const ::nana::color& clr)
	{
		auto rgb = (clr.px_color().value & 0xFFFFFF);
		if (text_color_ != rgb)
		{
			::SetTextColor(context, NANA_RGB(rgb));
			text_color_ = rgb;
		}
	}

	void drawable_impl_type::update_pen()
	{
		if (pen.color != color_)
		{
			pen.handle = ::CreatePen(PS_SOLID, 1, NANA_RGB(color_));
			::DeleteObject(::SelectObject(context, pen.handle));
			pen.color = color_;
		}
	}

	void drawable_impl_type::update_brush()
	{
		if (brush.color != color_)
			brush.set(context, brush.style, color_);
	}

	void drawable_impl_type::pen_spec::set(HDC context, int style, int width, unsigned clr)
	{
		if (this->color != clr || this->width != width || this->style != style)
		{
			this->color = clr;
			this->width = width;
			this->style = style;
			this->handle = ::CreatePen(style, width, NANA_RGB(clr));
			::DeleteObject(::SelectObject(context, this->handle));
		}
	}

	void drawable_impl_type::brush_spec::set(HDC context, drawable_impl_type::brush_spec::t style, unsigned rgb)
	{
		if (this->color != rgb || this->style != style)
		{
			this->color = rgb;
			this->style = style;
			switch(style)
			{
			case brush_spec::HatchBDiagonal:
				this->handle = ::CreateHatchBrush(HS_BDIAGONAL, NANA_RGB(rgb));
				break;
			case brush_spec::Solid:
			default:
				this->style = brush_spec::Solid;
				this->handle = ::CreateSolidBrush(NANA_RGB(color));
				break;
			}
			::DeleteObject(::SelectObject(context, this->handle));
		}
	}

	void drawable_impl_type::round_region_spec::set(const nana::rectangle& r, unsigned radius_x, unsigned radius_y)
	{
		if(this->r != r || this->radius_x != radius_x || this->radius_y != radius_y)
		{
			if(handle)
				::DeleteObject(this->handle);
			this->r = r;
			this->radius_x = radius_x;
			this->radius_y = radius_y;
			handle = ::CreateRoundRectRgn(r.x, r.y, r.x + static_cast<int>(r.width) + 1, r.y + static_cast<int>(r.height) + 1, static_cast<int>(radius_x + 1), static_cast<int>(radius_y + 1));
		}
	}

	//struct font_tag::deleter
	void font_tag::deleter::operator()(const font_tag* tag) const
	{
		if(tag && tag->handle)
			::DeleteObject(tag->handle);
		delete tag;
	}
	//end struct font_tag::deleter

	//class platform_spec
	platform_spec::co_initializer::co_initializer()
		: ole32_(::LoadLibrary(L"OLE32.DLL"))
	{
		if(ole32_)
		{
			typedef HRESULT (__stdcall *CoInitializeEx_t)(LPVOID, DWORD);
			CoInitializeEx_t fn_init = reinterpret_cast<CoInitializeEx_t>(::GetProcAddress(ole32_, "CoInitializeEx"));
			if(0 == fn_init)
			{
				::FreeLibrary(ole32_);
				ole32_ = 0;
				throw std::runtime_error("Nana.PlatformSpec.Co_initializer: Can't locate the CoInitializeEx().");
			}
			else
				fn_init(0, COINIT_APARTMENTTHREADED | /*COINIT_DISABLE_OLE1DDE =*/0x4);
		}
		else
			throw std::runtime_error("Nana.PlatformSpec.Co_initializer: No Ole32.DLL Loaded.");
	}

	platform_spec::co_initializer::~co_initializer()
	{
		if(ole32_)
		{
			typedef void (__stdcall *CoUninitialize_t)(void);
			CoUninitialize_t fn_unin = reinterpret_cast<CoUninitialize_t>(::GetProcAddress(ole32_, "CoUninitialize"));
			if(fn_unin)
				fn_unin();
			::FreeLibrary(ole32_);
		}
	}

	platform_spec::platform_spec()
	{
		//Create default font object.
		NONCLIENTMETRICS metrics = {};
		metrics.cbSize = sizeof metrics;
#if(WINVER >= 0x0600)
#if defined(NANA_MINGW)
		OSVERSIONINFO osvi = {};
		osvi.dwOSVersionInfoSize = sizeof(osvi);
		::GetVersionEx(&osvi);
		if (osvi.dwMajorVersion < 6)
			metrics.cbSize -= sizeof(metrics.iPaddedBorderWidth);
#else
		if(!IsWindowsVistaOrGreater())
			metrics.cbSize -= sizeof(metrics.iPaddedBorderWidth);
#endif
#endif
		::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof metrics, &metrics, 0);
		def_font_ptr_ = make_native_font(to_utf8(metrics.lfMessageFont.lfFaceName).c_str(), font_size_to_height(9), 400, false, false, false);
	}

	const platform_spec::font_ptr_t& platform_spec::default_native_font() const
	{
		return def_font_ptr_;
	}

	void platform_spec::default_native_font(const font_ptr_t& fp)
	{
		def_font_ptr_ = fp;
	}

	unsigned platform_spec::font_size_to_height(unsigned size) const
	{
		HDC hdc = ::GetDC(0);
		size = ::MulDiv(int(size), ::GetDeviceCaps(hdc, LOGPIXELSY), 72);
		::ReleaseDC(0, hdc);
		return size;
	}

	unsigned platform_spec::font_height_to_size(unsigned height) const
	{
		HDC hdc = ::GetDC(0);
		unsigned pixels = ::GetDeviceCaps(hdc, LOGPIXELSY);
		::ReleaseDC(0, hdc);

		height = static_cast<unsigned>(static_cast<long long>(72) * height / pixels);
		return height;
	}

	platform_spec::font_ptr_t platform_spec::make_native_font(const char* name, unsigned height, unsigned weight, bool italic, bool underline, bool strike_out)
	{
		::LOGFONT logfont;
		memset(&logfont, 0, sizeof logfont);

		if (name && *name)
			std::wcscpy(logfont.lfFaceName, to_wstring(name).c_str());
		else
			std::wcscpy(logfont.lfFaceName, def_font_ptr_->name.c_str());

		logfont.lfCharSet = DEFAULT_CHARSET;
		HDC hdc = ::GetDC(0);
		logfont.lfHeight = -static_cast<int>(height);
		::ReleaseDC(0, hdc);

		logfont.lfWidth = 0;
		logfont.lfWeight = weight;
		logfont.lfQuality = PROOF_QUALITY;
		logfont.lfPitchAndFamily = FIXED_PITCH;
		logfont.lfItalic = italic;
		logfont.lfUnderline = underline;
		logfont.lfStrikeOut = strike_out;
		HFONT result = ::CreateFontIndirect(&logfont);

		if(result)
		{
			font_tag * impl = new font_tag;
			impl->name = logfont.lfFaceName;
			impl->height = height;
			impl->weight = weight;
			impl->italic = italic;
			impl->underline = underline;
			impl->strikeout = strike_out;
			impl->handle = result;
			return std::shared_ptr<font_tag>(impl, font_tag::deleter());
		}
		return nullptr;
	}

	platform_spec& platform_spec::instance()
	{
		static platform_spec object;
		return object;
	}

	void platform_spec::keep_window_icon(native_window_type wd, const paint::image& sml_icon, const paint::image& big_icon)
	{
		auto & icons = iconbase_[wd];
		icons.sml_icon = sml_icon;
		icons.big_icon = big_icon;
	}

	void platform_spec::release_window_icon(native_window_type wd)
	{
		iconbase_.erase(wd);
	}
}//end namespace detail
}//end namespace nana

#endif //NANA_WINDOWS
