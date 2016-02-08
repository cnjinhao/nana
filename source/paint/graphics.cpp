/*
 *	Paint Graphics Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/paint/graphics.cpp
 */

#include <nana/detail/platform_spec_selector.hpp>
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

namespace nana
{
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
			typedef nana::detail::platform_spec::font_ptr_t ptr_t;
			ptr_t font_ptr;
		};

		font::font()
			: impl_(new impl_type)
		{
			impl_->font_ptr = nana::detail::platform_spec::instance().default_native_font();
		}

		font::font(drawable_type dw)
			: impl_(new impl_type)
		{
			impl_->font_ptr = dw->font;
		}

		font::font(const font& rhs)
			: impl_(new impl_type)
		{
			if(rhs.impl_)
				impl_->font_ptr = rhs.impl_->font_ptr;
		}

		font::font(const std::string& name, unsigned size, bool bold, bool italic, bool underline, bool strike_out)
			: impl_(new impl_type)
		{
			make(name, size, bold, italic, underline, strike_out);
		}

		font::~font()
		{
			delete impl_;
		}

		bool font::empty() const
		{
			return ((nullptr == impl_) || (nullptr == impl_->font_ptr));
		}

		void font::make(const std::string& name, unsigned size, bool bold, bool italic, bool underline, bool strike_out)
		{
			size = nana::detail::platform_spec::instance().font_size_to_height(size);
			make_raw(name, size, bold ? 700 : 400, italic, underline, strike_out);
		}

		void font::make_raw(const std::string& name, unsigned height, unsigned weight, bool italic, bool underline, bool strike_out)
		{
			if(impl_)
			{
				auto t = nana::detail::platform_spec::instance().make_native_font(name.c_str(), height, weight, italic, underline, strike_out);
				if(t)
					impl_->font_ptr = t;
			}
		}

		void font::set_default() const
		{
			if(empty())
				return;

			nana::detail::platform_spec::instance().default_native_font(impl_->font_ptr);
		}

		std::string font::name() const
		{
			if(empty()) return std::string();

			return to_utf8(impl_->font_ptr->name);
		}

		unsigned font::size() const
		{
			if(empty()) return 0;
			return nana::detail::platform_spec::instance().font_height_to_size(impl_->font_ptr->height);
		}

		bool font::bold() const
		{
			if(empty()) return false;
			return (impl_->font_ptr->weight >= 700);
		}

		unsigned font::height() const
		{
			if(empty()) return false;
			return (impl_->font_ptr->height);
		}

		unsigned font::weight() const
		{
			if(empty()) return false;
			return (impl_->font_ptr->weight);
		}

		bool font::italic() const
		{
			if(empty()) return false;
			return (impl_->font_ptr->italic);
		}

		native_font_type font::handle() const
		{
			if(empty())	return nullptr;
			return reinterpret_cast<native_font_type>(impl_->font_ptr->handle);
		}

		void font::release()
		{
			if(impl_)
				impl_->font_ptr.reset();
		}

		font & font::operator=(const font& rhs)
		{
			if(impl_ && rhs.impl_ && (this != &rhs))
				impl_->font_ptr = rhs.impl_->font_ptr;

			return *this;
		}

		bool font::operator==(const font& rhs) const
		{
			if(empty() == rhs.empty())
				return (empty() || (impl_->font_ptr->handle == rhs.impl_->font_ptr->handle));

			return false;
		}

		bool font::operator!=(const font& rhs) const
		{
			if(empty() == rhs.empty())
				return ((empty() == false) && (impl_->font_ptr->handle != rhs.impl_->font_ptr->handle));

			return true;
		}
	//end class font

	//class graphics
		graphics::graphics()
			:handle_(nullptr), changed_(false)
		{}

		graphics::graphics(const nana::size& sz)
			:handle_(nullptr), changed_(true)
		{
			make(sz);
		}

		graphics::graphics(const graphics& rhs)
			:dwptr_(rhs.dwptr_), handle_(rhs.handle_), size_(rhs.size_), changed_(true)
		{}

		graphics& graphics::operator=(const graphics& rhs)
		{
			if(this != &rhs)
			{
				size_ = rhs.size_;
				dwptr_ = rhs.dwptr_;
				handle_ = rhs.handle_;
				if(changed_ == false) changed_ = true;
			}
			return *this;
		}

		bool graphics::changed() const
		{
			return this->changed_;
		}

		bool graphics::empty() const { return (handle_ == nullptr); }

		graphics::operator const void *() const
		{
			return handle_;
		}

		drawable_type graphics::handle() const
		{
			return handle_;
		}

		const void* graphics::pixmap() const
		{
			//The reinterpret_cast is used for same platform. Under Windows, the type
			//of pixmap can be conversed into void* directly, but under X11, the type is not a pointer.
			return (handle_? reinterpret_cast<void*>(handle_->pixmap) : nullptr);
		}

		const void* graphics::context() const
		{
			return (handle_? handle_->context : nullptr);
		}

		void graphics::make(const ::nana::size& sz)
		{
			if(handle_ == nullptr || size_ != sz)
			{
				//The object will be delete while dwptr_ is performing a release.
				drawable_type dw = new nana::detail::drawable_impl_type;
				//Reuse the old font
				if(dwptr_)
				{
					drawable_type reuse = dwptr_.get();
					dw->font = reuse->font;
					dw->string.tab_length = reuse->string.tab_length;
				}
				else
					dw->font = font_shadow_.impl_->font_ptr;

#if defined(NANA_WINDOWS)
				HDC hdc = ::GetDC(0);
				HDC cdc = ::CreateCompatibleDC(hdc);

				BITMAPINFO bmi;
				bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
				bmi.bmiHeader.biWidth = sz.width;
				bmi.bmiHeader.biHeight = -static_cast<int>(sz.height);
				bmi.bmiHeader.biPlanes = 1;
				bmi.bmiHeader.biBitCount = 32;         // four 8-bit components
				bmi.bmiHeader.biCompression = BI_RGB;
				bmi.bmiHeader.biSizeImage = (sz.width * sz.height) << 2;

				HBITMAP bmp = ::CreateDIBSection(cdc, &bmi, DIB_RGB_COLORS, reinterpret_cast<void**>(&(dw->pixbuf_ptr)), 0, 0);

				if(bmp)
				{
					::DeleteObject((HBITMAP)::SelectObject(cdc, bmp));
					::DeleteObject(::SelectObject(cdc, dw->font->handle));

					dw->context = cdc;
					dw->pixmap = bmp;
					::SetBkMode(cdc, TRANSPARENT);
					dw->brush.set(cdc, dw->brush.Solid, 0xFFFFFF);
				}
				else
				{
					::DeleteDC(cdc);
					delete dw;
					dw = nullptr;
					release();
				}

				::ReleaseDC(0, hdc);
#elif defined(NANA_X11)
				auto & spec = nana::detail::platform_spec::instance();
				Display* disp = spec.open_display();
				int screen = DefaultScreen(disp);
				Window root = ::XRootWindow(disp, screen);
				dw->pixmap = ::XCreatePixmap(disp, root, (sz.width ? sz.width : 1), (sz.height ? sz.height : 1), DefaultDepth(disp, screen));
				dw->context = ::XCreateGC(disp, dw->pixmap, 0, 0);
	#if defined(NANA_USE_XFT)
				dw->xftdraw = ::XftDrawCreate(disp, dw->pixmap, spec.screen_visual(), spec.colormap());
	#endif
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
					dwptr_.reset(dw, detail::drawable_deleter{});
					handle_ = dw;
					size_ = sz;

					handle_->string.tab_pixels = detail::raw_text_extent_size(handle_, L"\t", 1).width;
					handle_->string.whitespace_pixels = detail::raw_text_extent_size(handle_, L" ", 1).width;
				}
			}

			if(changed_ == false) changed_ = true;
		}

		void graphics::resize(const ::nana::size& sz)
		{
			graphics duplicate(*this);
			make(sz);
			bitblt(0, 0, duplicate);
		}

		void graphics::typeface(const font& f)
		{
			//Keep the font as a shadow, even if the graphics is empty. Setting the font is futile when the size
			//of a widget is zero.
			font_shadow_ = f;
			if(handle_ && (false == f.empty()))
			{
				handle_->font = f.impl_->font_ptr;
#if defined(NANA_WINDOWS)
				::SelectObject(handle_->context, reinterpret_cast<HFONT>(f.impl_->font_ptr->handle));
#endif
				handle_->string.tab_pixels = detail::raw_text_extent_size(handle_, L"\t", 1).width;
				handle_->string.whitespace_pixels = detail::raw_text_extent_size(handle_, L" ", 1).width;
				if(changed_ == false) changed_ = true;
			}
		}

		font graphics::typeface() const
		{
			//The font may be set when the graphics is still empty.
			//it should return the shadow font when the graphics is empty.
			return (handle_ ? font(handle_) : font_shadow_);
		}

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
			return detail::text_extent_size(handle_, str, len);
		}

		nana::size	graphics::text_extent_size(const std::wstring& str, std::size_t len)	const
		{
			return detail::text_extent_size(handle_, str.c_str(), len);
		}

		nana::size graphics::glyph_extent_size(const wchar_t * str, std::size_t len, std::size_t begin, std::size_t end) const
		{
			if(len < end) end = len;
			if (nullptr == handle_ || nullptr == str || 0 == len || begin >= end) return{};

			nana::size sz;
#if defined(NANA_WINDOWS)
			int * dx = new int[len];
			SIZE extents;
			::GetTextExtentExPoint(handle_->context, str, static_cast<int>(len), 0, 0, dx, &extents);
			sz.width = dx[end - 1] - (begin ? dx[begin - 1] : 0);
			unsigned tab_pixels = handle_->string.tab_length * handle_->string.whitespace_pixels;
			const wchar_t * pend = str + end;
			for(const wchar_t * p = str + begin; p != pend; ++p)
			{
				if(*p == '\t')
					sz.width += tab_pixels;
			}
			sz.height = extents.cy;
			delete [] dx;
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
			if(nullptr == handle_ || nullptr == handle_->context || nullptr == str || nullptr == pxbuf) return false;
			if(len == 0) return true;

			unsigned tab_pixels = handle_->string.tab_length * handle_->string.whitespace_pixels;
#if defined(NANA_WINDOWS)
			int * dx = new int[len];
			SIZE extents;
			::GetTextExtentExPoint(handle_->context, str, static_cast<int>(len), 0, 0, dx, &extents);

			pxbuf[0] = (str[0] == '\t' ? tab_pixels  : dx[0]);

			for(std::size_t i = 1; i < len; ++i)
			{
				pxbuf[i] = (str[i] == '\t' ? tab_pixels : dx[i] - dx[i - 1]);
			}
			delete [] dx;
#elif defined(NANA_X11) && defined(NANA_USE_XFT)

			Display * disp = nana::detail::platform_spec::instance().open_display();
			XftFont * xft = handle_->font->handle;

			XGlyphInfo extents;
			for(std::size_t i = 0; i < len; ++i)
			{
				if(str[i] != '\t')
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
			if(handle_ && handle_->context && str.size())
			{
				std::vector<unicode_bidi::entity> reordered;
				unicode_bidi bidi;
				bidi.linestr(str.c_str(), str.size(), reordered);
				for(auto & i: reordered)
				{
					nana::size t = text_extent_size(i.begin, i.end - i.begin);
					sz.width += t.width;
					if(sz.height < t.height)
						sz.height = t.height;
				}
			}
			return sz;
		}

		::nana::size graphics::bidi_extent_size(const std::string& str) const
		{
			return bidi_extent_size(static_cast<std::wstring>(::nana::charset(str, ::nana::unicode::utf8)));
		}

		bool graphics::text_metrics(unsigned & ascent, unsigned& descent, unsigned& internal_leading) const
		{
			if(handle_)
			{
#if defined(NANA_WINDOWS)
				::TEXTMETRIC tm;
				::GetTextMetrics(handle_->context, &tm);
				ascent = static_cast<unsigned>(tm.tmAscent);
				descent = static_cast<unsigned>(tm.tmDescent);
				internal_leading = static_cast<unsigned>(tm.tmInternalLeading);
				return true;
#elif defined(NANA_X11)
				if(handle_->font)
				{
	#if defined(NANA_USE_XFT)
					XftFont * fs = reinterpret_cast<XftFont*>(handle_->font->handle);
					ascent = fs->ascent;
					descent = fs->descent;
					internal_leading = 0;
	#else
					XFontSet fs = reinterpret_cast<XFontSet>(handle_->font->handle);
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
			if(!handle_)	return;
#if defined(NANA_WINDOWS)
			::MoveToEx(handle_->context, x, y, 0);

#elif defined(NANA_X11)
			handle_->line_begin_pos.x = x;
			handle_->line_begin_pos.y = y;
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
			if(handle_)
			{
#if defined(NANA_WINDOWS)
				HDC dc = ::GetDC(reinterpret_cast<HWND>(src));
				::BitBlt(handle_->context, r_dst.x, r_dst.y, r_dst.width, r_dst.height, dc, 0, 0, SRCCOPY);
				::ReleaseDC(reinterpret_cast<HWND>(src), dc);
#elif defined(NANA_X11)
				::XCopyArea(nana::detail::platform_spec::instance().open_display(),
						reinterpret_cast<Window>(src), handle_->pixmap, handle_->context,
						0, 0, r_dst.width, r_dst.height, r_dst.x, r_dst.y);
#endif
				if(changed_ == false) changed_ = true;
			}
		}

		void graphics::bitblt(const nana::rectangle& r_dst, native_window_type src, const nana::point& p_src)
		{
			if(handle_)
			{
#if defined(NANA_WINDOWS)
				HDC dc = ::GetDC(reinterpret_cast<HWND>(src));
				::BitBlt(handle_->context, r_dst.x, r_dst.y, r_dst.width, r_dst.height, dc, p_src.x, p_src.y, SRCCOPY);
				::ReleaseDC(reinterpret_cast<HWND>(src), dc);
#elif defined(NANA_X11)
				::XCopyArea(nana::detail::platform_spec::instance().open_display(),
						reinterpret_cast<Window>(src), handle_->pixmap, handle_->context,
						p_src.x, p_src.y, r_dst.width, r_dst.height, r_dst.x, r_dst.y);
#endif
				if(changed_ == false) changed_ = true;
			}
		}

		void graphics::bitblt(const nana::rectangle& r_dst, const graphics& src)
		{
			if(handle_ && src.handle_)
			{
#if defined(NANA_WINDOWS)
				::BitBlt(handle_->context, r_dst.x, r_dst.y, r_dst.width, r_dst.height, src.handle_->context, 0, 0, SRCCOPY);
#elif defined(NANA_X11)
				::XCopyArea(nana::detail::platform_spec::instance().open_display(),
						src.handle_->pixmap, handle_->pixmap, handle_->context,
						0, 0, r_dst.width, r_dst.height, r_dst.x, r_dst.y);
#endif
				if(changed_ == false) changed_ = true;
			}
		}

		void graphics::bitblt(const nana::rectangle& r_dst, const graphics& src, const nana::point& p_src)
		{
			if(handle_ && src.handle_)
			{
#if defined(NANA_WINDOWS)
				::BitBlt(handle_->context, r_dst.x, r_dst.y, r_dst.width, r_dst.height, src.handle_->context, p_src.x, p_src.y, SRCCOPY);
#elif defined(NANA_X11)
				::XCopyArea(nana::detail::platform_spec::instance().open_display(),
						src.handle_->pixmap, handle_->pixmap, handle_->context,
						p_src.x, p_src.y, r_dst.width, r_dst.height, r_dst.x, r_dst.y);
#endif
				if(changed_ == false) changed_ = true;
			}
		}

		void graphics::blend(const nana::rectangle& s_r, graphics& dst, const nana::point& d_pos, double fade_rate) const
		{
			if(dst.handle_ && handle_ && (dst.handle_ != handle_))
			{
				pixel_buffer s_pixbuf;
				s_pixbuf.attach(handle_, ::nana::rectangle{ size() });

				s_pixbuf.blend(s_r, dst.handle_, d_pos, fade_rate);

				if(dst.changed_ == false) dst.changed_ = true;
			}
		}

		void graphics::blur(const nana::rectangle& r, std::size_t radius)
		{
			if(handle_)
			{
				pixel_buffer pixbuf(handle_, 0, 0);
				pixbuf.blur(r, radius);
				pixbuf.paste(handle_, point{});
			}
		}

		void graphics::rgb_to_wb()
		{
			if(handle_)
			{
				//Create the color table for perfermance
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

				pixel_buffer pixbuf(handle_, 0, 0);

				auto pixels = pixbuf.raw_ptr(0);

				const nana::size sz = paint::detail::drawable_size(handle_);
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

				pixbuf.paste(handle_, point{});
				if(changed_ == false) changed_ = true;
			}
		}

		void graphics::paste(graphics& dst, int x, int y) const
		{
			if(handle_ && dst.handle_ && handle_ != dst.handle_)
			{
#if defined(NANA_WINDOWS)
				::BitBlt(dst.handle_->context, x, y, size_.width, size_.height, handle_->context, 0, 0, SRCCOPY);
#elif defined(NANA_X11)
				Display* display = nana::detail::platform_spec::instance().open_display();
				::XCopyArea(display,
						handle_->pixmap, dst.handle_->pixmap, handle_->context,
						0, 0, size_.width, size_.height, x, y);

				::XFlush(display);
#endif
				dst.changed_ = true;
			}
		}

		void graphics::paste(native_window_type dst, const nana::rectangle& r, int x, int y) const
		{
			paste(dst, r.x, r.y, r.width, r.height, x, y);
		}

		void graphics::paste(native_window_type dst, int dx, int dy, unsigned width, unsigned height, int sx, int sy) const
		{
			if(handle_)
			{
#if defined(NANA_WINDOWS)
				HDC dc = ::GetDC(reinterpret_cast<HWND>(dst));
				if(dc)
				{
					::BitBlt(dc, dx, dy, width, height, handle_->context, sx, sy, SRCCOPY);
					::ReleaseDC(reinterpret_cast<HWND>(dst), dc);
				}
#elif defined(NANA_X11)
				Display * display = nana::detail::platform_spec::instance().open_display();
				::XCopyArea(display,
						handle_->pixmap, reinterpret_cast<Window>(dst), handle_->context,
						sx, sy, width, height, dx, dy);

				::XMapWindow(display, reinterpret_cast<Window>(dst));
				::XFlush(display);
#endif
			}
		}

		void graphics::paste(drawable_type dst, int x, int y) const
		{
			if(handle_ && dst && handle_ != dst)
			{
#if defined (NANA_WINDOWS)
				::BitBlt(dst->context, x, y, size_.width, size_.height, handle_->context, 0, 0, SRCCOPY);
#elif defined(NANA_X11)
				Display * display = nana::detail::platform_spec::instance().open_display();
				::XCopyArea(display,
						handle_->pixmap, dst->pixmap, handle_->context,
						0, 0, size_.width, size_.height, x, y);
				::XFlush(display);
#endif
			}
		}


		void graphics::paste(const nana::rectangle& r_src, graphics& dst, int x, int y) const
		{
			if(handle_ && dst.handle_ && handle_ != dst.handle_)
			{
#if defined(NANA_WINDOWS)
				::BitBlt(dst.handle_->context, x, y, r_src.width, r_src.height, handle_->context, r_src.x, r_src.y, SRCCOPY);
#elif defined(NANA_X11)
				Display* display = nana::detail::platform_spec::instance().open_display();
				::XCopyArea(display,
						handle_->pixmap, dst.handle_->pixmap, handle_->context,
						r_src.x, r_src.y, r_src.width, r_src.height, x, y);

				::XFlush(display);
#endif
			}
		}

		void graphics::stretch(const nana::rectangle& src_r, graphics& dst, const nana::rectangle& r) const
		{
			if(handle_ && dst.handle_ && (handle_ != dst.handle_))
			{
				pixel_buffer pixbuf(handle_, 0, 0);
				pixbuf.stretch(src_r, dst.handle_, r);
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
			return size_.width;
		}

		unsigned graphics::height() const{
			return size_.height;
		}

		nana::size graphics::size() const{
			return this->size_;
		}

		void graphics::setsta()
		{
			changed_ = false;
		}

		void graphics::set_changed()
		{
			changed_ = true;
		}

		void graphics::release()
		{
			dwptr_.reset();
			handle_ = nullptr;
			size_.width = size_.height = 0;
		}

		void graphics::save_as_file(const char* file_utf8) const throw()
		{
			if(handle_)
			{
#if defined(NANA_WINDOWS)
				int iWidth = static_cast<int>(size_.width);
				int iHeight = static_cast<int>(size_.height);
				BITMAPINFO bmpInfo = {};
				bmpInfo.bmiHeader.biSize = sizeof(bmpInfo.bmiHeader);
				bmpInfo.bmiHeader.biWidth = static_cast<int>(size_.width);
				bmpInfo.bmiHeader.biHeight = static_cast<int>(size_.height);
				bmpInfo.bmiHeader.biPlanes = 1;
				bmpInfo.bmiHeader.biBitCount = 24;

				const size_t lineBytes = ((bmpInfo.bmiHeader.biWidth * 3) + 3) & (~3);
				const size_t imageBytes = iHeight * lineBytes;

				HDC hdcMem = ::CreateCompatibleDC(handle_->context);
				BYTE *pData = nullptr;
				HBITMAP hBmp = ::CreateDIBSection(hdcMem, &bmpInfo, DIB_RGB_COLORS, reinterpret_cast<void**>(&pData), 0, 0);

				::SelectObject(hdcMem, hBmp);

				BitBlt(hdcMem, 0, 0, iWidth, iHeight, handle_->context, 0, 0, SRCCOPY);

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

#endif
			}
		}

		::nana::color graphics::palette(bool for_text) const
		{
			if (handle_)
				return static_cast<color_rgb>(for_text ? handle_->get_text_color() : handle_->get_color());

			return{};
		}

		graphics& graphics::palette(bool for_text, const ::nana::color& clr)
		{
			if (handle_)
			{
				if (for_text)
					handle_->set_text_color(clr);
				else
					handle_->set_color(clr);
			}

			return *this;
		}

		unsigned graphics::bidi_string(const nana::point& pos, const wchar_t * str, std::size_t len)
		{
			auto moved_pos = pos;
			unicode_bidi bidi;
			std::vector<unicode_bidi::entity> reordered;
			bidi.linestr(str, len, reordered);
			for (auto & i : reordered)
			{
				string(moved_pos, i.begin, i.end - i.begin);
				moved_pos.x += static_cast<int>(text_extent_size(i.begin, i.end - i.begin).width);
			}
			return static_cast<unsigned>(moved_pos.x - pos.x);
		}

		unsigned graphics::bidi_string(const point& pos, const char* str, std::size_t len)
		{
			std::wstring wstr = ::nana::charset(std::string(str, str + len), ::nana::unicode::utf8);
			return bidi_string(pos, wstr.data(), wstr.size());
		}

		void graphics::blend(const nana::rectangle& r, const ::nana::color& clr, double fade_rate)
		{
			if (handle_)
			{
				nana::paint::detail::blend(handle_, r, clr.px_color(), fade_rate);
				if (changed_ == false) changed_ = true;
			}
		}

		void graphics::set_pixel(int x, int y, const ::nana::color& clr)
		{
			if (handle_)
			{
				handle_->set_color(clr);
				set_pixel(x, y);
			}
		}

		void graphics::set_pixel(int x, int y)
		{
			if (handle_)
			{
#if defined(NANA_WINDOWS)
				::SetPixel(handle_->context, x, y, NANA_RGB(handle_->get_color()));
#elif defined(NANA_X11)
				Display* disp = nana::detail::platform_spec::instance().open_display();
				handle_->update_color();
				::XDrawPoint(disp, handle_->pixmap, handle_->context, x, y);
#endif
				if (changed_ == false) changed_ = true;
			}
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
			if (handle_ && str && len)
			{
				auto const end = str + len;
				auto i = std::find(str, end, '\t');
#if defined(NANA_LINUX) || defined(NANA_MACOS)
				handle_->update_text_color();
#endif
				if (i != end)
				{
					std::size_t tab_pixels = handle_->string.tab_length * handle_->string.tab_pixels;
					while (true)
					{
						len = i - str;
						if (len)
						{
							//Render a part that does not contains a tab
							detail::draw_string(handle_, pos, str, len);
							pos.x += detail::raw_text_extent_size(handle_, str, len).width;
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
					detail::draw_string(handle_, pos, str, len);
				if (changed_ == false) changed_ = true;
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

		void graphics::line(const nana::point& pos1, const nana::point& pos2)
		{
			if (!handle_)	return;
#if defined(NANA_WINDOWS)
			handle_->update_pen();
			if (pos1 != pos2)
			{
				::MoveToEx(handle_->context, pos1.x, pos1.y, 0);
				::LineTo(handle_->context, pos2.x, pos2.y);
			}
			::SetPixel(handle_->context, pos2.x, pos2.y, NANA_RGB(handle_->pen.color));
#elif defined(NANA_X11)
			Display* disp = nana::detail::platform_spec::instance().open_display();
			handle_->update_color();
			::XDrawLine(disp, handle_->pixmap, handle_->context, pos1.x, pos1.y, pos2.x, pos2.y);
#endif
			if (changed_ == false) changed_ = true;
		}

		void graphics::line(const point& pos_a, const point& pos_b, const color& clr)
		{
			palette(false, clr);
			line(pos_a, pos_b);
		}

		void graphics::line_to(const point& pos, const color& clr)
		{
			if (!handle_) return;
			handle_->set_color(clr);
			line_to(pos);
		}

		void graphics::line_to(const point& pos)
		{
			if (!handle_)	return;
#if defined(NANA_WINDOWS)
			handle_->update_pen();
			::LineTo(handle_->context, pos.x, pos.y);
#elif defined(NANA_X11)
			Display* disp = nana::detail::platform_spec::instance().open_display();
			handle_->update_color();
			::XDrawLine(disp, handle_->pixmap, handle_->context,
				handle_->line_begin_pos.x, handle_->line_begin_pos.y,
				pos.x, pos.y);
			handle_->line_begin_pos = pos;
#endif
			if (changed_ == false) changed_ = true;
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
			if (r.width && r.height && handle_ && r.right() > 0 && r.bottom() > 0)
			{
#if defined(NANA_WINDOWS)
				::RECT native_r = { r.x, r.y, r.right(), r.bottom()};
				handle_->update_brush();
				(solid ? ::FillRect : ::FrameRect)(handle_->context, &native_r, handle_->brush.handle);
#elif defined(NANA_X11)
				Display* disp = nana::detail::platform_spec::instance().open_display();
				handle_->update_color();
				if (solid)
					::XFillRectangle(disp, handle_->pixmap, handle_->context, r.x, r.y, r.width, r.height);
				else
					::XDrawRectangle(disp, handle_->pixmap, handle_->context, r.x, r.y, r.width - 1, r.height - 1);
#endif
				if (changed_ == false) changed_ = true;
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

		void graphics::gradual_rectangle(const ::nana::rectangle& rct, const ::nana::color& from, const ::nana::color& to, bool vertical)
		{
#if defined(NANA_WINDOWS)
			if (pxbuf_.open(handle_))
			{
				pxbuf_.gradual_rectangle(rct, from, to, 0.0, vertical);
				pxbuf_.paste(handle_, point{});
			}
#elif defined(NANA_X11)
			if (nullptr == handle_) return;

			double deltapx = double(vertical ? rct.height : rct.width);
			double r, g, b;
			const double delta_r = (to.r() - (r = from.r())) / deltapx;
			const double delta_g = (to.g() - (g = from.g())) / deltapx;
			const double delta_b = (to.b() - (b = from.b())) / deltapx;

			unsigned last_color = (int(r) << 16) | (int(g) << 8) | int(b);

			Display * disp = nana::detail::platform_spec::instance().open_display();
			handle_->fgcolor(static_cast<color_rgb>(last_color));
			const int endpos = deltapx + (vertical ? rct.y : rct.x);
			if (endpos > 0)
			{
				if (vertical)
				{
					int x1 = rct.x, x2 = rct.right();
					auto y = rct.y;
					for (; y < endpos; ++y)
					{
						::XDrawLine(disp, handle_->pixmap, handle_->context, x1, y, x2, y);
						unsigned new_color = (int(r += delta_r) << 16) | (int(g += delta_g) << 8) | int(b += delta_b);
						if (new_color != last_color)
						{
							last_color = new_color;
							handle_->fgcolor(static_cast<color_rgb>(last_color));
						}
					}
				}
				else
				{
					int y1 = rct.y, y2 = rct.bottom();
					auto x = rct.x;
					for (; x < endpos; ++x)
					{
						::XDrawLine(disp, handle_->pixmap, handle_->context, x, y1, x, y2);
						unsigned new_color = (int(r += delta_r) << 16) | (int(g += delta_g) << 8) | int(b += delta_b);
						if (new_color != last_color)
						{
							last_color = new_color;
							handle_->fgcolor(static_cast<color_rgb>(last_color));
						}
					}
				}
			}
#endif
			if (changed_ == false) changed_ = true;
		}

		void graphics::round_rectangle(const ::nana::rectangle& r, unsigned radius_x, unsigned radius_y, const color& clr, bool solid, const color& solid_clr)
		{
			if (handle_)
			{
#if defined(NANA_WINDOWS)
				handle_->set_color(clr);
				if (solid)
				{
					handle_->update_pen();
					handle_->brush.set(handle_->context, handle_->brush.Solid, solid_clr.px_color().value);
					::RoundRect(handle_->context, r.x, r.y, r.right(), r.bottom(), static_cast<int>(radius_x * 2), static_cast<int>(radius_y * 2));
				}
				else
				{
					handle_->update_brush();
					handle_->round_region.set(r, radius_x, radius_y);
					::FrameRgn(handle_->context, handle_->round_region.handle, handle_->brush.handle, 1, 1);
				}
				if(changed_ == false) changed_ = true;
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
#endif
			}
		}
	//end class graphics

}//end namespace paint
}//end namespace nana
