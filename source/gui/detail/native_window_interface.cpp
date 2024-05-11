/**
 *	Platform Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2022 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file nana/gui/detail/native_window_interface.cpp
 */

#include <cstring>
#include <iostream>  // for print_monitor_dpi() for debugging

#include "../../detail/platform_spec_selector.hpp"
#include "../../detail/platform_abstraction.hpp"
#include <nana/gui/detail/native_window_interface.hpp>
#include <nana/gui/screen.hpp>
#include <nana/gui/detail/bedrock.hpp>
#include <nana/gui/detail/window_manager.hpp>

#if defined(NANA_WINDOWS)
#	include <mutex>
#	include <map>
#elif defined(NANA_X11)
#	include <nana/system/platform.hpp>
#	include "inner_fwd_implement.hpp"
#endif

#include "../../paint/image_accessor.hpp"



namespace nana
{
namespace detail{

  #if defined(NANA_WINDOWS)
    nana::point scale_to_dpi(int x, int y, int dpi)
    {
        auto  scaled_p = nana::point(MulDiv(x, dpi, 96), 
									           MulDiv(y, dpi, 96));
		if constexpr (dpi_debugging)
		{
		    std::cout << "   orig point= " << x          << ", " << y          << '\n';
		    std::cout << " scaled point= " << scaled_p.x << ", " << scaled_p.y << '\n';
		}
		return scaled_p;
    }
	nana::point scale_to_dpi(native_window_type wd, int x, int y)
    {
		int dpi = native_interface::window_dpi(wd);
		return scale_to_dpi(x, y, dpi);
    }
	nana::point unscale_dpi(native_window_type wd, int x, int y)
	{
	    int dpi = static_cast<int>(native_interface::window_dpi(wd));
	    auto scaled_p = nana::point(MulDiv(x, 96, dpi), 
									          MulDiv(y, 96, dpi));
	    if constexpr (dpi_debugging)
	        std::cout << " unscaled point= " << scaled_p.x << ", " << scaled_p.y << '\n';
	    return scaled_p;
	}
    // create helper function to scale nana::rectangle to dpi
    nana::rectangle scale_to_dpi(const nana::rectangle& r, int dpi)
    {
        auto scaled_r = nana::rectangle(MulDiv(r.x,      dpi, 96), 
								                     MulDiv(r.y,      dpi, 96),
							                         MulDiv(r.width,  dpi, 96), 
								                     MulDiv(r.height, dpi, 96));
		if constexpr (dpi_debugging)
		{
			std::cout << "   orig rect= " << r.x            << ", " << r.y             <<
			             "   with size= " << r.width        << ", " << r.height        << '\n';
			std::cout << " scaled rect= " << scaled_r.x     << ", " << scaled_r.y      << 
					     "   with size= " << scaled_r.width << ", " << scaled_r.height << '\n';
		}
		return scaled_r;
    }
	nana::rectangle scale_to_dpi(native_window_type wd, const nana::rectangle& r)
    {
		int dpi = static_cast<int>(native_interface::window_dpi(wd));
		return scale_to_dpi(r, dpi);
    }
	nana::rectangle unscale_dpi(const nana::rectangle& r, int dpi)
    {
        auto scaled_r = nana::rectangle(MulDiv(r.x,      96, dpi), 
								                     MulDiv(r.y,      96, dpi),
							                         MulDiv(r.width,  96, dpi), 
								                     MulDiv(r.height, 96, dpi));
		if constexpr (dpi_debugging)
		{
			std::cout << " unscaled rect= " << scaled_r.x     << ", " << scaled_r.y      << 
					     "   with size= " << scaled_r.width << ", " << scaled_r.height << '\n';
		}
		return scaled_r;
    }
	// create helper function to scale ::RECT to dpi
	::RECT scale_to_dpi(const ::RECT& r, int dpi)
    {
        auto scaled_r = ::RECT(MulDiv(r.left,   dpi, 96), 
						            MulDiv(r.top,    dpi, 96),
						            MulDiv(r.right,  dpi, 96), 
							        MulDiv(r.bottom, dpi, 96));

        if constexpr (dpi_debugging)
        {
            std::cout << "   orig rect= " << r.left                         << ", " << r.top                          <<
                         "   with size= " << r.right - r.left               << ", " << r.bottom - r.top               << '\n';
            std::cout << " scaled rect= " << scaled_r.left                  << ", " << scaled_r.top                   << 
                         "   with size= " << scaled_r.right - scaled_r.left << ", " << scaled_r.bottom - scaled_r.top << '\n';
        }
        return scaled_r;
    }
	::RECT scale_to_dpi(native_window_type wd, const ::RECT& r)
    {
		int dpi = static_cast<int>(native_interface::window_dpi(wd));
		return scale_to_dpi(r, dpi);
    }

	::RECT unscale_dpi(const ::RECT& r, int dpi)
    {
        auto scaled_r = ::RECT(MulDiv(r.left,   96, dpi), 
							       MulDiv(r.top,    96, dpi),
							       MulDiv(r.right,  96, dpi), 
							       MulDiv(r.bottom, 96, dpi));

        if constexpr (dpi_debugging)
        {
            std::cout << " unscaled rect= " << scaled_r.left                  << ", " << scaled_r.top                   << 
                           "   with size= " << scaled_r.right - scaled_r.left << ", " << scaled_r.bottom - scaled_r.top << '\n';
        }
        return scaled_r;
    }

	nana::size scale_to_dpi(const nana::size& sz, int dpi)
    {
	    auto scaled_sz = nana::size(MulDiv(sz.width,  dpi, 96), 
										     MulDiv(sz.height, dpi, 96));
	    if constexpr (dpi_debugging)
	    {
			std::cout << "   orig size= " << sz.width        << ", " << sz.height        << '\n';
			std::cout << " scaled size= " << scaled_sz.width << ", " << scaled_sz.height << '\n';
		}
	    return scaled_sz;
	}
	nana::size scale_to_dpi(native_window_type wd, const nana::size& sz)
    {
	    int dpi = static_cast<int>(native_interface::window_dpi(wd));
	    return scale_to_dpi(sz, dpi);
	}
	nana::size unscale_dpi(const nana::size& sz, int dpi)
    {
	    auto scaled_sz = nana::size(MulDiv(sz.width,  96, dpi), 
										     MulDiv(sz.height, 96, dpi));
	    if constexpr (dpi_debugging)
	    {
			std::cout << " unscaled size= " << scaled_sz.width << ", " << scaled_sz.height << '\n';
		}
	    return scaled_sz;
	}
	nana::size unscale_dpi(native_window_type wd, const nana::size& sz)
    {
	    int dpi = static_cast<int>(native_interface::window_dpi(wd));
	    return unscale_dpi(sz, dpi);
	}



	struct DPI_AWARENESS_CONTEXT___ { int unused; }; ///< introduce named dummy type, avoid including windows.h
	typedef struct DPI_AWARENESS_CONTEXT___* DPI_AWARENESS_CONTEXT_; ///< introduce named dummy pointer type

	/// force conversion of numbers '-1', '-2'... into a value of type pointer to some named structure
	/// this is useful only for comparitions/identification, but, please, don't dereference that pointer!
	/// why not use just an enum class? see windef.h
	#define DPI_AWARENESS_CONTEXT_UNAWARE_               ((DPI_AWARENESS_CONTEXT_)-1)
	#define DPI_AWARENESS_CONTEXT_SYSTEM_AWARE_          ((DPI_AWARENESS_CONTEXT_)-2)
	#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_     ((DPI_AWARENESS_CONTEXT_)-3)
	#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2_  ((DPI_AWARENESS_CONTEXT_)-4)
	#define DPI_AWARENESS_CONTEXT_UNAWARE_GDISCALED_     ((DPI_AWARENESS_CONTEXT_)-5)

	/// Dynamically load Windows DPI functions to check these APIs are supported by the SDK and OS.
	struct dpi_function
	{
		enum PROCESS_DPI_AWARENESS {
			PROCESS_DPI_UNAWARE,
			PROCESS_SYSTEM_DPI_AWARE,
			PROCESS_PER_MONITOR_DPI_AWARE
		};

		enum MONITOR_DPI_TYPE {
			MDT_EFFECTIVE_DPI,
			MDT_ANGULAR_DPI,
			MDT_RAW_DPI,
			MDT_DEFAULT
		};

		/// define function pointers types for each API
		using SetProcessDPIAware_ftype            = HRESULT(__stdcall*)(                      );
		using SetProcessDpiAwareness_ftype        = HRESULT(__stdcall*)(PROCESS_DPI_AWARENESS );
		using SetProcessDpiAwarenessContext_ftype = HRESULT(__stdcall*)(DPI_AWARENESS_CONTEXT_);
		using GetDpiForWindow_ftype               = UINT   (__stdcall*)(HWND                  );
		using GetDpiForSystem_ftype               = UINT   (__stdcall*)(                      );
		using GetDpiForMonitor_ftype              = HRESULT(__stdcall*)(HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT*);
		using GetDpiFromDpiAwarenessContext_ftype = UINT   (__stdcall*)(void*                 );
		using GetThreadDpiAwarenessContext_ftype  = void*  (__stdcall*)(                      );
		using SetThreadDpiAwarenessContext_ftype  = HRESULT(__stdcall*)(DPI_AWARENESS_CONTEXT_);
		using GetSystemMetrics_ftype              = int    (__stdcall*)(int                   );
		using GetSystemMetricsForDpi_ftype        = int    (__stdcall*)(int, UINT             );
		using MonitorFromPoint_ftype              = HMONITOR(__stdcall*)(POINT, DWORD         );

		/// define function pointers members for each API
		SetProcessDPIAware_ftype            SetProcessDPIAware            { nullptr }; ///< https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setprocessdpiaware
		SetProcessDpiAwareness_ftype        SetProcessDpiAwareness        { nullptr }; ///< https://learn.microsoft.com/en-us/windows/win32/api/shellscalingapi/nf-shellscalingapi-setprocessdpiawareness
		SetProcessDpiAwarenessContext_ftype SetProcessDpiAwarenessContext { nullptr }; ///< https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setprocessdpiawarenesscontext
		GetDpiForWindow_ftype               GetDpiForWindow               { nullptr };
		GetDpiForSystem_ftype               GetDpiForSystem               { nullptr };
		GetDpiForMonitor_ftype              GetDpiForMonitor              { nullptr };
		GetDpiFromDpiAwarenessContext_ftype GetDpiFromDpiAwarenessContext { nullptr };
		GetThreadDpiAwarenessContext_ftype  GetThreadDpiAwarenessContext  { nullptr };
		SetThreadDpiAwarenessContext_ftype  SetThreadDpiAwarenessContext  { nullptr };
		GetSystemMetrics_ftype              GetSystemMetrics              { nullptr };
		GetSystemMetricsForDpi_ftype        GetSystemMetricsForDpi        { nullptr };
		MonitorFromPoint_ftype              MonitorFromPoint              { nullptr };

