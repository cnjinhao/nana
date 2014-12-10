/*
 *	Data Exchanger Implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
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
#if defined(NANA_WINDOWS)
	#include <windows.h>
#elif defined(NANA_X11)
	#include PLATFORM_SPEC_HPP
	#include GUI_BEDROCK_HPP
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
				nana::string::size_type pos = str.find_last_not_of(nana::char_t(0));
				if(pos != str.npos)
					str.erase(pos + 1);
#endif

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
					::SetClipboardData(type, g);
					res = true;
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

