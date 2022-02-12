#include <set>
#include <mutex>

#include "platform_abstraction.hpp"
#include <nana/deploy.hpp>
#include <nana/paint/detail/ptdefs.hpp>
#include "../paint/truetype.hpp"
#include <nana/gui/detail/native_window_interface.hpp>
#include <nana/gui/detail/internal_scope_guard.hpp>
#include <nana/system/platform.hpp>


#ifdef NANA_WINDOWS

#	ifndef _WIN32_WINNT
#		define _WIN32_WINNT  0x0501
#	endif

#	include <windows.h>
///////////////////////////////////////////////////////////////////////////////////////////////////////

/******************************************************************
*                                                                 *
*  VersionHelpers.h -- This module defines helper functions to    *
*                      promote version check with proper          *
*                      comparisons.                               *
*                                                                 *
*  Copyright (c) Microsoft Corp.  All rights reserved.            *
*                                                                 *
******************************************************************/

#include <specstrings.h>    // for _In_, etc.

#if !defined(__midl) && !defined(SORTPP_PASS)

#if (NTDDI_VERSION >= NTDDI_WINXP)

#ifdef __cplusplus

#define VERSIONHELPERAPI inline bool

#else  // __cplusplus

#define VERSIONHELPERAPI FORCEINLINE BOOL

#endif // __cplusplus

VERSIONHELPERAPI
IsWindowsVersionOrGreater(WORD wMajorVersion, WORD wMinorVersion, WORD wServicePackMajor)
{
	OSVERSIONINFOEXW osvi = { sizeof(osvi), 0, 0, 0, 0, { 0 }, 0, 0 };
	DWORDLONG        const dwlConditionMask = VerSetConditionMask(
		VerSetConditionMask(
		VerSetConditionMask(
		0, VER_MAJORVERSION, VER_GREATER_EQUAL),
		VER_MINORVERSION, VER_GREATER_EQUAL),
		VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);

	osvi.dwMajorVersion = wMajorVersion;
	osvi.dwMinorVersion = wMinorVersion;
	osvi.wServicePackMajor = wServicePackMajor;

	return VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR, dwlConditionMask) != FALSE;
}

#ifndef _WIN32_WINNT_WINXP
#	define _WIN32_WINNT_WINXP                  0x0501
#endif // _WIN32_WINNT_WINXP

#ifndef _WIN32_WINNT_VISTA
#	define _WIN32_WINNT_VISTA                  0x0600
#endif // _WIN32_WINNT_VISTA

#ifndef _WIN32_WINNT_WIN7
#	define _WIN32_WINNT_WIN7                   0x0601
#endif // _WIN32_WINNT_WIN7


VERSIONHELPERAPI
IsWindowsXPOrGreater()
{
	return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WINXP), LOBYTE(_WIN32_WINNT_WINXP), 0);
}

VERSIONHELPERAPI
IsWindowsXPSP1OrGreater()
{
	return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WINXP), LOBYTE(_WIN32_WINNT_WINXP), 1);
}

VERSIONHELPERAPI
IsWindowsXPSP2OrGreater()
{
	return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WINXP), LOBYTE(_WIN32_WINNT_WINXP), 2);
}

VERSIONHELPERAPI
IsWindowsXPSP3OrGreater()
{
	return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WINXP), LOBYTE(_WIN32_WINNT_WINXP), 3);
}

VERSIONHELPERAPI
IsWindowsVistaOrGreater()
{
	return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_VISTA), LOBYTE(_WIN32_WINNT_VISTA), 0);
}

VERSIONHELPERAPI
IsWindowsVistaSP1OrGreater()
{
	return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_VISTA), LOBYTE(_WIN32_WINNT_VISTA), 1);
}

VERSIONHELPERAPI
IsWindowsVistaSP2OrGreater()
{
	return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_VISTA), LOBYTE(_WIN32_WINNT_VISTA), 2);
}

VERSIONHELPERAPI
IsWindows7OrGreater()
{
	return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WIN7), LOBYTE(_WIN32_WINNT_WIN7), 0);
}

VERSIONHELPERAPI
IsWindows7SP1OrGreater()
{
	return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WIN7), LOBYTE(_WIN32_WINNT_WIN7), 1);
}

#ifndef	_WIN32_WINNT_WIN8    //  (0x0602)
#define	_WIN32_WINNT_WIN8 (0x0602)
#endif  //	_WIN32_WINNT_WIN8(0x0602)

VERSIONHELPERAPI
IsWindows8OrGreater()
{

	return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WIN8), LOBYTE(_WIN32_WINNT_WIN8), 0);
}

#ifndef	_WIN32_WINNT_WINBLUE   // (0x0602)
#define	_WIN32_WINNT_WINBLUE (0x0602)
#endif  //	_WIN32_WINNT_WINBLUE (0x0602)


