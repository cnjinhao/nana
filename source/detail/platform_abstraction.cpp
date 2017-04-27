#include "platform_abstraction.hpp"
#include <nana/deploy.hpp>
#include "../paint/truetype.hpp"

#ifdef NANA_WINDOWS
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

VERSIONHELPERAPI
IsWindows8Point1OrGreater()
{
	return IsWindowsVersionOrGreater(HIBYTE(_WIN32_WINNT_WINBLUE), LOBYTE(_WIN32_WINNT_WINBLUE), 0);
}

VERSIONHELPERAPI
IsWindowsServer()
{
	OSVERSIONINFOEXW osvi = { sizeof(osvi), 0, 0, 0, 0, { 0 }, 0, 0, 0, VER_NT_WORKSTATION };
	DWORDLONG        const dwlConditionMask = VerSetConditionMask(0, VER_PRODUCT_TYPE, VER_EQUAL);

	return !VerifyVersionInfoW(&osvi, VER_PRODUCT_TYPE, dwlConditionMask);
}

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

	class internal_font
		: public font_interface
	{
	public:
		using path_type = std::experimental::filesystem::path;

		internal_font(const path_type& ttf, const std::string& font_family, double font_size, const font_style& fs, native_font_type native_font):
			ttf_(ttf),
			family_(font_family),
			size_(font_size),
			style_(fs),
			native_handle_(native_font)
		{}

		~internal_font()
		{
#ifdef NANA_WINDOWS
			::DeleteObject(reinterpret_cast<HFONT>(native_handle_));
#elif defined(NANA_X11)
			auto disp = ::nana::detail::platform_spec::instance().open_display();
#	ifdef NANA_USE_XFT
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
	private:
		path_type	const ttf_;
		std::string	const family_;
		double		const size_;
		font_style	const style_;
		native_font_type const native_handle_;
	};

	struct platform_runtime
	{
		std::shared_ptr<font_interface> font;

#ifdef NANA_X11
		std::map<std::string, std::size_t> fontconfig_counts;
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
			font_family = '*';

		std::string pat_str = font_family + '-' + std::to_string(size_pt ? size_pt : 10);
		auto pat = ::XftNameParse(pat_str.c_str());
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
			return std::make_shared<internal_font>(std::move(ttf), std::move(font_family), size_pt, fs, reinterpret_cast<native_font_type>(fd));
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
}