		dpi_function()
		{
			/// Dynamically load User32.DLL to check these APIs are supported by the SDK and OS.
			/// here we can find SetProcessDPIAware, (GetDpiForWindow, GetDpiForSystem, 
			///      GetDpiFromDpiAwarenessContext, GetThreadDpiAwarenessContext, SetThreadDpiAwarenessContext, SetProcessDpiAwarenessContext,
			/// EnableChildWindowDpiMessage, GetDpiMetrics, GetDpiForMonitorInternal, GetProcessDpiAwarenessInternal, GetWindowDPI, IsChildWindowDpiMessageEnabled
			/// IsProcessDPIAware, IsWindowBroadcastingDpiToChildren, LogicalToPhysicalPointForPerMonitorDPI, PhysicalToLogicalPointForPerMonitorDPI, SetProcessDpiAwarenessInternal
			auto user32 = ::GetModuleHandleW(L"User32.DLL");
			if (nullptr == user32)  // ??
			{
				// std::cerr << "User32.DLL not loaded for GetModuleHandleW" << std::endl;   // for debugging
				user32 = ::LoadLibraryW(L"User32.DLL");
				// if (nullptr == user32) std::cerr << "User32.DLL not loaded :  ERROR !!!!" << std::endl;    
				// led to a crash in the program? just all pointers to DPI functions will be nullptr
			}  
			if (user32)
			{
				this->SetProcessDPIAware = reinterpret_cast<SetProcessDPIAware_ftype>
								(::GetProcAddress(user32, "SetProcessDPIAware"));

				this->GetDpiForWindow = reinterpret_cast<GetDpiForWindow_ftype>
								(::GetProcAddress(user32, "GetDpiForWindow"));
 
				this->GetDpiForSystem = reinterpret_cast<GetDpiForSystem_ftype>
								(::GetProcAddress(user32, "GetDpiForSystem"));
 
				this->GetDpiFromDpiAwarenessContext = reinterpret_cast<GetDpiFromDpiAwarenessContext_ftype>
								(::GetProcAddress(user32, "GetDpiFromDpiAwarenessContext"));
 
				this->GetThreadDpiAwarenessContext = reinterpret_cast<GetThreadDpiAwarenessContext_ftype>
								(::GetProcAddress(user32, "GetThreadDpiAwarenessContext"));
 
				this->SetThreadDpiAwarenessContext = reinterpret_cast<SetThreadDpiAwarenessContext_ftype>
								(::GetProcAddress(user32, "SetThreadDpiAwarenessContext"));
 
				this->SetProcessDpiAwarenessContext = reinterpret_cast<SetProcessDpiAwarenessContext_ftype>
								(::GetProcAddress(user32, "SetProcessDpiAwarenessContext"));

				this->GetSystemMetrics = reinterpret_cast<GetSystemMetrics_ftype>
                                (::GetProcAddress(user32, "GetSystemMetrics"));

				this->GetSystemMetricsForDpi = reinterpret_cast<GetSystemMetricsForDpi_ftype>
                                (::GetProcAddress(user32, "GetSystemMetricsForDpi"));

				this->MonitorFromPoint = reinterpret_cast<MonitorFromPoint_ftype>
                                (::GetProcAddress(user32, "MonitorFromPoint"));
			}
 
			/// Dynamically load Shcore.DLL to check these APIs are supported by the SDK and OS.
			/// here we can finf GetDpiForMonitor and SetProcessDpiAwareness; 
			///   and   GetDpiForShellUIComponent, GetProcessDpiAwareness
			auto shcore = ::GetModuleHandleW(L"Shcore.DLL");
			if (nullptr == shcore)
			{
				// std::cerr << "Shcore.DLL not loaded for GetModuleHandleW" << std::endl;   // for debugging
				shcore = ::LoadLibraryW(L"Shcore.DLL");
                // if (nullptr == shcore) std::cerr << "Shcore.DLL not loaded :  ERROR !!!!" << std::endl;   // for debugging
 			}  

			if (shcore)
			{
				this->SetProcessDpiAwareness = reinterpret_cast<SetProcessDpiAwareness_ftype>(
					::GetProcAddress(shcore, "SetProcessDpiAwareness"));
 
				this->GetDpiForMonitor = reinterpret_cast<GetDpiForMonitor_ftype>(
					::GetProcAddress(shcore, "GetDpiForMonitor"));
 			}
		}
	};

	static dpi_function& wdpi_fns() ///< Windows specific DPI functions
	{
		static dpi_function df;  ///< static object, so it is created only once
		return df;
	};

	// debuging function to print all monitor DPIs
	void print_monitor_dpi()
    {
		// compile only if dpi_debugging is enabled
		if constexpr (dpi_debugging) {
        ::EnumDisplayMonitors(
			nullptr,
		nullptr,
		[](HMONITOR	Arg1,
					HDC		Arg2,
					LPRECT		Arg3,
					LPARAM		Arg4)
		{
			MONITORINFOEXA mif;
			mif.cbSize = sizeof(MONITORINFOEXA);
			if (::GetMonitorInfoA(Arg1, &mif) != 0)
			{
				std::cout << mif.szDevice << '\n';
				std::cout
					<< "monitor rect:    "
					<< '(' << mif.rcMonitor.left << ',' << mif.rcMonitor.top << ")-"
					<< '(' << mif.rcMonitor.right << ',' << mif.rcMonitor.bottom << ")\n";
				std::cout
					<< "work rect:       "
					<< '(' << mif.rcWork.left << ',' << mif.rcWork.top << ")-"
					<< '(' << mif.rcWork.right << ',' << mif.rcWork.bottom << ")\n";
			}
			UINT xdpi, ydpi;
			LRESULT success = wdpi_fns().GetDpiForMonitor(Arg1, dpi_function::MDT_EFFECTIVE_DPI, &xdpi, &ydpi);
			if (success == S_OK)
			{
				std::cout << "DPI (effective): " << xdpi << ',' << ydpi << '\n';
			}
			success = wdpi_fns().GetDpiForMonitor(Arg1, dpi_function::MDT_ANGULAR_DPI, &xdpi, &ydpi);
			if (success == S_OK)
			{
				std::cout << "DPI (angular):   " << xdpi << ',' << ydpi << '\n';
			}
			success = wdpi_fns().GetDpiForMonitor(Arg1, dpi_function::MDT_RAW_DPI, &xdpi, &ydpi);
			if (success == S_OK)
			{
				std::cout << "DPI (raw):       " << xdpi << ',' << ydpi << '\n';
			}
			DEVMODEA dm;
			dm.dmSize = sizeof(DEVMODEA);
			if (::EnumDisplaySettingsA(mif.szDevice, ENUM_CURRENT_SETTINGS, &dm) != 0)
			{
				std::cout << "BPP:             " << dm.dmBitsPerPel << '\n';
				std::cout << "resolution:      " << dm.dmPelsWidth << ',' << dm.dmPelsHeight << '\n';
				std::cout << "frequency:       " << dm.dmDisplayFrequency << '\n';
			}
			std::cout << '\n';
			return TRUE;
		},  0);
		} // end of dpi_debugging
    }
	 

	//This function is defined in bedrock_windows.cpp
	HINSTANCE windows_module_handle();

	/// A map to return the HICON associated with a window
	class tray_manager
	{
		struct window_extra_t
		{
			HICON ico{nullptr};
		};

		typedef std::map<native_window_type, window_extra_t> map_t;

	private:
		tray_manager() = default;
	public:
		typedef window_extra_t extra_t;

		static tray_manager& instance()
		{
			static tray_manager object;
			return object;
		}

		bool remove(native_window_type wd, extra_t & ext)
		{
			std::lock_guard<decltype(mutex_)> lock(mutex_);

			auto i = const_cast<const map_t&>(map_).find(wd);
			if(i != map_.cend())
			{
				ext = i->second;
				map_.erase(i);
				return true;
			}
			return false;
		}

		HICON set_icon(native_window_type wd, HICON ico)
		{
			std::lock_guard<decltype(mutex_)> lock(mutex_);

			auto i = map_.find(wd);
			if(i != map_.end())
			{
				HICON ret = i->second.ico;
				i->second.ico = ico;
				return ret;
			}

			map_[wd].ico = ico;
			return 0;
		}
	private:
		std::recursive_mutex mutex_;
		map_t map_;
	};


	/// This proxy for ShowWindow/ShowWindowAsync determines which API should be called.
	void msw_show_window(HWND wd, int cmd)
	{
		bool async = true;
		const DWORD tid = ::GetCurrentThreadId();
		if(tid == ::GetWindowThreadProcessId(wd, nullptr))
		{
			HWND owner = ::GetWindow(wd, GW_OWNER);
			if ((nullptr == owner) || (tid == ::GetWindowThreadProcessId(owner, nullptr)))
			{
				async = false;
				HWND owned = ::GetWindow(wd, GW_HWNDPREV);
				while (owned)
				{
					if (::GetWindow(owned, GW_OWNER) == wd)
					{
						if (tid != ::GetWindowThreadProcessId(owned, nullptr))
						{
							async = true;
							break;
						}
					}
					owned = ::GetWindow(owned, GW_HWNDPREV);
				}
			}
		}
		if (async)
		{
			::ShowWindowAsync(wd, cmd);
			return;
		}
		
		internal_revert_guard revert;
		::ShowWindow(wd, cmd);
	}
  #elif defined(NANA_X11)
	namespace restrict
	{
		nana::detail::platform_spec & spec = nana::detail::platform_spec::instance();
	}


		//The XMoveWindow and XMoveResizeWindow don't take effect if the specified window is
		//unmapped, and the members x and y of XSetSizeHints is obsoleted. So the position that
		//set to a unmapped windows should be kept and use the position when the window is mapped.
		std::map<Window, ::nana::point> exposed_positions;	//locked by platform_scope_guard

		//Returns the parent window.
		//It may return a decoration frame window if the requested window is a top level and WM is a
		//reparenting window manager.
		native_window_type x11_parent_window(native_window_type wd)
		{
			Window root;
			Window parent;
			Window * children;
			unsigned size;

			platform_scope_guard lock;

			if(0 != ::XQueryTree(restrict::spec.open_display(), reinterpret_cast<Window>(wd),
				&root, &parent, &children, &size))
			{
				::XFree(children);
				return reinterpret_cast<native_window_type>(parent);
			}
			return nullptr;
		}

		native_window_type x11_decoration_frame(native_window_type wd)
		{
			auto const owner = restrict::spec.get_owner(wd);
			auto const root_wd = restrict::spec.root_window();

			if(owner)
			{
				auto test_wd = wd;
				while(true)
				{
					auto upper = x11_parent_window(test_wd);
					if((reinterpret_cast<Window>(upper) != root_wd) && (upper != owner))
					{
						test_wd = upper;
					}
					else if(wd != test_wd)
						return test_wd;
					else
						return nullptr;
				}
			}

			return nullptr;
		}


		void x11_apply_exposed_position(native_window_type wd)
		{
			nana::detail::platform_scope_guard lock;

			auto i = exposed_positions.find(reinterpret_cast<Window>(wd));
			if(i == exposed_positions.cend())
				return;

			native_interface::move_window(wd, i->second.x, i->second.y);

			//Don't remove the record with the iterator, the move_window() may remove the
			//record, it makes the iterator invalid.
			exposed_positions.erase(reinterpret_cast<Window>(wd));
		}

		namespace x11_wait
		{
			struct param
			{
				Window handle;
				root_misc * misc;
				std::size_t comp_value;
			};

			static Bool configure(Display *disp, XEvent *evt, char *arg)
			{
				auto p = reinterpret_cast<param*>(arg);
				if(p)
				{
					if(p->misc->x11msg.config != p->comp_value)
						return true;

					if(disp && evt && (evt->type == ConfigureNotify))
					{
						if(evt->xconfigure.window == p->handle)
							return true;
					}
				}
				return false;
			}

			static Bool map(Display *disp, XEvent *evt, char *arg)
			{
				auto p = reinterpret_cast<param*>(arg);
				if(p)
				{
					if(p->misc->x11msg.map != p->comp_value)
						return true;

					if(disp && evt && (evt->type == MapNotify))
					{
						if(evt->xmap.window == p->handle)
							return true;
					}
				}
				return false;
			}

			static Bool unmap(Display *disp, XEvent *evt, char *arg)
			{
				auto p = reinterpret_cast<param*>(arg);
				if(p)
				{
					if(p->misc->x11msg.unmap != p->comp_value)
						return true;

					if(disp && evt && (evt->type == UnmapNotify))
					{
						if(evt->xunmap.window == p->handle)
							return true;
					}
				}
				return false;
			}
		}

		static void x11_wait_for(Window wd, Bool(*pred_fn)(Display*, XEvent*, char*), std::size_t comp_value)
		{
			auto misc = bedrock::instance().wd_manager().root_runtime(reinterpret_cast<native_window_type>(wd));
			x11_wait::param p;
			p.handle = wd;
			p.misc = misc;

			if(pred_fn == &x11_wait::configure)
				p.comp_value = misc->x11msg.config;
			else if(pred_fn == &x11_wait::map)
				p.comp_value = misc->x11msg.map;
			else if(pred_fn == &x11_wait::unmap)
				p.comp_value = misc->x11msg.unmap;

			//Checks whether the msg is received.
			if(p.comp_value != comp_value)
				return;

			p.comp_value = comp_value;

			XEvent dummy;
			::XPeekIfEvent(restrict::spec.open_display(), &dummy, pred_fn, reinterpret_cast<XPointer>(&p));
		}
  #endif

