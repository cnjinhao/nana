/*
 *	Platform Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/detail/native_window_interface.cpp
 */

#include <nana/detail/platform_spec_selector.hpp>
#include <nana/gui/detail/native_window_interface.hpp>
#include <nana/gui/screen.hpp>
#if defined(NANA_WINDOWS)
	#if defined(STD_THREAD_NOT_SUPPORTED)
        #include <nana/std_mutex.hpp>
    #else
        #include <mutex>
	#endif
	#include <map>
	#include "../../paint/detail/image_ico.hpp"
#elif defined(NANA_X11)
	#include <nana/system/platform.hpp>
	#include <nana/gui/detail/bedrock.hpp>
	#include <nana/gui/detail/window_manager.hpp>
#endif

namespace nana{
	namespace paint
	{
		class image_accessor
		{
		public:
#if defined(NANA_WINDOWS)
			static HICON icon(const nana::paint::image& img)
			{
				auto ico = dynamic_cast<paint::detail::image_ico*>(img.image_ptr_.get());
				if(ico && ico->ptr())
						return *(ico->ptr());
				return nullptr;
			}
#endif
		};
	}

	namespace detail{

#if defined(NANA_WINDOWS)
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


	//This function is a proxy for ShowWindow/ShowWindowAsync
	//It determines which API should be called.
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
			::ShowWindowAsync(wd, cmd);
		else
			::ShowWindow(wd, cmd);
	}
#elif defined(NANA_X11)
	namespace restrict
	{
		nana::detail::platform_spec & spec = nana::detail::platform_spec::instance();
	}
#endif

	//struct native_interface
		nana::size native_interface::primary_monitor_size()
		{
#if defined(NANA_WINDOWS)
			return nana::size(::GetSystemMetrics(SM_CXSCREEN), ::GetSystemMetrics(SM_CYSCREEN));
#elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;
			Screen* s = ::XScreenOfDisplay(restrict::spec.open_display(), ::XDefaultScreen(restrict::spec.open_display()));
			return nana::size(::XWidthOfScreen(s), ::XHeightOfScreen(s));
#endif
		}

		rectangle native_interface::screen_area_from_point(const point& pos)
		{
#if defined(NANA_WINDOWS)
			typedef HMONITOR (__stdcall * MonitorFromPointT)(POINT,DWORD);

			MonitorFromPointT mfp = reinterpret_cast<MonitorFromPointT>(::GetProcAddress(::GetModuleHandleA("User32.DLL"), "MonitorFromPoint"));
			if(mfp)
			{
				POINT native_pos = {pos.x, pos.y};
				HMONITOR monitor = mfp(native_pos, 2 /*MONITOR_DEFAULTTONEAREST*/);

				MONITORINFO mi;
				mi.cbSize = sizeof mi;
				if(::GetMonitorInfo(monitor, &mi))
				{
					return rectangle(mi.rcWork.left, mi.rcWork.top,
									mi.rcWork.right - mi.rcWork.left, mi.rcWork.bottom - mi.rcWork.top);
				}
			}
#endif
			return rectangle{ primary_monitor_size() };
		}

