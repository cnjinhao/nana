/*
 *	Screen Informations
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/screen.cpp
 */
#include <nana/gui/screen.hpp>
#include <vector>
#include <memory>
#include <nana/gui/detail/native_window_interface.hpp>
#include <nana/gui/programming_interface.hpp>
#if defined(NANA_WINDOWS)
	#include <windows.h>
#endif

namespace nana
{
	//class display
	class real_display
		: public display
	{
	public:
		real_display(std::size_t number)
			: index_(number)
		{
#if defined(NANA_WINDOWS)
			DISPLAY_DEVICE disp;
			disp.cb = sizeof disp;
			if (::EnumDisplayDevices(nullptr, static_cast<DWORD>(index_), &disp, 0))
			{
				DEVMODE mode;
				mode.dmSize = sizeof mode;
				if (::EnumDisplaySettings(disp.DeviceName, ENUM_CURRENT_SETTINGS, &mode))
				{
					area_.x = mode.dmPosition.x;
					area_.y = mode.dmPosition.y;
					area_.width = mode.dmPelsWidth;
					area_.height = mode.dmPelsHeight;
					return;
				}
			}
#else
			if (0 == index_)
			{
				position_.x = position_.y = 0;
				size_ = detail::native_interface::primary_monitor_size();
				return;
			}
#endif
			throw std::invalid_argument("Nana.Screen: Invalid monitor index.");
		}

		real_display(std::size_t number, const ::nana::rectangle& r)
			: index_(number), area_(r)
		{
		}
	public:
		//Implementation of display
		std::size_t index() const override
		{
			return index_;
		}

		const ::nana::rectangle& area() const override
		{
			return area_;
		}
	private:
		const std::size_t	index_;
		::nana::rectangle area_;
	};

	//class screen

	::nana::size screen::desktop_size()
	{
#if defined(NANA_WINDOWS)
		auto w = static_cast<size::value_type>(::GetSystemMetrics(SM_CXVIRTUALSCREEN));
		auto h = static_cast<size::value_type>(::GetSystemMetrics(SM_CYVIRTUALSCREEN));
		return{w, h};
#else
		return ::nana::detail::native_interface::primary_monitor_size();
#endif
	}

	::nana::size screen::primary_monitor_size()
	{
		return ::nana::detail::native_interface::primary_monitor_size();
	}

	std::shared_ptr<display> screen::from_point(const point& pos)
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
				DISPLAY_DEVICE disp;
				disp.cb = sizeof disp;

				DWORD index = 0;
				while (::EnumDisplayDevices(nullptr, index++, &disp, 0))
				{
					DEVMODE mode;
					mode.dmSize = sizeof mode;
					if (::EnumDisplaySettings(disp.DeviceName, ENUM_CURRENT_SETTINGS, &mode))
					{
						if (mode.dmPosition.x == mi.rcWork.left && mode.dmPosition.y == mi.rcWork.top &&
							(static_cast<int>(mode.dmPelsWidth) == mi.rcWork.right - mi.rcWork.left) &&
							(static_cast<int>(mode.dmPelsHeight) == mi.rcWork.bottom - mi.rcWork.top))
						{
							return std::make_shared<real_display>(static_cast<std::size_t>(index - 1), rectangle{ mode.dmPosition.x, mode.dmPosition.y, static_cast<unsigned>(mode.dmPelsWidth), static_cast<unsigned>(mode.dmPelsHeight) });
						}
					}
				}
			}
		}
#endif
		return screen().get_primary();
	}

	std::shared_ptr<display> screen::from_window(window wd)
	{
		::nana::point pos;
		API::calc_screen_point(wd, pos);
		return from_point(pos);
	}

	std::size_t screen::count() const
	{
#if defined(NANA_WINDOWS)
		DISPLAY_DEVICE disp;
		disp.cb = sizeof disp;

		DWORD index = 0;
		while (::EnumDisplayDevices(nullptr, index++, &disp, 0));
		return static_cast<std::size_t>(index - 1);
#else
		return 1;
#endif
	}

	std::shared_ptr<display> screen::get_display(std::size_t index) const
	{
		return std::make_shared<real_display>(index);
	}

	std::shared_ptr<display> screen::get_primary() const
	{
#if defined(NANA_WINDOWS)
		//return rectangle(mi.rcWork.left, mi.rcWork.top,
		//	mi.rcWork.right - mi.rcWork.left, mi.rcWork.bottom - mi.rcWork.top);
		DISPLAY_DEVICE disp;
		disp.cb = sizeof disp;

		DWORD index = 0;
		while (::EnumDisplayDevices(nullptr, index++, &disp, 0))
		{
			DEVMODE mode;
			mode.dmSize = sizeof mode;
			if (::EnumDisplaySettings(disp.DeviceName, ENUM_CURRENT_SETTINGS, &mode))
			{
				if (mode.dmPosition.x == 0 && mode.dmPosition.y == 0)
					return std::make_shared<real_display>(static_cast<std::size_t>(index - 1), rectangle{ mode.dmPosition.x, mode.dmPosition.y, static_cast<unsigned>(mode.dmPelsWidth), static_cast<unsigned>(mode.dmPelsHeight) });
			}
		}
#endif
		return std::make_shared<real_display>(0);
	}

	void screen::for_each(std::function<void(display&)> fn) const
	{
		auto n = count();
		for (decltype(n) i = 0; i < n; ++i)
		{
			real_display disp(i);
			fn(disp);
		}
	}
	//end class screen
}
