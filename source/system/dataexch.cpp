/*
 *	Data Exchanger Implementation
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file:			nana/system/dataexch.cpp
 *	@description:	An implementation of a data exchange mechanism through Windows Clipboard, X11 Selection.
 */

#include <nana/system/dataexch.hpp>
#include <nana/traits.hpp>
#include <nana/paint/graphics.hpp>
#include <vector>
#include <cassert>

#if defined(NANA_WINDOWS)
	#include <windows.h>
#elif defined(NANA_X11)
	#include <nana/detail/platform_spec_selector.hpp>
	#include <nana/gui/detail/bedrock.hpp>
	#include <nana/gui/detail/basic_window.hpp>
#endif

namespace nana{ namespace system{

	//class dataexch
		void dataexch::set(const nana::char_t* text)
		{
			_m_set(std::is_same<char, nana::char_t>::value ? format::text : format::unicode, text, (nana::strlen(text) + 1) * sizeof(nana::char_t));
		}

		void dataexch::set(const nana::string& text)
		{
			_m_set(std::is_same<char, nana::char_t>::value ? format::text : format::unicode, text.c_str(), (text.length() + 1) * sizeof(nana::char_t));
		}

		bool dataexch::set(const nana::paint::graphics& g)
		{
#if defined(NANA_WINDOWS)
			size sz = g.size();
			paint::pixel_buffer pbuffer;
			rectangle r;
			r.x = 0;
			r.y = 0;
			r.width = sz.width;
			r.height = sz.height;
			pbuffer.attach(g.handle(), r);
			size_t bytes_per_line = pbuffer.bytes_per_line();
			size_t bitmap_bytes = bytes_per_line * r.height;

			struct {
				BITMAPINFOHEADER bmiHeader;
				RGBQUAD bmiColors[256];
			} bmi = {0};
			bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			HDC hDC = ::GetDC(NULL);
			if (::GetDIBits(hDC, (HBITMAP)g.pixmap(), 0, 1, NULL, (BITMAPINFO *)&bmi, DIB_RGB_COLORS) == 0) {
				assert(false);
				::ReleaseDC(NULL, hDC);
				return false;
			}
			if (!::ReleaseDC(NULL, hDC)) {
				return false;
			}

			size_t header_size = sizeof(bmi.bmiHeader);

			// Bitmaps are huge, so to avoid unnegligible extra copy, this routine does not use private _m_set method.
			HGLOBAL h_gmem = ::GlobalAlloc(GHND | GMEM_SHARE, header_size + bitmap_bytes);
			void * gmem = ::GlobalLock(h_gmem);
			if (gmem) {
                char* p = (char*)gmem;
                // Fix BITMAPINFOHEADER obtained from GetDIBits WinAPI
                bmi.bmiHeader.biCompression = BI_RGB;
                bmi.bmiHeader.biHeight = ::abs(bmi.bmiHeader.biHeight);
                memcpy(p, &bmi, header_size);
                p += header_size;
                // many programs do not support bottom-up DIB, so reversing row order is needed.
                for (int y=0; y<bmi.bmiHeader.biHeight; ++y) {
                    memcpy(p, pbuffer.raw_ptr(bmi.bmiHeader.biHeight - 1 - y), bytes_per_line);
                    p += bytes_per_line;
                }
                if (::GlobalUnlock(h_gmem) || GetLastError() == NO_ERROR)
                    if (::OpenClipboard(::GetFocus()))
                        if (::EmptyClipboard())
                            if (::SetClipboardData(CF_DIB, h_gmem))
                                if (::CloseClipboard())
                                    return true;
			}
		    assert(false);
			::GlobalFree(h_gmem);
			return false;

//#elif defined(NANA_X11)
#else
			throw "not implemented yet.";
			return false;
#endif
		}