#endif // NTDDI_VERSION

#endif // defined(__midl)
#else
#	include "posix/platform_spec.hpp"
#	include <fontconfig/fontconfig.h>
#	if defined(NANA_USE_XFT)
#		include <nana/unicode_bidi.hpp>
#		include "text_reshaping.hpp"
#		include <X11/Xft/Xft.h>
#		include <iconv.h>
#		include <fstream>
#	endif
#endif

namespace nana
{
#ifdef NANA_USE_XFT

	// Manages fallback font
	class fallback_font_store
	{
	public:
		struct font_element
		{
			::XftFont * handle;
			std::string family;
			std::vector<std::string> patterns;

			~font_element()
			{
				auto disp = ::nana::detail::platform_spec::instance().open_display();
				::XftFontClose(disp, handle);
			}
		};
	
		std::shared_ptr<font_element> open(Display* disp, const std::string& patstr, const std::map<std::string, std::string>& exclude)
		{
			{
				auto i = store_.find(patstr);
				if(i != store_.end())
					return i->second;
			}
			
			auto pat = ::XftNameParse(patstr.c_str());
			XftResult res;
			auto match_pat = ::XftFontMatch(disp, ::XDefaultScreen(disp), pat, &res);

			if (match_pat)
			{
				std::string family_name;
				char * sf;
				if(XftResultTypeMismatch != ::XftPatternGetString(match_pat, "family", 0, &sf))
				{
					family_name = sf;

					//Avoid loading the same font again
					auto i = exclude.find(family_name);
					if(i != exclude.cend())
					{
						auto & fe = store_[i->second];
						fe->patterns.push_back(patstr);
						store_[patstr] = fe;
						return {};
					}
				}

				auto xft = ::XftFontOpenPattern(disp, match_pat);
				if(xft)
				{
					auto fe = std::make_shared<font_element>();
					fe->handle = xft;
					fe->family = family_name;
					fe->patterns.push_back(patstr);
					
					store_[patstr] = fe;
					return fe;
				}
			}
			return {};
		}

		void free()
		{
			for(auto i = store_.cbegin(); i != store_.cend(); )
			{
				if(static_cast<std::size_t>(i->second.use_count()) == i->second->patterns.size())
				{
					i = store_.erase(i);
				}
				else
					++i;
			}
		}

		std::size_t size() const
		{
			return store_.size();
		}
	private:
		std::map<std::string, std::shared_ptr<font_element>> store_;
	};


	//A fallback fontset provides the multiple languages support.
	class fallback_fontset
	{
	public:
		using font_element = fallback_font_store::font_element;

		fallback_fontset(fallback_font_store& store):
			disp_(::nana::detail::platform_spec::instance().open_display()),
			store_(store)
		{
		}

		~fallback_fontset()
		{
			_m_clear();
		}

		void open(const std::string& font_desc, const std::set<std::string>& langs)
		{
			_m_clear();

			std::map<std::string, std::string> exclude; //family -> pattern
			for(auto & lang : langs)
			{
				std::string patstr = "*" + font_desc + ":lang=" + lang;

				auto fe = store_.open(disp_, patstr, exclude);
				if(fe)
				{
					if(!fe->family.empty())
						exclude[fe->family] = patstr;

					xftset_.push_back(fe);
				}	
			}
		}

		int draw(::XftDraw* xftdraw, ::XftColor * xftcolor, ::XftFont* xft, int x, int y, const wchar_t* str, std::size_t len)
		{
			if(nullptr == str || 0 == len)
				return 0;

			int const init_x = x;
			std::unique_ptr<FT_UInt[]> glyph_indexes(new FT_UInt[len]);

			//The RTL and shaping should be handled manually, because the libXft and X doesn't support these language features.
			std::wstring rtl;
			auto ents = unicode_reorder(str, len);

			nana::internal_scope_guard lock;
			for(auto & e : ents)
			{
				auto size = static_cast<std::size_t>(e.end - e.begin);
				auto p = e.begin;

				if(unicode_bidi::is_text_right(e))
				{
					auto restr = nana::reshaping::arabic::reshape(std::wstring{e.begin, e.end});
					rtl.assign(restr.crbegin(), restr.crend());

					p = rtl.c_str();
					size = rtl.size();
				}

				while(true)
				{
					//Scan the string until the character which font is not same with the font of the first character where the scan begins.
					auto preferred = _m_scan_fonts(xft, p, size, glyph_indexes.get());
					x += _m_draw(xftdraw, xftcolor, preferred.first, x, y, p, preferred.second, glyph_indexes.get());

					if(size == preferred.second)
						break;

					size -= preferred.second;
					p += preferred.second;
				}

				rtl.clear();
			}

			return x - init_x;
		}

