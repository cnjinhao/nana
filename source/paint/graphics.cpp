/*
 *	Paint Graphics Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/paint/graphics.cpp
 */

#include <nana/config.hpp>
#include PLATFORM_SPEC_HPP
#include GUI_BEDROCK_HPP
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
	#if defined(NANA_UNICODE)
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

		font::font(const nana::char_t* name, unsigned size, bool bold, bool italic, bool underline, bool strike_out)
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

		void font::make(const nana::char_t* name, unsigned size, bool bold, bool italic, bool underline, bool strike_out)
		{
			size = nana::detail::platform_spec::instance().font_size_to_height(size);
			make_raw(name, size, bold ? 700 : 400, italic, underline, strike_out);
		}

		void font::make_raw(const nana::char_t*name, unsigned height, unsigned weight, bool italic, bool underline, bool strike_out)
		{
			if(impl_)
			{
				auto t = nana::detail::platform_spec::instance().make_native_font(name, height, weight, italic, underline, strike_out);
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

		nana::string font::name() const
		{
			if(empty()) return nana::string();
			return impl_->font_ptr->name;
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

		graphics::graphics(unsigned width, unsigned height)
			:handle_(nullptr), changed_(true)
		{
			make(width, height);
		}

		graphics::graphics(const nana::size& sz)
			:handle_(nullptr), changed_(true)
		{
			make(sz.width, sz.height);
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

		void graphics::make(unsigned width, unsigned height)
		{
			if(handle_ == nullptr || size_ != nana::size(width, height))
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
				bmi.bmiHeader.biWidth = width;
				bmi.bmiHeader.biHeight = -static_cast<int>(height);
				bmi.bmiHeader.biPlanes = 1;
				bmi.bmiHeader.biBitCount = 32;         // four 8-bit components
				bmi.bmiHeader.biCompression = BI_RGB;
				bmi.bmiHeader.biSizeImage = (width * height) << 2;

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
				dw->pixmap = ::XCreatePixmap(disp, root, (width ? width : 1), (height ? height : 1), DefaultDepth(disp, screen));
				dw->context = ::XCreateGC(disp, dw->pixmap, 0, 0);
	#if defined(NANA_UNICODE)
				dw->xftdraw = ::XftDrawCreate(disp, dw->pixmap, spec.screen_visual(), spec.colormap());
	#endif
#endif
				if(dw)
				{
					dw->fgcolor(0);
#if defined(NANA_WINDOWS)
					dw->bytes_per_line = width * sizeof(pixel_rgb_t);
#endif
					dwptr_ = std::shared_ptr<nana::detail::drawable_impl_type>(dw, detail::drawable_deleter());
					handle_ = dw;
					size_.width = width;
					size_.height = height;

					handle_->string.tab_pixels = detail::raw_text_extent_size(handle_, STR("\t"), 1).width;
					handle_->string.whitespace_pixels = detail::raw_text_extent_size(handle_, STR(" "), 1).width;
				}
			}

			if(changed_ == false) changed_ = true;
		}

		void graphics::resize(unsigned width, unsigned height)
		{
			graphics duplicate(*this);
			make(width, height);
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
				handle_->string.tab_pixels = detail::raw_text_extent_size(handle_, STR("\t"), 1).width;
				handle_->string.whitespace_pixels = detail::raw_text_extent_size(handle_, STR(" "), 1).width;
				if(changed_ == false) changed_ = true;
			}
		}

		font graphics::typeface() const
		{
			//The font may be set when the graphics is still empty.
			//it should return the shadow font when the graphics is empty.
			return (handle_ ? font(handle_) : font_shadow_);
		}

		nana::size	graphics::text_extent_size(const nana::char_t* text)	const
		{
			return text_extent_size(text, nana::strlen(text));
		}

		nana::size	graphics::text_extent_size(const nana::string& text)	const
		{
			return text_extent_size(text.c_str(), static_cast<unsigned>(text.length()));
		}

		nana::size	graphics::text_extent_size(const nana::char_t* str, std::size_t len)	const
		{
			return detail::text_extent_size(handle_, str, len);
		}

		nana::size	graphics::text_extent_size(const nana::string& str, std::size_t len)	const
		{
			return detail::text_extent_size(handle_, str.c_str(), len);
		}

		nana::size graphics::glyph_extent_size(const nana::char_t * str, std::size_t len, std::size_t begin, std::size_t end) const
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
			const nana::char_t * pend = str + end;
			for(const nana::char_t * p = str + begin; p != pend; ++p)
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

		nana::size graphics::glyph_extent_size(const nana::string& str, std::size_t len, std::size_t begin, std::size_t end) const
		{
			return glyph_extent_size(str.c_str(), len, begin, end);
		}

		bool graphics::glyph_pixels(const nana::char_t * str, std::size_t len, unsigned* pxbuf) const
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
#elif defined(NANA_X11)
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

		nana::size	graphics::bidi_extent_size(const nana::string& str) const
		{
			nana::size sz;
#if defined NANA_UNICODE
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
#endif
			return sz;
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
	#if defined(NANA_UNICODE)
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

		unsigned graphics::bidi_string(int x, int y, color_t col, const nana::char_t* str, std::size_t len)
		{
			int origin_x = x;
			unicode_bidi bidi;
			std::vector<unicode_bidi::entity> reordered;
			bidi.linestr(str, len, reordered);
			for(auto & i : reordered)
			{
				string(x, y, col, i.begin, i.end - i.begin);
				x += static_cast<int>(text_extent_size(i.begin, i.end - i.begin).width);
			}
			return static_cast<unsigned>(x - origin_x);
		}

		void graphics::string(int x, int y, color_t color, const nana::string& str, std::size_t len)
		{
			string(x, y, color, str.c_str(), len);
		}

		void graphics::string(int x, int y, color_t color, const nana::string& str)
		{
			string(x, y, color, str.c_str(), str.size());
		}

		void graphics::string(int x, int y, color_t color, const nana::char_t* str, std::size_t len)
		{
			if(handle_ && str && len)
			{
				handle_->fgcolor(color);
				const nana::char_t * end = str + len;
				const nana::char_t * i = std::find(str, end, '\t');
				if(i != end)
				{
					std::size_t tab_pixels = handle_->string.tab_length * handle_->string.tab_pixels;
					while(true)
					{
						len = i - str;
						if(len)
						{
							//Render a part that does not contains a tab
							detail::draw_string(handle_, x, y, str, len);
							x += detail::raw_text_extent_size(handle_, str, len).width;
						}

						str = i;
						while(str != end && (*str == '\t'))
							++str;

						if(str != end)
						{
							//Now i_tab is not a tab, but a non-tab character following the previous tabs
							x += static_cast<int>(tab_pixels * (str - i));
							i = std::find(str, end, '\t');
						}
						else
							break;
					}
				}
				else
					detail::draw_string(handle_, x, y, str, len);
				if(changed_ == false) changed_ = true;
			}
		}

		void graphics::string(int x, int y, color_t c, const nana::char_t* str)
		{
			string(x, y, c, str, nana::strlen(str));
		}

		void graphics::set_pixel(int x, int y, color_t color)
		{
			if(handle_)
			{
#if defined(NANA_WINDOWS)
				::SetPixel(handle_->context, x, y, NANA_RGB(color));
#elif defined(NANA_X11)
				Display* disp = nana::detail::platform_spec::instance().open_display();
				handle_->fgcolor(color);
				::XDrawPoint(disp, handle_->pixmap, handle_->context,x, y);
#endif
				if(changed_ == false) changed_ = true;
			}
		}

		void graphics::rectangle(int x, int y, unsigned width, unsigned height, color_t color, bool solid)
		{
			if((static_cast<int>(width) > -x) && (static_cast<int>(height) > -y) && width && height && handle_)
			{
#if defined(NANA_WINDOWS)
				::RECT r = {x, y, static_cast<long>(x + width), static_cast<long>(y + height)};
				handle_->brush.set(handle_->context, handle_->brush.Solid, color);
				(solid ? ::FillRect : ::FrameRect)(handle_->context, &r, handle_->brush.handle);
#elif defined(NANA_X11)
				Display* disp = nana::detail::platform_spec::instance().open_display();
				handle_->fgcolor(color);
				if(solid)
					::XFillRectangle(disp, handle_->pixmap, handle_->context, x, y, width, height);
				else
					::XDrawRectangle(disp, handle_->pixmap, handle_->context, x, y, width - 1, height - 1);
#endif
				if(changed_ == false) changed_ = true;
			}
		}

		void graphics::rectangle(nana::color_t color, bool solid)
		{
			rectangle(0, 0, size_.width, size_.height, color, solid);
		}

		void graphics::rectangle(const nana::rectangle & r, color_t color, bool solid)
		{
			rectangle(r.x, r.y, r.width, r.height, color, solid);
		}

		void graphics::rectangle_line(const nana::rectangle& r, color_t color_left, color_t color_top, color_t color_right, color_t color_bottom)
		{
			int right = r.x + r.width - 1;
			int bottom = r.y + r.height -1;
			line_begin(r.x, r.y);
			line_to(right, r.y, color_top);
			line_to(right, bottom, color_right);
			line_to(r.x, bottom, color_bottom);
			line_to(r.x, r.y, color_left);
		}

		void graphics::round_rectangle(int x, int y, unsigned width, unsigned height, unsigned radius_x, unsigned radius_y, color_t color, bool solid, color_t color_if_solid)
		{
			if(handle_)
			{
#if defined(NANA_WINDOWS)
				handle_->pen.set(handle_->context, PS_SOLID, 1, color);
				if(solid)
				{
					handle_->brush.set(handle_->context, handle_->brush.Solid, color_if_solid);
					::RoundRect(handle_->context, x, y, x + static_cast<int>(width), y + static_cast<int>(height), static_cast<int>(radius_x * 2), static_cast<int>(radius_y * 2));
				}
				else
				{
					handle_->brush.set(handle_->context, handle_->brush.Solid, color);
					handle_->round_region.set(nana::rectangle(x, y, width, height), radius_x , radius_y);
					::FrameRgn(handle_->context, handle_->round_region.handle, handle_->brush.handle, 1, 1);
				}
				if(changed_ == false) changed_ = true;
#elif defined(NANA_X11)
				if(solid && (color == color_if_solid))
				{
					rectangle(x, y, width, height, color, true);
				}
				else
				{
					rectangle(x, y, width, height, color, false);
					if(solid)
						rectangle(x + 1, y + 1, width - 2, height - 2, color_if_solid, true);
				}
#endif
			}
		}

		void graphics::round_rectangle(const nana::rectangle& r, unsigned radius_x, unsigned radius_y, color_t color, bool solid, color_t color_if_solid)
		{
			round_rectangle(r.x, r.y, r.width, r.height, radius_x, radius_y, color, solid, color_if_solid);
		}

		void graphics::shadow_rectangle(const nana::rectangle& r, color_t beg_color, color_t end_color, bool vertical)
		{
#if defined(NANA_WINDOWS)
			if(pxbuf_.open(handle_))
			{
				pxbuf_.shadow_rectangle(r, beg_color, end_color, 0.0, vertical);
				pxbuf_.paste(handle_, 0, 0);
			}
#elif defined(NANA_X11)
			shadow_rectangle(r.x, r.y, r.width, r.height, beg_color, end_color, vertical);
#endif
		}

		void graphics::shadow_rectangle(int x, int y, unsigned width, unsigned height, color_t color_begin, color_t color_end, bool vertical)
		{
#if defined(NANA_WINDOWS)
			if(pxbuf_.open(handle_))
			{
				pxbuf_.shadow_rectangle(nana::rectangle(x, y, width, height), color_begin, color_end, 0.0, vertical);
				pxbuf_.paste(handle_, 0, 0);
			}
#elif defined(NANA_X11)
			if(0 == handle_) return;

			int deltapx = int(vertical ? height : width);
			double r, g, b;
			const double delta_r = int(((color_end & 0xFF0000) >> 16) - (r = ((color_begin & 0xFF0000) >> 16))) / double(deltapx);
			const double delta_g = int(((color_end & 0xFF00) >> 8) - (g = ((color_begin & 0xFF00) >> 8))) / double(deltapx);
			const double delta_b = int((color_end & 0xFF) - (b = (color_begin & 0xFF))) / double(deltapx);

			unsigned last_color =  (int(r) << 16) | (int(g) << 8) | int(b);

			Display * disp = nana::detail::platform_spec::instance().open_display();
			handle_->fgcolor(last_color);
			const int endpos = deltapx + (vertical ? y : x);
			if(endpos > 0)
			{
				if(vertical)
				{
					int x1 = x, x2 = x + static_cast<int>(width);
					for(; y < endpos; ++y)
					{
						::XDrawLine(disp, handle_->pixmap, handle_->context, x1, y, x2, y);
						unsigned new_color = (int(r += delta_r) << 16) | (int(g += delta_g) << 8) | int(b += delta_b);
						if(new_color != last_color)
						{
							last_color = new_color;
							handle_->fgcolor(last_color);
						}
					}
				}
				else
				{
					int y1 = y, y2 = y + static_cast<int>(height);
					for(; x < endpos; ++x)
					{
						::XDrawLine(disp, handle_->pixmap, handle_->context, x, y1, x, y2);
						unsigned new_color = (int(r += delta_r) << 16) | (int(g += delta_g) << 8) | int(b += delta_b);
						if(new_color != last_color)
						{
							last_color = new_color;
							handle_->fgcolor(last_color);
						}
					}
				}
			}
#endif
			if(changed_ == false) changed_ = true;
		}

		void graphics::line(int x1, int y1, int x2, int y2, color_t color)
		{
			if(!handle_)	return;
#if defined(NANA_WINDOWS)
			if(x1 != x2 || y1 != y2)
			{
				handle_->pen.set(handle_->context, PS_SOLID, 1, color);

				::MoveToEx(handle_->context, x1, y1, 0);
				::LineTo(handle_->context, x2, y2);
			}
			::SetPixel(handle_->context, x2, y2, NANA_RGB(color));
#elif defined(NANA_X11)
			Display* disp = nana::detail::platform_spec::instance().open_display();
			handle_->fgcolor(color);
			::XDrawLine(disp, handle_->pixmap, handle_->context, x1, y1, x2, y2);
#endif
			if(changed_ == false) changed_ = true;
		}

		void graphics::line(const point& beg, const point& end, color_t color)
		{
			line(beg.x, beg.y, end.x, end.y, color);
		}

		void graphics::lines(const point* points, std::size_t n_of_points, color_t color)
		{
			if(!handle_ || nullptr == points || 0 == n_of_points)	return;
#if defined(NANA_WINDOWS)

			handle_->pen.set(handle_->context, PS_SOLID, 1, color);

			::MoveToEx(handle_->context, points->x, points->y, nullptr);
			const point * end = points + n_of_points;
			for(const point * i = points + 1; i != end; ++i)
				::LineTo(handle_->context, i->x, i->y);

			if(*points != *(end - 1))
				::SetPixel(handle_->context, (end-1)->x, (end-1)->y, NANA_RGB(color));
#elif defined(NANA_X11)
			Display* disp = nana::detail::platform_spec::instance().open_display();
			handle_->fgcolor(color);

			XPoint * const x11points = new XPoint[n_of_points];
			XPoint * end = x11points + n_of_points;
			for(XPoint * i = x11points; i != end; ++i)
			{
				i->x = points->x;
				i->y = points->y;
				++points;
			}
			::XDrawLines(disp, handle_->pixmap, handle_->context, x11points, static_cast<int>(n_of_points), CoordModePrevious);
			delete [] x11points;
#endif
			if(changed_ == false) changed_ = true;
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

		void graphics::line_to(int x, int y, color_t color)
		{
			if(!handle_)	return;
#if defined(NANA_WINDOWS)
			handle_->pen.set(handle_->context, PS_SOLID, 1, color);
			::LineTo(handle_->context, x, y);
#elif defined(NANA_X11)
			Display* disp = nana::detail::platform_spec::instance().open_display();
			handle_->fgcolor(color);
			::XDrawLine(disp, handle_->pixmap, handle_->context,
						handle_->line_begin_pos.x, handle_->line_begin_pos.y,
						x, y);
			handle_->line_begin_pos.x = x;
			handle_->line_begin_pos.y = y;
#endif
			if(changed_ == false) changed_ = true;
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
				s_pixbuf.attach(handle_, size());

				s_pixbuf.blend(s_r, dst.handle_, d_pos, fade_rate);

				if(dst.changed_ == false) dst.changed_ = true;
			}
		}

		void graphics::blend(const nana::rectangle& r, nana::color_t color, double fade_rate)
		{
			if(handle_)
			{
				nana::paint::detail::blend(handle_, r, color, fade_rate);
				if(changed_ == false) changed_ = true;
			}
		}

		void graphics::blur(const nana::rectangle& r, std::size_t radius)
		{
			if(handle_)
			{
				pixel_buffer pixbuf(handle_, 0, 0);
				pixbuf.blur(r, radius);
				pixbuf.paste(handle_, 0, 0);
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

				nana::pixel_rgb_t * pixels = pixbuf.raw_ptr(0);

				const nana::size sz = paint::detail::drawable_size(handle_);
				const int rest = sz.width % 4;
				const int length_align4 = sz.width - rest;

				for(unsigned y = 0; y < sz.height; ++y)
				{
					pixel_rgb_t * end = pixels + length_align4;
					for(; pixels < end; pixels += 4)
					{
						unsigned char gray = static_cast<unsigned char>(table_red[pixels[0].u.element.red] + table_green[pixels[0].u.element.green] + table_blue[pixels[0].u.element.blue] + 0.5f);
						pixels[0].u.color = gray << 16 | gray << 8| gray;

						gray = static_cast<unsigned char>(table_red[pixels[1].u.element.red] + table_green[pixels[1].u.element.green] + table_blue[pixels[1].u.element.blue] + 0.5f);
						pixels[1].u.color = gray << 16 | gray << 8| gray;

						gray = static_cast<unsigned char>(table_red[pixels[2].u.element.red] + table_green[pixels[2].u.element.green] + table_blue[pixels[2].u.element.blue] + 0.5f);
						pixels[2].u.color = gray << 16 | gray << 8| gray;

						gray = static_cast<unsigned char>(table_red[pixels[3].u.element.red] + table_green[pixels[3].u.element.green] + table_blue[pixels[3].u.element.blue] + 0.5f);
						pixels[3].u.color = gray << 16 | gray << 8| gray;
					}

					for(int i = 0; i < rest; ++i)
					{
						unsigned char gray = static_cast<unsigned char>(table_red[pixels[i].u.element.red] + table_green[pixels[i].u.element.green] + table_blue[pixels[i].u.element.blue] + 0.5f);
						pixels[i].u.element.red = gray;
						pixels[i].u.element.green = gray;
						pixels[i].u.element.blue = gray;
					}

					pixels += rest;
				}
				delete [] tablebuf;

				pixbuf.paste(handle_, 0, 0);
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

		void graphics::release()
		{
			dwptr_.reset();
			handle_ = nullptr;
			size_.width = size_.height = 0;
		}

		void graphics::save_as_file(const char* file)
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

				HDC hdcMem = ::CreateCompatibleDC(handle_->context);
				BYTE *pData = nullptr;
				HBITMAP hBmp = CreateDIBSection(hdcMem, &bmpInfo, DIB_RGB_COLORS, reinterpret_cast<void**>(&pData), 0, 0);

				::SelectObject(hdcMem, hBmp);

				BitBlt(hdcMem, 0, 0, iWidth, iHeight, handle_->context, 0, 0, SRCCOPY);

				BITMAPFILEHEADER bmFileHeader = {0};
				bmFileHeader.bfType = 0x4d42;  //bmp
				bmFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
				bmFileHeader.bfSize = bmFileHeader.bfOffBits + ((bmpInfo.bmiHeader.biWidth * bmpInfo.bmiHeader.biHeight) * 3); ///3=(24 / 8)

				HANDLE hFile = CreateFileA(file,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
				DWORD dwWrite = 0;
				WriteFile(hFile,&bmFileHeader,sizeof(BITMAPFILEHEADER),&dwWrite,NULL);
				WriteFile(hFile,&bmpInfo.bmiHeader, sizeof(BITMAPINFOHEADER),&dwWrite,NULL);
				WriteFile(hFile,pData, iWidth * iHeight * 3,&dwWrite,NULL);
				CloseHandle(hFile);

				::DeleteObject(hBmp);
				::DeleteDC(hdcMem);
#elif defined(NANA_X11)

#endif
			}
		}

		color_t graphics::mix(color_t a, color_t b, double fade_rate)
		{
			pixel_rgb_t pa, pb, ret;
			ret.u.color = 0;
			pa.u.color = a;
			pb.u.color = b;

			ret.u.element.red = static_cast<unsigned char>(pa.u.element.red * fade_rate + pb.u.element.red * (1 - fade_rate));
			ret.u.element.green = static_cast<unsigned char>(pa.u.element.green * fade_rate + pb.u.element.green * (1 - fade_rate));
			ret.u.element.blue = static_cast<unsigned char>(pa.u.element.blue * fade_rate + pb.u.element.blue * (1 - fade_rate));

			return ret.u.color;
		}
	//end class graphics

}//end namespace paint
}//end namespace nana
