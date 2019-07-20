/*
 *	Paint Graphics Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/paint/graphics.cpp
 */

#include "../detail/platform_spec_selector.hpp"
#include <nana/gui/detail/bedrock.hpp>
#include <nana/paint/graphics.hpp>
#include <nana/paint/detail/native_paint_interface.hpp>
#include <nana/paint/pixel_buffer.hpp>
#include <nana/gui/layout_utility.hpp>
#include <nana/unicode_bidi.hpp>
#include <algorithm>
#if defined(NANA_WINDOWS)
	#include <windows.h>
#elif defined(NANA_X11)
	#include <X11/Xlib.h>
#endif

#include "../detail/platform_abstraction.hpp"

namespace nana
{
	//Forward-declarations
	//These names are defined platform_abstraction.cpp
	std::unique_ptr<unsigned[]> nana_xft_glyph_pixels(font_interface*, const wchar_t* str, std::size_t len);

	namespace detail
	{
		font_style::font_style(unsigned weight, bool italic, bool underline, bool strike_out) :
			weight(weight),
			italic(italic),
			underline(underline),
			strike_out(strike_out)
		{}
	}
namespace paint
{
	namespace detail
	{
		struct drawable_deleter
		{
			void operator()(const drawable_type p) const
			{
#if defined(NANA_WINDOWS)
				delete p;
#elif defined(NANA_X11)
				if(p)
				{
					Display* disp = reinterpret_cast<Display*>(nana::detail::platform_spec::instance().open_display());
	#if defined(NANA_USE_XFT)
					::XftDrawDestroy(p->xftdraw);
	#endif
					::XFreeGC(disp, p->context);
					::XFreePixmap(disp, p->pixmap);
					delete p;
				}
#endif
			}
		};
		//end struct graphics_handle_deleter
	}//end namespace detail

	//class font
		struct font::impl_type
		{
			std::shared_ptr<font_interface> real_font;
		};

		font::font()
			: impl_(new impl_type)
		{
			impl_->real_font = platform_abstraction::default_font(nullptr);
		}

		font::font(drawable_type dw)
			: impl_(new impl_type)
		{
			impl_->real_font = dw->font;
		}

		font::font(const font& rhs)
			: impl_(new impl_type)
		{
			if(rhs.impl_)
				impl_->real_font = rhs.impl_->real_font;
		}

		font::font(const std::string& font_family, double size_pt, const font_style& fs):
			impl_(new impl_type)
		{
			impl_->real_font = platform_abstraction::make_font(font_family, size_pt, fs);
		}


		font::font(double size_pt, const path_type& truetype, const font_style& fs) :
			impl_(new impl_type)
		{
			impl_->real_font = platform_abstraction::make_font_from_ttf(truetype, size_pt, fs);
		}

		font::~font()
		{
			delete impl_;
		}

		bool font::empty() const
		{
			return ((nullptr == impl_) || (nullptr == impl_->real_font));
		}

		void font::set_default() const
		{
			if(empty())
				return;

			platform_abstraction::default_font(impl_->real_font);
		}

		std::string font::name() const
		{
			if(empty()) return std::string();

			return impl_->real_font->family();
		}

		double font::size(bool fixed) const
		{
			double size_pt = (empty() ? 0.0 : impl_->real_font->size());

			if (fixed && (0.0 == size_pt))
				return platform_abstraction::font_default_pt();

			return size_pt;
		}

		bool font::bold() const
		{
			if(empty()) return false;
			return (impl_->real_font->style().weight >= 700);
		}

		unsigned font::weight() const
		{
			if(empty()) return 0;
			return impl_->real_font->style().weight;
		}

		bool font::italic() const
		{
			if(empty()) return false;
			return impl_->real_font->style().italic;
		}

		bool font::underline() const
		{
			if(empty()) return false;
			return impl_->real_font->style().underline;
		}

		bool font::strikeout() const
		{
			if(empty()) return false;
			return impl_->real_font->style().strike_out;
		}

		native_font_type font::handle() const
		{
			if(empty())	return nullptr;
			return impl_->real_font->native_handle();
		}

		void font::release()
		{
			if(impl_)
				impl_->real_font.reset();
		}

		font & font::operator=(const font& rhs)
		{
			if(impl_ && rhs.impl_ && (this != &rhs))
				impl_->real_font = rhs.impl_->real_font;

			return *this;
		}

		bool font::operator==(const font& rhs) const
		{
			if(empty() == rhs.empty())
				return (empty() || (impl_->real_font == rhs.impl_->real_font));

			return false;
		}

		bool font::operator!=(const font& rhs) const
		{
			if(empty() == rhs.empty())
				return ((empty() == false) && (impl_->real_font != rhs.impl_->real_font));

			return true;
		}
	//end class font

	//class graphics
		struct graphics::implementation
		{
			std::shared_ptr<::nana::detail::drawable_impl_type> platform_drawable;
			font			font_shadow;
			drawable_type	handle{ nullptr };
			::nana::size	size;
			pixel_buffer	pxbuf;
			bool changed{ false };
		};

		graphics::graphics()
			: impl_(new implementation)
		{}

		graphics::graphics(const nana::size& sz)
			: impl_(new implementation)
		{
			make(sz);
		}

		graphics::graphics(const graphics& rhs)
			: impl_(new implementation(*rhs.impl_))
		{}

		graphics& graphics::operator=(const graphics& rhs)
		{
			if(this != &rhs)
			{
				*impl_ = *rhs.impl_;
				impl_->changed = true;
			}
			return *this;
		}


		graphics::graphics(graphics&& other)
			: impl_(std::move(other.impl_))
		{
			other.impl_.reset(new implementation);
		}

		graphics& graphics::operator=(graphics&& other)
		{
			if (this != &other)
			{
				impl_ = std::move(other.impl_);
				other.impl_.reset(new implementation);
			}

			return *this;
		}

		graphics::~graphics()
		{
			//For instance of unique_ptr pimpl
		}

		bool graphics::changed() const
		{
			return impl_->changed;
		}

		bool graphics::empty() const
		{
			return (!impl_->handle);
		}

		graphics::operator bool() const noexcept
		{
			return (impl_->handle != nullptr);
		}

		drawable_type graphics::handle() const
		{
			return impl_->handle;
		}

		const void* graphics::pixmap() const
		{
			//The reinterpret_cast is used for same platform. Under Windows, the type
			//of pixmap can be converted into void* directly, but under X11, the type is not a pointer.
			return (impl_->handle ? reinterpret_cast<void*>(impl_->handle->pixmap) : nullptr);
		}

		const void* graphics::context() const
		{
			return (impl_->handle ? impl_->handle->context : nullptr);
		}

		void graphics::swap(graphics& other) noexcept
		{
			if (context() != other.context())
				impl_.swap(other.impl_);
		}