    /// Invokes a function in the thread of the specified window.
	void native_interface::affinity_execute(native_window_type native_handle, bool post, std::function<void()>&& fn)
	{
		if (!fn)
			return;

  #if defined(NANA_WINDOWS)
		auto mswin = reinterpret_cast<HWND>(native_handle);
		if (::IsWindow(mswin))
		{
			if (::GetCurrentThreadId() != ::GetWindowThreadProcessId(mswin, nullptr))
			{
				auto arg = new detail::messages::arg_affinity_execute;

				arg->function = std::move(fn);

				if(post)
				{
					::PostMessage(mswin, detail::messages::affinity_execute, reinterpret_cast<WPARAM>(arg), 0);
				}
				else
				{
					internal_revert_guard rev;
					::SendMessage(mswin, detail::messages::affinity_execute, reinterpret_cast<WPARAM>(arg), 0);
				}
				return;
			}
		}

		fn();
  #else
		auto & platform_spec = nana::detail::platform_spec::instance();
		if(post)
		{
			platform_spec.affinity_execute(native_handle, std::move(fn));
			return;
		}

		auto wd = bedrock::instance().wd_manager().root(native_handle);

		if(!wd)
			return;

		if(nana::system::this_thread_id() == wd->thread_id)
		{
			fn();
		}
		else
		{
			internal_revert_guard rev;

			std::mutex mutex;
			std::condition_variable condvar;

			std::unique_lock<std::mutex> lock{mutex};

			platform_spec.affinity_execute(native_handle, [fn, &mutex, &condvar] {
				fn();

				std::lock_guard<std::mutex> lock{mutex};
				condvar.notify_one();
			});

			condvar.wait(lock);
		}
  #endif	
	}

	/// generalized to Windows dpi awareness v2 to return already 'DPI' unscaled (user-side) size
	/// this result is used to calculate the area available to draw with may be smaller than the monitor size
	nana::size native_interface::primary_monitor_size()
	{
		nana::size sz{0, 0};
  #if defined(NANA_WINDOWS)

		if (wdpi_fns().GetDpiForMonitor && !wdpi_fns().GetDpiForWindow) // priorize possible x_dpi != y_dpi. Really?? 
		{
            // get the main monitor HWND
            HWND primary_monitor = ::GetDesktopWindow();
            HMONITOR pmonitor = ::MonitorFromWindow(primary_monitor, MONITOR_DEFAULTTOPRIMARY);
            UINT x_dpi, y_dpi; // here x_dpi != y_dpi
            if (S_OK == wdpi_fns().GetDpiForMonitor(pmonitor, dpi_function::MDT_EFFECTIVE_DPI, &x_dpi, &y_dpi))
			{
				if constexpr (dpi_debugging) 
					std::cout << "primary_monitor_size(): DPI= " << x_dpi << " x " << y_dpi << std::endl;

				if (false) //wdpi_fns().GetSystemMetricsForDpi) // do not scale ?
					return nana::size(wdpi_fns().GetSystemMetricsForDpi(SM_CXSCREEN, x_dpi),
									  wdpi_fns().GetSystemMetricsForDpi(SM_CYSCREEN, y_dpi));  //  x_dpi != y_dpi ?? 

				else // fallback to GetSystemMetrics: here x_dpi != y_dpi
					return nana::size(MulDiv(::GetSystemMetrics(SM_CXSCREEN), 96, x_dpi),
									  MulDiv(::GetSystemMetrics(SM_CYSCREEN), 96, y_dpi));
			}
        }

		int dpi = native_interface::system_dpi();  // originaly got from UINT or int: safe to get back to that
		if constexpr (dpi_debugging) std::cout << "primary_monitor_size(): DPI= " << dpi << std::endl;

		if (false) //wdpi_fns().GetSystemMetricsForDpi) // do not scale ?
        {
			if constexpr (dpi_debugging) 
				std::cout << "primary_monitor_size() with GetSystemMetrics: \n" ;
		    
			sz = nana::size(::GetSystemMetrics(SM_CXSCREEN),
					        ::GetSystemMetrics(SM_CYSCREEN));
            sz = unscale_dpi(sz, dpi);
						
			
			sz = nana::size(wdpi_fns().GetSystemMetricsForDpi(SM_CXSCREEN, UINT(dpi)), 
						    wdpi_fns().GetSystemMetricsForDpi(SM_CYSCREEN, UINT(dpi)));
			if constexpr (dpi_debugging) 
				std::cout << "primary_monitor_size() with GetSystemMetricsForDpi: size= " 
				          << sz.width << " x " << sz.height << std::endl;
			
			return sz;
        }

		sz = nana::size(::GetSystemMetrics(SM_CXSCREEN), 
						::GetSystemMetrics(SM_CYSCREEN));

		if constexpr (dpi_debugging) 
			std::cout << "primary_monitor_size() with GetSystemMetrics: \n" ;
			
		return unscale_dpi(sz, dpi);  /// \todo: use platform_abstraction::unscale_dpi() instead, to include X11 ? 

  #elif defined(NANA_X11)
		nana::detail::platform_scope_guard psg;
		Screen* s = ::XScreenOfDisplay(restrict::spec.open_display(), ::XDefaultScreen(restrict::spec.open_display()));
		return nana::size(::XWidthOfScreen(s), ::XHeightOfScreen(s));
  #endif
	}
    	/// \todo: generalize dpi to v2 awareness 
		rectangle native_interface::screen_area_from_system_point(const point& system_point) ///< unused ?
		{
#if defined(NANA_WINDOWS)
			// led assume somehow coordinates in the united global big and 'fake'-96 DPI sytem monitor are provided 
			// in pos (POINT structure that specifies the point of interest in virtual-screen coordinates.)
			if(wdpi_fns().MonitorFromPoint)
			{
				POINT native_pos = {system_point.x, system_point.y};
				HMONITOR monitor = wdpi_fns().MonitorFromPoint(native_pos, 2 /*MONITOR_DEFAULTTONEAREST*/);
				UINT x_dpi, y_dpi;
                if (S_OK == wdpi_fns().GetDpiForMonitor(monitor, dpi_function::MDT_EFFECTIVE_DPI, &x_dpi, &y_dpi))
				{
					if constexpr (dpi_debugging) 
						std::cout << "screen_area_from_system_point(): DPI= " << x_dpi << " x " << y_dpi << std::endl;
					MONITORINFO mi;
					mi.cbSize = sizeof mi;
 
					if(::GetMonitorInfo(monitor, &mi))
					{
						return unscale_dpi(rectangle(mi.rcWork.left,                   mi.rcWork.top,
										             mi.rcWork.right - mi.rcWork.left, mi.rcWork.bottom - mi.rcWork.top),
										   x_dpi); 
						/// \todo: use platform_abstraction::unscale_dpi() instead, to include X11 ? 
					}
				}
			}
#else
			static_cast<void>(pos); //eliminate unused parameter compiler warning.
#endif
			return rectangle{ primary_monitor_size() };
		}

		//platform-dependent
		native_interface::window_result native_interface::create_window(native_window_type owner, bool nested, const rectangle& r, const appearance& app)
		{
			/// \todo: use platform_abstraction::dpi_scale(r, dpi) instead, to include X11 ? 

#if defined(NANA_WINDOWS)
			DWORD style = WS_SYSMENU | WS_CLIPCHILDREN;
			DWORD style_ex= WS_EX_NOPARENTNOTIFY;

			if(app.minimize)	style |= WS_MINIMIZEBOX;
			if(app.maximize)	style |= WS_MAXIMIZEBOX;

			if(app.sizable)	style |= WS_THICKFRAME;

			if(app.decoration)
				style |= WS_OVERLAPPED | WS_CAPTION;

			style |= (nested ? WS_CHILD : WS_POPUP);
			style_ex |= (app.taskbar ? WS_EX_APPWINDOW : WS_EX_TOOLWINDOW);

			if(app.floating)	style_ex |= WS_EX_TOPMOST;

			int dpi = static_cast<int>(native_interface::window_dpi(owner));

			if constexpr (dpi_debugging)
				std::wcout << "   ---  create_window():\n";  // on:" << window_caption(owner) << "\n";

			nana::rectangle scaled_r = scale_to_dpi(r, dpi);

			POINT pt = {scaled_r.x, scaled_r.y};

			if(owner && (nested == false))
				::ClientToScreen(reinterpret_cast<HWND>(owner), &pt);

			HWND native_wd = ::CreateWindowEx(style_ex, L"NanaWindowInternal", L"Nana Window",
											style,
											pt.x, pt.y, 100, 100,
											reinterpret_cast<HWND>(owner), 0, windows_module_handle(), 0);

			//A window may have a border, this should be adjusted the client area fit for the specified size.
			::RECT client;
			::GetClientRect(native_wd, &client);	//The right and bottom of client by GetClientRect indicate the width and height of the area
			::RECT wd_area;
			::GetWindowRect(native_wd, &wd_area);

			if constexpr (dpi_debugging) {
				// print for debuging the position of creation of the window pt
				std::cout << "          pt= " << pt.x << ", " << pt.y << 
					         "   with size= " << 100  << ", " << 100  << std::endl;
				// with a client area of:
				std::cout << "      client= " << client.left                << ", " << client.top                 << 
					         "   with size= " << client.right - client.left << ", " << client.bottom - client.top << std::endl;
				// and a window area of:
				std::cout << "     wd_area= " << wd_area.left                 << ", " << wd_area.top                  << 
					         "   with size= " << wd_area.right - wd_area.left << ", " << wd_area.bottom - wd_area.top << std::endl;
				}


			//a dimension with borders and caption title
			wd_area.right -= wd_area.left;	//wd_area.right = width
			wd_area.bottom -= wd_area.top;	//wd_area.bottom = height
			if (nested)
			{
				wd_area.left = pt.x;
				wd_area.top = pt.y;
			}

			int delta_w = static_cast<int>(scaled_r.width ) - client.right;
			int delta_h = static_cast<int>(scaled_r.height) - client.bottom;

			::MoveWindow(native_wd, wd_area.left, wd_area.top, wd_area.right + delta_w, wd_area.bottom + delta_h, true);
			// the window was moved to:
			if constexpr (dpi_debugging) 
				std::cout << "    moved to= " << wd_area.left                           << ", " << wd_area.top                           << 
				             "   with size= " << wd_area.right + delta_w - wd_area.left << ", " << wd_area.bottom + delta_h -wd_area.top << std::endl;

			::GetClientRect(native_wd, &client);
			::GetWindowRect(native_wd, &wd_area);
						
			if constexpr (dpi_debugging) {
				// with a client area of:
				std::cout << " moved client= " << client.left                << ", " << client.top                 << 
			   			     "    with size= " << client.right - client.left << ", " << client.bottom - client.top << std::endl;
				// and a window area of:
				std::cout << " moved wd_area= " << wd_area.left                 << ", " << wd_area.top                  << 
					          "    with size= " << wd_area.right - wd_area.left << ", " << wd_area.bottom - wd_area.top << std::endl;
			}

			wd_area.right -= wd_area.left;
			wd_area.bottom -= wd_area.top;

			// unscale from system coordinates to App scale
			window_result result = { reinterpret_cast<native_window_type>(native_wd),
										static_cast<unsigned>(MulDiv(client.right,                   96, dpi)), 
				                        static_cast<unsigned>(MulDiv(client.bottom,                  96, dpi)),
										static_cast<unsigned>(MulDiv(wd_area.right - client.right,   96, dpi)), 
				                        static_cast<unsigned>(MulDiv(wd_area.bottom - client.bottom, 96, dpi))
			                       };
#elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;

			XSetWindowAttributes win_attr;
			unsigned long attr_mask = CWBackPixmap | CWBackPixel | CWBorderPixel |
							 CWColormap | CWEventMask;

			Display * disp = restrict::spec.open_display();
			win_attr.colormap = restrict::spec.colormap();

			win_attr.background_pixmap = None;
			win_attr.background_pixel = 0xFFFFFF;
			win_attr.border_pixmap = None;
			win_attr.border_pixel = 0x0;
			win_attr.backing_store = 0;
			win_attr.backing_planes = 0;
			win_attr.backing_pixel = 0;
			win_attr.colormap = restrict::spec.colormap();

			if(app.floating && !app.decoration)
			{
				win_attr.override_redirect = True;
				attr_mask |= CWOverrideRedirect;
			}

			Window parent = (owner ? reinterpret_cast<Window>(owner) : restrict::spec.root_window());

			//The position passed to XCreateWindow is a screen coordinate.
			auto pos = r.position();
			if((false == nested) && owner)
			{
				win_attr.save_under = True;
				attr_mask |= CWSaveUnder;

				///The parameter of XCreateWindow to create a top level window must be root.
				///But after creation, the real parent is the reparenting frame window.
				parent = restrict::spec.root_window();
				calc_screen_point(owner, pos);
			}

			win_attr.event_mask = ButtonPressMask | ButtonReleaseMask | PointerMotionMask | KeyPressMask | KeyReleaseMask | ExposureMask | StructureNotifyMask | EnterWindowMask | LeaveWindowMask | FocusChangeMask;

			Window handle = ::XCreateWindow(disp, parent,
							pos.x, pos.y, (r.width ? r.width : 1), (r.height ? r.height : 1), 0,
							restrict::spec.screen_depth(), InputOutput, restrict::spec.screen_visual(),
							attr_mask, &win_attr);
			if(handle)
			{
				//make owner if it is a popup window
				if(!nested)
				{
					auto origin_owner = (owner ? owner : reinterpret_cast<native_window_type>(restrict::spec.root_window()));
					restrict::spec.make_owner(origin_owner, reinterpret_cast<native_window_type>(handle));

					//The exposed_position is a relative position to its owner/parent.
					exposed_positions[handle] = r.position();
				}

				XChangeWindowAttributes(disp, handle, attr_mask, &win_attr);

				XTextProperty name;
				char text[] = "Nana Window";
				char * str = text;
				::XStringListToTextProperty(&str, 1, &name);
				::XSetWMName(disp, handle, &name);

				const nana::detail::atombase_tag & ab = restrict::spec.atombase();
				::XSetWMProtocols(disp, handle, const_cast<Atom*>(&ab.wm_delete_window), 1);

				struct
				{
					long flags;
					long functions;
					long decorations;
					long input;
					long status;
				}motif;
				//MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS;// | MWM_HINTS_INPUT_MODE;
				motif.flags = 1 | 2; //| 4;
				motif.functions = 4;//MWM_FUNC_MOVE;
				motif.decorations = 0;
				motif.input = 0;//MWM_INPUT_MODELESS;
				motif.status = 0;

				XSizeHints hints;
				hints.flags = USPosition;
				hints.x = pos.x;
				hints.y = pos.y;

				if(app.sizable)
				{
					motif.decorations |= 4; //MWM_DECOR_RESIZEH;
					motif.functions |= 2; //MWM_FUNC_RESIZE;
				}
				else
				{
					hints.min_width = hints.max_width = r.width;
					hints.min_height = hints.max_height = r.height;
					hints.flags |= (PMinSize | PMaxSize);
				}
				::XSetWMNormalHints(disp, handle, &hints);

				if(app.decoration)
				{
					if(app.minimize)
					{
						motif.decorations |= (1 << 5);	//MWM_DECOR_MINIMIZE;
						motif.functions |= (1 << 3);	//MWM_FUNC_MINIMIZE;
					}

					if(app.maximize)
					{
						motif.decorations |= (1 << 6);//MWM_DECOR_MAXIMIZE;
						motif.functions |= ( 1 << 4);//MWM_FUNC_MAXIMIZE;
					}
					motif.functions |= (1<<5); //MWM_FUNC_CLOSE
					motif.decorations |= (2) | 8; //MWM_DECOR_BORDER | MWM_DECOR_TITLE
				}

				if((false == nested) && owner)
				{
					::XChangeProperty(disp, handle, ab.net_wm_window_type, XA_ATOM, 32, PropModeReplace,
							reinterpret_cast<unsigned char*>(const_cast<Atom*>(&ab.net_wm_window_type_dialog)), 1);
					::XSetTransientForHint(disp, handle, reinterpret_cast<Window>(owner));
				}

				::XChangeProperty(disp, handle, ab.motif_wm_hints, ab.motif_wm_hints, 32, PropModeReplace,
									reinterpret_cast<unsigned char*>(&motif), sizeof(motif)/sizeof(long));

				if(app.floating)
				{
					::XChangeProperty(disp, handle, ab.net_wm_window_type, XA_ATOM, 32, PropModeReplace,
								reinterpret_cast<unsigned char*>(const_cast<Atom*>(&ab.net_wm_window_type_normal)), 1);
					::XSetTransientForHint(disp, handle, restrict::spec.root_window());
				}

				if(false == app.taskbar)
				{
					::XChangeProperty(disp, handle, ab.net_wm_state, XA_ATOM, 32, PropModeAppend,
										reinterpret_cast<unsigned char*>(const_cast<Atom*>(&ab.net_wm_state_skip_taskbar)), 1);
				}
			}
			window_result result = {reinterpret_cast<native_window_type>(handle), r.width, r.height, 0, 0};
			restrict::spec.msg_insert(reinterpret_cast<native_window_type>(handle));
#endif
			return result;
		}