		std::unique_ptr<unsigned[]> glyph_pixels(::XftFont* xft, const wchar_t* str, std::size_t len)
		{
			if(nullptr == xft || nullptr == str || 0 == len)
				return {};

			std::unique_ptr<FT_UInt[]> glyph_indexes{new FT_UInt[len]};

			std::unique_ptr<unsigned[]> pxbuf{new unsigned[len]};

			auto pbuf = pxbuf.get();

			nana::internal_scope_guard lock;

			//Don't reverse the string
			_m_reorder_reshaping(std::wstring_view{str, len}, false, [&,xft, str](const wchar_t* p, std::size_t size, const wchar_t* pstr) mutable{
				while(true)
				{
					auto preferred = _m_scan_fonts(xft, p, size, glyph_indexes.get());

					_m_glyph_px(preferred.first, p, preferred.second, glyph_indexes.get(), pbuf + (pstr - str));

					if(size == preferred.second)
						break;

					size -= preferred.second;
					p += preferred.second;
					pstr += preferred.second;
				}				
			});

			return pxbuf;
		}

		nana::size extents(::XftFont* xft, const wchar_t* str, std::size_t len)
		{
			nana::size extent;

			if(nullptr == str || 0 == len)
				return extent;

			std::unique_ptr<FT_UInt[]> glyph_indexes(new FT_UInt[len]);

			nana::internal_scope_guard lock;

			//Don't reverse the string
			_m_reorder_reshaping(std::wstring_view{str, len}, false, [&,xft, str](const wchar_t* p, std::size_t size, const wchar_t* /*pstr*/) mutable{
				while(true)
				{
					auto preferred = _m_scan_fonts(xft, p, size, glyph_indexes.get());

					extent.width += _m_extents(preferred.first, p, preferred.second, glyph_indexes.get());

					if(preferred.first->ascent + preferred.first->descent > static_cast<int>(extent.height))
						extent.height = preferred.first->ascent + preferred.first->descent;

					if(size == preferred.second)
						break;

					size -= preferred.second;
					p += preferred.second;
				}				
			});

			return extent;
		}
	private:

		void _m_clear()
		{
			xftset_.clear();
			store_.free();
		}

		/// @param reverse Indicates whether to reverse the string, it only reverse the RTL language string.
		template<typename Function>
		void _m_reorder_reshaping(std::wstring_view str, bool reverse, Function fn)
		{
			//The RTL and shaping should be handled manually, because the libXft and X doesn't support these language features.
			std::wstring rtl;
			auto ents = unicode_reorder(str.data(), str.size());
			for(auto & e : ents)
			{
				auto size = static_cast<std::size_t>(e.end - e.begin);
				auto p = e.begin;

				if(unicode_bidi::is_text_right(e))
				{
					//Reshape the str
					auto restr = nana::reshaping::arabic::reshape(std::wstring{e.begin, e.end});
					
					if(reverse)
						rtl.assign(restr.crbegin(), restr.crend());
					else
						rtl.swap(restr);

					p = rtl.c_str();
					size = rtl.size();
				}

				fn(p, size, e.begin);

				rtl.clear();
			}
		}

		//Tab is a invisible character
		int _m_draw(::XftDraw* xftdraw, ::XftColor* xftcolor, ::XftFont* xft, int x, int y, const wchar_t* str, std::size_t len, const FT_UInt* glyph_indexes)
		{
			int const init_x = x;

			auto p = str;
			auto const end = str + len;

			y += xft->ascent;

			::XGlyphInfo ext;
			while(p < end)
			{
				auto off = p - str;
				auto ptab = _m_find_tab(p, end);
				if(ptab == p)
				{
					++p;
					continue;
				}

				auto const size = ptab - p;

				::XftDrawGlyphs(xftdraw, xftcolor, xft, x, y, glyph_indexes + off, size);
				::XftGlyphExtents(disp_, xft, glyph_indexes + off, size, &ext);
				
				x += ext.xOff;

				if(ptab == end)
					break;

				p = ptab + 1;
			}

			return x - init_x;
		}

		//Tab is a invisible character
		unsigned _m_extents(::XftFont* xft, const wchar_t* const str, const std::size_t len, const FT_UInt* glyph_indexes)
		{
			unsigned pixels = 0;
			auto p = str;
			auto const end = str + len;

			::XGlyphInfo ext;
			while(p < end)
			{
				auto off = p - str;
				auto ptab = _m_find_tab(p, end);
				if(ptab == p)
				{
					++p;
					//extents->xOff += tab_pixels_;
					continue;
				}

				::XftGlyphExtents(disp_, xft, glyph_indexes + off, ptab - p, &ext);

				pixels += ext.xOff;

				if(end == ptab)
					break;
				p = ptab + 1;
			}

			return pixels;
		}

