/**
 *	Screen Informations
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2024 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file nana/gui/screen.cpp
 */

#include <vector>
#include <memory>

#include <nana/gui/detail/native_window_interface.hpp>
#include <nana/gui/programming_interface.hpp>
#include <nana/gui/screen.hpp>

#include "../detail/platform_abstraction.hpp"

#if defined(NANA_WINDOWS)
	#include <windows.h>
#endif

/// \todo: move all to nana::detail::native_interface in native_window_interface.hpp ???

namespace nana
{
	//class display
	class real_display
		: public display
	{
	public:
		real_display() = default;	// requirement of vector

#if defined(NANA_WINDOWS)
		real_display(std::size_t number, const MONITORINFOEX& mi, int dpi)
			:	index_(number),
				is_primary_(mi.dwFlags & /*MONITORINFOF_PRIMARY*/ 0x1),
				area_    (mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top),
				workarea_(mi.rcWork.left   , mi.rcWork.top   , mi.rcWork.right    - mi.rcWork.left   , mi.rcWork.bottom    - mi.rcWork.top),
			    dpi_(dpi), scale_(dpi / 96.0)
		{
		}
#else
		real_display(std::size_t number, const ::nana::rectangle& r)
			: index_(number), is_primary_(true), area_(r), workarea_(r)
		{
		}
#endif
	public:
		//Implementation of display
		std::size_t get_index() const override
		{
			return index_;
		}

		bool is_primary_monitor() const override
		{
			return is_primary_;
		}

		::nana::rectangle area() const override
		{
			return platform_abstraction::unscale_dpi(area_, dpi_);
		}

		::nana::rectangle workarea() const override
		{
			return platform_abstraction::unscale_dpi(workarea_, dpi_);
		}

		int dpi() const override
        {
            return dpi_;
        }

		double scaling() const override
		{
		    return scale_;
		}

	private:
		std::size_t			index_;
		bool				is_primary_;
		::nana::rectangle	area_;
		::nana::rectangle	workarea_;
		int                 dpi_;
		double              scale_{ 1.0 };
	};

	//class screen

	::nana::size screen::desktop_size()
	{
#if defined(NANA_WINDOWS)
		/// \todo: add to dpi_function GetSystemMetricsForDpi and replace this
		auto w = static_cast<size::value_type>(::GetSystemMetrics(SM_CXVIRTUALSCREEN));
		auto h = static_cast<size::value_type>(::GetSystemMetrics(SM_CYVIRTUALSCREEN));
		return{w, h};
#else
		return ::nana::detail::native_interface::primary_monitor_size();
#endif
	}
	/// \todo: generalize dpi to v2 awareness 
	::nana::size screen::primary_monitor_size()
	{
		return ::nana::detail::native_interface::primary_monitor_size();
	}


	struct screen::implement
	{
		std::vector<real_display> displays;


#if defined(NANA_WINDOWS)

		/// \todo: move all to nana::detail::native_interface in native_window_interface.hpp ???
		enum MONITOR_DPI_TYPE {
			MDT_EFFECTIVE_DPI,
			MDT_ANGULAR_DPI,
			MDT_RAW_DPI,
			MDT_DEFAULT
		};
		using  GetDpiForMonitor_ftype = HRESULT(__stdcall*)(HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT*);
		inline static GetDpiForMonitor_ftype  GetDpiForMonitor { nullptr };

		void load_monitors()
		{
			std::vector<real_display> tmp;
			if (!GetDpiForMonitor)
			{
                HMODULE shcore = ::LoadLibraryA("SHCORE.DLL");
                if (shcore)
                {
                    GetDpiForMonitor = reinterpret_cast<GetDpiForMonitor_ftype>(::GetProcAddress(shcore, "GetDpiForMonitor"));
                    ::FreeLibrary(shcore);
                }
            }
			::EnumDisplayMonitors(nullptr, nullptr, implement::enum_proc, reinterpret_cast<LPARAM>(&tmp));
			tmp.swap(displays);
		}

		static BOOL __stdcall enum_proc(HMONITOR handle, HDC /*context*/, LPRECT /*r*/, LPARAM self_ptr)
		{
			auto disp_cont = reinterpret_cast<std::vector<real_display>*>(self_ptr);
			MONITORINFOEX mi;
			mi.cbSize = sizeof(MONITORINFOEX);
			if (! ::GetMonitorInfo(handle, &mi)) return TRUE;
			int dpi = 96;

			UINT xdpi, ydpi;
			LRESULT success = GetDpiForMonitor(handle, MDT_EFFECTIVE_DPI, &xdpi, &ydpi);
			if (success == S_OK) dpi = xdpi;
			
			disp_cont->emplace_back(disp_cont->size(), mi, dpi);

			return TRUE;
		}
#else
		void load_monitors()
		{
			displays.clear();
			displays.emplace_back(0, rectangle{primary_monitor_size()});
		}
#endif

	};

	screen::screen()
		: impl_(std::make_shared<implement>())
	{
		impl_->load_monitors();
	}

	void screen::reload()
	{
		//It is only when the screen is a moved-from object that impl_ is empty
		if (!impl_)
			std::make_shared<implement>().swap(impl_);

		impl_->load_monitors();
	}

	std::size_t screen::count() const
	{
		return impl_->displays.size();
	}


	display& screen::from_point(const point& pos)
	{
#if defined(NANA_WINDOWS)
		typedef HMONITOR(__stdcall * MonitorFromPointT)(POINT, DWORD);

		MonitorFromPointT mfp = reinterpret_cast<MonitorFromPointT>(::GetProcAddress(::GetModuleHandleA("User32.DLL"), "MonitorFromPoint"));
		if (mfp)
		{
			POINT native_pos = { pos.x, pos.y };
			HMONITOR monitor = mfp(native_pos, 2 /*MONITOR_DEFAULTTONEAREST*/);

			MONITORINFO mi;
			mi.cbSize = sizeof mi;
			if (::GetMonitorInfo(monitor, &mi))
			{
				for (auto & disp : impl_->displays)
				{
					auto  r = disp.area();
					if (r.x == mi.rcMonitor.left && r.y == mi.rcMonitor.top &&
						r.width == unsigned(mi.rcMonitor.right - mi.rcMonitor.left) &&
						r.height == unsigned(mi.rcMonitor.bottom - mi.rcMonitor.top)
						)
						return disp;
				}
			}
		}
#else
		static_cast<void>(pos); //to eliminate unused parameter compiler warning.
#endif
		return get_primary();
	}

	display& screen::from_window(window wd)
	{
		::nana::point pos;
		api::calc_screen_point(wd, pos);
		return from_point(pos);
	}


	display& screen::get_display(std::size_t index) const
	{
		return impl_->displays.at(index);
	}

	display& screen::get_primary() const
	{
		for (auto & disp : impl_->displays)
			if (disp.is_primary_monitor())
				return disp;

		throw std::logic_error("no primary monitor found");
	}

	void screen::for_each(std::function<void(display&)> fn) const
	{
		for (auto & disp : impl_->displays)
			fn(disp);
	}
	//end class screen
}