		native_window_type native_interface::create_child_window(native_window_type parent, const rectangle& r) ///< unused ?
		{
			if(nullptr == parent) return nullptr;
			/// \todo: use platform_abstraction::dpi_scale(r, dpi) instead, to include X11 ? 
  #if defined(NANA_WINDOWS)

			if constexpr (dpi_debugging) std::cout << "   ---  create_child_window():\n";

			nana::rectangle scaled_r = scale_to_dpi(parent, r);

			HWND handle = ::CreateWindowEx(WS_EX_CONTROLPARENT,		// Extended possibilities for variation
										L"NanaWindowInternal",
										L"Nana Child Window",	// Title Text
										WS_CHILD | WS_VISIBLE | WS_TABSTOP  | WS_CLIPSIBLINGS,
										scaled_r.x, scaled_r.y, scaled_r.width, scaled_r.height,
										reinterpret_cast<HWND>(parent),	// The window is a child-window to desktop
										0, windows_module_handle(), 0);

  #elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;

			XSetWindowAttributes win_attr;
			unsigned long attr_mask = CWBackPixmap | CWBackPixel | CWBorderPixel |
							 CWColormap | CWEventMask | CWOverrideRedirect;

			Display * disp = restrict::spec.open_display();
			win_attr.colormap = restrict::spec.colormap();

			win_attr.background_pixmap = None;
			win_attr.background_pixel = 0xFFFFFF;
			win_attr.border_pixmap = None;
			win_attr.border_pixel = 0x0;
			win_attr.backing_store = 0;
			win_attr.backing_planes = 0;
			win_attr.backing_pixel = 0;
			win_attr.colormap = restrict::spec.colormap();

			win_attr.override_redirect = True;

			nana::point pos(r.x, r.y);
			win_attr.event_mask = ButtonPressMask | ButtonReleaseMask | PointerMotionMask | KeyPressMask | KeyReleaseMask | ExposureMask | StructureNotifyMask | EnterWindowMask | LeaveWindowMask | FocusChangeMask;

			Window handle = ::XCreateWindow(disp, reinterpret_cast<Window>(parent),
							pos.x, pos.y, (r.width ? r.width : 1), (r.height ? r.height : 1), 0,
							restrict::spec.screen_depth(), InputOutput, restrict::spec.screen_visual(),
							attr_mask, &win_attr);

			if(handle)
			{
				XTextProperty name;
				char text[] = "Nana Child Window";
				char * str = text;
				::XStringListToTextProperty(&str, 1, &name);
				::XSetWMName(disp, handle, &name);

				const nana::detail::atombase_tag & ab = restrict::spec.atombase();
				::XSetWMProtocols(disp, handle, const_cast<Atom*>(&ab.wm_delete_window), 1);

				struct
				{
					long flags;
					long functions;
					long decorations;
					long input;
					long status;
				}motif;
				//MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS;// | MWM_HINTS_INPUT_MODE;
				motif.flags = 1 | 2; //| 4;
				motif.functions = 4;//MWM_FUNC_MOVE;
				motif.decorations = 0;
				motif.input = 0;//MWM_INPUT_MODELESS;
				motif.status = 0;

				XSizeHints hints;
				hints.flags = USPosition;
				hints.x = pos.x;
				hints.y = pos.y;
				hints.min_width = hints.max_width = r.width;
				hints.min_height = hints.max_height = r.height;
				hints.flags |= (PMinSize | PMaxSize);
				::XSetWMNormalHints(disp, handle, &hints);

				::XChangeProperty(disp, handle, ab.motif_wm_hints, ab.motif_wm_hints, 32, PropModeReplace,
									reinterpret_cast<unsigned char*>(&motif), sizeof(motif)/sizeof(long));

				::XChangeProperty(disp, handle, ab.net_wm_state, XA_ATOM, 32, PropModeAppend,
						reinterpret_cast<unsigned char*>(const_cast<Atom*>(&ab.net_wm_state_skip_taskbar)), 1);
			}
  #endif
			return reinterpret_cast<native_window_type>(handle);
		}

#if defined(NANA_X11)
		void native_interface::set_modal(native_window_type wd)
		{
			Window owner = reinterpret_cast<Window>(restrict::spec.get_owner(wd));
			if(wd && owner)
			{
				if(is_window_visible(wd))
					show_window(wd, false, true);
				auto disp = restrict::spec.open_display();
				auto & atombase = restrict::spec.atombase();
				::XSetTransientForHint(disp, reinterpret_cast<Window>(wd), owner);
				::XChangeProperty(disp, reinterpret_cast<Window>(wd),
								atombase.net_wm_state, XA_ATOM, sizeof(int) * 8,
								PropModeReplace,
								reinterpret_cast<const unsigned char*>(&atombase.net_wm_state_modal), 1);
			}
		}
#endif

		void native_interface::enable_dropfiles(native_window_type wd, bool enb)
		{
#if defined(NANA_WINDOWS)
			::DragAcceptFiles(reinterpret_cast<HWND>(wd), enb);
#else
			int dndver = (enb ? 4: 0);
			::XChangeProperty(restrict::spec.open_display(), reinterpret_cast<Window>(wd), restrict::spec.atombase().xdnd_aware, XA_ATOM, sizeof(int) * 8,
				PropModeReplace, reinterpret_cast<unsigned char*>(&dndver), 1);
#endif
		}

		void native_interface::enable_window(native_window_type wd, bool is_enabled)
		{
#if defined(NANA_WINDOWS)
			::EnableWindow(reinterpret_cast<HWND>(wd), is_enabled);
#else
			int mask = ExposureMask | StructureNotifyMask;
			if(is_enabled)
			{
				mask |= (ButtonPressMask | ButtonReleaseMask | PointerMotionMask);
				mask |= (KeyPressMask | KeyReleaseMask);
				mask |= (EnterWindowMask | LeaveWindowMask | FocusChangeMask);
			}

			::XSelectInput(restrict::spec.open_display(), reinterpret_cast<Window>(wd), mask);
#endif
		}

		bool native_interface::window_icon(native_window_type wd, const nana::paint::image& sml_icon, const ::nana::paint::image& big_icon)
		{
#if defined(NANA_WINDOWS)
			HICON sml_handle = paint::image_accessor::icon(sml_icon);
			HICON big_handle = paint::image_accessor::icon(big_icon);
			if(sml_handle || big_handle)
			{
				nana::detail::platform_spec::instance().keep_window_icon(wd, sml_icon, big_icon);
				if (sml_handle)
					::SendMessage(reinterpret_cast<HWND>(wd), WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(sml_handle));

				if (big_handle)
					::SendMessage(reinterpret_cast<HWND>(wd), WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(big_handle));
				return true;
			}
#elif defined(NANA_X11)
			if(wd && (!sml_icon.empty() || !big_icon.empty()))
			{
				auto & img = (sml_icon.empty() ? big_icon : sml_icon);

				const nana::paint::graphics & graph = restrict::spec.keep_window_icon(wd, img);
				XWMHints hints;
				hints.flags = IconPixmapHint;
				hints.icon_pixmap = graph.handle()->pixmap;

				nana::detail::platform_scope_guard psg;
				::XSetWMHints(restrict::spec.open_display(), reinterpret_cast<Window>(wd), &hints);
				return true;
			}
#endif
			return false;
		}