		void graphics::make(const ::nana::size& sz)
		{
			if (impl_->handle == nullptr || impl_->size != sz)
			{
				if (sz.empty())
				{
					release();
					return;
				}

				//The object will be deleted while dwptr_ is performing a release.
				std::shared_ptr<nana::detail::drawable_impl_type> dw{ new nana::detail::drawable_impl_type, detail::drawable_deleter{} };

				//Reuse the old font
				if (impl_->platform_drawable)
				{
					drawable_type reuse = impl_->platform_drawable.get();
					dw->font = reuse->font;
					dw->string.tab_length = reuse->string.tab_length;
				}
				else
					dw->font = impl_->font_shadow.impl_->real_font;

#if defined(NANA_WINDOWS)
				HDC hdc = ::GetDC(nullptr);
				HDC cdc = ::CreateCompatibleDC(hdc);
				if (nullptr == cdc)
				{
					::ReleaseDC(nullptr, hdc);
					throw std::bad_alloc{};
				}

				BITMAPINFO bmi;
				bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
				bmi.bmiHeader.biWidth = sz.width;
				bmi.bmiHeader.biHeight = -static_cast<int>(sz.height);
				bmi.bmiHeader.biPlanes = 1;
				bmi.bmiHeader.biBitCount = 32;         // four 8-bit components
				bmi.bmiHeader.biCompression = BI_RGB;
				bmi.bmiHeader.biSizeImage = (sz.width * sz.height) << 2;

				HBITMAP bmp = ::CreateDIBSection(cdc, &bmi, DIB_RGB_COLORS, reinterpret_cast<void**>(&(dw->pixbuf_ptr)), 0, 0);

				if (nullptr == bmp)
				{
					::DeleteDC(cdc);
					::ReleaseDC(nullptr, hdc);
					throw std::bad_alloc{};
				}

				::DeleteObject((HBITMAP)::SelectObject(cdc, bmp));
				::DeleteObject(::SelectObject(cdc, dw->font->native_handle()));

				dw->context = cdc;
				dw->pixmap = bmp;
				::SetBkMode(cdc, TRANSPARENT);

				::ReleaseDC(0, hdc);
#elif defined(NANA_X11)
				auto & spec = nana::detail::platform_spec::instance();
				{
					nana::detail::platform_scope_guard psg;

					spec.set_error_handler();

					Display* disp = spec.open_display();
					int screen = DefaultScreen(disp);
					Window root = ::XRootWindow(disp, screen);
					auto pixmap = ::XCreatePixmap(disp, root, sz.width, sz.height, DefaultDepth(disp, screen));
					if(spec.error_code)
					{
						spec.rev_error_handler();
						throw std::bad_alloc();
					}
					auto context = ::XCreateGC(disp, pixmap, 0, 0);
					if (spec.error_code)
					{
						::XFreePixmap(disp, pixmap);
						spec.rev_error_handler();
						throw std::bad_alloc();
					}
#	if defined(NANA_USE_XFT)
					auto xftdraw = ::XftDrawCreate(disp, pixmap, spec.screen_visual(), spec.colormap());
					if (spec.error_code)
					{
						::XFreeGC(disp, context);
						::XFreePixmap(disp, pixmap);

						spec.rev_error_handler();
						throw std::bad_alloc();
					}

					dw->xftdraw = xftdraw;
#	endif
					dw->pixmap = pixmap;
					dw->context = context;
				}
#endif
				if(dw)
				{
					dw->set_color(colors::black);
					dw->set_text_color(colors::black);
#if defined(NANA_WINDOWS)
					dw->bytes_per_line = sz.width * sizeof(pixel_argb_t);
#else
					dw->update_text_color();
#endif
					impl_->platform_drawable = dw;
					impl_->handle = dw.get();
					impl_->size = sz;

					impl_->handle->string.tab_pixels = detail::real_text_extent_size(impl_->handle, L"\t", 1).width;
					impl_->handle->string.whitespace_pixels = detail::real_text_extent_size(impl_->handle, L" ", 1).width;
				}
			}

			if(impl_->changed == false)
				impl_->changed = true;
		}

		void graphics::resize(const ::nana::size& sz)
		{
			graphics duplicate(std::move(*this));
			make(sz);
			bitblt(0, 0, duplicate);
		}

		void graphics::typeface(const font& f)
		{
			//Keep the font as a shadow, even if the graphics is empty. Setting the font is futile when the size
			//of a widget is zero.
			impl_->font_shadow = f;
			if(impl_->handle && (false == f.empty()))
			{
				impl_->handle->font = f.impl_->real_font;
#if defined(NANA_WINDOWS)
				::SelectObject(impl_->handle->context, reinterpret_cast<HFONT>(f.impl_->real_font->native_handle()));
#endif

				impl_->handle->string.tab_pixels = detail::real_text_extent_size(impl_->handle, L"\t", 1).width;
				impl_->handle->string.whitespace_pixels = detail::real_text_extent_size(impl_->handle, L" ", 1).width;

				if (impl_->changed == false)
					impl_->changed = true;
			}
		}

		font graphics::typeface() const
		{
			//The font may be set when the graphics is still empty.
			//it should return the shadow font when the graphics is empty.
			return (impl_->handle ? font(impl_->handle) : impl_->font_shadow);
		}

#ifdef _nana_std_has_string_view
		size graphics::text_extent_size(std::string_view text) const
		{
			throw_not_utf8(text);
			return detail::text_extent_size(impl_->handle, text.data(), text.length());
		}

		size graphics::text_extent_size(std::wstring_view text) const
		{
			return detail::text_extent_size(impl_->handle, text.data(), text.length());
		}
		
		nana::size graphics::glyph_extent_size(std::wstring_view text, std::size_t begin, std::size_t end) const
		{
			end = std::clamp(end, static_cast<std::size_t>(0), static_cast<std::size_t>(text.size()));

			if (nullptr == impl_->handle || text.empty() || begin >= end) return{};

			nana::size sz;
#if defined(NANA_WINDOWS)
			int * dx = new int[text.size()];

			SIZE extents;
			::GetTextExtentExPoint(impl_->handle->context, text.data(), static_cast<int>(text.size()), 0, 0, dx, &extents);
			sz.width = dx[end - 1] - (begin ? dx[begin - 1] : 0);
			unsigned tab_pixels = impl_->handle->string.tab_length * impl_->handle->string.whitespace_pixels;
			const wchar_t * pend = text.data() + end;
			for (const wchar_t * p = text.data() + begin; p != pend; ++p)
			{
				if (*p == '\t')
					sz.width += tab_pixels;
			}
			sz.height = extents.cy;
			delete[] dx;
#elif defined(NANA_X11)
			sz = text_extent_size(text.substr(begin, end - begin));
#endif
			return sz;
		}

