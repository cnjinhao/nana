#include "platform_abstraction.hpp"
#include <set>
#include <nana/deploy.hpp>
#include "../paint/truetype.hpp"

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
#		include <X11/Xft/Xft.h>
#		include <iconv.h>
#		include <fstream>
#	endif
#endif

namespace nana
{
#ifdef NANA_USE_XFT
	//A fallback fontset provides the multiple languages support.
	class fallback_fontset
	{
	public:
		fallback_fontset():
			disp_(::nana::detail::platform_spec::instance().open_display())
		{
		}

		~fallback_fontset()
		{
			for(auto xft: xftset_)
				::XftFontClose(disp_, xft);
		}

		void open(const std::string& font_desc, const std::set<std::string>& langs)
		{
			for(auto xft: xftset_)
				::XftFontClose(disp_, xft);

			xftset_.clear();

			std::set<std::string> loaded;
			for(auto & lang : langs)
			{
				std::string patstr = "*" + font_desc + ":lang=" + lang;

				auto pat = ::XftNameParse(patstr.c_str());
				XftResult res;
				auto match_pat = ::XftFontMatch(disp_, ::XDefaultScreen(disp_), pat, &res);

				if (match_pat)
				{
					char * sf;
					if(XftResultTypeMismatch != ::XftPatternGetString(match_pat, "family", 0, &sf))
					{
						//Avoid loading a some font repeatedly
						if(loaded.count(sf))
							continue;
					}

					auto xft = ::XftFontOpenPattern(disp_, match_pat);
					if(xft)
						xftset_.push_back(xft);
				}				
			}
		}

		int draw(::XftDraw* xftdraw, ::XftColor * xftcolor, ::XftFont* xft, int x, int y, const wchar_t* str, std::size_t len)
		{
			if(nullptr == str || 0 == len)
				return 0;

			int const init_x = x;
			std::unique_ptr<FT_UInt[]> glyph_indexes(new FT_UInt[len]);

			while(true)
			{
				auto preferred = _m_scan_fonts(xft, str, len, glyph_indexes.get());
				x += _m_draw(xftdraw, xftcolor, preferred.first, x, y, str, preferred.second, glyph_indexes.get());

				if(len == preferred.second)
					break;

				len -= preferred.second;
				str += preferred.second;
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
			auto pstr = str;
			auto size = len;

			while(true)
			{
				auto preferred = _m_scan_fonts(xft, pstr, size, glyph_indexes.get());

				_m_glyph_px(preferred.first, pstr, preferred.second, glyph_indexes.get(), pbuf);

				if(size == preferred.second)
					break;

				size -= preferred.second;
				pstr += preferred.second;
				pbuf += preferred.second;
			}

			return pxbuf;
		}

		nana::size extents(::XftFont* xft, const wchar_t* str, std::size_t len)
		{
			nana::size extent;

			if(nullptr == str || 0 == len)
				return extent;

			std::unique_ptr<FT_UInt[]> glyph_indexes(new FT_UInt[len]);

			while(len > 0)
			{
				auto preferred = _m_scan_fonts(xft, str, len, glyph_indexes.get());

				extent.width += _m_extents(preferred.first, str, preferred.second, glyph_indexes.get());

				if(preferred.first->ascent + preferred.first->descent > static_cast<int>(extent.height))
					extent.height = preferred.first->ascent + preferred.first->descent;

				len -= preferred.second;
				str += preferred.second;
			}
			return extent;
		}
	private:
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
					//x += static_cast<int>(tab_pixels_);
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
					*pxbuf = extent.xOff;
				}
				else
					*pxbuf = 0;//tab_pixels_;

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
					idx = ::XftCharIndex(disp_, ft, *str);
					if(idx)
					{
						preferred = ft;
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
						if(::XftCharIndex(disp_, ft, str[i]))
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
		std::vector<::XftFont*> xftset_;
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
			
			auto fb = std::make_shared<fallback_fontset>();

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
		std::map<std::string, std::shared_ptr<fallback_fontset>> xft_table_;
	};
#endif

	struct platform_runtime
	{
		std::shared_ptr<font_interface> font;

#ifdef NANA_X11
		std::map<std::string, std::size_t> fontconfig_counts;
#endif
#ifdef NANA_USE_XFT
		fallback_manager fb_manager;
#endif
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


	class internal_font
		: public font_interface
	{
	public:
		using path_type = std::filesystem::path;

#ifdef NANA_USE_XFT
		internal_font(const path_type& ttf, const std::string& font_family, double font_size, const font_style& fs, native_font_type native_font, std::shared_ptr<fallback_fontset> fallback):
			ttf_(ttf),
			family_(font_family),
			size_(font_size),
			style_(fs),
			native_handle_(native_font),
			fallback_(fallback)
		{}
#else
		internal_font(const path_type& ttf, const std::string& font_family, double font_size, const font_style& fs, native_font_type native_font):
			ttf_(ttf),
			family_(font_family),
			size_(font_size),
			style_(fs),
			native_handle_(native_font)
		{}		
#endif

		~internal_font()
		{
#ifdef NANA_WINDOWS
			::DeleteObject(reinterpret_cast<HFONT>(native_handle_));
#elif defined(NANA_X11)
			auto disp = ::nana::detail::platform_spec::instance().open_display();
#	ifdef NANA_USE_XFT
			platform_storage().fb_manager.release_fallback(fallback_);
			::XftFontClose(disp, reinterpret_cast<XftFont*>(native_handle_));
#	else
			::XFreeFontSet(disp, reinterpret_cast<XFontSet>(native_handle_));
#	endif
#endif
			if (!ttf_.empty())
				platform_abstraction::font_resource(false, ttf_);
		}
	public:
		const std::string& family() const override
		{
			return family_;
		}

		double size() const override
		{
			return size_;
		}

		const font_style & style() const override
		{
			return style_;
		}

		native_font_type native_handle() const override
		{
			return native_handle_;
		}

#ifdef NANA_USE_XFT
		fallback_fontset* fallback() const
		{
			return fallback_.get();
		}
#endif
	private:
		path_type	const ttf_;
		std::string	const family_;
		double		const size_;
		font_style	const style_;
		native_font_type const native_handle_;
#ifdef NANA_USE_XFT
		std::shared_ptr<fallback_fontset> fallback_;
#endif
	};

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

