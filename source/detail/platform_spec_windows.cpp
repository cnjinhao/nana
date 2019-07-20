/**
 *	Platform Specification Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/detail/platform_spec.cpp
 *
 *	@brief basis classes and data structures required by nana
 */

#include "platform_spec_selector.hpp"
#include "platform_abstraction.hpp"

#if defined(NANA_WINDOWS)

#include <stdexcept>
#include <map>

#include <windows.h>
#include <shellapi.h>



namespace nana
{
namespace detail
{
	drawable_impl_type::drawable_impl_type()
	{
		string.tab_length = 4;
		string.tab_pixels = 0;
		string.whitespace_pixels = 0;
	}

	drawable_impl_type::~drawable_impl_type()
	{
		::DeleteDC(context);
		::DeleteObject(pixmap);
	}

#define NANA_WINDOWS_RGB(a)	(((DWORD)(a) & 0xFF)<<16) |  ((DWORD)(a) & 0xFF00) | (((DWORD)(a) & 0xFF0000) >> 16 )

	void drawable_impl_type::set_color(const ::nana::color& clr)
	{
		bgcolor_rgb = (clr.px_color().value & 0xFFFFFF);
		bgcolor_native = NANA_WINDOWS_RGB(bgcolor_rgb);
	}

	void drawable_impl_type::set_text_color(const ::nana::color& clr)
	{
		auto rgb = (clr.px_color().value & 0xFFFFFF);
		if (fgcolor_rgb != rgb)
		{
			fgcolor_rgb = rgb;
			fgcolor_native = NANA_WINDOWS_RGB(rgb);
			::SetTextColor(context, fgcolor_native);
		}
	}

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

	void platform_spec::co_initializer::task_mem_free(void* p)
	{
		if (ole32_)
		{
			using CoTaskMemFree_t = void (__stdcall *)(LPVOID pv);

			CoTaskMemFree_t free_fn = reinterpret_cast<CoTaskMemFree_t>(::GetProcAddress(ole32_, "CoTaskMemFree"));
			if (free_fn)
				free_fn(p);
		}
	}

	struct platform_spec::implementation
	{
		std::map<native_window_type, window_icons> iconbase;
	};

	platform_spec::platform_spec()
		: impl_{ new implementation}
	{
		platform_abstraction::initialize();
	}

	platform_spec::~platform_spec()
	{
		platform_abstraction::shutdown();
		delete impl_;
	}

	platform_spec& platform_spec::instance()
	{
		static platform_spec object;
		return object;
	}

	void platform_spec::keep_window_icon(native_window_type wd, const paint::image& sml_icon, const paint::image& big_icon)
	{
		auto & icons = impl_->iconbase[wd];
		icons.sml_icon = sml_icon;
		icons.big_icon = big_icon;
	}

	void platform_spec::release_window_icon(native_window_type wd)
	{
		impl_->iconbase.erase(wd);
	}
}//end namespace detail
}//end namespace nana

#endif //NANA_WINDOWS