		std::unique_ptr<unsigned[]> graphics::glyph_pixels(std::wstring_view text) const
		{
			if (nullptr == impl_->handle || nullptr == impl_->handle->context) return {};

			auto pxbuf = std::unique_ptr<unsigned[]>{ new unsigned[text.size() ? text.size() : 1] };

			if (!text.empty())
			{
#if defined(NANA_WINDOWS)
				unsigned tab_pixels = impl_->handle->string.tab_length * impl_->handle->string.whitespace_pixels;
				int * dx = new int[text.size()];
				SIZE extents;
				::GetTextExtentExPoint(impl_->handle->context, text.data(), static_cast<int>(text.size()), 0, 0, dx, &extents);

				pxbuf[0] = (text[0] == '\t' ? tab_pixels : dx[0]);

				for (std::size_t i = 1; i < text.size(); ++i)
				{
					pxbuf[i] = (text[i] == '\t' ? tab_pixels : dx[i] - dx[i - 1]);
				}
				delete[] dx;
#elif defined(NANA_X11) && defined(NANA_USE_XFT)
				return nana_xft_glyph_pixels(impl_->handle->font.get(), text.data(), text.size());
#endif
			}
			return pxbuf;
		}

		::nana::size graphics::bidi_extent_size(std::string_view utf8str) const
		{
			return bidi_extent_size(to_wstring(utf8str));
		}

		nana::size	graphics::bidi_extent_size(std::wstring_view text) const
		{
			nana::size sz;
			if (impl_->handle && impl_->handle->context && text.size())
			{
				auto const reordered = unicode_reorder(text.data(), text.size());
				for (auto & i : reordered)
				{
#ifdef _nana_std_has_string_view
					nana::size t = text_extent_size(std::wstring_view(i.begin, i.end - i.begin));
#else
					nana::size t = text_extent_size(i.begin, i.end - i.begin);
#endif
					sz.width += t.width;
					if (sz.height < t.height)
						sz.height = t.height;
				}
			}
			return sz;
		}
#else
		::nana::size graphics::text_extent_size(const ::std::string& text) const
		{
			throw_not_utf8(text);
			return text_extent_size(to_wstring(text));
		}

		::nana::size graphics::text_extent_size(const char* text, std::size_t len) const
		{
			return text_extent_size(std::string(text, text + len));
		}

		nana::size	graphics::text_extent_size(const wchar_t* text)	const
		{
			return text_extent_size(text, std::wcslen(text));
		}

		nana::size	graphics::text_extent_size(const std::wstring& text)	const
		{
			return text_extent_size(text.c_str(), static_cast<unsigned>(text.length()));
		}

		nana::size	graphics::text_extent_size(const wchar_t* str, std::size_t len)	const
		{
			return detail::text_extent_size(impl_->handle, str, len);
		}

		nana::size	graphics::text_extent_size(const std::wstring& str, std::size_t len)	const
		{
			return detail::text_extent_size(impl_->handle, str.c_str(), len);
		}

		nana::size graphics::glyph_extent_size(const wchar_t * str, std::size_t len, std::size_t begin, std::size_t end) const
		{
			if (len < end) end = len;
			if (nullptr == impl_->handle || nullptr == str || 0 == len || begin >= end) return{};

			nana::size sz;
#if defined(NANA_WINDOWS)
			int * dx = new int[len];
			SIZE extents;
			::GetTextExtentExPoint(impl_->handle->context, str, static_cast<int>(len), 0, 0, dx, &extents);
			sz.width = dx[end - 1] - (begin ? dx[begin - 1] : 0);
			unsigned tab_pixels = impl_->handle->string.tab_length * impl_->handle->string.whitespace_pixels;
			const wchar_t * pend = str + end;
			for (const wchar_t * p = str + begin; p != pend; ++p)
			{
				if (*p == '\t')
					sz.width += tab_pixels;
			}
			sz.height = extents.cy;
			delete[] dx;
#elif defined(NANA_X11)
			sz = text_extent_size(str + begin, end - begin);
#endif
			return sz;
		}

		nana::size graphics::glyph_extent_size(const std::wstring& str, std::size_t len, std::size_t begin, std::size_t end) const
		{
			return glyph_extent_size(str.c_str(), len, begin, end);
		}

		bool graphics::glyph_pixels(const wchar_t * str, std::size_t len, unsigned* pxbuf) const
		{
			if (nullptr == impl_->handle || nullptr == impl_->handle->context || nullptr == str || nullptr == pxbuf) return false;
			if (len == 0) return true;

			unsigned tab_pixels = impl_->handle->string.tab_length * impl_->handle->string.whitespace_pixels;
#if defined(NANA_WINDOWS)
			int * dx = new int[len];
			SIZE extents;
			::GetTextExtentExPoint(impl_->handle->context, str, static_cast<int>(len), 0, 0, dx, &extents);

			pxbuf[0] = (str[0] == '\t' ? tab_pixels : dx[0]);

			for (std::size_t i = 1; i < len; ++i)
			{
				pxbuf[i] = (str[i] == '\t' ? tab_pixels : dx[i] - dx[i - 1]);
			}
			delete[] dx;
#elif defined(NANA_X11) && defined(NANA_USE_XFT)

			auto disp = nana::detail::platform_spec::instance().open_display();
			auto xft = reinterpret_cast<XftFont*>(impl_->handle->font->native_handle());

			XGlyphInfo extents;
			for (std::size_t i = 0; i < len; ++i)
			{
				if (str[i] != '\t')
				{
					FT_UInt glyphs = ::XftCharIndex(disp, xft, str[i]);
					::XftGlyphExtents(disp, xft, &glyphs, 1, &extents);
					pxbuf[i] = extents.xOff;
				}
				else
					pxbuf[i] = tab_pixels;
			}
#endif
			return true;
		}

		nana::size	graphics::bidi_extent_size(const std::wstring& str) const
		{
			nana::size sz;
			if (impl_->handle && impl_->handle->context && str.size())
			{
				auto const reordered = unicode_reorder(str.c_str(), str.size());
				for (auto & i : reordered)
				{
#ifdef _nana_std_has_string_view
					nana::size t = text_extent_size(std::wstring_view(i.begin, i.end - i.begin));
#else
					nana::size t = text_extent_size(i.begin, i.end - i.begin);
#endif
					sz.width += t.width;
					if (sz.height < t.height)
						sz.height = t.height;
				}
			}
			return sz;
		}

		::nana::size graphics::bidi_extent_size(const std::string& str) const
		{
			return bidi_extent_size(static_cast<std::wstring>(::nana::charset(str, ::nana::unicode::utf8)));
		}
#endif	//end _nana_std_has_string_view