		void native_interface::activate_owner(native_window_type wd)
		{
#if defined(NANA_WINDOWS)
			activate_window(reinterpret_cast<native_window_type>(
								::GetWindow(reinterpret_cast<HWND>(wd), GW_OWNER)
							));
#else
			static_cast<void>(wd);	//eliminate unused parameter compiler warning.
#endif
		}

		void native_interface::activate_window(native_window_type wd)
		{
#if defined(NANA_WINDOWS)
			auto native_wd = reinterpret_cast<HWND>(wd);
			if (::IsWindow(native_wd))
			{
				if (::GetWindowThreadProcessId(native_wd, nullptr) == ::GetCurrentThreadId())
				{
					::EnableWindow(native_wd, true);
					::SetActiveWindow(native_wd);
					::SetForegroundWindow(native_wd);
				}
				else
					::PostMessage(native_wd, nana::detail::messages::async_activate, 0, 0);
			}
#else
			static_cast<void>(wd);	//eliminate unused parameter compiler warning.
#endif
		}

		//close_window
		//Destroy a window
		void native_interface::close_window(native_window_type wd)
		{
#if defined(NANA_WINDOWS)
			if(wd && (::DestroyWindow(reinterpret_cast<HWND>(wd)) == false))
			{
				//DestroyWindow would be failed if the calling thread is not the window thread
				//It should send a WM_DESTROY message into window thread for destroying window
				if(::GetLastError() == ERROR_ACCESS_DENIED)
					::PostMessage(reinterpret_cast<HWND>(wd), nana::detail::messages::remote_thread_destroy_window, 0, 0);
			}
#elif defined(NANA_X11)
			//Under X, XDestroyWindow destroys the specified window and generates a DestroyNotify
			//event, when the client receives the event, the specified window has been already
			//destroyed. This is a feature which is different from Windows. So the following
			//works should be handled before calling XDestroyWindow.
			auto & brock = bedrock::instance();
			if(wd == brock.get_menu())
			{
				brock.erase_menu(false);
				brock.delay_restore(3);	//Restores if delay_restore is not declared
			}

			Display* disp = restrict::spec.open_display();
			restrict::spec.remove(wd);
			auto iwd = brock.wd_manager().root(wd);
			if(iwd)
			{
				{
					//Before calling window_manager::destroy, make sure the window is invisible.
					//It is a behavior like Windows.
					nana::detail::platform_scope_guard lock;
					restrict::spec.set_error_handler();
					::XUnmapWindow(disp, reinterpret_cast<Window>(wd));
					::XFlush(disp);
					restrict::spec.rev_error_handler();
				}
				brock.wd_manager().destroy(iwd);
				brock.manage_form_loader(iwd, false);
				brock.wd_manager().destroy_handle(iwd);
			}

			nana::detail::platform_scope_guard psg;
			restrict::spec.set_error_handler();
			::XDestroyWindow(disp, reinterpret_cast<Window>(wd));
			restrict::spec.rev_error_handler();
#endif
		}

		void native_interface::show_window(native_window_type wd, bool show, bool active)
		{
#if defined(NANA_WINDOWS)
			int cmd = (show ? (active ? SW_SHOW : SW_SHOWNA) : SW_HIDE);
			msw_show_window(reinterpret_cast<HWND>(wd), cmd);
#elif defined(NANA_X11)
			if(wd)
			{
				nana::detail::platform_scope_guard psg;
				Display* disp = restrict::spec.open_display();

				//Returns if the requested visibility is same with the current status.
				//In some X-Server versions/implementations, XMapWindow() doesn't generate
				//a ConfigureNotify if the requested visibility is same with the current status.
				//It causes that x11_wait_for always waiting for the ConfigureNotify.
				if(show == is_window_visible(wd))
					return;

				auto misc = bedrock::instance().wd_manager().root_runtime(wd);

				if(show)
				{
					std::size_t cmp_value = misc->x11msg.map;

					::XMapWindow(disp, reinterpret_cast<Window>(wd));

					//Wait for the mapping notify to update the local attribute of visibility so that
					//the followed window_visible() call can return the updated visibility value.
					x11_wait_for(reinterpret_cast<Window>(wd), x11_wait::map, cmp_value);
					
					Window grab = restrict::spec.grab(0);
					if(grab == reinterpret_cast<Window>(wd))
						capture_window(wd, true);
				}
				else
				{
					std::size_t cmp_value = misc->x11msg.unmap;
					::XUnmapWindow(disp, reinterpret_cast<Window>(wd));

					//Wait for the mapping notify to update the local attribute of visibility so that
					//the followed window_visible() call can return the updated visibility value.
					x11_wait_for(reinterpret_cast<Window>(wd), x11_wait::unmap, cmp_value);
				}
			}
			static_cast<void>(active);	//eliminate unused parameter compiler warning.
#endif
		}

		void native_interface::restore_window(native_window_type wd)
		{
#if defined(NANA_WINDOWS)
			msw_show_window(reinterpret_cast<HWND>(wd), SW_RESTORE);
#elif defined(NANA_X11)
			//Restore the window by removing NET_WM_STATE_MAXIMIZED_HORZ,
			//_NET_WM_STATE_MAXIMIZED_VERT and _NET_WM_STATE_FULLSCREEN.
			Display * disp = restrict::spec.open_display();
			Window default_root = XDefaultRootWindow(disp);
			const nana::detail::atombase_tag & atombase = restrict::spec.atombase();
			XEvent evt;
			evt.xclient.type = ClientMessage;
			evt.xclient.display = restrict::spec.open_display();
			evt.xclient.message_type = atombase.net_wm_state;
			evt.xclient.format = 32;
			evt.xclient.window = reinterpret_cast<Window>(wd);
			evt.xclient.data.l[0] = 0;	//_NET_WM_STATE_REMOVE
			evt.xclient.data.l[1] = atombase.net_wm_state_maximized_horz;
			evt.xclient.data.l[2] = atombase.net_wm_state_maximized_vert;
			evt.xclient.data.l[3] = evt.xclient.data.l[4] = 0;

			nana::detail::platform_scope_guard psg;
			::XSendEvent(disp, default_root, False, SubstructureRedirectMask | SubstructureNotifyMask, &evt);
			evt.xclient.data.l[1] = atombase.net_wm_state_fullscreen;
			evt.xclient.data.l[2] = 0;
			::XSendEvent(disp, default_root, False, SubstructureRedirectMask | SubstructureNotifyMask, &evt);

			//Transfer the window from IconState to NormalState.
			evt.xclient.message_type = atombase.wm_change_state;
			evt.xclient.data.l[0] = NormalState;
			evt.xclient.data.l[1] = 0;
			::XSendEvent(disp, default_root, False, SubstructureRedirectMask | SubstructureNotifyMask, &evt);
			::XMapWindow(disp, reinterpret_cast<Window>(wd));
			restrict::spec.set_error_handler();
			::XSetInputFocus(disp, reinterpret_cast<Window>(wd), RevertToPointerRoot, CurrentTime);
#endif
		}

		void native_interface::zoom_window(native_window_type wd, bool ask_for_max)
		{
#if defined(NANA_WINDOWS)
			msw_show_window(reinterpret_cast<HWND>(wd), ask_for_max ? SW_MAXIMIZE : SW_MINIMIZE);
#elif defined(NANA_X11)
			Display * disp = restrict::spec.open_display();
			if (ask_for_max)
			{
				const nana::detail::atombase_tag & atombase = restrict::spec.atombase();
				XEvent evt;
				evt.xclient.type = ClientMessage;
				evt.xclient.display = restrict::spec.open_display();
				evt.xclient.message_type = atombase.net_wm_state;
				evt.xclient.format = 32;
				evt.xclient.window = reinterpret_cast<Window>(wd);
				evt.xclient.data.l[0] = 1;	//_NET_WM_STATE_ADD
				evt.xclient.data.l[1] = atombase.net_wm_state_maximized_horz;
				evt.xclient.data.l[2] = atombase.net_wm_state_maximized_vert;
				evt.xclient.data.l[3] = evt.xclient.data.l[4] = 0;

				nana::detail::platform_scope_guard psg;
				::XSendEvent(disp, XDefaultRootWindow(disp), False, SubstructureRedirectMask | SubstructureNotifyMask, &evt);
				::XMapWindow(disp, reinterpret_cast<Window>(wd));
			}
			else
				::XIconifyWindow(disp, reinterpret_cast<Window>(wd), XDefaultScreen(disp));
#endif
		}

		void native_interface::refresh_window(native_window_type native_wd)
		{
#if defined(NANA_WINDOWS)
			auto wd = reinterpret_cast<HWND>(native_wd);
			RECT r;
			::GetClientRect(wd, &r);
			::InvalidateRect(wd, &r, FALSE);
#elif defined(NANA_X11)
			Display * disp = restrict::spec.open_display();
			::XClearArea(disp, reinterpret_cast<Window>(native_wd), 0, 0, 1, 1, true);
			::XFlush(disp);
#endif
		}

		bool native_interface::is_window(native_window_type wd)
		{
#if defined(NANA_WINDOWS)
			return (FALSE != ::IsWindow(reinterpret_cast<HWND>(wd)));
#elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;
			XWindowAttributes attr;
			restrict::spec.set_error_handler();
			::XGetWindowAttributes(restrict::spec.open_display(), reinterpret_cast<Window>(wd), &attr);
			return (BadWindow != restrict::spec.rev_error_handler());
#endif
		}

		bool native_interface::is_window_visible(native_window_type wd)
		{
#if defined(NANA_WINDOWS)
			return (FALSE != ::IsWindowVisible(reinterpret_cast<HWND>(wd)));
#elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;
			XWindowAttributes attr;
			restrict::spec.set_error_handler();
			::XGetWindowAttributes(restrict::spec.open_display(), reinterpret_cast<Window>(wd), &attr);
			return (BadWindow != restrict::spec.rev_error_handler() &&
					attr.map_state != IsUnmapped);
#endif
		}

		bool native_interface::is_window_zoomed(native_window_type wd, bool ask_for_max)
		{
#if defined(NANA_WINDOWS)
			return (FALSE != (ask_for_max ? ::IsZoomed(reinterpret_cast<HWND>(wd)) : ::IsIconic(reinterpret_cast<HWND>(wd))));
#elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;
			bool zoomed = false;
    		unsigned long n,i; Atom type; unsigned char *prop; int format;
			if(Success== ::XGetWindowProperty(restrict::spec.open_display(), reinterpret_cast<Window>(wd), restrict::spec.atombase().net_wm_state, 0, 2, false, AnyPropertyType, &type, &format, &n, &i, &prop))
			{
				if(32 == format)
				{
					if(ask_for_max)
					{
						if(type == XA_ATOM)
						{
							for(i=0; i<n; i++)
							{
								if(reinterpret_cast<Atom*>(prop)[i] == restrict::spec.atombase().net_wm_state_fullscreen)
								{
									zoomed = true;
									break;
								}
          					}
						}
					}
					else
						zoomed = (IconicState == *reinterpret_cast<unsigned*>(prop));
				}
				XFree(prop);
			}
			return zoomed;
#endif
		}

		nana::point native_interface::window_position(native_window_type wd)
		{
			/// \todo: use platform_abstraction::unscale_dpi(p, dpi) instead, to include X11 ? 
#if defined(NANA_WINDOWS)
			if constexpr (dpi_debugging)
				std::wcout << "   ---  window_position() " << window_caption(wd) << ":\n";

			::RECT r;
			::GetWindowRect(reinterpret_cast<HWND>(wd), & r);
			HWND coord_wd = ::GetWindow(reinterpret_cast<HWND>(wd), GW_OWNER);

			if (!coord_wd)
				coord_wd = ::GetParent(reinterpret_cast<HWND>(wd));

			if (coord_wd)
			{
				::POINT pos = {r.left, r.top};
				::ScreenToClient(coord_wd, &pos);
				return unscale_dpi(wd, pos.x, pos.y);
			}
			return unscale_dpi(wd, r.left, r.top);
#elif defined(NANA_X11)
			point scr_pos;
			nana::detail::platform_scope_guard lock;


			point origin{};

			auto coord_wd = restrict::spec.get_owner(wd);
			if(coord_wd)
			{
				auto fm_extents = window_frame_extents(wd);
				origin.x = -fm_extents.left;
				origin.y = -fm_extents.top;
			}
			else
				coord_wd = get_window(wd, window_relationship::parent);

			Window child;
			::XTranslateCoordinates(restrict::spec.open_display(), reinterpret_cast<Window>(wd), reinterpret_cast<Window>(coord_wd), origin.x, origin.y, &scr_pos.x, &scr_pos.y, &child);

			return scr_pos;
#endif
		}