		void dataexch::get(nana::string& str)
		{
			std::size_t size;
			void* res = _m_get(std::is_same<char, nana::char_t>::value ? format::text : format::unicode, size);
			if(res)
			{
#if defined(NANA_X11) && defined(NANA_UNICODE)
				nana::detail::charset_conv conv("UTF-32", "UTF-8");
				const std::string & utf32str = conv.charset(reinterpret_cast<char*>(res), size);
				const nana::char_t * utf32ptr = reinterpret_cast<const nana::char_t*>(utf32str.c_str());
				str.append(utf32ptr + 1, utf32ptr + utf32str.size() / sizeof(nana::char_t));
#else
				str.reserve(size / sizeof(nana::char_t));
				str.append(reinterpret_cast<nana::char_t*>(res), reinterpret_cast<nana::char_t*>(res) + size / sizeof(nana::char_t));
#endif
				auto pos = str.find_last_not_of(nana::char_t(0));
				if(pos != str.npos)
					str.erase(pos + 1);

#if defined(NANA_X11)
				::XFree(res);
#endif
			}
		}
	//private:
		bool dataexch::_m_set(unsigned type, const void* buf, std::size_t size)
		{
			bool res = false;
#if defined(NANA_WINDOWS)
			if(type < format::end && ::OpenClipboard(::GetFocus()))
			{
				if(::EmptyClipboard())
				{
					HGLOBAL g = ::GlobalAlloc(GHND | GMEM_SHARE, size);
					void * addr = ::GlobalLock(g);

					memcpy(addr, buf, size);
					::GlobalUnlock(g);

					switch(type)
					{
					case format::text:		type = CF_TEXT;			break;
					case format::unicode:	type = CF_UNICODETEXT;	break;
					case format::pixmap:	type = CF_BITMAP;		break;
					}
					HANDLE h = ::SetClipboardData(type, g);
					res = (h != NULL);
				}
				::CloseClipboard();
			}
#elif defined(NANA_X11)
			auto & spec = ::nana::detail::platform_spec::instance();
			native_window_type owner = nullptr;
			{
				internal_scope_guard lock;
				auto wd = detail::bedrock::instance().focus();
				if(wd)	owner = wd->root;
			}

			if(owner)
			{
				Atom atom_type;
				switch(type)
				{
				case format::text:	atom_type = XA_STRING;			break;
				case format::unicode:	atom_type = spec.atombase().utf8_string;	break;
				default:
					return false;
				}
	#if defined(NANA_UNICODE)
				//The internal clipboard stores UTF8_STRING, the parameter string should be converted from utf32 to utf8.
				nana::detail::charset_conv conv("UTF-8", "UTF-32");
				std::string utf8str = conv.charset(reinterpret_cast<const char*>(buf), size);
				buf = utf8str.c_str();
				size = utf8str.size();
	#endif
				spec.write_selection(owner, atom_type, buf, size);
				return true;
			}
#endif
			return res;
		}

		void* dataexch::_m_get(unsigned type, size_t& size)
		{
			void* res = 0;
#if defined(NANA_WINDOWS)
			if(type < format::end && ::OpenClipboard(::GetFocus()))
			{
				switch(type)
				{
				case format::text:		type = CF_TEXT;			break;
				case format::unicode:	type = CF_UNICODETEXT;	break;
				case format::pixmap:	type = CF_BITMAP;		break;
				}
				HANDLE handle = ::GetClipboardData(type);
				if(handle)
				{
					res = reinterpret_cast<HGLOBAL>(::GlobalLock(handle));
					if(res)
						size = ::GlobalSize(handle);
				}

				::CloseClipboard();
			}
#elif defined(NANA_X11)
			nana::detail::platform_spec & spec = nana::detail::platform_spec::instance();
			native_window_type requester = nullptr;
			spec.lock_xlib();

			{
				internal_scope_guard isg;
				detail::bedrock::core_window_t * wd = detail::bedrock::instance().focus();
				if(wd)	requester = wd->root;
			}
			spec.unlock_xlib();

			if(requester)
			{
				Atom atom;

				switch(type)
				{
				case format::text:		atom = XA_STRING;			break;
				case format::unicode:	atom = spec.atombase().utf8_string;	break;
				default:
					return 0;
				}
				res = spec.request_selection(requester, atom, size);
			}
#endif
			return res;
		}
	//end class dataexch

}//end namespace system
}//end namespace nana