		//Tab is a invisible character
		void _m_glyph_px(::XftFont* xft, const wchar_t* str, std::size_t len, const FT_UInt* glyph_indexes, unsigned* pxbuf)
		{
			auto const end = str + len;

			::XGlyphInfo extent;
			for(auto p = str; p < end; ++p)
			{
				if('\t' != *p)
				{
					::XftGlyphExtents(disp_, xft, glyph_indexes, 1, &extent);
					*pxbuf++ = extent.xOff;
				}
				else
					*pxbuf++ = 0;//tab_pixels_;

				++glyph_indexes;
			}
		}

		static const wchar_t* _m_find_tab(const wchar_t* begin, const wchar_t* end)
		{
			while(begin < end)
			{
				if('\t' == *begin)
					return begin;

				++begin;
			}
			return end;
		}

		std::pair<::XftFont*, std::size_t> _m_scan_fonts(::XftFont* xft, const wchar_t* str, std::size_t len, FT_UInt* const glyphs) const
		{
			auto preferred = xft;
			auto idx = ::XftCharIndex(disp_, xft, *str);
			if(0 == idx)
			{
				for(auto ft : xftset_)
				{
					idx = ::XftCharIndex(disp_, ft->handle, *str);
					if(idx)
					{
						preferred = ft->handle;
						break;
					}
				}				
			}

			*glyphs = idx;

			if(0 == idx)
			{
				//scan the str with all fonts until a char index is found.
				for(std::size_t i = 1; i < len; ++i)
				{
					if(::XftCharIndex(disp_, xft, str[i]))
						return {preferred, i};

					for(auto ft : xftset_)
					{
						if(::XftCharIndex(disp_, ft->handle, str[i]))
							return {preferred, i};
					}
					glyphs[i] = 0;
				}

				return {preferred, len};
			}

			//scan the str with preferred font until a char index is invalid.
			for(std::size_t i = 1; i < len; ++i)
			{
				idx = ::XftCharIndex(disp_, preferred, str[i]);
				if(0 == idx)
					return {preferred, i};

				glyphs[i] = idx;
			}

			return {preferred, len};
		}
	private:
		Display* const disp_;
		fallback_font_store& store_;
		std::vector<std::shared_ptr<font_element>> xftset_;
	};

	/// Fallback fontset manager
	class fallback_manager
	{
	public:
		fallback_manager():
			langs_(_m_split_lang("ar,hi,zh-cn,zh-tw,ja,ko,th"))
		{
		}

		void languages(const std::string& lang)
		{
			langs_ = _m_split_lang(lang);

			for(auto & xft : xft_table_)
			{
				xft.second->open(xft.first, langs_);
			}
		}

		std::shared_ptr<fallback_fontset> make_fallback(const std::string& font_desc)
		{
			auto i = xft_table_.find(font_desc);
			if(i != xft_table_.end())
				return i->second;

			auto fb = std::make_shared<fallback_fontset>(store_);
			fb->open(font_desc, langs_);
			xft_table_[font_desc] = fb;

			return fb;
		}

		void release_fallback(std::shared_ptr<fallback_fontset>& p)
		{
			for(auto i = xft_table_.cbegin(); i != xft_table_.cend(); ++i)
			{
				if(i->second == p)
				{
					if(p.use_count() <= 2)
						xft_table_.erase(i);
					break;
				}
			}
		}

	private:
		static std::set<std::string> _m_split_lang(const std::string& lang)
		{
			std::set<std::string> langs;
			std::size_t start_pos = 0;
			while(true)
			{
				auto pos = lang.find(',', start_pos);
				auto l = lang.substr(start_pos, lang.npos == pos? lang.npos : pos - start_pos);

				if(!l.empty())
					langs.insert(l);

				if(lang.npos == pos)
					break;

				start_pos = pos + 1;
			}

			return langs;
		}
	private:
		std::set<std::string> langs_;
		fallback_font_store store_;
		std::map<std::string, std::shared_ptr<fallback_fontset>> xft_table_;
	};
#endif

	class internal_font
		: public font_interface
	{
	public:
#ifdef NANA_WINDOWS
		using font_height_type = LONG;
#else
		using font_height_type = double;
#endif
		using path_type = std::filesystem::path;

#ifdef NANA_USE_XFT
		internal_font(const path_type& ttf, const paint::font_info& fi, font_height_type height, native_font_type native_font, std::shared_ptr<fallback_fontset> fallback) :
			ttf_(ttf),
			font_info_(fi),
			height_(height),
			native_handle_(native_font),
			fallback_(fallback)
		{}
#else
		internal_font(const path_type& ttf, const paint::font_info& fi, font_height_type height, native_font_type native_font) :
			ttf_(ttf),
			font_info_(fi),
			height_(height),
			native_handle_(native_font)
		{}
#endif

		~internal_font();


		font_height_type font_height() const
		{
			return height_;
		}
	public:
		native_font_type native_handle() const override
		{
			return native_handle_;
		}

		const paint::font_info& font_info() const override
		{
			return font_info_;
		}

#ifdef NANA_USE_XFT
		fallback_fontset* fallback() const
		{
			return fallback_.get();
		}
#endif
	private:
		path_type	const ttf_;
		paint::font_info const font_info_;
		font_height_type const height_;
		native_font_type const native_handle_;
#ifdef NANA_USE_XFT
		std::shared_ptr<fallback_fontset> fallback_;
#endif
	};