		void native_interface::move_window(native_window_type wd, int x, int y)
		{
			/// \todo: use platform_abstraction::dpi_scale(p, dpi) instead, to include X11 ? 
#if defined(NANA_WINDOWS)
			if constexpr (dpi_debugging) std::wcout << "   ---  move_window(x,y):" << window_caption(wd) << "\n";
			auto p = scale_to_dpi(wd, x, y);
			::RECT r;
			::GetWindowRect(reinterpret_cast<HWND>(wd), &r);
			HWND owner = ::GetWindow(reinterpret_cast<HWND>(wd), GW_OWNER);
			if(owner)
			{
				::RECT owner_rect;
				::GetWindowRect(owner, &owner_rect);
				::POINT pos = {owner_rect.left, owner_rect.top};
				::ScreenToClient(owner, &pos);
				p.x += (owner_rect.left - pos.x);
				p.y += (owner_rect.top - pos.y);
			}

			
			if (::GetWindowThreadProcessId(reinterpret_cast<HWND>(wd), 0) != ::GetCurrentThreadId())
			{
				nana::internal_revert_guard irg;
				::MoveWindow(reinterpret_cast<HWND>(wd), p.x, p.y, r.right - r.left, r.bottom - r.top, true);
			}
			else
				::MoveWindow(reinterpret_cast<HWND>(wd), p.x, p.y, r.right - r.left, r.bottom - r.top, true);

#elif defined(NANA_X11)
			Display * disp = restrict::spec.open_display();

			nana::detail::platform_scope_guard lock;

			if(point{x, y} == window_position(wd))
			{
				//Returns if the requested position is same with the current position.
				//In some X-Server versions/implementations, XMoveWindow() doesn't generate
				//a ConfigureNotify if the requested position is same with the current position.
				//It causes that x11_wait_for always waiting for the ConfigureNotify.
				return;
			}

			XWindowAttributes attr;
			::XGetWindowAttributes(disp, reinterpret_cast<Window>(wd), &attr);
			if(attr.map_state == IsUnmapped)
				exposed_positions[reinterpret_cast<Window>(wd)] = ::nana::point{x, y};
			else
			{
				//Removes the record of position. If move_window() is called during mapping the window,
				//the existing record will mistakenly move the window to the old position after x11_apply_exposed_position.
				exposed_positions.erase(reinterpret_cast<Window>(wd));
			}

			auto const owner = restrict::spec.get_owner(wd);
			if(owner && (owner != reinterpret_cast<native_window_type>(restrict::spec.root_window())))
			{
				int origin_x, origin_y;
				Window child_useless_for_API;
				::XTranslateCoordinates(disp, reinterpret_cast<Window>(owner), restrict::spec.root_window(), 0, 0, &origin_x, &origin_y, &child_useless_for_API);
				x += origin_x;
				y += origin_y;
			}

			auto misc = bedrock::instance().wd_manager().root_runtime(reinterpret_cast<native_window_type>(wd));
			std::size_t cmp_value = misc->x11msg.config;

			::XMoveWindow(disp, reinterpret_cast<Window>(wd), x, y);

			//Wait for the configuration notify to update the local attribute of position so that
			//the followed window_position() call can return the updated position value.

			x11_wait_for(reinterpret_cast<Window>(wd), x11_wait::configure, cmp_value);
#endif
		}

		bool native_interface::move_window(native_window_type wd, const rectangle& r)
		{
			/// \todo: use platform_abstraction::dpi_scale(r, dpi) instead, to include X11 ? 
#if defined(NANA_WINDOWS)
			
			if constexpr (dpi_debugging) std::cout << "   ---  move_window(rectangle):\n";
			auto scaled_r = scale_to_dpi(wd, r);

			HWND owner = ::GetWindow(reinterpret_cast<HWND>(wd), GW_OWNER);
			if(owner)
			{
				::RECT owner_rect;
				::GetWindowRect(owner, &owner_rect);
				::POINT pos = {owner_rect.left, owner_rect.top};
				::ScreenToClient(owner, &pos);
				scaled_r.x += (owner_rect.left - pos.x);
				scaled_r.y += (owner_rect.top - pos.y);
			}

			RECT client, wd_area;
			::GetClientRect(reinterpret_cast<HWND>(wd), &client);
			::GetWindowRect(reinterpret_cast<HWND>(wd), &wd_area);
			unsigned ext_w = (wd_area.right - wd_area.left) - client.right;
			unsigned ext_h = (wd_area.bottom - wd_area.top) - client.bottom;
			
			if (::GetWindowThreadProcessId(reinterpret_cast<HWND>(wd), 0) != ::GetCurrentThreadId())
			{
				nana::internal_revert_guard irg;
				return (FALSE != ::MoveWindow(reinterpret_cast<HWND>(wd), scaled_r.x, scaled_r.y, 
											  scaled_r.width + ext_w, scaled_r.height + ext_h, true));
			}
			
			return (FALSE != ::MoveWindow(reinterpret_cast<HWND>(wd), scaled_r.x, scaled_r.y, 
										  scaled_r.width + ext_w, scaled_r.height + ext_h, true));
#elif defined(NANA_X11)
			Display * disp = restrict::spec.open_display();
			long supplied;
			XSizeHints hints;
			nana::detail::platform_scope_guard psg;

			//Returns if the requested rectangle is same with the current rectangle.
			//In some X-Server versions/implementations, XMapWindow() doesn't generate
			//a ConfigureNotify if the requested rectangle is same with the current rectangle.
			//It causes that x11_wait_for always waiting for the ConfigureNotify.
			rectangle current_r;
			get_window_rect(wd, current_r);
			if(r == current_r)
				return true;

			::XGetWMNormalHints(disp, reinterpret_cast<Window>(wd), &hints, &supplied);
			if((hints.flags & (PMinSize | PMaxSize)) && (hints.min_width == hints.max_width) && (hints.min_height == hints.max_height))
			{
				hints.flags = PMinSize | PMaxSize;
				hints.min_width = hints.max_width = r.width;
				hints.min_height = hints.max_height = r.height;
			}
			else
				hints.flags = 0;

			XWindowAttributes attr;
			::XGetWindowAttributes(disp, reinterpret_cast<Window>(wd), &attr);
			if(attr.map_state == IsUnmapped)
			{
				hints.flags |= USSize;
				hints.width = r.width;
				hints.height = r.height;

				exposed_positions[reinterpret_cast<Window>(wd)] = r.position();
			}
			else
			{
				//Removes the record of position. If move_window() is called during mapping the window,
				//the existing record will mistakenly move the window to the old position after x11_apply_exposed_position.
				exposed_positions.erase(reinterpret_cast<Window>(wd));
			}

			if(hints.flags)
				::XSetWMNormalHints(disp, reinterpret_cast<Window>(wd), &hints);

			int x = r.x;
			int y = r.y;

			auto const owner = restrict::spec.get_owner(wd);
			if(owner && (owner != reinterpret_cast<native_window_type>(restrict::spec.root_window())))
			{
				int origin_x, origin_y;
				Window child_useless_for_API;
				::XTranslateCoordinates(disp, reinterpret_cast<Window>(owner), restrict::spec.root_window(), 0, 0, &origin_x, &origin_y, &child_useless_for_API);
				x += origin_x;
				y += origin_y;
			}

			auto misc = bedrock::instance().wd_manager().root_runtime(reinterpret_cast<native_window_type>(wd));
			std::size_t cmp_value = misc->x11msg.config;

			::XMoveResizeWindow(disp, reinterpret_cast<Window>(wd), x, y, r.width, r.height);

			//Wait for the configuration notify to update the local attribute of position so that
			//the followed window_position() call can return the updated position value.

			//It seems that XMoveResizeWindow doesn't need x11_wait_for. But x11_wait_for is still called
			//to make sure the local attribute is updated.
			x11_wait_for(reinterpret_cast<Window>(wd), x11_wait::configure, cmp_value);

			return true;
#endif
		}