		bool graphics::text_metrics(unsigned & ascent, unsigned& descent, unsigned& internal_leading) const
		{
			if(impl_->handle)
			{
#if defined(NANA_WINDOWS)
				::TEXTMETRIC tm;
				::GetTextMetrics(impl_->handle->context, &tm);
				ascent = static_cast<unsigned>(tm.tmAscent);
				descent = static_cast<unsigned>(tm.tmDescent);
				internal_leading = static_cast<unsigned>(tm.tmInternalLeading);
				return true;
#elif defined(NANA_X11)
				if(impl_->handle->font)
				{
	#if defined(NANA_USE_XFT)
					auto fs = reinterpret_cast<XftFont*>(impl_->handle->font->native_handle());
					ascent = fs->ascent;
					descent = fs->descent;
					internal_leading = 0;
	#else
					auto fs = reinterpret_cast<XFontSet>(impl_->handle->font->native_handle());
					XFontSetExtents * ext = ::XExtentsOfFontSet(fs);
					XFontStruct ** fontstructs;
					char ** font_names;
					int size = ::XFontsOfFontSet(fs, &fontstructs, &font_names);
					ascent = 0;
					descent = 0;
					XFontStruct **fontstructs_end = fontstructs + size;
					for(XFontStruct** i = fontstructs; i < fontstructs_end; ++i)
					{
						if(ascent < (*i)->ascent)
							ascent = (*i)->ascent;
						if(descent < (*i)->descent)
							descent = (*i)->descent;
					}
	#endif
					return true;
				}
#endif
			}
			return false;
		}

		void graphics::line_begin(int x, int y)
		{
			if(!impl_->handle)	return;
#if defined(NANA_WINDOWS)
			::MoveToEx(impl_->handle->context, x, y, 0);

#elif defined(NANA_X11)
			impl_->handle->line_begin_pos.x = x;
			impl_->handle->line_begin_pos.y = y;
#endif
		}

		void graphics::bitblt(int x, int y, const graphics& src)
		{
			nana::rectangle r(src.size());
			r.x = x;
			r.y = y;
			bitblt(r, src);
		}

		void graphics::bitblt(const nana::rectangle& r_dst, native_window_type src)
		{
			if(impl_->handle)
			{
#if defined(NANA_WINDOWS)
				HDC dc = ::GetDC(reinterpret_cast<HWND>(src));
				::BitBlt(impl_->handle->context, r_dst.x, r_dst.y, r_dst.width, r_dst.height, dc, 0, 0, SRCCOPY);
				::ReleaseDC(reinterpret_cast<HWND>(src), dc);
#elif defined(NANA_X11)
				::XCopyArea(nana::detail::platform_spec::instance().open_display(),
						reinterpret_cast<Window>(src), impl_->handle->pixmap, impl_->handle->context,
						0, 0, r_dst.width, r_dst.height, r_dst.x, r_dst.y);
#endif
				if(impl_->changed == false) impl_->changed = true;
			}
		}

		void graphics::bitblt(const nana::rectangle& r_dst, native_window_type src, const nana::point& p_src)
		{
			if(impl_->handle)
			{
#if defined(NANA_WINDOWS)
				HDC dc = ::GetDC(reinterpret_cast<HWND>(src));
				::BitBlt(impl_->handle->context, r_dst.x, r_dst.y, r_dst.width, r_dst.height, dc, p_src.x, p_src.y, SRCCOPY);
				::ReleaseDC(reinterpret_cast<HWND>(src), dc);
#elif defined(NANA_X11)
				::XCopyArea(nana::detail::platform_spec::instance().open_display(),
						reinterpret_cast<Window>(src), impl_->handle->pixmap, impl_->handle->context,
						p_src.x, p_src.y, r_dst.width, r_dst.height, r_dst.x, r_dst.y);
#endif
				if (impl_->changed == false) impl_->changed = true;
			}
		}

		void graphics::bitblt(const nana::rectangle& r_dst, const graphics& src)
		{
			if(impl_->handle && src.impl_->handle)
			{
#if defined(NANA_WINDOWS)
				::BitBlt(impl_->handle->context, r_dst.x, r_dst.y, r_dst.width, r_dst.height, src.impl_->handle->context, 0, 0, SRCCOPY);
#elif defined(NANA_X11)
				::XCopyArea(nana::detail::platform_spec::instance().open_display(),
						src.impl_->handle->pixmap, impl_->handle->pixmap, impl_->handle->context,
						0, 0, r_dst.width, r_dst.height, r_dst.x, r_dst.y);
#endif
				if (impl_->changed == false) impl_->changed = true;
			}
		}

		void graphics::bitblt(const nana::rectangle& r_dst, const graphics& src, const nana::point& p_src)
		{
			if(impl_->handle && src.impl_->handle)
			{
#if defined(NANA_WINDOWS)
				::BitBlt(impl_->handle->context, r_dst.x, r_dst.y, r_dst.width, r_dst.height, src.impl_->handle->context, p_src.x, p_src.y, SRCCOPY);
#elif defined(NANA_X11)
				::XCopyArea(nana::detail::platform_spec::instance().open_display(),
						src.impl_->handle->pixmap, impl_->handle->pixmap, impl_->handle->context,
						p_src.x, p_src.y, r_dst.width, r_dst.height, r_dst.x, r_dst.y);
#endif
				if (impl_->changed == false) impl_->changed = true;
			}
		}

		void graphics::blend(const nana::rectangle& r, const ::nana::color& clr, double fade_rate)
		{
			if (impl_->handle)
			{
				nana::paint::detail::blend(impl_->handle, r, clr.px_color(), fade_rate);
				if (impl_->changed == false) impl_->changed = true;
			}
		}

		void graphics::blend(const ::nana::rectangle& blend_r, const graphics& graph, const point& blend_graph_point, double fade_rate)///< blends with the blend_graph.
		{
			if (graph.impl_->handle && impl_->handle && (graph.impl_->handle != impl_->handle))
			{
				pixel_buffer graph_px;

				nana::rectangle r{ blend_graph_point, blend_r.dimension() };

				graph_px.attach(graph.impl_->handle, r);
				graph_px.blend(r, impl_->handle, blend_r.position(), 1 - fade_rate);

				if (graph.impl_->changed == false) graph.impl_->changed = true;
			}
		}

		void graphics::blur(const nana::rectangle& r, std::size_t radius)
		{
			if(impl_->handle)
			{
				pixel_buffer pixbuf(impl_->handle, 0, 0);
				pixbuf.blur(r, radius);
				pixbuf.paste(impl_->handle, point{});
			}
		}