	class font_service
	{
	public:
		using font_info = paint::font_info;
		using path_type = std::filesystem::path;
#ifdef NANA_WINDOWS
		using font_height_type = LONG;
#else
		using font_height_type = double;
#endif

		~font_service()
		{
			// The font destructor may usefallback_manager,
			//therefore, all font objects must be destroyed before destroying fallback_manager
			fontbase_.clear();
		}

		// Checks whether there is a font that still has been refered.
		void check_fonts()
		{
			for (auto& font : fontbase_)
			{
				if(font.use_count() > 1)
					throw std::runtime_error("There is a font has not been closed");
			}
		}

		std::shared_ptr<font_interface> open_font(font_info fi, std::size_t dpi, const path_type& ttf)
		{
			if (0 == dpi)
			{
				dpi = platform_abstraction::current_dpi();
				if( 0 == dpi)
					dpi = detail::native_interface::system_dpi();
			}

			if (!ttf.empty())
			{
				::nana::paint::detail::truetype truetype{ ttf };
				if (truetype.font_family().empty())
					return nullptr;

				platform_abstraction::font_resource(true, ttf);

				fi.family = truetype.font_family();
			}

			auto const font_height = _m_set_default_values(fi, dpi);

			std::shared_ptr<font_interface> font;

			for (auto& f : fontbase_)
			{
				//Find the matched font info and the font height of two fonts also need to be equal.
				if (_m_compare_font_info(f->font_info(), fi))
				{
					// Compare font height
					if (font_height == static_cast<internal_font*>(f.get())->font_height())
					{
						font = f;
						break;
					}
				}
			}

			if (!font)
			{
				// The font is not matched, create a new font

				font = _m_create(fi, dpi, ttf);

				if (font)
					fontbase_.push_back(font);
			}

			// Remove the unused fonts.
			for (auto i = fontbase_.begin(); i != fontbase_.end();)
			{
				if (i->use_count() == 1)
					i = fontbase_.erase(i);
				else
					++i;
			}

			return font;
		}

#ifdef NANA_X11
		bool fc_count(bool try_add, const std::string& path)
		{
			if(try_add)
			{
				if(1 == ++(fc_table_[path]))
					return true;
			}
			else
			{
				auto i = fc_table_.find(path);
				if(i != fc_table_.end())
				{
					if(0 == --(i->second))
						fc_table_.erase(i);

					return (0 == fc_table_.size());
				}
			}

			return false;
		}
#endif

#ifdef NANA_USE_XFT
		fallback_manager& fb_manager()
		{
			return fb_manager_;
		}
#endif
	private:

		// Compares whether two font_info objects are equal
		static bool _m_compare_font_info(const font_info& a, const font_info& b)
		{
			return (
				a.family == b.family &&
				a.style.weight == b.style.weight &&
				a.style.italic == b.style.italic &&
				a.style.strike_out == b.style.strike_out &&
				a.style.underline == b.style.underline &&
				a.size_pt == b.size_pt
				);
		}

		// Returns the DPI-dependent font size, in pixels
		static font_height_type _m_set_default_values(font_info& fi, std::size_t dpi)
		{
#ifdef NANA_WINDOWS
			std::wstring wfont_family = nana::detail::to_nstring(fi.family);
			//Make sure the length of font family less than LF_FACESIZE which is defined by Windows
			if (wfont_family.length() + 1 > LF_FACESIZE)
				wfont_family.clear();

			//Translate pt to px
			auto hDC = ::GetDC(nullptr);
			auto font_height = -static_cast<LONG>(fi.size_pt * dpi / 72);

			if (wfont_family.empty() || (0 == font_height))
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
				if (!IsWindowsVistaOrGreater())
					metrics.cbSize -= sizeof(metrics.iPaddedBorderWidth);
#endif
#endif
				::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof metrics, &metrics, 0);

				if (wfont_family.empty())
				{
					wfont_family = metrics.lfMessageFont.lfFaceName;
					fi.family = to_utf8(wfont_family);
				}

				if (0 == font_height)
				{
					auto correspond_dpi = detail::native_interface::system_dpi();
					fi.size_pt = static_cast<double>(std::abs(metrics.lfMessageFont.lfHeight) * 72 / correspond_dpi);
					font_height = -static_cast<LONG>(fi.size_pt * dpi / 72);
				}
			}