		void native_interface::bring_top(native_window_type wd, bool activated)
		{
#if defined(NANA_WINDOWS)
			HWND native_wd = reinterpret_cast<HWND>(wd);

			if (FALSE == ::IsWindow(native_wd))
				return;

			HWND fg_wd = ::GetForegroundWindow();
			DWORD fg_tid = ::GetWindowThreadProcessId(fg_wd, nullptr);
			::AttachThreadInput(::GetCurrentThreadId(), fg_tid, TRUE);
			::ShowWindow(native_wd, activated ? SW_SHOWNORMAL : SW_SHOWNOACTIVATE);
			::SetWindowPos(native_wd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
			::SetWindowPos(native_wd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
			::AttachThreadInput(::GetCurrentThreadId(), fg_tid, FALSE);
#else
			static_cast<void>(activated); //eliminate unused parameter compiler warning.
			set_window_z_order(wd, nullptr, z_order_action::top);
#endif
		}

		void native_interface::set_window_z_order(native_window_type wd, native_window_type wd_after, z_order_action action_if_no_wd_after)
		{
#if defined(NANA_WINDOWS)
			HWND wa = reinterpret_cast<HWND>(wd_after);
			if(wa == 0)
			{
				switch(action_if_no_wd_after)
				{
				case z_order_action::bottom : wa = HWND_BOTTOM;	    break;
				case z_order_action::top:     wa = HWND_TOP;		break;
				case z_order_action::topmost: wa = HWND_TOPMOST;	break;
				case z_order_action::foreground:
					::SetForegroundWindow(reinterpret_cast<HWND>(wd));
					return;
				default:    				  wa = HWND_NOTOPMOST;
				}
			}
			if(::GetCurrentThreadId() != ::GetWindowThreadProcessId(reinterpret_cast<HWND>(wd), 0))
				::PostMessage(reinterpret_cast<HWND>(wd), nana::detail::messages::remote_thread_set_window_pos, reinterpret_cast<WPARAM>(wa), SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
			else
				::SetWindowPos(reinterpret_cast<HWND>(wd), wa, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
#elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;
			Display * disp = restrict::spec.open_display();
			if(0 == wd_after)
			{
				switch(action_if_no_wd_after)
				{
				case z_order_action::bottom:
					::XLowerWindow(disp, reinterpret_cast<Window>(wd));
					break;
				case z_order_action::foreground:
				case z_order_action::top:
				case z_order_action::topmost:
					::XRaiseWindow(disp, reinterpret_cast<Window>(wd));
					break;
				default:	//z_order_action::none
					break;
				}
			}
			else
			{
				//If the handle wd is a top level, XConfigureWindow() will be failed with a BadMatch.
				//The fix is to use XReconfigureWMWindow() instead.

				XWindowChanges values;
				values.sibling = reinterpret_cast<Window>(wd_after);
				values.stack_mode = Below;
				::XReconfigureWMWindow(disp, reinterpret_cast<Window>(wd), ::XDefaultScreen(disp), CWSibling | CWStackMode, &values);
			}
#endif
		}

		native_interface::frame_extents native_interface::window_frame_extents(native_window_type wd)
		{
			frame_extents fm_extents{0, 0, 0, 0};
			/// \todo: use platform_abstraction::unscale_dpi(fm, dpi) instead, to include X11 ? 

	#if defined(NANA_WINDOWS)
			if constexpr (dpi_debugging)
				std::cout << "   ---  window_frame_extents():\n";
			int dpi = static_cast<int>(native_interface::window_dpi(wd));

			::RECT client;
			::GetClientRect(reinterpret_cast<HWND>(wd), &client);
			//The right and bottom of client by GetClientRect indicate the width and height of the area
			::RECT wd_area;
			::GetWindowRect(reinterpret_cast<HWND>(wd), &wd_area);

			client  = unscale_dpi(client, dpi);  // first unscale to disminish rounding errors on small values
			wd_area = unscale_dpi(wd_area, dpi);

			fm_extents.left  = client.left - wd_area.left;
			fm_extents.right = wd_area.right - client.right;
			fm_extents.top   = client.top - wd_area.top;
			fm_extents.bottom = wd_area.bottom - client.bottom;
	#elif defined(NANA_X11)
			Atom type;
			int format;
			unsigned long len, bytes_left = 0;
			unsigned char *data;

			nana::detail::platform_scope_guard lock;
			if(Success == ::XGetWindowProperty(restrict::spec.open_display(), reinterpret_cast<Window>(wd), 
									restrict::spec.atombase().net_frame_extents, 0, 16, 0,
									XA_CARDINAL, &type, &format,
									&len, &bytes_left, &data))
			{
				if(type != None && len == 4)
				{
					fm_extents.left = ((std::int32_t*)data)[0];
					fm_extents.right = ((std::int32_t*)data)[1];
					fm_extents.top = ((std::int32_t*)data)[2];
					fm_extents.bottom = ((std::int32_t*)data)[3];
				}
				::XFree(data);
			}
	#endif

			return fm_extents;
		}

		bool native_interface::window_size(native_window_type wd, const size& sz)       ///< change to new_size if possible
		{
			/// \todo: use platform_abstraction::dpi_scale(sz, dpi) instead, to include X11 ? 
#if defined(NANA_WINDOWS)
			if constexpr (dpi_debugging) std::cout << "   ---  window_size(sz):\n";
			auto p = scale_to_dpi(wd, static_cast<int>(sz.width), static_cast<int>(sz.height));

			::RECT r;
			::GetWindowRect(reinterpret_cast<HWND>(wd), &r);    // original position

			HWND owner  = ::GetWindow(reinterpret_cast<HWND>(wd), GW_OWNER);
			HWND parent = ::GetParent(reinterpret_cast<HWND>(wd));
			if(parent && (parent != owner))  // ?
			{
				::POINT pos = {r.left, r.top};                      // original system? position
				::ScreenToClient(parent, &pos);
				r.left = pos.x;                                     // now, relative to parent
				r.top  = pos.y;
			}

			// move to the original position but with the new size
			if (::GetWindowThreadProcessId(reinterpret_cast<HWND>(wd), 0) != ::GetCurrentThreadId())
			{
				nana::internal_revert_guard irg;
				return (FALSE != ::MoveWindow(reinterpret_cast<HWND>(wd), r.left, r.top, p.x, p.y, true));
			}

			return     (FALSE != ::MoveWindow(reinterpret_cast<HWND>(wd), r.left, r.top, p.x, p.y, true));
#elif defined(NANA_X11)
			auto disp = restrict::spec.open_display();
			nana::detail::platform_scope_guard psg;

			//Returns if the requested size is same with the current size.
			//In some X-Server versions/implementations, XMapWindow() doesn't generate
			//a ConfigureNotify if the requested size is same with the current size.
			//It causes that x11_wait_for always waiting for the ConfigureNotify.
			rectangle current_r;
			get_window_rect(wd, current_r);
			if(current_r.dimension() == sz)
				return true;

			//Check the XSizeHints for testing whether the window is sizable.
			XSizeHints hints;
			long supplied;
			::XGetWMNormalHints(disp, reinterpret_cast<Window>(wd), &hints, &supplied);
			if((hints.flags & (PMinSize | PMaxSize)) && (hints.min_width == hints.max_width) && (hints.min_height == hints.max_height))
			{
				hints.flags = PMinSize | PMaxSize;
				hints.min_width = hints.max_width = sz.width;
				hints.min_height = hints.max_height = sz.height;
				::XSetWMNormalHints(disp, reinterpret_cast<Window>(wd), &hints);
			}

			auto misc = bedrock::instance().wd_manager().root_runtime(reinterpret_cast<native_window_type>(wd));
			std::size_t cmp_value = misc->x11msg.config;

			::XResizeWindow(disp, reinterpret_cast<Window>(wd), sz.width, sz.height);
			
			//It seems that XResizeWindow doesn't need x11_wait_for. But x11_wait_for is still called
			//to make sure the local attribute is updated.
			x11_wait_for(reinterpret_cast<Window>(wd), x11_wait::configure, cmp_value);
			return true;
#endif
		}

		void native_interface::get_window_rect(native_window_type wd, rectangle& r) ///< unused ?
		{
			/// \todo: use platform_abstraction::unscale_dpi(r, dpi) instead, to include X11 ? 
#if defined(NANA_WINDOWS)
			if constexpr (dpi_debugging)
				std::wcout << "   ---  get_window_rect() " << window_caption(wd) << ":\n";
			::RECT winr;
			::GetWindowRect(reinterpret_cast<HWND>(wd), &winr);
			winr = unscale_dpi(winr, static_cast<int>(native_interface::window_dpi(wd)));

			r.x = winr.left;
			r.y = winr.top;
			r.width = winr.right - winr.left;
			r.height = winr.bottom - winr.top;
#elif defined(NANA_X11)
			Window root;
			int x, y;
			unsigned border, depth;
			nana::detail::platform_scope_guard psg;
			::XGetGeometry(restrict::spec.open_display(), reinterpret_cast<Window>(wd), &root, &x, &y, &r.width, &r.height, &border, &depth);

			auto pos = window_position(wd);
			r.position(pos);
#endif
		}

		void native_interface::window_caption(native_window_type wd, const native_string_type& title)
		{
#if defined(NANA_WINDOWS)
			if(::GetCurrentThreadId() != ::GetWindowThreadProcessId(reinterpret_cast<HWND>(wd), 0))
			{
				wchar_t * wstr = new wchar_t[title.length() + 1];
				std::wcscpy(wstr, title.c_str());   
				::PostMessage(reinterpret_cast<HWND>(wd), nana::detail::messages::remote_thread_set_window_text, reinterpret_cast<WPARAM>(wstr), 0);
			}
			else
				::SetWindowText(reinterpret_cast<HWND>(wd), title.c_str());
#elif defined(NANA_X11)
			::XTextProperty name;
			char * text = const_cast<char*>(title.c_str());

			nana::detail::platform_scope_guard psg;
			::XStringListToTextProperty(&text, 1, &name);
			::XSetWMName(restrict::spec.open_display(), reinterpret_cast<Window>(wd), &name);
			::XChangeProperty(restrict::spec.open_display(), reinterpret_cast<Window>(wd),
					restrict::spec.atombase().net_wm_name, restrict::spec.atombase().utf8_string, 8,
					PropModeReplace, reinterpret_cast<unsigned char*>(text), title.size());
#endif
		}

		auto native_interface::window_caption(native_window_type wd) -> native_string_type
		{
			native_string_type str;

#if defined(NANA_WINDOWS)
			auto& lock = platform_abstraction::internal_mutex();
			bool is_current_thread = (::GetCurrentThreadId() == ::GetWindowThreadProcessId(reinterpret_cast<HWND>(wd), nullptr));

			if (!is_current_thread)
				lock.revert();

			int length = ::GetWindowTextLength(reinterpret_cast<HWND>(wd));
			if(length > 0)
			{
                //One for NULL terminator which GetWindowText will write.
				str.resize(length+1);

				::GetWindowText(reinterpret_cast<HWND>(wd), &(str[0]), static_cast<int>(str.size()));

				//"Remove" the null terminator writtien by GetWindowText
				str.resize(length);
			}

			if (!is_current_thread)
				lock.forward();

#elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;
			::XTextProperty txtpro;
			if(::XGetWMName(restrict::spec.open_display(), reinterpret_cast<Window>(wd), &txtpro))
			{
				char ** strlist;
				int size;
				if(::XTextPropertyToStringList(&txtpro, &strlist, &size))
				{
					if(size > 1)
					{
						str = *strlist;
						::XFreeStringList(strlist);
					}
				}
			}
#endif
			return str;
		}

		void native_interface::capture_window(native_window_type wd, bool cap)
		{
#if defined(NANA_WINDOWS)
			if(cap)
				::SetCapture(reinterpret_cast<HWND>(wd));
			else
				::ReleaseCapture();
#elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;
			if(cap)
			{
				const unsigned mask = ButtonPressMask|ButtonReleaseMask|PointerMotionMask|EnterWindowMask|LeaveWindowMask;
				if(GrabNotViewable == ::XGrabPointer(restrict::spec.open_display(), reinterpret_cast<Window>(wd), false, mask, GrabModeAsync, GrabModeAsync, None, None, CurrentTime))
				{
					restrict::spec.grab(reinterpret_cast<Window>(wd));
				}
			}
			else
			{
				::XUngrabPointer(restrict::spec.open_display(), CurrentTime);
				::XFlush(restrict::spec.open_display());
				restrict::spec.grab(0);
			}
#endif
		}

		nana::point native_interface::cursor_screen_position()
		{
			return platform_abstraction::unscale_dpi(cursor_sytem_position(), system_dpi());
		}

		nana::point native_interface::cursor_sytem_position()
		{
#if defined(NANA_WINDOWS)
			// return system point, unscaled
			//if constexpr (dpi_debugging) std::wcout << "   ---  cursor_position():\n";
			POINT point;
			::GetCursorPos(&point);
			return nana::point(point.x, point.y);
#elif defined(NANA_X11)
			nana::point pos;
			Window drop_wd;
			int x, y;
			unsigned mask;
			nana::detail::platform_scope_guard lock;
			::XQueryPointer(restrict::spec.open_display(), restrict::spec.root_window(), &drop_wd, &drop_wd,  &pos.x, &pos.y, &x, &y, &mask);
			return pos;
#endif
		}

		native_window_type native_interface::get_window(native_window_type wd, window_relationship rsp)
		{
#ifdef NANA_WINDOWS
			if(window_relationship::either_po == rsp)
				return reinterpret_cast<native_window_type>(::GetParent(reinterpret_cast<HWND>(wd)));
			else if(window_relationship::parent == rsp)
				return reinterpret_cast<native_window_type>(::GetAncestor(reinterpret_cast<HWND>(wd), GA_PARENT));
			else if(window_relationship::owner == rsp)
				return reinterpret_cast<native_window_type>(::GetWindow(reinterpret_cast<HWND>(wd), GW_OWNER));
#elif defined(NANA_X11)
			platform_scope_guard lock;

			auto owner = restrict::spec.get_owner(wd);

			if(window_relationship::either_po == rsp)
			{
				if(owner)
					return owner;

				return x11_parent_window(wd);
			}
			else if(window_relationship::owner == rsp)
				return owner;
			else if(window_relationship::parent == rsp)
				return x11_parent_window(wd);
#endif
			return nullptr;
		}

		native_window_type native_interface::parent_window(native_window_type child, native_window_type new_parent, bool returns_previous)
		{
#ifdef NANA_WINDOWS
			auto prev = ::SetParent(reinterpret_cast<HWND>(child), reinterpret_cast<HWND>(new_parent));

			if (prev)
				::PostMessage(prev, /*WM_CHANGEUISTATE*/0x0127, /*UIS_INITIALIZE*/ 3, 0);

			::SetWindowPos(reinterpret_cast<HWND>(child), NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);

			return reinterpret_cast<native_window_type>(returns_previous ? prev : nullptr);
#elif defined(NANA_X11)
			native_window_type prev = nullptr;

			platform_scope_guard lock;

			if(returns_previous)
				prev = get_window(child, window_relationship::either_po);

			if(native_window_type{} == new_parent)
				new_parent = reinterpret_cast<native_window_type>(restrict::spec.root_window());

			::XReparentWindow(restrict::spec.open_display(),
				reinterpret_cast<Window>(child), reinterpret_cast<Window>(new_parent),
				0, 0);


			// If umake_owner returns true, it indicates the child windows is a popup window.
			// So make the ownership of new_parent and child.
			if(restrict::spec.umake_owner(child))
				restrict::spec.make_owner(new_parent, child);

			return prev;
#endif
		}

		void native_interface::caret_create(native_window_type wd, const ::nana::size& caret_sz)
		{
#if defined(NANA_WINDOWS)
			if constexpr 
				(dpi_debugging) std::wcout << "   ---  caret_create() " << window_caption(wd) << ":\n";

			auto p = scale_to_dpi(wd, static_cast<int>(caret_sz.width), 
								                static_cast<int>(caret_sz.height));
			
			::CreateCaret(reinterpret_cast<HWND>(wd), 0, p.x, p.y);

#elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;
			restrict::spec.caret_open(wd, caret_sz);
#endif
		}

		void native_interface::caret_destroy(native_window_type wd)
		{
#if defined(NANA_WINDOWS)
			if(::GetCurrentThreadId() != ::GetWindowThreadProcessId(reinterpret_cast<HWND>(wd), 0))
				::PostMessage(reinterpret_cast<HWND>(wd), nana::detail::messages::operate_caret, 1, 0);
			else
				::DestroyCaret();
#elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;
			restrict::spec.caret_close(wd);
#endif
		}

		void native_interface::caret_pos(native_window_type wd, const point& pos_ori)
		{
#if defined(NANA_WINDOWS)
			if constexpr (dpi_debugging) std::wcout << "   ---  caret_pos() " << window_caption(wd) << ":\n";
			auto pos = scale_to_dpi(wd, pos_ori.x, pos_ori.y);

			if(::GetCurrentThreadId() != ::GetWindowThreadProcessId(reinterpret_cast<HWND>(wd), 0))
			{
				auto cp = new nana::detail::messages::caret;
				cp->x = pos.x;
				cp->y = pos.y;
				::PostMessage(reinterpret_cast<HWND>(wd), nana::detail::messages::operate_caret, 2, reinterpret_cast<LPARAM>(cp));
			}
			else
				::SetCaretPos(pos.x, pos.y);
#elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;
			restrict::spec.caret_pos(wd, pos);
#endif
		}

		void native_interface::caret_visible(native_window_type wd, bool vis)
		{
#if defined(NANA_WINDOWS)
			(vis ? ::ShowCaret : ::HideCaret)(reinterpret_cast<HWND>(wd));
#elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;
			restrict::spec.caret_visible(wd, vis);
#endif
		}

		void native_interface::set_focus(native_window_type wd)
		{
#if defined(NANA_WINDOWS)
			if(wd && (::GetFocus() != reinterpret_cast<HWND>(wd)))
			{
				if(::GetCurrentThreadId() != ::GetWindowThreadProcessId(reinterpret_cast<HWND>(wd), nullptr))
					::PostMessage(reinterpret_cast<HWND>(wd), nana::detail::messages::async_set_focus, 0, 0);
				else
				{
					internal_revert_guard revert;
					::SetFocus(reinterpret_cast<HWND>(wd));
				}
			}
#elif defined(NANA_X11)
			nana::detail::platform_scope_guard lock;
			XWindowAttributes attr;
			::XGetWindowAttributes(restrict::spec.open_display(), reinterpret_cast<Window>(wd), &attr);
			//Make sure the window is mapped before setting focus.
			if(IsViewable == attr.map_state)
			{
				//X has a very weird focus controlling. It generates a FocusOut event before FocusIn when XSetInputFocus,
				//The FocusOut should be ignored in this situation, for precisely focus controlling.

				restrict::spec.add_ignore_once(wd, FocusOut);
				::XSetInputFocus(restrict::spec.open_display(), reinterpret_cast<Window>(wd), RevertToParent, CurrentTime);
			}
#endif
		}

		native_window_type native_interface::get_focus_window()
		{
#if defined(NANA_WINDOWS)
			return reinterpret_cast<native_window_type>(::GetFocus());
#elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;
			Window wd;
			int revert;
			::XGetInputFocus(restrict::spec.open_display(), &wd, &revert);
			return reinterpret_cast<native_window_type>(wd);
#endif
		}

		bool native_interface::calc_screen_point(native_window_type wd, nana::point& pos)
		{
#if defined(NANA_WINDOWS)
			if constexpr (dpi_debugging) 
				std::wcout << "   ---  calc_screen_point() " << window_caption(wd) << ":\n";
			pos = scale_to_dpi(wd, pos.x, pos.y);
			POINT point = {pos.x, pos.y};
			if(::ClientToScreen(reinterpret_cast<HWND>(wd), &point))
			{
				pos.x = point.x;
				pos.y = point.y;
				return true;
			}
#elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;
			int x = pos.x, y = pos.y;
			Window child;
			if(True == ::XTranslateCoordinates(restrict::spec.open_display(),
													reinterpret_cast<Window>(wd), restrict::spec.root_window(), x, y, &pos.x, &pos.y, &child))
			{
				return true;
			}
#endif
			return false;
		}
		bool native_interface::calc_window_point(native_window_type wd, nana::point& pos)
		{
			platform_abstraction::dpi_transform(pos, system_dpi());
			if (!transform_screen_system_point_into_window_sytem_point(wd, pos)) return false;
			platform_abstraction::untransform_dpi(pos, window_dpi(wd));
			return true;
		}
		nana::point	native_interface::cursor_window_position(native_window_type wd)
        {
            auto pos = cursor_sytem_position();
            transform_screen_system_point_into_window_sytem_point(wd, pos);
            return platform_abstraction::untransform_dpi(pos, window_dpi(wd));
        }

		bool native_interface::transform_screen_system_point_into_window_sytem_point(native_window_type wd, nana::point& pos)
		{
#if defined(NANA_WINDOWS)
			if constexpr (dpi_debugging) 
				std::wcout << "   ---  transform_screen_system_point_into_window_sytem_point() " << window_caption(wd) << ":\n";

			POINT point = {pos.x, pos.y};
			if(::ScreenToClient(reinterpret_cast<HWND>(wd), &point))
			{
				//pos = unscale_dpi(wd, pos.x, pos.y); // work unscaled? wd_manager().find_window(native_handle, pos);
				pos.x = point.x;
				pos.y = point.y;
				return true;
			}
#elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;
			int x = pos.x, y = pos.y;
			Window child;
			if(True == ::XTranslateCoordinates(restrict::spec.open_display(), restrict::spec.root_window(), reinterpret_cast<Window>(wd), x, y, &pos.x, &pos.y, &child))
			{
				return true;
			}
#endif
			return false;
		}

		native_window_type native_interface::find_window_from_system_screen_point(const nana::point& p)
		{
#if defined(NANA_WINDOWS)
			POINT pos = {p.x, p.y};
			return reinterpret_cast<native_window_type>(::WindowFromPoint(pos));
#elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;

			Window root = restrict::spec.root_window();
			Window wd = root;
			Window child = 0;
			int dropx = 0, dropy = 0;
			while(True == ::XTranslateCoordinates(restrict::spec.open_display(), root, wd, p.x, p.y, &dropx, &dropy, &child))
			{
				if(0 == child) break;
				wd = child;
			}
			return reinterpret_cast<native_window_type>(wd);
#endif
		}
		native_window_type native_interface::find_cursor_window(nana::point& point)
        {
            auto pos = cursor_sytem_position();
			auto wd = find_window_from_system_screen_point(pos);
			if (wd)
				calc_window_point(wd, point);
			platform_abstraction::untransform_dpi(point, window_dpi(wd));
            return wd;
        }

		nana::size native_interface::check_track_size(nana::size sz, unsigned ext_width, unsigned ext_height, bool true_for_max)
		{
#if defined(NANA_WINDOWS)
			int x;
			int y;
			if(true_for_max)
			{
				/// \todo: add to dpi_function GetSystemMetricsForDpi and replace this
				x = ::GetSystemMetrics(SM_CXMAXTRACK);
				y = ::GetSystemMetrics(SM_CYMAXTRACK);
				if(static_cast<unsigned>(x) < sz.width + ext_width)
					sz.width = static_cast<unsigned>(x);
				if(static_cast<unsigned>(y) < sz.height + ext_height)
					sz.height = static_cast<unsigned>(y);
			}
			else
			{
				/// \todo: add to dpi_function GetSystemMetricsForDpi and replace this
				x = ::GetSystemMetrics(SM_CXMINTRACK);
				y = ::GetSystemMetrics(SM_CYMINTRACK);
				if(static_cast<unsigned>(x) > sz.width + ext_width)
					sz.width = static_cast<unsigned>(x);
				if(static_cast<unsigned>(y) > sz.height + ext_height)
					sz.height = static_cast<unsigned>(y);
			}
#else
			//eliminate unused parameter compiler warning.
			static_cast<void>(ext_width);
			static_cast<void>(ext_height);
			static_cast<void>(true_for_max);
#endif
			return sz;
		}

		void native_interface::start_dpi_awareness(bool aware)  //bool aware = false
		{
			if (!aware) return;
         #ifdef NANA_WINDOWS
			auto& dpi_fn = wdpi_fns();
			// set SetProcessDpiAwarenessContext, or SetProcessDpiAwareness, or SetProcessDPIAware
			if (dpi_fn.SetProcessDpiAwarenessContext)
			{
				dpi_fn.SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2_);
			}
			else if (dpi_fn.SetProcessDpiAwareness)
            {
			    dpi_fn.SetProcessDpiAwareness(dpi_function::PROCESS_PER_MONITOR_DPI_AWARE);
			}
            else if (dpi_fn.SetProcessDPIAware)
            {
			    dpi_fn.SetProcessDPIAware();
			}
			if constexpr (dpi_debugging) {
				std::cout << "start_dpi_awareness(): system_dpi = " << system_dpi() << '\n';
				print_monitor_dpi();
			}
         #endif
		}

		int native_interface::window_dpi(native_window_type wd)  /// \todo: add bool x_requested = true)
		{
#ifdef NANA_WINDOWS
			
			HWND hwnd = reinterpret_cast<HWND>(wd);

			if (!::IsWindow(hwnd))
				return system_dpi();

			if (wdpi_fns().GetDpiForWindow)  // how to get x_dpi or y_dpi?
				return static_cast<int>(wdpi_fns().GetDpiForWindow(hwnd));
						
			if (wdpi_fns().GetDpiForMonitor)
			{
				HMONITOR pmonitor = ::MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);
				UINT x_dpi, y_dpi;
				if (S_OK == wdpi_fns().GetDpiForMonitor(pmonitor, dpi_function::MDT_EFFECTIVE_DPI, &x_dpi, &y_dpi))
					return  static_cast<int>(x_dpi);  // x_requested ? x_dpi, y_dpi
			}

			HDC hdc = ::GetDC(hwnd);  // the old way. Works in any Windows version
			if (hdc)
			{
				auto dpi = ::GetDeviceCaps(hdc, LOGPIXELSX); // x_requested ? LOGPIXELSX : LOGPIXELSY
				::ReleaseDC(nullptr, hdc);
				return dpi;
			}

#endif
			static_cast<void>(wd);	//eliminate the unused warning
			return system_dpi();
		}

		int native_interface::system_dpi()
		{
  #ifdef NANA_WINDOWS

			if (wdpi_fns().GetDpiForMonitor)
			{
				if constexpr (dpi_debugging) std::cout << "GetDpiForMonitor" << std::endl;
				// get the main monitor HWND
				HWND primary_monitor = ::GetDesktopWindow();
				HMONITOR pmonitor = ::MonitorFromWindow(primary_monitor, MONITOR_DEFAULTTOPRIMARY);
				UINT x_dpi, y_dpi;
				if (S_OK == wdpi_fns().GetDpiForMonitor(pmonitor, dpi_function::MDT_EFFECTIVE_DPI, &x_dpi, &y_dpi))
					return  static_cast<int>(x_dpi);  //  x_dpi != y_dpi ??
			}
			if (wdpi_fns().GetDpiForSystem)  
			{
				if constexpr (dpi_debugging) std::cout << "GetDpiForSystem" << std::endl;
				return static_cast<int>(wdpi_fns().GetDpiForSystem());
			}

			if constexpr (dpi_debugging) std::cout << "GetDeviceCaps" << std::endl;

			//When DPI-aware APIs are not supported by the running Windows, it returns the system DPI
			auto hdc = ::GetDC(nullptr);
			auto dpi = ::GetDeviceCaps(hdc, LOGPIXELSX);
			::ReleaseDC(nullptr, hdc);
			return dpi;

#endif
			return 96;
		}
	//end struct native_interface
	}//end namespace detail
}//end namespace nana