		//platform-dependent
		native_interface::window_result native_interface::create_window(native_window_type owner, bool nested, const rectangle& r, const appearance& app)
		{
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

			POINT pt = {r.x, r.y};

			if(owner && (nested == false))
				::ClientToScreen(reinterpret_cast<HWND>(owner), &pt);

			HWND native_wd = ::CreateWindowEx(style_ex, L"NanaWindowInternal", L"Nana Window",
											style,
											pt.x, pt.y, 100, 100,
											reinterpret_cast<HWND>(owner), 0, ::GetModuleHandle(0), 0);

			//A window may have a border, this should be adjusted the client area fit for the specified size.
			::RECT client;
			::GetClientRect(native_wd, &client);	//The right and bottom of client by GetClientRect indicate the width and height of the area
			::RECT wd_area;
			::GetWindowRect(native_wd, &wd_area);

			//a dimension with borders and caption title
			wd_area.right -= wd_area.left;	//wd_area.right = width
			wd_area.bottom -= wd_area.top;	//wd_area.bottom = height
			if (nested)
			{
				wd_area.left = pt.x;
				wd_area.top = pt.y;
			}

			int delta_w = static_cast<int>(r.width) - client.right;
			int delta_h = static_cast<int>(r.height) - client.bottom;

			::MoveWindow(native_wd, wd_area.left, wd_area.top, wd_area.right + delta_w, wd_area.bottom + delta_h, true);

			::GetClientRect(native_wd, &client);
			::GetWindowRect(native_wd, &wd_area);

			wd_area.right -= wd_area.left;
			wd_area.bottom -= wd_area.top;

			window_result result = { reinterpret_cast<native_window_type>(native_wd),
										static_cast<unsigned>(client.right), static_cast<unsigned>(client.bottom),
										static_cast<unsigned>(wd_area.right - client.right), static_cast<unsigned>(wd_area.bottom - client.bottom)};
#elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;

			XSetWindowAttributes win_attr;
			unsigned long attr_mask = CWBackPixmap | CWBackPixel | CWBorderPixel |
							CWWinGravity | CWBitGravity | CWColormap | CWEventMask;

			Display * disp = restrict::spec.open_display();
			win_attr.colormap = restrict::spec.colormap();

			win_attr.background_pixmap = None;
			win_attr.background_pixel = 0xFFFFFF;
			win_attr.border_pixmap = None;
			win_attr.border_pixel = 0x0;
			win_attr.bit_gravity = 0;
			win_attr.win_gravity = NorthWestGravity;
			win_attr.backing_store = 0;
			win_attr.backing_planes = 0;
			win_attr.backing_pixel = 0;
			win_attr.colormap = restrict::spec.colormap();

			if(app.decoration == false)
			{
				win_attr.override_redirect = True;
				attr_mask |= CWOverrideRedirect;
			}

			Window parent = (owner ? reinterpret_cast<Window>(owner) : restrict::spec.root_window());
			nana::point pos(r.x, r.y);
			if((false == nested) && owner)
			{
				win_attr.save_under = True;
				attr_mask |= CWSaveUnder;
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
				if((!nested) && owner)
					restrict::spec.make_owner(owner, reinterpret_cast<native_window_type>(handle));

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

		native_window_type native_interface::create_child_window(native_window_type parent, const rectangle& r)
		{
			if(nullptr == parent) return nullptr;
#if defined(NANA_WINDOWS)
			HWND handle = ::CreateWindowEx(WS_EX_CONTROLPARENT,		// Extended possibilites for variation
										L"NanaWindowInternal",
										L"Nana Child Window",	// Title Text
										WS_CHILD | WS_VISIBLE | WS_TABSTOP  | WS_CLIPSIBLINGS,
										r.x, r.y, r.width, r.height,
										reinterpret_cast<HWND>(parent),	// The window is a child-window to desktop
										0, ::GetModuleHandle(0), 0);
#elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;

			XSetWindowAttributes win_attr;
			unsigned long attr_mask = CWBackPixmap | CWBackPixel | CWBorderPixel |
							CWWinGravity | CWBitGravity | CWColormap | CWEventMask;

			Display * disp = restrict::spec.open_display();
			win_attr.colormap = restrict::spec.colormap();

			win_attr.background_pixmap = None;
			win_attr.background_pixel = 0xFFFFFF;
			win_attr.border_pixmap = None;
			win_attr.border_pixel = 0x0;
			win_attr.bit_gravity = 0;
			win_attr.win_gravity = NorthWestGravity;
			win_attr.backing_store = 0;
			win_attr.backing_planes = 0;
			win_attr.backing_pixel = 0;
			win_attr.colormap = restrict::spec.colormap();

			win_attr.override_redirect = True;
			attr_mask |= CWOverrideRedirect;

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
					::SendMessage(reinterpret_cast<HWND>(wd), WM_SETICON, ICON_SMALL, reinterpret_cast<WPARAM>(sml_handle));

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
			//Under X, XDestroyWindow destroys the specified window and generats a DestroyNotify
			//event, when the client receives the event, the specified window has been already
			//destroyed. This is a feature which is different from Windows. So the following
			//works should be handled before calling XDestroyWindow.
			auto & brock = bedrock::instance();
			if(wd == brock.get_menu())
			{
				brock.erase_menu(false);
				brock.delay_restore(3);	//Restores if delay_restore is not decleard
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
				if(show)
				{
					::XMapWindow(disp, reinterpret_cast<Window>(wd));
					Window grab = restrict::spec.grab(0);
					if(grab == reinterpret_cast<Window>(wd))
						capture_window(wd, true);
				}
				else
					::XUnmapWindow(disp, reinterpret_cast<Window>(wd));

				::XFlush(disp);
			}
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

		void native_interface::refresh_window(native_window_type wd)
		{
#if defined(NANA_WINDOWS)
			::InvalidateRect(reinterpret_cast<HWND>(wd), nullptr, true);
#elif defined(NANA_X11)
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
#if defined(NANA_WINDOWS)
			::RECT r;
			::GetWindowRect(reinterpret_cast<HWND>(wd), & r);
			HWND coord_wd = ::GetWindow(reinterpret_cast<HWND>(wd), GW_OWNER);

			if (!coord_wd)
				coord_wd = ::GetParent(reinterpret_cast<HWND>(wd));

			if (coord_wd)
			{
				::POINT pos = {r.left, r.top};
				::ScreenToClient(coord_wd, &pos);
				return nana::point(pos.x, pos.y);
			}
			return nana::point(r.left, r.top);
#elif defined(NANA_X11)
			int x, y;
			nana::detail::platform_scope_guard psg;
			Window coord_wd = reinterpret_cast<Window>(restrict::spec.get_owner(wd));
			if(!coord_wd)
			{
				coord_wd = reinterpret_cast<Window>(parent_window(wd));
				if(!coord_wd)
					coord_wd = restrict::spec.root_window();
			}
			Window child;
			if(True == ::XTranslateCoordinates(restrict::spec.open_display(), reinterpret_cast<Window>(wd), coord_wd, 0, 0, &x, &y, &child))
				return nana::point(x, y);
			return nana::point(0, 0);
#endif
		}

		void native_interface::move_window(native_window_type wd, int x, int y)
		{
#if defined(NANA_WINDOWS)
			if(::GetWindowThreadProcessId(reinterpret_cast<HWND>(wd), 0) != ::GetCurrentThreadId())
			{
				nana::detail::messages::move_window * mw = new nana::detail::messages::move_window;
				mw->x = x;
				mw->y = y;
				mw->ignore = mw->Size;
				::PostMessage(reinterpret_cast<HWND>(wd), nana::detail::messages::remote_thread_move_window, reinterpret_cast<WPARAM>(mw), 0);
			}
			else
			{
				::RECT r;
				::GetWindowRect(reinterpret_cast<HWND>(wd), &r);
				HWND owner = ::GetWindow(reinterpret_cast<HWND>(wd), GW_OWNER);
				if(owner)
				{
					::RECT owner_rect;
					::GetWindowRect(owner, &owner_rect);
					::POINT pos = {owner_rect.left, owner_rect.top};
					::ScreenToClient(owner, &pos);
					x += (owner_rect.left - pos.x);
					y += (owner_rect.top - pos.y);
				}
				::MoveWindow(reinterpret_cast<HWND>(wd), x, y, r.right - r.left, r.bottom - r.top, true);
			}
#elif defined(NANA_X11)
			Display * disp = restrict::spec.open_display();

			nana::detail::platform_scope_guard psg;
			Window owner = reinterpret_cast<Window>(restrict::spec.get_owner(wd));
			if(owner)
			{
				Window child;
				::XTranslateCoordinates(disp, owner, restrict::spec.root_window(),
										x, y, &x, &y, &child);
			}

			XWindowAttributes attr;
			::XGetWindowAttributes(disp, reinterpret_cast<Window>(wd), &attr);
			if(attr.map_state == IsUnmapped)
			{
				XSizeHints hints;
				hints.flags = USPosition;
				hints.x = x;
				hints.y = y;
				::XSetWMNormalHints(disp, reinterpret_cast<Window>(wd), &hints);
			}

			::XMoveWindow(disp, reinterpret_cast<Window>(wd), x, y);
#endif
		}

		void native_interface::move_window(native_window_type wd, const rectangle& r)
		{
#if defined(NANA_WINDOWS)
			if(::GetWindowThreadProcessId(reinterpret_cast<HWND>(wd), 0) != ::GetCurrentThreadId())
			{
				auto * mw = new nana::detail::messages::move_window;
				mw->x = r.x;
				mw->y = r.y;
				mw->width = r.width;
				mw->height = r.height;
				mw->ignore = 0;
				::PostMessage(reinterpret_cast<HWND>(wd), nana::detail::messages::remote_thread_move_window, reinterpret_cast<WPARAM>(mw), 0);
			}
			else
			{
				int x = r.x;
				int y = r.y;
				HWND owner = ::GetWindow(reinterpret_cast<HWND>(wd), GW_OWNER);
				if(owner)
				{
					::RECT owner_rect;
					::GetWindowRect(owner, &owner_rect);
					::POINT pos = {owner_rect.left, owner_rect.top};
					::ScreenToClient(owner, &pos);
					x += (owner_rect.left - pos.x);
					y += (owner_rect.top - pos.y);
				}

				RECT client, wd_area;
				::GetClientRect(reinterpret_cast<HWND>(wd), &client);
				::GetWindowRect(reinterpret_cast<HWND>(wd), &wd_area);
				unsigned ext_w = (wd_area.right - wd_area.left) - client.right;
				unsigned ext_h = (wd_area.bottom - wd_area.top) - client.bottom;
				::MoveWindow(reinterpret_cast<HWND>(wd), x, y, r.width + ext_w, r.height + ext_h, true);
			}
#elif defined(NANA_X11)
			Display * disp = restrict::spec.open_display();
			long supplied;
			XSizeHints hints;
			nana::detail::platform_scope_guard psg;

			::XGetWMNormalHints(disp, reinterpret_cast<Window>(wd), &hints, &supplied);
			if((hints.flags & (PMinSize | PMaxSize)) && (hints.min_width == hints.max_width) && (hints.min_height == hints.max_height))
			{
				hints.flags = PMinSize | PMaxSize;
				hints.min_width = hints.max_width = r.width;
				hints.min_height = hints.max_height = r.height;
			}
			else
				hints.flags = 0;

			Window owner = reinterpret_cast<Window>(restrict::spec.get_owner(wd));
			int x = r.x;
			int y = r.y;
			if(owner)
			{
				Window child;
				::XTranslateCoordinates(disp, owner, restrict::spec.root_window(),
										r.x, r.y, &x, &y, &child);
			}

			XWindowAttributes attr;
			::XGetWindowAttributes(disp, reinterpret_cast<Window>(wd), &attr);
			if(attr.map_state == IsUnmapped)
			{
				hints.flags |= (USPosition | USSize);
				hints.x = x;
				hints.y = y;
				hints.width = r.width;
				hints.height = r.height;
			}

			if(hints.flags)
				::XSetWMNormalHints(disp, reinterpret_cast<Window>(wd), &hints);

			::XMoveResizeWindow(disp, reinterpret_cast<Window>(wd), x, y, r.width, r.height);
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
				case z_order_action::bottom : wa = HWND_BOTTOM;	break;
				case z_order_action::top: wa = HWND_TOP;		break;
				case z_order_action::topmost: wa = HWND_TOPMOST;	break;
				case z_order_action::foreground:
					::SetForegroundWindow(reinterpret_cast<HWND>(wd));
					return;
				default:
					wa = HWND_NOTOPMOST;
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

		void native_interface::window_size(native_window_type wd, const size& sz)
		{
#if defined(NANA_WINDOWS)
			if(::GetWindowThreadProcessId(reinterpret_cast<HWND>(wd), 0) != ::GetCurrentThreadId())
			{
				auto * mw = new nana::detail::messages::move_window;
				mw->width = sz.width;
				mw->height = sz.height;
				mw->ignore = mw->Pos;
				::PostMessage(reinterpret_cast<HWND>(wd), nana::detail::messages::remote_thread_move_window, reinterpret_cast<WPARAM>(mw), 0);
			}
			else
			{
				::RECT r;
				::GetWindowRect(reinterpret_cast<HWND>(wd), &r);
				HWND owner = ::GetWindow(reinterpret_cast<HWND>(wd), GW_OWNER);
				HWND parent = ::GetParent(reinterpret_cast<HWND>(wd));
				if(parent && (parent != owner))
				{
					::POINT pos = {r.left, r.top};
					::ScreenToClient(parent, &pos);
					r.left = pos.x;
					r.top = pos.y;
				}
				::MoveWindow(reinterpret_cast<HWND>(wd), r.left, r.top, static_cast<int>(sz.width), static_cast<int>(sz.height), true);
			}
#elif defined(NANA_X11)
			auto disp = restrict::spec.open_display();
			nana::detail::platform_scope_guard psg;

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
			::XResizeWindow(disp, reinterpret_cast<Window>(wd), sz.width, sz.height);
#endif
		}

		void native_interface::get_window_rect(native_window_type wd, rectangle& r)
		{
#if defined(NANA_WINDOWS)
			::RECT winr;
			::GetWindowRect(reinterpret_cast<HWND>(wd), &winr);
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
#if defined(NANA_WINDOWS)
			int length = ::GetWindowTextLength(reinterpret_cast<HWND>(wd));
			if(length > 0)
			{
				native_string_type str;
                //One for NULL terminator which GetWindowText will write.
				str.resize(length+1);

				::GetWindowText(reinterpret_cast<HWND>(wd), &(str[0]), static_cast<int>(str.size()));

				//Remove the null terminator writtien by GetWindowText
				str.resize(length);

				return str;
			}
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
						std::string text = *strlist;
						::XFreeStringList(strlist);
						return text;
					}
				}
			}
#endif
			return native_string_type();
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

		nana::point native_interface::cursor_position()
		{
#if defined(NANA_WINDOWS)
			POINT point;
			::GetCursorPos(&point);
			return nana::point(point.x, point.y);
#elif defined(NANA_X11)
			nana::point pos;
			Window drop_wd;
			int x, y;
			unsigned mask;
			nana::detail::platform_scope_guard psg;
			::XQueryPointer(restrict::spec.open_display(), restrict::spec.root_window(), &drop_wd, &drop_wd,  &pos.x, &pos.y, &x, &y, &mask);
			return pos;
#endif
		}

		native_window_type native_interface::get_owner_window(native_window_type wd)
		{
#if defined(NANA_WINDOWS)
			return reinterpret_cast<native_window_type>(::GetWindow(reinterpret_cast<HWND>(wd), GW_OWNER));
#elif defined(NANA_X11)
			return restrict::spec.get_owner(wd);
#endif
		}

		native_window_type native_interface::parent_window(native_window_type wd)
		{
#ifdef NANA_WINDOWS
			return reinterpret_cast<native_window_type>(::GetParent(reinterpret_cast<HWND>(wd)));
#elif defined(NANA_X11)
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
#endif
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
				prev = parent_window(child);

			::XReparentWindow(restrict::spec.open_display(),
				reinterpret_cast<Window>(child), reinterpret_cast<Window>(new_parent),
				0, 0);
			return prev;
#endif
		}

		void native_interface::caret_create(native_window_type wd, const ::nana::size& caret_sz)
		{
#if defined(NANA_WINDOWS)
			::CreateCaret(reinterpret_cast<HWND>(wd), 0, int(caret_sz.width), int(caret_sz.height));
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

		void native_interface::caret_pos(native_window_type wd, const point& pos)
		{
#if defined(NANA_WINDOWS)
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
					::SetFocus(reinterpret_cast<HWND>(wd));
			}
#elif defined(NANA_X11)
			nana::detail::platform_scope_guard lock;
			XWindowAttributes attr;
			::XGetWindowAttributes(restrict::spec.open_display(), reinterpret_cast<Window>(wd), &attr);
			//Make sure the window is mapped before setting focus.
			if(IsViewable == attr.map_state)
				::XSetInputFocus(restrict::spec.open_display(), reinterpret_cast<Window>(wd), RevertToPointerRoot, CurrentTime);
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
			POINT point = {pos.x, pos.y};
			if(::ClientToScreen(reinterpret_cast<HWND>(wd), &point))
			{
				pos.x = point.x;
				pos.y = point.y;
				return true;
			}
			return false;
#elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;
			int x = pos.x, y = pos.y;
			Window child;
			return (True == ::XTranslateCoordinates(restrict::spec.open_display(),
													reinterpret_cast<Window>(wd), restrict::spec.root_window(), x, y, &pos.x, &pos.y, &child));
#endif
		}

		bool native_interface::calc_window_point(native_window_type wd, nana::point& pos)
		{
#if defined(NANA_WINDOWS)
			POINT point = {pos.x, pos.y};
			if(::ScreenToClient(reinterpret_cast<HWND>(wd), &point))
			{
				pos.x = point.x;
				pos.y = point.y;
				return true;
			}
			return false;
#elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;
			int x = pos.x, y = pos.y;
			Window child;
			return (True == ::XTranslateCoordinates(restrict::spec.open_display(),
													restrict::spec.root_window(), reinterpret_cast<Window>(wd), x, y, &pos.x, &pos.y, &child));
#endif
		}

		native_window_type native_interface::find_window(int x, int y)
		{
#if defined(NANA_WINDOWS)
			POINT pos = {x, y};
			return reinterpret_cast<native_window_type>(::WindowFromPoint(pos));
#elif defined(NANA_X11)
			nana::detail::platform_scope_guard psg;

			Window root = restrict::spec.root_window();
			Window wd = root;
			Window child = 0;
			int dropx = 0, dropy = 0;
			while(True == ::XTranslateCoordinates(restrict::spec.open_display(), root, wd, x, y, &dropx, &dropy, &child))
			{
				if(0 == child) break;
				wd = child;
			}
			return reinterpret_cast<native_window_type>(wd);
#endif
		}

		nana::size native_interface::check_track_size(nana::size sz, unsigned ext_width, unsigned ext_height, bool true_for_max)
		{
#if defined(NANA_WINDOWS)
			int x;
			int y;
			if(true_for_max)
			{
				x = ::GetSystemMetrics(SM_CXMAXTRACK);
				y = ::GetSystemMetrics(SM_CYMAXTRACK);
				if(static_cast<unsigned>(x) < sz.width + ext_width)
					sz.width = static_cast<unsigned>(x);
				if(static_cast<unsigned>(y) < sz.height + ext_height)
					sz.height = static_cast<unsigned>(y);
			}
			else
			{
				x = ::GetSystemMetrics(SM_CXMINTRACK);
				y = ::GetSystemMetrics(SM_CYMINTRACK);
				if(static_cast<unsigned>(x) > sz.width + ext_width)
					sz.width = static_cast<unsigned>(x);
				if(static_cast<unsigned>(y) > sz.height + ext_height)
					sz.height = static_cast<unsigned>(y);
			}
#endif
			return sz;
		}
	//end struct native_interface
	}//end namespace detail
}//end namespace nana