			::ReleaseDC(nullptr, hDC);
			return font_height;
#else
			if(0 == fi.size_pt)
				fi.size_pt = platform_abstraction::font_default_pt();

			return fi.size_pt * dpi / 72;
#endif
		}

		std::shared_ptr<font_interface> _m_create(const font_info& fi, std::size_t dpi, const path_type& ttf)
		{
			using native_font_type = platform_abstraction::font::native_font_type;

#ifdef NANA_WINDOWS
			// The font_family and size_pt are reliable, they have been checked by _m_set_default_values
			std::wstring wfont_family = nana::detail::to_nstring(fi.family);

			//Translate pt to px
			auto hDC = ::GetDC(nullptr);
			auto font_height = -static_cast<LONG>(fi.size_pt * dpi / 72);
			::ReleaseDC(nullptr, hDC);

			::LOGFONT lf{};

			std::wcscpy(lf.lfFaceName, wfont_family.c_str());
			lf.lfHeight = font_height;
			lf.lfCharSet = DEFAULT_CHARSET;
			lf.lfWeight = fi.style.weight;
			lf.lfQuality = fi.style.antialiasing ? PROOF_QUALITY : NONANTIALIASED_QUALITY;
			lf.lfPitchAndFamily = FIXED_PITCH;
			lf.lfItalic = fi.style.italic;
			lf.lfUnderline = fi.style.underline;
			lf.lfStrikeOut = fi.style.strike_out;

			auto fd = ::CreateFontIndirect(&lf);
#elif defined(NANA_X11)
			auto disp = ::nana::detail::platform_spec::instance().open_display();
#	ifdef NANA_USE_XFT
			std::string font_family = fi.family;
			if (font_family.empty())
				font_family = "*";

			//Calculate the DPI-dependent font size
			auto dpi_size_pt = (fi.size_pt ? fi.size_pt : platform_abstraction::font_default_pt()) * dpi / 96;
			auto font_height = dpi_size_pt / 72;
			
			std::string pat_str = '-' + std::to_string(dpi_size_pt);
			if (fi.style.weight < 400)
				pat_str += ":light";
			else if (400 == fi.style.weight)
				pat_str += ":medium";
			else if (fi.style.weight < 700)
				pat_str += ":demibold";
			else
				pat_str += (700 == fi.style.weight ? ":bold" : ":black");

			if (fi.style.italic)
				pat_str += ":slant=italic";

			if (!fi.style.antialiasing)
				pat_str += ":antialias=false";

			auto pat = ::XftNameParse((font_family + pat_str).c_str());
			XftResult res;
			auto match_pat = ::XftFontMatch(disp, ::XDefaultScreen(disp), pat, &res);

			::XftFont* fd = nullptr;
			if (match_pat)
				fd = ::XftFontOpenPattern(disp, match_pat);
#	else
			std::string pat_str;
			if (font_family.empty())
				pat_str = "-misc-fixed-*";
			else
				pat_str = "-misc-fixed-" + font_family;

			char** missing_list;
			int missing_count;
			char* defstr;
			XFontSet fd = ::XCreateFontSet(display_, const_cast<char*>(pat_str.c_str()), &missing_list, &missing_count, &defstr);
#	endif
#endif

			if (fd)
			{
#ifdef NANA_USE_XFT
				auto fallback = fb_manager_.make_fallback(pat_str);
				return std::make_shared<internal_font>(ttf, fi, font_height, reinterpret_cast<native_font_type>(fd), fallback);
#else
				return std::make_shared<internal_font>(ttf, fi, font_height, reinterpret_cast<native_font_type>(fd));
#endif
			}
			return{};
		}
	private:
		std::vector<std::shared_ptr<font_interface>> fontbase_;

#ifdef NANA_X11
		std::map<std::string, std::size_t> fc_table_;
#endif
#ifdef NANA_USE_XFT
		fallback_manager fb_manager_;