		void graphics::rgb_to_wb()
		{
			if(impl_->handle)
			{
				//Create the color table for performance
				float* tablebuf = new float[0x100 * 3];
				float* table_red = tablebuf;
				float* table_green = tablebuf + 0x100;
				float* table_blue = tablebuf + 0x200;

				for(int i = 0; i < 0x100; ++i)
				{
					table_red[i] = (i * 0.3f);
					table_green[i] = (i * 0.59f);
					table_blue[i] = (i * 0.11f);
				}

				pixel_buffer pixbuf(impl_->handle, 0, 0);

				auto pixels = pixbuf.raw_ptr(0);

				const nana::size sz = paint::detail::drawable_size(impl_->handle);
				const int rest = sz.width % 4;
				const int length_align4 = sz.width - rest;

				for(unsigned y = 0; y < sz.height; ++y)
				{
					const auto end = pixels + length_align4;
					for(; pixels < end; pixels += 4)
					{
						unsigned char gray = static_cast<unsigned char>(table_red[pixels[0].element.red] + table_green[pixels[0].element.green] + table_blue[pixels[0].element.blue] + 0.5f);
						pixels[0].value = gray << 16 | gray << 8| gray;

						gray = static_cast<unsigned char>(table_red[pixels[1].element.red] + table_green[pixels[1].element.green] + table_blue[pixels[1].element.blue] + 0.5f);
						pixels[1].value = gray << 16 | gray << 8 | gray;

						gray = static_cast<unsigned char>(table_red[pixels[2].element.red] + table_green[pixels[2].element.green] + table_blue[pixels[2].element.blue] + 0.5f);
						pixels[2].value = gray << 16 | gray << 8 | gray;

						gray = static_cast<unsigned char>(table_red[pixels[3].element.red] + table_green[pixels[3].element.green] + table_blue[pixels[3].element.blue] + 0.5f);
						pixels[3].value = gray << 16 | gray << 8 | gray;
					}

					for(int i = 0; i < rest; ++i)
					{
						unsigned char gray = static_cast<unsigned char>(table_red[pixels[i].element.red] + table_green[pixels[i].element.green] + table_blue[pixels[i].element.blue] + 0.5f);
						pixels[i].element.red = gray;
						pixels[i].element.green = gray;
						pixels[i].element.blue = gray;
					}

					pixels += rest;
				}
				delete [] tablebuf;

				pixbuf.paste(impl_->handle, point{});
				if (impl_->changed == false) impl_->changed = true;
			}
		}

		void graphics::paste(graphics& dst, int x, int y) const
		{
			if(impl_->handle && dst.impl_->handle && impl_->handle != dst.impl_->handle)
			{
#if defined(NANA_WINDOWS)
				::BitBlt(dst.impl_->handle->context, x, y, impl_->size.width, impl_->size.height, impl_->handle->context, 0, 0, SRCCOPY);
#elif defined(NANA_X11)
				Display* display = nana::detail::platform_spec::instance().open_display();
				::XCopyArea(display,
					impl_->handle->pixmap, dst.impl_->handle->pixmap, impl_->handle->context,
						0, 0, impl_->size.width, impl_->size.height, x, y);

				::XFlush(display);
#endif
				dst.impl_->changed = true;
			}
		}

		void graphics::paste(native_window_type dst, const nana::rectangle& r, int x, int y) const
		{
			paste(dst, r.x, r.y, r.width, r.height, x, y);
		}

		void graphics::paste(native_window_type dst, int dx, int dy, unsigned width, unsigned height, int sx, int sy) const
		{
			if(impl_->handle)
			{
#if defined(NANA_WINDOWS)
				HDC dc = ::GetDC(reinterpret_cast<HWND>(dst));
				if(dc)
				{
					::BitBlt(dc, dx, dy, width, height, impl_->handle->context, sx, sy, SRCCOPY);
					::ReleaseDC(reinterpret_cast<HWND>(dst), dc);
				}
#elif defined(NANA_X11)
				auto & spec = nana::detail::platform_spec::instance();

				Display * display = spec.open_display();

				nana::detail::platform_scope_guard lock;

				::XCopyArea(display,
					impl_->handle->pixmap, reinterpret_cast<Window>(dst), impl_->handle->context,
						sx, sy, width, height, dx, dy);

				XWindowAttributes attr;
				spec.set_error_handler();
				::XGetWindowAttributes(display, reinterpret_cast<Window>(dst), &attr);
				if(BadWindow != spec.rev_error_handler() && attr.map_state != IsUnmapped)
					::XMapWindow(display, reinterpret_cast<Window>(dst));

				::XFlush(display);
#endif
			}
		}

		void graphics::paste(drawable_type dst, int x, int y) const
		{
			if(impl_->handle && dst && impl_->handle != dst)
			{
#if defined (NANA_WINDOWS)
				::BitBlt(dst->context, x, y, impl_->size.width, impl_->size.height, impl_->handle->context, 0, 0, SRCCOPY);
#elif defined(NANA_X11)
				Display * display = nana::detail::platform_spec::instance().open_display();
				::XCopyArea(display,
					impl_->handle->pixmap, dst->pixmap, impl_->handle->context,
						0, 0, impl_->size.width, impl_->size.height, x, y);
				::XFlush(display);
#endif
			}
		}


		void graphics::paste(const nana::rectangle& r_src, graphics& dst, int x, int y) const
		{
			if(impl_->handle && dst.impl_->handle && impl_->handle != dst.impl_->handle)
			{
#if defined(NANA_WINDOWS)
				::BitBlt(dst.impl_->handle->context, x, y, r_src.width, r_src.height, impl_->handle->context, r_src.x, r_src.y, SRCCOPY);
#elif defined(NANA_X11)
				Display* display = nana::detail::platform_spec::instance().open_display();
				::XCopyArea(display,
					impl_->handle->pixmap, dst.impl_->handle->pixmap, impl_->handle->context,
						r_src.x, r_src.y, r_src.width, r_src.height, x, y);

				::XFlush(display);
#endif
			}
		}

		void graphics::stretch(const nana::rectangle& src_r, graphics& dst, const nana::rectangle& r) const
		{
			if(impl_->handle && dst.impl_->handle && (impl_->handle != dst.impl_->handle))
			{
				pixel_buffer pixbuf(impl_->handle, 0, 0);
				pixbuf.stretch(src_r, dst.impl_->handle, r);
			}
		}

		void graphics::stretch(graphics& dst, const nana::rectangle & r) const
		{
			stretch(nana::rectangle(size()), dst, r);
		}

		void graphics::flush()
		{
#if defined(NANA_WINDOWS)
			::GdiFlush();
#endif
		}

		unsigned graphics::width() const{
			return impl_->size.width;
		}

		unsigned graphics::height() const{
			return impl_->size.height;
		}

		nana::size graphics::size() const{
			return this->impl_->size;
		}

		void graphics::setsta()
		{
			impl_->changed = false;
		}

		void graphics::set_changed()
		{
			impl_->changed = true;
		}