		if (r.font.use_count() > 1)
			throw std::runtime_error("platform_abstraction is disallowed to shutdown");

		r.font.reset();

		delete data::storage;
		data::storage = nullptr;
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
		platform_storage().fb_manager.languages(langs);
#endif
	}

	::std::shared_ptr<platform_abstraction::font> platform_abstraction::default_font(const ::std::shared_ptr<font>& new_font)
	{
		auto & r = platform_storage();
		if (new_font)
		{
			auto f = r.font;
			if (new_font != r.font)
				r.font = new_font;

			return f;
		}

		if (!r.font)
			r.font = make_font({}, 0, {});

		return r.font;
	}

	static std::shared_ptr<platform_abstraction::font> font_factory(::std::string font_family, double size_pt, const platform_abstraction::font::font_style& fs, internal_font::path_type ttf)
	{
		using native_font_type = platform_abstraction::font::native_font_type;
#ifdef NANA_WINDOWS
		std::wstring wfont_family = to_nstring(font_family);

		//Make sure the length of font family less than LF_FACESIZE which is defined by Windows
		if (wfont_family.length() + 1 > LF_FACESIZE)
			wfont_family.clear();

		//Translate pt to px
		auto hDC = ::GetDC(nullptr);
		auto font_height = -static_cast<LONG>(size_pt * ::GetDeviceCaps(hDC, LOGPIXELSY) / 72);
		::ReleaseDC(nullptr, hDC);

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
				font_family = to_utf8(wfont_family);
			}

			if (0 == font_height)
				font_height = metrics.lfMessageFont.lfHeight;
		}


		::LOGFONT lf{};

		std::wcscpy(lf.lfFaceName, wfont_family.c_str());
		lf.lfHeight = font_height;
		lf.lfCharSet = DEFAULT_CHARSET;
		lf.lfWeight = fs.weight;
		lf.lfQuality = PROOF_QUALITY;
		lf.lfPitchAndFamily = FIXED_PITCH;
		lf.lfItalic = fs.italic;
		lf.lfUnderline = fs.underline;
		lf.lfStrikeOut = fs.strike_out;

		auto fd = ::CreateFontIndirect(&lf);
#elif defined(NANA_X11)
		auto disp = ::nana::detail::platform_spec::instance().open_display();
#	ifdef NANA_USE_XFT
		if(font_family.empty())
			font_family = "*";

		std::string pat_str = '-' + std::to_string(size_pt ? size_pt : platform_abstraction::font_default_pt());
		if(fs.weight < 400)
			pat_str += ":light";
		else if(400 == fs.weight)
			pat_str += ":medium";
		else if(fs.weight < 700)
			pat_str += ":demibold";
		else
			pat_str += (700 == fs.weight ? ":bold": ":black"); 

		if(fs.italic)
			pat_str += ":slant=italic";

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

		char ** missing_list;
		int missing_count;
		char * defstr;
		XFontSet fd = ::XCreateFontSet(display_, const_cast<char*>(pat_str.c_str()), &missing_list, &missing_count, &defstr);
#	endif
#endif

		if (fd)
		{
#ifdef NANA_USE_XFT
			auto fallback = platform_storage().fb_manager.make_fallback(pat_str);
			return std::make_shared<internal_font>(std::move(ttf), std::move(font_family), size_pt, fs, reinterpret_cast<native_font_type>(fd), fallback);
#else
			return std::make_shared<internal_font>(std::move(ttf), std::move(font_family), size_pt, fs, reinterpret_cast<native_font_type>(fd));
#endif
		}
		return{};
	}

	::std::shared_ptr<platform_abstraction::font> platform_abstraction::make_font(const std::string& font_family, double size_pt, const font::font_style& fs)
	{
		return font_factory(font_family, size_pt, fs, {});
	}

	::std::shared_ptr<platform_abstraction::font> platform_abstraction::make_font_from_ttf(const path_type& ttf, double size_pt, const font::font_style& fs)
	{
		::nana::spec::truetype truetype{ ttf };
		if (truetype.font_family().empty())
			return nullptr;

		font_resource(true, ttf);

		return font_factory(truetype.font_family(), size_pt, fs, ttf);
	}

	void platform_abstraction::font_resource(bool try_add, const path_type& ttf)
	{
#ifdef NANA_WINDOWS
		if (try_add)
			::AddFontResourceEx(ttf.wstring().c_str(), FR_PRIVATE, nullptr);
		else
			::RemoveFontResourceEx(ttf.wstring().c_str(), FR_PRIVATE, nullptr);
#else
		auto & fc = platform_storage().fontconfig_counts;
		if(try_add)
		{
			if(1 == ++(fc[ttf.string()]))
			{
				::FcConfigAppFontAddFile(nullptr, reinterpret_cast<const FcChar8*>(ttf.string().c_str()));
			}
		}
		else
		{
			auto i = fc.find(ttf.string());
			if(i != fc.end())
			{
				if(0 == --(i->second))
					fc.erase(i);

				if(0 == fc.size())
					::FcConfigAppFontClear(nullptr);
			}
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