#endif
	};//end class font_service


		//class revertible_mutex
			struct platform_abstraction::revertible_mutex::implementation
			{
				struct thread_refcount
				{
					thread_t tid;	//Thread ID
					std::vector<unsigned> callstack_refs;

					thread_refcount(thread_t thread_id, unsigned refs)
						: tid(thread_id)
					{
						callstack_refs.push_back(refs);
					}
				};

				std::recursive_mutex mutex;

				thread_t thread_id{ 0 };	//Thread ID
				unsigned refs{ 0 };	//Ref count

				std::vector<thread_refcount> records;
			};
		//public:
			platform_abstraction::revertible_mutex::revertible_mutex()
				: impl_(new implementation)
			{
			}

			platform_abstraction::revertible_mutex::~revertible_mutex()
			{
				delete impl_;
			}

			void  platform_abstraction::revertible_mutex::lock()
			{
				impl_->mutex.lock();

				if (0 == impl_->thread_id)
					impl_->thread_id = nana::system::this_thread_id();

				++(impl_->refs);
			}

			bool  platform_abstraction::revertible_mutex::try_lock()
			{
				if (impl_->mutex.try_lock())
				{
					if (0 == impl_->thread_id)
						impl_->thread_id = nana::system::this_thread_id();

					++(impl_->refs);
					return true;
				}
				return false;
			}

			void  platform_abstraction::revertible_mutex::unlock()
			{
				if (impl_->thread_id == nana::system::this_thread_id())
					if (0 == --(impl_->refs))
						impl_->thread_id = 0;

				impl_->mutex.unlock();				
			}

			void  platform_abstraction::revertible_mutex::revert()
			{
				if (impl_->thread_id == nana::system::this_thread_id())
				{
					auto const current_refs = impl_->refs;

					//Check if there is a record
					for (auto & r : impl_->records)
					{
						if (r.tid == impl_->thread_id)
						{
							r.callstack_refs.push_back(current_refs);
							impl_->thread_id = 0;	//Indicates a record is existing
							break;
						}
					}

					if (impl_->thread_id)
					{
						//Creates a new record
						impl_->records.emplace_back(impl_->thread_id, current_refs);
						impl_->thread_id = 0;
					}

					impl_->refs = 0;

					for (std::size_t i = 0; i < current_refs; ++i)
						impl_->mutex.unlock();
				}
				else
					throw std::runtime_error("The revert is not allowed");
			}

			void  platform_abstraction::revertible_mutex::forward()
			{
				impl_->mutex.lock();

				if (impl_->records.size())
				{
					auto const this_tid = nana::system::this_thread_id();

					for (auto i = impl_->records.begin(); i != impl_->records.end(); ++i)
					{
						if (this_tid != i->tid)
							continue;

						auto const refs = i->callstack_refs.back();

						for (std::size_t u = 1; u < refs; ++u)
							impl_->mutex.lock();

						impl_->thread_id = this_tid;
						impl_->refs = refs;

						if (i->callstack_refs.size() > 1)
							i->callstack_refs.pop_back();
						else
							impl_->records.erase(i);
						return;
					}

					throw std::runtime_error("The forward is not matched. Please report this issue");
				}

				impl_->mutex.unlock();
			}

		//end class revertible_mutex


	struct platform_runtime
	{
		platform_abstraction::revertible_mutex mutex;
		std::size_t		dpi{ 0 };
		std::shared_ptr<font_interface> font;
		font_service font_svc;
	};

	namespace
	{
		namespace data
		{
			static platform_runtime* storage;
		}
	}

	static platform_runtime& platform_storage()
	{
		if (nullptr == data::storage)
			throw std::runtime_error("platform_abstraction is empty");

		return *data::storage;
	}

	//class internal_scope_guard
		internal_scope_guard::internal_scope_guard()
		{
			if(data::storage)
				data::storage->mutex.lock();
		}

		internal_scope_guard::~internal_scope_guard()
		{
			if(data::storage)
				data::storage->mutex.unlock();
		}
	//end class internal_scope_guard

	//class internal_revert_guard
		internal_revert_guard::internal_revert_guard()
		{
			if(data::storage)
				data::storage->mutex.revert();
		}

		internal_revert_guard::~internal_revert_guard()
		{
			if(data::storage)
				data::storage->mutex.forward();
		}
		//end class internal_revert_guard

	internal_font::~internal_font()
	{
#ifdef NANA_WINDOWS
		::DeleteObject(reinterpret_cast<HFONT>(native_handle_));
#elif defined(NANA_X11)
		auto disp = ::nana::detail::platform_spec::instance().open_display();
#	ifdef NANA_USE_XFT
		nana::internal_scope_guard lock;
		platform_storage().font_svc.fb_manager().release_fallback(fallback_);
		::XftFontClose(disp, reinterpret_cast<XftFont*>(native_handle_));
#	else
		::XFreeFontSet(disp, reinterpret_cast<XFontSet>(native_handle_));
#	endif
#endif
		if (!ttf_.empty())
			platform_abstraction::font_resource(false, ttf_);
	}