		void graphics::release()
		{
			impl_->platform_drawable.reset();
			impl_->handle = nullptr;
			impl_->size.width = impl_->size.height = 0;
		}

		void graphics::save_as_file(const char* file_utf8) const noexcept
		{
			if(impl_->handle)
			{
#if defined(NANA_WINDOWS)
				const int iWidth = static_cast<int>(impl_->size.width);
				const int iHeight = static_cast<int>(impl_->size.height);
				BITMAPINFO bmpInfo = {};
				bmpInfo.bmiHeader.biSize = sizeof(bmpInfo.bmiHeader);
				bmpInfo.bmiHeader.biWidth = iWidth;
				bmpInfo.bmiHeader.biHeight = iHeight;
				bmpInfo.bmiHeader.biPlanes = 1;
				bmpInfo.bmiHeader.biBitCount = 24;

				const size_t lineBytes = ((bmpInfo.bmiHeader.biWidth * 3) + 3) & (~3);
				const size_t imageBytes = iHeight * lineBytes;

				HDC hdcMem = ::CreateCompatibleDC(impl_->handle->context);
				BYTE *pData = nullptr;
				HBITMAP hBmp = ::CreateDIBSection(hdcMem, &bmpInfo, DIB_RGB_COLORS, reinterpret_cast<void**>(&pData), 0, 0);

				::SelectObject(hdcMem, hBmp);

				BitBlt(hdcMem, 0, 0, iWidth, iHeight, impl_->handle->context, 0, 0, SRCCOPY);

				BITMAPFILEHEADER bmFileHeader = { 0 };
				bmFileHeader.bfType = 0x4d42;  //bmp
				bmFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
				bmFileHeader.bfSize = bmFileHeader.bfOffBits + static_cast<DWORD>(imageBytes);

				HANDLE hFile = ::CreateFileW(static_cast<std::wstring>(::nana::charset(file_utf8, ::nana::unicode::utf8)).data(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
				DWORD dwWrite = 0;
				::WriteFile(hFile, &bmFileHeader, sizeof(BITMAPFILEHEADER), &dwWrite, nullptr);
				::WriteFile(hFile, &bmpInfo.bmiHeader, sizeof(BITMAPINFOHEADER), &dwWrite, nullptr);
				::WriteFile(hFile, pData, static_cast<DWORD>(imageBytes), &dwWrite, nullptr);
				::CloseHandle(hFile);

				::DeleteObject(hBmp);
				::DeleteDC(hdcMem);
#elif defined(NANA_X11)
				static_cast<void>(file_utf8);	//eliminate unused parameter compil warning.
#endif
			}
		}

		::nana::color graphics::palette(bool for_text) const
		{
			if (impl_->handle)
				return static_cast<color_rgb>(for_text ? impl_->handle->fgcolor_rgb : impl_->handle->bgcolor_rgb);

			return{};
		}

		graphics& graphics::palette(bool for_text, const ::nana::color& clr)
		{
			if (impl_->handle)
			{
				if (for_text)
					impl_->handle->set_text_color(clr);
				else
					impl_->handle->set_color(clr);
			}

			return *this;
		}

		void graphics::set_pixel(int x, int y, const ::nana::color& clr)
		{
			if (impl_->handle)
			{
				impl_->handle->set_color(clr);
				set_pixel(x, y);
			}
		}

		void graphics::set_pixel(int x, int y)
		{
			if (impl_->handle)
			{
#if defined(NANA_WINDOWS)
				::SetPixel(impl_->handle->context, x, y, impl_->handle->bgcolor_native);
#elif defined(NANA_X11)
				Display* disp = nana::detail::platform_spec::instance().open_display();
				impl_->handle->update_color();
				::XDrawPoint(disp, impl_->handle->pixmap, impl_->handle->context, x, y);
#endif
				if (impl_->changed == false) impl_->changed = true;
			}
		}

#ifdef _nana_std_has_string_view
		unsigned graphics::bidi_string(const point& pos, std::string_view utf8str)
		{
			return bidi_string(pos, to_wstring(utf8str));
		}

		unsigned graphics::bidi_string(const nana::point& pos, std::wstring_view str)
		{
			auto moved_pos = pos;

			auto const reordered = unicode_reorder(str.data(), str.size());
			for (auto & i : reordered)
			{
				
#ifdef _nana_std_has_string_view
				this->string(moved_pos, std::wstring_view{ i.begin, static_cast<std::wstring_view::size_type>(i.end - i.begin) });
				moved_pos.x += static_cast<int>(text_extent_size(std::wstring_view(i.begin, i.end - i.begin)).width);
#else
				this->string(moved_pos, i.begin, i.end - i.begin);
				moved_pos.x += static_cast<int>(text_extent_size(i.begin, i.end - i.begin).width);
#endif
			}
			return static_cast<unsigned>(moved_pos.x - pos.x);
		}

		void graphics::string(const point& pos, std::string_view utf8str)
		{
			this->string(pos, to_wstring(utf8str));
		}

		void graphics::string(const point& pos, std::string_view utf8str, const nana::color& text_color)
		{
			palette(true, text_color);
			string(pos, utf8str);
		}

		void graphics::string(const nana::point& text_pos, std::wstring_view str)
		{
			if (impl_->handle && !str.empty())
			{
#if defined(NANA_POSIX)
				impl_->handle->update_text_color();
#endif
				auto begin = str.data();
				auto const end = begin + str.size();
				auto i = std::find(begin, end, '\t');

				if (i != end)
				{
					auto pos = text_pos;
					std::size_t tab_pixels = impl_->handle->string.tab_length * impl_->handle->string.tab_pixels;
					while (true)
					{
						auto len = i - begin;
						if (len)
						{
							//Render a part that does not contains a tab
							detail::draw_string(impl_->handle, pos, begin, len);
							pos.x += detail::real_text_extent_size(impl_->handle, begin, len).width;
						}

						begin = i;
						while (begin != end && (*begin == '\t'))
							++begin;

						if (begin != end)
						{
							//Now i_tab is not a tab, but a non-tab character following the previous tabs
							pos.x += static_cast<int>(tab_pixels * (begin - i));
							i = std::find(begin, end, '\t');
						}
						else
							break;
					}
				}
				else
					detail::draw_string(impl_->handle, text_pos, str.data(), str.size());
				if (impl_->changed == false) impl_->changed = true;
			}
		}

		void graphics::string(const point& pos, std::wstring_view str, const nana::color& text_color)
		{
			palette(true, text_color);
			string(pos, str);
		}
#else
		unsigned graphics::bidi_string(const nana::point& pos, const wchar_t * str, std::size_t len)
		{
			auto moved_pos = pos;

			auto const reordered = unicode_reorder(str, len);
			for (auto & i : reordered)
			{
				string(moved_pos, i.begin, i.end - i.begin);
#ifdef _nana_std_has_string_view
				moved_pos.x += static_cast<int>(text_extent_size(std::wstring_view(i.begin, i.end - i.begin)).width);
#else
				moved_pos.x += static_cast<int>(text_extent_size(i.begin, i.end - i.begin).width);
#endif
			}
			return static_cast<unsigned>(moved_pos.x - pos.x);
		}

		unsigned graphics::bidi_string(const point& pos, const char* str, std::size_t len)
		{
			std::wstring wstr = ::nana::charset(std::string(str, str + len), ::nana::unicode::utf8);
			return bidi_string(pos, wstr.data(), wstr.size());
		}

		void graphics::string(const point& pos, const std::string& text_utf8)
		{
			string(pos, to_wstring(text_utf8));
		}

		void graphics::string(const point& pos, const std::string& text_utf8, const color& clr)
		{
			palette(true, clr);
			string(pos, text_utf8);
		}

		void graphics::string(nana::point pos, const wchar_t* str, std::size_t len)
		{
			if (impl_->handle && str && len)
			{
				auto const end = str + len;
				auto i = std::find(str, end, '\t');
#if defined(NANA_POSIX)
				impl_->handle->update_text_color();
#endif
				if (i != end)
				{
					std::size_t tab_pixels = impl_->handle->string.tab_length * impl_->handle->string.tab_pixels;
					while (true)
					{
						len = i - str;
						if (len)
						{
							//Render a part that does not contains a tab
							detail::draw_string(impl_->handle, pos, str, len);
							pos.x += detail::real_text_extent_size(impl_->handle, str, len).width;
						}

						str = i;
						while (str != end && (*str == '\t'))
							++str;

						if (str != end)
						{
							//Now i_tab is not a tab, but a non-tab character following the previous tabs
							pos.x += static_cast<int>(tab_pixels * (str - i));
							i = std::find(str, end, '\t');
						}
						else
							break;
					}
				}
				else
					detail::draw_string(impl_->handle, pos, str, len);
				if (impl_->changed == false) impl_->changed = true;
			}
		}

		void graphics::string(const nana::point& pos, const wchar_t* str)
		{
			string(pos, str, std::wcslen(str));
		}

		void graphics::string(const nana::point& pos, const std::wstring& str)
		{
			string(pos, str.data(), str.size());
		}

		void graphics::string(const point& pos, const ::std::wstring& text, const color& clr)
		{
			palette(true, clr);
			string(pos, text.data(), text.size());
		}
#endif //_nana_std_has_string_view

		void graphics::line(const nana::point& pos1, const nana::point& pos2)
		{
			if (!impl_->handle)	return;
#if defined(NANA_WINDOWS)
			if (pos1 != pos2)
			{
				auto prv_pen = ::SelectObject(impl_->handle->context, ::CreatePen(PS_SOLID, 1, impl_->handle->bgcolor_native));

				::MoveToEx(impl_->handle->context, pos1.x, pos1.y, 0);
				::LineTo(impl_->handle->context, pos2.x, pos2.y);

				::DeleteObject(::SelectObject(impl_->handle->context, prv_pen));
			}
			::SetPixel(impl_->handle->context, pos2.x, pos2.y, impl_->handle->bgcolor_native);
#elif defined(NANA_X11)
			Display* disp = nana::detail::platform_spec::instance().open_display();
			impl_->handle->update_color();
			::XDrawLine(disp, impl_->handle->pixmap, impl_->handle->context, pos1.x, pos1.y, pos2.x, pos2.y);
#endif
			if (impl_->changed == false) impl_->changed = true;
		}

		void graphics::line(const point& pos_a, const point& pos_b, const color& clr)
		{
			palette(false, clr);
			line(pos_a, pos_b);
		}

		void graphics::line_to(const point& pos, const color& clr)
		{
			if (!impl_->handle) return;
			impl_->handle->set_color(clr);
			line_to(pos);
		}

		void graphics::line_to(const point& pos)
		{
			if (!impl_->handle)	return;
#if defined(NANA_WINDOWS)
			auto prv_pen = ::SelectObject(impl_->handle->context, ::CreatePen(PS_SOLID, 1, impl_->handle->bgcolor_native));

			::LineTo(impl_->handle->context, pos.x, pos.y);

			::DeleteObject(::SelectObject(impl_->handle->context, prv_pen));
#elif defined(NANA_X11)
			Display* disp = nana::detail::platform_spec::instance().open_display();
			impl_->handle->update_color();
			::XDrawLine(disp, impl_->handle->pixmap, impl_->handle->context,
				impl_->handle->line_begin_pos.x, impl_->handle->line_begin_pos.y,
				pos.x, pos.y);
			impl_->handle->line_begin_pos = pos;
#endif
			if (impl_->changed == false) impl_->changed = true;
		}

		void graphics::rectangle(bool solid)
		{
			rectangle(::nana::rectangle{ size() }, solid);
		}

		void graphics::rectangle(bool solid, const ::nana::color& clr)
		{
			palette(false, clr);
			rectangle(::nana::rectangle{ size() }, solid);
		}

		void graphics::rectangle(const ::nana::rectangle& r, bool solid)
		{
			if (r.width && r.height && impl_->handle && r.right() > 0 && r.bottom() > 0)
			{
#if defined(NANA_WINDOWS)

				auto brush = ::CreateSolidBrush(impl_->handle->bgcolor_native);

				::RECT native_r = { r.x, r.y, r.right(), r.bottom()};

				(solid ? ::FillRect : ::FrameRect)(impl_->handle->context, &native_r, brush);

				::DeleteObject(brush);
#elif defined(NANA_X11)
				Display* disp = nana::detail::platform_spec::instance().open_display();
				impl_->handle->update_color();
				if (solid)
					::XFillRectangle(disp, impl_->handle->pixmap, impl_->handle->context, r.x, r.y, r.width, r.height);
				else
					::XDrawRectangle(disp, impl_->handle->pixmap, impl_->handle->context, r.x, r.y, r.width - 1, r.height - 1);
#endif
				if (impl_->changed == false) impl_->changed = true;
			}
		}

		void graphics::rectangle(const ::nana::rectangle& r, bool solid, const color& clr)
		{
			palette(false, clr);
			rectangle(r, solid);
		}

		void graphics::frame_rectangle(const ::nana::rectangle& r, const ::nana::color& left_clr, const ::nana::color& top_clr, const ::nana::color& right_clr, const ::nana::color& bottom_clr)
		{
			int right = r.right() - 1;
			int bottom = r.bottom() - 1;
			line_begin(r.x, r.y);
			line_to({ right, r.y }, top_clr);
			line_to({ right, bottom }, right_clr);
			line_to({ r.x, bottom }, bottom_clr);
			line_to({ r.x, r.y }, left_clr);
		}

		void graphics::frame_rectangle(const ::nana::rectangle& r, const color& clr, unsigned gap)
		{
			palette(false, clr);

			if (r.width > gap * 2)
			{
				point left{ r.x + static_cast<int>(gap), r.y }, right{ r.right() - static_cast<int>(gap) - 1, r.y };
				line(left, right);

				left.y = right.y = r.bottom() - 1;
				line(left, right);
			}

			if (r.height > gap * 2)
			{
				point top{ r.x, r.y + static_cast<int>(gap) }, bottom{ r.x, r.bottom() - static_cast<int>(gap) - 1 };
				line(top, bottom);

				top.x = bottom.x = r.right() - 1;
				line(top, bottom);
			}
		}

		void graphics::gradual_rectangle(const ::nana::rectangle& rct, const ::nana::color& from, const ::nana::color& to, bool vertical)
		{
#if defined(NANA_WINDOWS)
			if (impl_->pxbuf.open(impl_->handle))
			{
				impl_->pxbuf.gradual_rectangle(rct, from, to, 0.0, vertical);
				impl_->pxbuf.paste(impl_->handle, point{});
			}
#elif defined(NANA_X11)
			if (nullptr == impl_->handle) return;

			nana::rectangle good_rct;
			if(!nana::overlap(nana::rectangle{ size() }, rct, good_rct))
				return;

			double deltapx = double(vertical ? good_rct.height : good_rct.width);
			double r, g, b;
			const double delta_r = (to.r() - (r = from.r())) / deltapx;
			const double delta_g = (to.g() - (g = from.g())) / deltapx;
			const double delta_b = (to.b() - (b = from.b())) / deltapx;

			unsigned last_color = (int(r) << 16) | (int(g) << 8) | int(b);

			Display * disp = nana::detail::platform_spec::instance().open_display();
			impl_->handle->set_color(static_cast<color_rgb>(last_color));
			impl_->handle->update_color();
			const int endpos = deltapx + (vertical ? good_rct.y : good_rct.x);
			if (endpos > 0)
			{
				if (vertical)
				{
					int x1 = good_rct.x, x2 = good_rct.right();
					auto y = good_rct.y;
					for (; y < endpos; ++y)
					{
						::XDrawLine(disp, impl_->handle->pixmap, impl_->handle->context, x1, y, x2, y);
						unsigned new_color = (int(r += delta_r) << 16) | (int(g += delta_g) << 8) | int(b += delta_b);
						if (new_color != last_color)
						{
							last_color = new_color;
							impl_->handle->set_color(static_cast<color_rgb>(last_color));
							impl_->handle->update_color();
						}
					}
				}
				else
				{
					int y1 = good_rct.y, y2 = good_rct.bottom();
					auto x = good_rct.x;
					for (; x < endpos; ++x)
					{
						::XDrawLine(disp, impl_->handle->pixmap, impl_->handle->context, x, y1, x, y2);
						unsigned new_color = (int(r += delta_r) << 16) | (int(g += delta_g) << 8) | int(b += delta_b);
						if (new_color != last_color)
						{
							last_color = new_color;
							impl_->handle->set_color(static_cast<color_rgb>(last_color));
							impl_->handle->update_color();
						}
					}
				}
			}
#endif
			if (impl_->changed == false) impl_->changed = true;
		}

#define NANA_WINDOWS_RGB(a)	(((DWORD)(a) & 0xFF)<<16) |  ((DWORD)(a) & 0xFF00) | (((DWORD)(a) & 0xFF0000) >> 16 )
		void graphics::round_rectangle(const ::nana::rectangle& r, unsigned radius_x, unsigned radius_y, const color& clr, bool solid, const color& solid_clr)
		{
			if (impl_->handle)
			{
#if defined(NANA_WINDOWS)
				impl_->handle->set_color(clr);

				if (solid)
				{
					auto prv_pen = ::SelectObject(impl_->handle->context, ::CreatePen(PS_SOLID, 1, impl_->handle->bgcolor_native));
					auto prv_brush = ::SelectObject(impl_->handle->context, ::CreateSolidBrush(NANA_WINDOWS_RGB(solid_clr.px_color().value)));

					::RoundRect(impl_->handle->context, r.x, r.y, r.right(), r.bottom(), static_cast<int>(radius_x * 2), static_cast<int>(radius_y * 2));

					::DeleteObject(::SelectObject(impl_->handle->context, prv_brush));
					::DeleteObject(::SelectObject(impl_->handle->context, prv_pen));
				}
				else
				{
					auto brush = ::CreateSolidBrush(impl_->handle->bgcolor_native);

					auto region = ::CreateRoundRectRgn(r.x, r.y, r.x + static_cast<int>(r.width) + 1, r.y + static_cast<int>(r.height) + 1, static_cast<int>(radius_x + 1), static_cast<int>(radius_y + 1));

					::FrameRgn(impl_->handle->context, region, brush, 1, 1);

					::DeleteObject(region);
					::DeleteObject(brush);
				}

				if (impl_->changed == false) impl_->changed = true;
#elif defined(NANA_X11)
				if(solid && (clr == solid_clr))
				{
					rectangle(r, true, clr);
				}
				else
				{
					rectangle(r, false, clr);
					if(solid)
						rectangle(::nana::rectangle(r).pare_off(1), true, solid_clr);
				}

				//eliminate unused parameter compiler warning.
				static_cast<void>(radius_x);
				static_cast<void>(radius_y);
#endif
			}
		}
	//end class graphics

	//class draw
		draw::draw(graphics& graph)
			: graph_(graph)
		{}

		void draw::corner(const rectangle& r, unsigned pixels)
		{
			if (1 == pixels)
			{
				graph_.set_pixel(r.x, r.y);
				graph_.set_pixel(r.right() - 1, r.y);

				graph_.set_pixel(r.x, r.bottom() - 1);
				graph_.set_pixel(r.right() - 1, r.bottom() - 1);
				return;
			}
			else if (1 < pixels)
			{
				graph_.line(r.position(), point(r.x + pixels, r.y));
				graph_.line(r.position(), point(r.x, r.y + pixels));

				int right = r.right() - 1;
				graph_.line(point(right, r.y), point(right - pixels, r.y));
				graph_.line(point(right, r.y), point(right, r.y - pixels));

				int bottom = r.bottom() - 1;
				graph_.line(point(r.x, bottom), point(r.x + pixels, bottom));
				graph_.line(point(r.x, bottom), point(r.x, bottom - pixels));

				graph_.line(point(right, bottom), point(right - pixels, bottom));
				graph_.line(point(right, bottom), point(right, bottom - pixels));
			}
		}
	//end class draw

}//end namespace paint
}//end namespace nana