#ifdef NANA_USE_XFT
	void nana_xft_draw_string(::XftDraw* xftdraw, ::XftColor* xftcolor, font_interface* ft, const nana::point& pos, const wchar_t * str, std::size_t len)
	{
		auto fallback = static_cast<internal_font*>(ft)->fallback();
		if(nullptr == fallback)
			return;

		auto xft = reinterpret_cast<XftFont*>(static_cast<internal_font*>(ft)->native_handle());
		fallback->draw(xftdraw, xftcolor, xft, pos.x, pos.y, str, len);
	}


	nana::size nana_xft_extents(font_interface* ft, const wchar_t* str, std::size_t len)
	{
		auto fallback = static_cast<internal_font*>(ft)->fallback();
		if(nullptr == fallback)
			return {};

		auto xft = reinterpret_cast<XftFont*>(static_cast<internal_font*>(ft)->native_handle());
		return fallback->extents(xft, str, len);
	}

	std::unique_ptr<unsigned[]> nana_xft_glyph_pixels(font_interface* ft, const wchar_t* str, std::size_t len)
	{
		auto fallback = static_cast<internal_font*>(ft)->fallback();
		if(nullptr == fallback)
			return {};

		auto xft = reinterpret_cast<XftFont*>(static_cast<internal_font*>(ft)->native_handle());
		return fallback->glyph_pixels(xft, str, len);
	}
#endif


	void platform_abstraction::initialize()
	{
		if (nullptr == data::storage)
			data::storage = new platform_runtime;
	}

	void platform_abstraction::shutdown()
	{
		auto & r = platform_storage();

		r.font.reset();
		r.font_svc.check_fonts();

		delete data::storage;
		data::storage = nullptr;
	}

	platform_abstraction::revertible_mutex& platform_abstraction::internal_mutex()
	{
		if (!data::storage)
			throw std::logic_error("invalid internal mutex");

		return data::storage->mutex;
	}

	double platform_abstraction::font_default_pt()
	{
#ifdef NANA_WINDOWS
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
		if (!IsWindowsVistaOrGreater())
			metrics.cbSize -= sizeof(metrics.iPaddedBorderWidth);
#endif
#endif
		::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof metrics, &metrics, 0);

		auto desktop = ::GetDC(nullptr);
		auto pt = std::abs(metrics.lfMessageFont.lfHeight) * 72.0 / ::GetDeviceCaps(desktop, LOGPIXELSY);
		::ReleaseDC(nullptr, desktop);
		return pt;
#else
		return 10;
#endif
	}

	void platform_abstraction::font_languages(const std::string& langs)
	{
#ifdef NANA_USE_XFT
		nana::internal_scope_guard lock;
		platform_storage().font_svc.fb_manager().languages(langs);
#endif
	}

	::std::shared_ptr<platform_abstraction::font> platform_abstraction::default_font(const ::std::shared_ptr<font>& new_font)
	{
		nana::internal_scope_guard lock;
		auto & r = platform_storage();
		if (new_font)
		{
			auto f = r.font;
			if (new_font != r.font)
				r.font = new_font;

			return f;
		}

		if (!r.font)
			r.font = r.font_svc.open_font({}, current_dpi(), {});

		return r.font;
	}

	void platform_abstraction::set_current_dpi(std::size_t dpi)
	{
		platform_storage().dpi = dpi;
	}

	std::size_t platform_abstraction::current_dpi()
	{
		return platform_storage().dpi;
	}

	std::shared_ptr<platform_abstraction::font> platform_abstraction::open_font(const font_info& fi, std::size_t dpi, const path_type& ttf)
	{
		nana::internal_scope_guard lock;
		return platform_storage().font_svc.open_font(fi, dpi, ttf);
	}

	void platform_abstraction::font_resource(bool try_add, const path_type& ttf)
	{
#ifdef NANA_WINDOWS
		if (try_add)
			::AddFontResourceEx(ttf.wstring().c_str(), FR_PRIVATE, nullptr);
		else
			::RemoveFontResourceEx(ttf.wstring().c_str(), FR_PRIVATE, nullptr);
#else
		auto p = absolute(ttf).string();

		nana::internal_scope_guard lock;

		if(platform_storage().font_svc.fc_count(try_add, p))
		{
			if(try_add)
				::FcConfigAppFontAddFile(nullptr, reinterpret_cast<const FcChar8*>(p.c_str()));
			else
				::FcConfigAppFontClear(nullptr);
		}
#endif
	}

	unsigned platform_abstraction::screen_dpi(bool x_requested)
	{
#ifdef NANA_WINDOWS
		auto hdc = ::GetDC(nullptr);
		auto dots = static_cast<unsigned>(::GetDeviceCaps(hdc, (x_requested ? LOGPIXELSX : LOGPIXELSY)));
		::ReleaseDC(nullptr, hdc);
		return dots;
#else
		auto & spec = ::nana::detail::platform_spec::instance();
		auto disp = spec.open_display();
		auto screen = ::XDefaultScreen(disp);

		double dots = 0.5;

		if (x_requested)
			dots += ((((double)DisplayWidth(disp, screen)) * 25.4) /
			((double)DisplayWidthMM(disp, screen)));
		else
			dots += ((((double)DisplayHeight(disp, screen)) * 25.4) /
			((double)DisplayHeightMM(disp, screen)));

		return static_cast<unsigned>(dots);
#endif
	}
}
