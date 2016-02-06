/*
 *	A Bedrock Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/detail/win32/bedrock.cpp
 *	@contributors: Ariel Vina-Rodriguez
 */

#include <nana/detail/platform_spec_selector.hpp>
#if defined(NANA_WINDOWS)
#include <nana/gui/detail/bedrock.hpp>
#include <nana/gui/detail/bedrock_pi_data.hpp>
#include <nana/gui/detail/event_code.hpp>
#include <nana/system/platform.hpp>
#include <sstream>
#include <nana/system/timepiece.hpp>
#include <nana/gui.hpp>
#include <nana/gui/detail/inner_fwd_implement.hpp>
#include <nana/gui/detail/native_window_interface.hpp>
#include <nana/gui/layout_utility.hpp>
#include <nana/gui/detail/element_store.hpp>
#include <nana/gui/detail/color_schemes.hpp>

#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL	0x020A
#endif

#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL	0x020E
#endif

typedef void (CALLBACK *win_event_proc_t)(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);

namespace nana
{
	void notifications_window_proc(HWND wd, WPARAM wparam, LPARAM lparam); //Defined in notifier.cpp

namespace detail
{
	namespace restrict
	{
		typedef struct tagTRACKMOUSEEVENT{
			   unsigned long cbSize;
			   unsigned long dwFlags;
			   void* hwndTrack;
			   unsigned long dwHoverTime;
		} TRACKMOUSEEVENT, *LPTRACKMOUSEEVENT;

		typedef int (__stdcall* track_mouse_event_type)(LPTRACKMOUSEEVENT);

		int __stdcall dummy_track_mouse_event(LPTRACKMOUSEEVENT)
		{
			return 1;
		}

		track_mouse_event_type track_mouse_event;

		typedef HIMC (__stdcall * imm_get_context_type)(HWND);
		imm_get_context_type imm_get_context;

		typedef BOOL (__stdcall* imm_release_context_type)(HWND, HIMC);
		imm_release_context_type imm_release_context;

		typedef BOOL (__stdcall* imm_set_composition_font_type)(HIMC, LOGFONTW*);
		imm_set_composition_font_type imm_set_composition_font;

		typedef BOOL (__stdcall* imm_set_composition_window_type)(HIMC, LPCOMPOSITIONFORM);
		imm_set_composition_window_type imm_set_composition_window;
	}
#pragma pack(1)
	//Decoder of WPARAM and LPARAM
	struct wparam_button
	{
		bool left:1;
		bool right:1;
		bool shift:1;
		bool ctrl:1;
		bool middle:1;
		bool place_holder:3;
		char place_holder_c[1];
		short wheel_delta;
	};

	template<int Bytes>
	struct param_mouse
	{
		wparam_button button;
		short x;
		short y;
	};

	template<>
	struct param_mouse<8>
	{
		wparam_button button;
		char _x64_placeholder[4];
		short x;
		short y;
	};

	template<int Bytes>
	struct param_size
	{
		unsigned long state;
		short width;
		short height;
	};

	template<>
	struct param_size<8>
	{
		unsigned long state;
		char _x64_placeholder[4];
		short width;
		short height;
	};

	union parameter_decoder
	{
		struct
		{
			WPARAM wparam;
			LPARAM lparam;
		}raw_param;

		param_mouse<sizeof(LPARAM)> mouse;
		param_size<sizeof(LPARAM)> size;
	};
#pragma pack()

	struct bedrock::thread_context
	{
		unsigned	event_pump_ref_count{0};
		int			window_count{0};	//The number of windows
		core_window_t* event_window{nullptr};

		struct platform_detail_tag
		{
			wchar_t keychar;
		}platform;

		struct cursor_tag
		{
			core_window_t * window;
			native_window_type native_handle;
			nana::cursor	predef_cursor;
			HCURSOR			handle;
		}cursor;

		thread_context()
		{
			cursor.window = nullptr;
			cursor.native_handle = nullptr;
			cursor.predef_cursor = nana::cursor::arrow;
			cursor.handle = nullptr;
		}
	};

	struct bedrock::private_impl
	{
		typedef std::map<unsigned, thread_context> thr_context_container;
		std::recursive_mutex mutex;
		thr_context_container thr_contexts;

		color_schemes	schemes;
		element_store	estore;

		struct cache_type
		{
			struct thread_context_cache
			{
				unsigned tid{ 0 };
				thread_context *object{ nullptr };
			}tcontext;
		}cache;

		struct menu_tag
		{
			core_window_t*	taken_window{ nullptr };
			bool			delay_restore{ false };
			native_window_type window{ nullptr };
			native_window_type owner{ nullptr };
			bool	has_keyboard{false};
		}menu;

		struct keyboard_tracking_state_tag
		{
			keyboard_tracking_state_tag()
				:alt(0)
			{}

			bool has_shortkey_occured = false;
			bool has_keyup	= true;
			unsigned long alt : 2;
		}keyboard_tracking_state;
	};

	//class bedrock defines a static object itself to implement a static singleton
	//here is the definition of this object
	bedrock bedrock::bedrock_object;

	static LRESULT WINAPI Bedrock_WIN32_WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	bedrock::bedrock()
		:	pi_data_(new pi_data),
			impl_(new private_impl)
	{
		nana::detail::platform_spec::instance(); //to guaranty the platform_spec object is initialized before using.


		WNDCLASSEX wincl;
		wincl.hInstance = ::GetModuleHandle(0);
		wincl.lpszClassName = L"NanaWindowInternal";
		wincl.lpfnWndProc = &Bedrock_WIN32_WindowProc;
		wincl.style = CS_DBLCLKS | CS_OWNDC;
		wincl.cbSize = sizeof(wincl);
		wincl.hIcon = ::LoadIcon (0, IDI_APPLICATION);
		wincl.hIconSm = wincl.hIcon;
		wincl.hCursor = ::LoadCursor (0, IDC_ARROW);
		wincl.lpszMenuName = 0;
		wincl.cbClsExtra = 0;
		wincl.cbWndExtra = 0;
		wincl.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);

		::RegisterClassEx(&wincl);

		restrict::track_mouse_event = (restrict::track_mouse_event_type)::GetProcAddress(::GetModuleHandleA("User32.DLL"), "TrackMouseEvent");

		if(!restrict::track_mouse_event)
			restrict::track_mouse_event = restrict::dummy_track_mouse_event;

		HMODULE imm32 = ::GetModuleHandleA("Imm32.DLL");
		restrict::imm_get_context = reinterpret_cast<restrict::imm_get_context_type>(
				::GetProcAddress(imm32, "ImmGetContext"));

		restrict::imm_release_context = reinterpret_cast<restrict::imm_release_context_type>(
				::GetProcAddress(imm32, "ImmReleaseContext"));

		restrict::imm_set_composition_font = reinterpret_cast<restrict::imm_set_composition_font_type>(
				::GetProcAddress(imm32, "ImmSetCompositionFontW"));

		restrict::imm_set_composition_window = reinterpret_cast<restrict::imm_set_composition_window_type>(
				::GetProcAddress(imm32, "ImmSetCompositionWindow"));
	}

	bedrock::~bedrock()
	{
		if(wd_manager().number_of_core_window())
		{
			std::stringstream ss;
			ss<<"Nana.GUI detects a memory leaks in window_manager, "<<static_cast<unsigned>(wd_manager().number_of_core_window())<<" window(s) are not uninstalled.";
			::MessageBoxA(0, ss.str().c_str(), ("Nana C++ Library"), MB_OK);
		}

		delete impl_;
		delete pi_data_;
	}

	//inc_window
	//@brief: increament the number of windows
	int bedrock::inc_window(unsigned tid)
	{
		//impl refers to the object of private_impl, the object is created when bedrock is creating.
		private_impl * impl = instance().impl_;
		std::lock_guard<decltype(impl->mutex)> lock(impl->mutex);

		int & cnt = (impl->thr_contexts[tid ? tid : nana::system::this_thread_id()].window_count);
		return (cnt < 0 ? cnt = 1 : ++cnt);
	}

	auto bedrock::open_thread_context(unsigned tid) -> thread_context*
	{
		if(0 == tid) tid = nana::system::this_thread_id();
		std::lock_guard<decltype(impl_->mutex)> lock(impl_->mutex);
		if(impl_->cache.tcontext.tid == tid)
			return impl_->cache.tcontext.object;

		impl_->cache.tcontext.tid = tid;
		auto i = impl_->thr_contexts.find(tid);
		thread_context * context = (i == impl_->thr_contexts.end() ? &(impl_->thr_contexts[tid]) : &(i->second));
		impl_->cache.tcontext.object = context;
		return context;
	}

	auto bedrock::get_thread_context(unsigned tid) -> thread_context *
	{
		if(0 == tid) tid = nana::system::this_thread_id();

		std::lock_guard<decltype(impl_->mutex)> lock(impl_->mutex);
		auto & cachectx = impl_->cache.tcontext;
		if(cachectx.tid == tid)
			return cachectx.object;

		auto i = impl_->thr_contexts.find(tid);
		if(i != impl_->thr_contexts.end())
		{
			cachectx.tid = tid;
			return (cachectx.object = &(i->second));
		}

		cachectx.tid = 0;
		return nullptr;
	}

	void bedrock::remove_thread_context(unsigned tid)
	{
		if(0 == tid) tid = nana::system::this_thread_id();

		std::lock_guard<decltype(impl_->mutex)> lock(impl_->mutex);

		if(impl_->cache.tcontext.tid == tid)
		{
			impl_->cache.tcontext.tid = 0;
			impl_->cache.tcontext.object = nullptr;
		}

		impl_->thr_contexts.erase(tid);
	}

	bedrock& bedrock::instance()
	{
		return bedrock_object;
	}

	void bedrock::map_thread_root_buffer(core_window_t* wd, bool forced, const rectangle* update_area)
	{
		auto stru = reinterpret_cast<detail::messages::map_thread*>(::HeapAlloc(::GetProcessHeap(), 0, sizeof(detail::messages::map_thread)));
		if (stru)
		{
			if (FALSE == ::PostMessage(reinterpret_cast<HWND>(wd->root), nana::detail::messages::map_thread_root_buffer, reinterpret_cast<WPARAM>(wd), reinterpret_cast<LPARAM>(stru)))
				::HeapFree(::GetProcessHeap(), 0, stru);
		}
	}

	void interior_helper_for_menu(MSG& msg, native_window_type menu_window)
	{
		switch(msg.message)
		{
		case WM_KEYDOWN:
		case WM_CHAR:
		case WM_KEYUP:
			msg.hwnd = reinterpret_cast<HWND>(menu_window);
			break;
		}
	}

	void bedrock::pump_event(window modal_window, bool is_modal)
	{
		const unsigned tid = ::GetCurrentThreadId();
		auto context = this->open_thread_context(tid);
		if(0 == context->window_count)
		{
			//test if there is not a window
			//GetMessage may block if there is not a window
			remove_thread_context();
			return;
		}

		++(context->event_pump_ref_count);

		auto & intr_locker = wd_manager().internal_lock();
		intr_locker.revert();

		try
		{
			MSG msg;
			if(modal_window)
			{
				HWND native_handle = reinterpret_cast<HWND>(reinterpret_cast<core_window_t*>(modal_window)->root);
				if (is_modal)
				{
					HWND owner = ::GetWindow(native_handle, GW_OWNER);
					if (owner && owner != ::GetDesktopWindow())
						::EnableWindow(owner, false);


					while (::IsWindow(native_handle))
					{
						::WaitMessage();
						while (::PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
						{
							if (msg.message == WM_QUIT)   break;
							if ((WM_KEYFIRST <= msg.message && msg.message <= WM_KEYLAST) || !::IsDialogMessage(native_handle, &msg))
							{
								auto menu_wd = get_menu(reinterpret_cast<native_window_type>(msg.hwnd), true);
								if (menu_wd) interior_helper_for_menu(msg, menu_wd);

								::TranslateMessage(&msg);
								::DispatchMessage(&msg);

								wd_manager().remove_trash_handle(tid);
							}
						}
					}
				}
				else
				{
					while (::IsWindow(native_handle))
					{
						if (-1 != ::GetMessage(&msg, 0, 0, 0))
						{
							auto menu_wd = get_menu(reinterpret_cast<native_window_type>(msg.hwnd), true);
							if (menu_wd) interior_helper_for_menu(msg, menu_wd);

							::TranslateMessage(&msg);
							::DispatchMessage(&msg);
						}

						wd_manager().call_safe_place(tid);
						wd_manager().remove_trash_handle(tid);
						if (msg.message == WM_DESTROY  && msg.hwnd == native_handle)
							break;
					}//end while
				}
			}
			else
			{
				while(context->window_count)
				{
					if(-1 != ::GetMessage(&msg, 0, 0, 0))
					{
						auto menu_wd = get_menu(reinterpret_cast<native_window_type>(msg.hwnd), true);
						if(menu_wd) interior_helper_for_menu(msg, menu_wd);

						::TranslateMessage(&msg);
						::DispatchMessage(&msg);
					}

					wd_manager().call_safe_place(tid);
					wd_manager().remove_trash_handle(tid);
				}//end while

				//Empty these rest messages, there is not a window to process these messages.
				while(::PeekMessage(&msg, 0, 0, 0, PM_REMOVE));
			}
		}
        catch(std::exception& e)
        {
             (msgbox(modal_window, "An uncaptured std::exception during message pumping: ").icon(msgbox::icon_information)
                                 <<"\n   in form: "<< API::window_caption(modal_window)
                                 <<"\n   exception : "<< e.what()
             ).show();

			 internal_scope_guard lock;
			 _m_except_handler();

			 intr_locker.forward();
			 if (0 == --(context->event_pump_ref_count))
			 {
				 if ((nullptr == modal_window) || (0 == context->window_count))
					 remove_thread_context();
			 }
			 throw;
        }
		catch(...)
		{
			(msgbox(modal_window, "An exception during message pumping!").icon(msgbox::icon_information)
				<<"An uncaptured non-std exception during message pumping!"
				).show();
			internal_scope_guard lock;
			_m_except_handler();

			intr_locker.forward();
			if(0 == --(context->event_pump_ref_count))
			{
				if((nullptr == modal_window) || (0 == context->window_count))
					remove_thread_context();
			}
			throw;
		}

		intr_locker.forward();
		if(0 == --(context->event_pump_ref_count))
		{
			if((nullptr == modal_window) || (0 == context->window_count))
				remove_thread_context();
		}
	}//end pump_event

	void assign_arg(nana::arg_mouse& arg, basic_window* wd, unsigned msg, const parameter_decoder& pmdec)
	{
		arg.window_handle = reinterpret_cast<window>(wd);

		bool set_key_state = true;
		switch (msg)
		{
		case WM_LBUTTONDOWN:
			arg.button = ::nana::mouse::left_button;
			arg.evt_code = event_code::mouse_down;
			break;
		case WM_RBUTTONDOWN:
			arg.button = ::nana::mouse::right_button;
			arg.evt_code = event_code::mouse_down;
			break;
		case WM_MBUTTONDOWN:
			arg.button = ::nana::mouse::middle_button;
			arg.evt_code = event_code::mouse_down;
			break;
		case WM_LBUTTONUP:
			arg.button = ::nana::mouse::left_button;
			arg.evt_code = event_code::mouse_up;
			break;
		case WM_RBUTTONUP:
			arg.button = ::nana::mouse::right_button;
			arg.evt_code = event_code::mouse_up;
			break;
		case WM_MBUTTONUP:
			arg.button = ::nana::mouse::middle_button;
			arg.evt_code = event_code::mouse_up;
			break;
		case WM_LBUTTONDBLCLK:
			arg.button = ::nana::mouse::left_button;
			arg.evt_code = (wd->flags.dbl_click ? event_code::dbl_click : event_code::mouse_down);
			break;
		case WM_MBUTTONDBLCLK:
			arg.button = ::nana::mouse::middle_button;
			arg.evt_code = (wd->flags.dbl_click ? event_code::dbl_click : event_code::mouse_down);
			break;
		case WM_RBUTTONDBLCLK:
			arg.button = ::nana::mouse::right_button;
			arg.evt_code = (wd->flags.dbl_click ? event_code::dbl_click : event_code::mouse_down);
			break;
		case WM_MOUSEMOVE:
			arg.button = ::nana::mouse::any_button;
			arg.evt_code = event_code::mouse_move;
			break;
		default:
			set_key_state = false;
		}

		if (set_key_state)
		{
			arg.pos.x = pmdec.mouse.x - wd->pos_root.x;
			arg.pos.y = pmdec.mouse.y - wd->pos_root.y;
			arg.alt = (::GetKeyState(VK_MENU) < 0);
			arg.shift = pmdec.mouse.button.shift;
			arg.ctrl = pmdec.mouse.button.ctrl;
			arg.left_button = pmdec.mouse.button.left;
			arg.mid_button = pmdec.mouse.button.middle;
			arg.right_button = pmdec.mouse.button.right;
		}
	}

	void assign_arg(arg_focus& arg, basic_window* wd, native_window_type recv, bool getting)
	{
		arg.window_handle = reinterpret_cast<window>(wd);
		arg.receiver = recv;
		arg.getting = getting;
	}

	void assign_arg(arg_wheel& arg, basic_window* wd, const parameter_decoder& pmdec)
	{
		arg.window_handle = reinterpret_cast<window>(wd);
		arg.evt_code = event_code::mouse_wheel;

		POINT point = { pmdec.mouse.x, pmdec.mouse.y };
		::ScreenToClient(reinterpret_cast<HWND>(wd->root), &point);

		arg.upwards = (pmdec.mouse.button.wheel_delta >= 0);
		arg.distance = static_cast<unsigned>(arg.upwards ? pmdec.mouse.button.wheel_delta : -pmdec.mouse.button.wheel_delta);

		arg.pos.x = static_cast<int>(point.x) - wd->pos_root.x;
		arg.pos.y = static_cast<int>(point.y) - wd->pos_root.y;
		arg.left_button = pmdec.mouse.button.left;
		arg.mid_button = pmdec.mouse.button.middle;
		arg.right_button = pmdec.mouse.button.right;
		arg.ctrl = pmdec.mouse.button.ctrl;
		arg.shift = pmdec.mouse.button.shift;
	}

	//trivial_message
	//	The Windows messaging always sends a message to the window thread queue when the calling is in other thread.
	//If messages can be finished without expecting Nana's window manager, the trivail_message function would
	//handle those messages. This is a method to avoid a deadlock, that calling waits for the handling and they require
	//Nana's window manager.
	bool trivial_message(HWND wd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT & ret)
	{
		bedrock & bedrock = bedrock::instance();
		switch(msg)
		{
		case nana::detail::messages::async_activate:
			::EnableWindow(wd, true);
			::SetActiveWindow(wd);
			return true;
		case nana::detail::messages::async_set_focus:
			::SetFocus(wd);
			return true;
		case nana::detail::messages::operate_caret:
			//Refer to basis.hpp for this specification.
			switch(wParam)
			{
			case 1: //Delete
				::DestroyCaret();
				break;
			case 2: //SetPos
				::SetCaretPos(reinterpret_cast<nana::detail::messages::caret*>(lParam)->x, reinterpret_cast<nana::detail::messages::caret*>(lParam)->y);
				delete reinterpret_cast<nana::detail::messages::caret*>(lParam);
				break;
			}
			return true;
		case nana::detail::messages::map_thread_root_buffer:
			{
				auto stru = reinterpret_cast<detail::messages::map_thread*>(lParam);
				bedrock.wd_manager().map(reinterpret_cast<bedrock::core_window_t*>(wParam), stru->forced, (stru->ignore_update_area ? nullptr : &stru->update_area));
				::UpdateWindow(wd);
				::HeapFree(::GetProcessHeap(), 0, stru);
			}
			return true;
		case nana::detail::messages::remote_thread_move_window:
			{
				auto * mw = reinterpret_cast<nana::detail::messages::move_window*>(wParam);

				::RECT r;
				::GetWindowRect(wd, &r);
				if(mw->ignore & mw->Pos)
				{
					mw->x = r.left;
					mw->y = r.top;
				}
				else
				{
					HWND owner = ::GetWindow(wd, GW_OWNER);
					if(owner)
					{
						::RECT owr;
						::GetWindowRect(owner, &owr);
						::POINT pos = {owr.left, owr.top};
						::ScreenToClient(owner, &pos);
						mw->x += (owr.left - pos.x);
						mw->y += (owr.top - pos.y);
					}
				}

				if(mw->ignore & mw->Size)
				{
					mw->width = r.right - r.left;
					mw->height = r.bottom - r.top;
				}
				::MoveWindow(wd, mw->x, mw->y, mw->width, mw->height, true);
				delete mw;
			}
			return true;
		case nana::detail::messages::remote_thread_set_window_pos:
			::SetWindowPos(wd, reinterpret_cast<HWND>(wParam), 0, 0, 0, 0, static_cast<UINT>(lParam));
			return true;
		case nana::detail::messages::remote_thread_set_window_text:
			::SetWindowTextW(wd, reinterpret_cast<wchar_t*>(wParam));
			delete [] reinterpret_cast<wchar_t*>(wParam);
			return true;
		case nana::detail::messages::remote_thread_destroy_window:
			detail::native_interface::close_window(reinterpret_cast<native_window_type>(wd));	//The owner would be actived before the message has posted in current thread.
			{
				internal_scope_guard sg;
				auto * thrd = bedrock.get_thread_context();
				if(thrd && (thrd->window_count == 0))
					::PostQuitMessage(0);
			}
			ret = ::DefWindowProc(wd, msg, wParam, lParam);
			return true;
		case nana::detail::messages::tray:
			notifications_window_proc(wd, wParam, lParam);
			return true;
		default:
			break;
		}

		switch(msg)
		{
		case WM_DESTROY:
		case WM_SHOWWINDOW:
		case WM_SIZING:
		case WM_MOVE:
		case WM_SIZE:
		case WM_SETFOCUS:
		case WM_KILLFOCUS:
		case WM_PAINT:
		case WM_CLOSE:
		case WM_MOUSEACTIVATE:
		case WM_GETMINMAXINFO:
		case WM_WINDOWPOSCHANGED:
		case WM_NCDESTROY:
		case WM_NCLBUTTONDOWN:
		case WM_NCRBUTTONDOWN:
		case WM_NCMBUTTONDOWN:
		case WM_IME_STARTCOMPOSITION:
		case WM_DROPFILES:
		case WM_MOUSELEAVE:
		case WM_MOUSEWHEEL:	//The WM_MOUSELAST may not include the WM_MOUSEWHEEL/WM_MOUSEHWHEEL when the version of SDK is low.
		case WM_MOUSEHWHEEL:
			return false;
		default:
			if((WM_MOUSEFIRST <= msg && msg <= WM_MOUSELAST) || (WM_KEYFIRST <= msg && msg <= WM_KEYLAST))
				return false;
		}

		ret = ::DefWindowProc(wd, msg, wParam, lParam);
		return true;
	}

	void adjust_sizing(bedrock::core_window_t* wd, ::RECT * const r, int edge, unsigned req_width, unsigned req_height)
	{
		unsigned width = static_cast<unsigned>(r->right - r->left) - wd->extra_width;
		unsigned height = static_cast<unsigned>(r->bottom - r->top) - wd->extra_height;

		if(wd->max_track_size.width && (wd->max_track_size.width < req_width))
			req_width = wd->max_track_size.width;
		else if(wd->min_track_size.width && (wd->min_track_size.width > req_width))
			req_width = wd->min_track_size.width;

		if(wd->max_track_size.height && (wd->max_track_size.height < req_height))
			req_height = wd->max_track_size.height;
		else if(wd->min_track_size.height && (wd->min_track_size.height > req_height))
			req_height = wd->min_track_size.height;

		if(req_width != width)
		{
			switch(edge)
			{
			case WMSZ_LEFT:
			case WMSZ_BOTTOMLEFT:
			case WMSZ_TOPLEFT:
				r->left = r->right - static_cast<int>(req_width) - wd->extra_width;
				break;
			case WMSZ_RIGHT:
			case WMSZ_BOTTOMRIGHT:
			case WMSZ_TOPRIGHT:
			case WMSZ_TOP:
			case WMSZ_BOTTOM:
				r->right = r->left + static_cast<int>(req_width) + wd->extra_width;
				break;
			}
		}

		if(req_height != height)
		{
			switch(edge)
			{
			case WMSZ_TOP:
			case WMSZ_TOPLEFT:
			case WMSZ_TOPRIGHT:
				r->top = r->bottom - static_cast<int>(req_height) - wd->extra_height;
				break;
			case WMSZ_BOTTOM:
			case WMSZ_BOTTOMLEFT:
			case WMSZ_BOTTOMRIGHT:
			case WMSZ_LEFT:
			case WMSZ_RIGHT:
				r->bottom = r->top + static_cast<int>(req_height) + wd->extra_height;
				break;
			}
		}
	}

	template<typename Arg>
	void emit_drawer(void (::nana::detail::drawer::*event_ptr)(const Arg&), basic_window* wd, const Arg& arg, bedrock::thread_context* thrd)
	{
		if (bedrock::instance().wd_manager().available(wd) == false)
			return;

		basic_window* prev_event_wd;
		if (thrd)
		{
			prev_event_wd = thrd->event_window;
			thrd->event_window = wd;
		}

		if (wd->other.upd_state == basic_window::update_state::none)
			wd->other.upd_state = basic_window::update_state::lazy;

		(wd->drawer.*event_ptr)(arg);

		if (thrd) thrd->event_window = prev_event_wd;
	}

	LRESULT CALLBACK Bedrock_WIN32_WindowProc(HWND root_window, UINT message, WPARAM wParam, LPARAM lParam)
	{
		LRESULT window_proc_value = 0;
		if(trivial_message(root_window, message, wParam, lParam, window_proc_value))
			return window_proc_value;

		static auto& brock = bedrock::instance();
		static restrict::TRACKMOUSEEVENT track = {sizeof track, 0x00000002};

		auto native_window = reinterpret_cast<native_window_type>(root_window);
		auto* root_runtime = brock.wd_manager().root_runtime(native_window);

		if(root_runtime)
		{
			bool def_window_proc = false;
			auto& context = *brock.get_thread_context();

			auto pressed_wd = root_runtime->condition.pressed;
			auto pressed_wd_space = root_runtime->condition.pressed_by_space;
			auto hovered_wd = root_runtime->condition.hovered;

			parameter_decoder pmdec;
			pmdec.raw_param.lparam = lParam;
			pmdec.raw_param.wparam = wParam;

			internal_scope_guard lock;
			auto msgwnd = root_runtime->window;

			switch(message)
			{
			case WM_IME_STARTCOMPOSITION:
				if(msgwnd->other.attribute.root->ime_enabled)
				{
					auto native_font = msgwnd->drawer.graphics.typeface().handle();
					LOGFONTW logfont;
					::GetObjectW(reinterpret_cast<HFONT>(native_font), sizeof logfont, &logfont);

					HIMC imc = restrict::imm_get_context(root_window);
					restrict::imm_set_composition_font(imc, &logfont);

					POINT pos;
					::GetCaretPos(&pos);

					COMPOSITIONFORM cf = {CFS_POINT};
					cf.ptCurrentPos = pos;
					restrict::imm_set_composition_window(imc, &cf);
					restrict::imm_release_context(root_window, imc);
				}
				def_window_proc = true;
				break;
			case WM_GETMINMAXINFO:
				{
					bool take_over = false;
					auto mmi = reinterpret_cast<MINMAXINFO*>(lParam);

					if(msgwnd->min_track_size.width && msgwnd->min_track_size.height)
					{
						mmi->ptMinTrackSize.x = static_cast<LONG>(msgwnd->min_track_size.width + msgwnd->extra_width);
						mmi->ptMinTrackSize.y = static_cast<LONG>(msgwnd->min_track_size.height + msgwnd->extra_height);
						take_over = true;
					}

					if(false == msgwnd->flags.fullscreen)
					{
						if(msgwnd->max_track_size.width && msgwnd->max_track_size.height)
						{
							mmi->ptMaxTrackSize.x = static_cast<LONG>(msgwnd->max_track_size.width + msgwnd->extra_width);
							mmi->ptMaxTrackSize.y = static_cast<LONG>(msgwnd->max_track_size.height + msgwnd->extra_height);
							if(mmi->ptMaxSize.x > mmi->ptMaxTrackSize.x)
								mmi->ptMaxSize.x = mmi->ptMaxTrackSize.x;
							if(mmi->ptMaxSize.y > mmi->ptMaxTrackSize.y)
								mmi->ptMaxSize.y = mmi->ptMaxTrackSize.y;

							take_over = true;
						}
					}

					if (take_over)
						return 0;
				}
				break;
			case WM_SHOWWINDOW:
				if (msgwnd->visible && (wParam == FALSE))
					brock.event_expose(msgwnd, false);
				else if ((!msgwnd->visible) && (wParam != FALSE))
					brock.event_expose(msgwnd, true);
				def_window_proc = true;
				break;
			case WM_WINDOWPOSCHANGED:
				if ((reinterpret_cast<WINDOWPOS*>(lParam)->flags & SWP_SHOWWINDOW) && (!msgwnd->visible))
					brock.event_expose(msgwnd, true);
				else if ((reinterpret_cast<WINDOWPOS*>(lParam)->flags & SWP_HIDEWINDOW) && msgwnd->visible)
					brock.event_expose(msgwnd, false);

				def_window_proc = true;
				break;
			case WM_SETFOCUS:
				if(msgwnd->flags.enabled && msgwnd->flags.take_active)
				{
					auto focus = msgwnd->other.attribute.root->focus;

					if(focus && focus->together.caret)
						focus->together.caret->set_active(true);

					arg_focus arg;
					assign_arg(arg, focus, native_window, true);
					if (!brock.emit(event_code::focus, focus, arg, true, &context))
						brock.wd_manager().set_focus(msgwnd, true);
				}
				def_window_proc = true;
				break;
			case WM_KILLFOCUS:
				if(msgwnd->other.attribute.root->focus)
				{
					auto focus = msgwnd->other.attribute.root->focus;

					arg_focus arg;
					assign_arg(arg, focus, reinterpret_cast<native_window_type>(wParam), false);
					if(brock.emit(event_code::focus, focus, arg, true, &context))
					{
						if(focus->together.caret)
							focus->together.caret->set_active(false);
					}

					//wParam indicates a handle of window that receives the focus.
					brock.close_menu_if_focus_other_window(reinterpret_cast<native_window_type>(wParam));
				}

				def_window_proc = true;
				break;
			case WM_MOUSEACTIVATE:
				if(msgwnd->flags.take_active == false)
					return MA_NOACTIVATE;

				def_window_proc = true;
				break;
			case WM_LBUTTONDBLCLK: case WM_MBUTTONDBLCLK: case WM_RBUTTONDBLCLK:
				//Ignore mouse events when a window has been pressed by pressing spacebar
				if (pressed_wd_space)
					break;

				pressed_wd = nullptr;
				msgwnd = brock.wd_manager().find_window(native_window, pmdec.mouse.x, pmdec.mouse.y);
				if(msgwnd && msgwnd->flags.enabled)
				{
					if (msgwnd->flags.take_active && !msgwnd->flags.ignore_mouse_focus)
					{
						auto killed = brock.wd_manager().set_focus(msgwnd, false);
						if (killed != msgwnd)
							brock.wd_manager().do_lazy_refresh(killed, false);
					}

					arg_mouse arg;
					assign_arg(arg, msgwnd, message, pmdec);
					if (brock.emit(arg.evt_code, msgwnd, arg, true, &context))
					{
						if (brock.wd_manager().available(msgwnd))
							pressed_wd = msgwnd;
					}
				}
				break;
			case WM_NCLBUTTONDOWN: case WM_NCMBUTTONDOWN: case WM_NCRBUTTONDOWN:
				brock.close_menu_if_focus_other_window(native_window);
				def_window_proc = true;
				break;
			case WM_LBUTTONDOWN: case WM_MBUTTONDOWN: case WM_RBUTTONDOWN:
				//Ignore mouse events when a window has been pressed by pressing spacebar
				if (pressed_wd_space)
					break;

				msgwnd = brock.wd_manager().find_window(native_window, pmdec.mouse.x, pmdec.mouse.y);
				if ((nullptr == msgwnd) || (pressed_wd && (msgwnd != pressed_wd)))
					break;

				//if event on the menubar, just remove the menu if it is not associating with the menubar
				if ((msgwnd == msgwnd->root_widget->other.attribute.root->menubar) && brock.get_menu(msgwnd->root, true))
					brock.erase_menu(true);
				else
					brock.close_menu_if_focus_other_window(msgwnd->root);

				if(msgwnd->flags.enabled)
				{
					pressed_wd = msgwnd;

					if (WM_LBUTTONDOWN == message)	//Sets focus only if left button is pressed
					{
						auto new_focus = (msgwnd->flags.take_active ? msgwnd : msgwnd->other.active_window);
						if (new_focus && (!new_focus->flags.ignore_mouse_focus))
						{
							auto kill_focus = brock.wd_manager().set_focus(new_focus, false);
							if (kill_focus != new_focus)
								brock.wd_manager().do_lazy_refresh(kill_focus, false);
						}
					}

					arg_mouse arg;
					assign_arg(arg, msgwnd, message, pmdec);
					msgwnd->flags.action = mouse_action::pressed;

					auto retain = msgwnd->together.events_ptr;
					if (brock.emit(event_code::mouse_down, msgwnd, arg, true, &context))
					{
						//If a root_window is created during the mouse_down event, Nana.GUI will ignore the mouse_up event.
						if (msgwnd->root != native_interface::get_focus_window())
						{
							auto pos = native_interface::cursor_position();
							auto rootwd = native_interface::find_window(pos.x, pos.y);
							native_interface::calc_window_point(rootwd, pos);
							if(msgwnd != brock.wd_manager().find_window(rootwd, pos.x, pos.y))
							{
								//call the drawer mouse up event for restoring the surface graphics
								msgwnd->flags.action = mouse_action::normal;

								arg.evt_code = event_code::mouse_up;
								emit_drawer(&drawer::mouse_up, msgwnd, arg, &context);
								brock.wd_manager().do_lazy_refresh(msgwnd, false);
							}
						}
					}
					else
						pressed_wd = nullptr;
				}
				break;
			//mouse_click, mouse_up
			case WM_LBUTTONUP:
			case WM_MBUTTONUP:
			case WM_RBUTTONUP:
				//Ignore mouse events when a window has been pressed by pressing spacebar
				if (pressed_wd_space)
					break;

       			msgwnd = brock.wd_manager().find_window(native_window, pmdec.mouse.x, pmdec.mouse.y);
				if(nullptr == msgwnd)
					break;

				msgwnd->flags.action = mouse_action::normal;
				if(msgwnd->flags.enabled)
				{
					auto retain = msgwnd->together.events_ptr;

					::nana::arg_mouse arg;
					assign_arg(arg, msgwnd, message, pmdec);

					::nana::arg_click click_arg;

					//the window_handle of click_arg is used as a flag to determinate whether to emit click event
					click_arg.window_handle = nullptr;
					click_arg.mouse_args = &arg;

					if (msgwnd->dimension.is_hit(arg.pos))
					{
						msgwnd->flags.action = mouse_action::over;
						if ((::nana::mouse::left_button == arg.button) && (pressed_wd == msgwnd))
						{
							click_arg.window_handle = reinterpret_cast<window>(msgwnd);
							emit_drawer(&drawer::click, msgwnd, click_arg, &context);
						}
					}

					//Do mouse_up, this handle may be closed by click handler.
					if(brock.wd_manager().available(msgwnd) && msgwnd->flags.enabled)
					{
						arg.evt_code = event_code::mouse_up;
						emit_drawer(&drawer::mouse_up, msgwnd, arg, &context);

						if (click_arg.window_handle)
							retain->click.emit(click_arg);

						if (brock.wd_manager().available(msgwnd))
						{
							arg.evt_code = event_code::mouse_up;
							retain->mouse_up.emit(arg);
						}
					}
					else if (click_arg.window_handle)
						retain->click.emit(click_arg);

					brock.wd_manager().do_lazy_refresh(msgwnd, false);
				}
				pressed_wd = nullptr;
				break;
			case WM_MOUSEMOVE:
				//Ignore mouse events when a window has been pressed by pressing spacebar
				if (pressed_wd_space)
					break;

				msgwnd = brock.wd_manager().find_window(native_window, pmdec.mouse.x, pmdec.mouse.y);
				if (brock.wd_manager().available(hovered_wd) && (msgwnd != hovered_wd))
				{
					brock.event_msleave(hovered_wd);
					hovered_wd->flags.action = mouse_action::normal;
					hovered_wd = nullptr;

					//if msgwnd is neither captured window nor the child of captured window,
					//redirect the msgwnd to the captured window.
					auto wd = brock.wd_manager().capture_redirect(msgwnd);
					if(wd)
						msgwnd = wd;
				}

				else if(msgwnd)
				{
					bool prev_captured_inside;
					if(brock.wd_manager().capture_window_entered(pmdec.mouse.x, pmdec.mouse.y, prev_captured_inside))
					{
						event_code evt_code;
						if(prev_captured_inside)
						{
							evt_code = event_code::mouse_leave;
							msgwnd->flags.action = mouse_action::normal;
						}
						else
						{
							evt_code = event_code::mouse_enter;
							if (pressed_wd == msgwnd)
								msgwnd->flags.action = mouse_action::pressed;
							else if (mouse_action::pressed != msgwnd->flags.action)
								msgwnd->flags.action = mouse_action::over;
						}
						arg_mouse arg;
						assign_arg(arg, msgwnd, message, pmdec);
						arg.evt_code = evt_code;
						brock.emit(evt_code, msgwnd, arg, true, &context);
					}
				}

				if(msgwnd)
				{
					arg_mouse arg;
					assign_arg(arg, msgwnd, message, pmdec);

					if (hovered_wd != msgwnd)
					{
						if (pressed_wd == msgwnd)
							msgwnd->flags.action = mouse_action::pressed;
						else if (mouse_action::pressed != msgwnd->flags.action)
							msgwnd->flags.action = mouse_action::over;

						hovered_wd = msgwnd;
						arg.evt_code = event_code::mouse_enter;
						brock.emit(event_code::mouse_enter, msgwnd, arg, true, &context);
					}

					arg.evt_code = event_code::mouse_move;
					brock.emit(event_code::mouse_move, msgwnd, arg, true, &context);
					track.hwndTrack = native_window;
					restrict::track_mouse_event(&track);
				}
				if (!brock.wd_manager().available(hovered_wd))
					hovered_wd = nullptr;
				break;
			case WM_MOUSELEAVE:
				brock.event_msleave(hovered_wd);
				hovered_wd = nullptr;
				break;
			case WM_MOUSEWHEEL:
			case WM_MOUSEHWHEEL:
				{
					//The focus window receives the message in Windows system, it should be redirected to the hovered window
                    ::POINT scr_pos{ pmdec.mouse.x, pmdec.mouse.y};  //Screen position
					auto pointer_wd = ::WindowFromPoint(scr_pos);
					if (pointer_wd == root_window)
					{
						::ScreenToClient(pointer_wd, &scr_pos);
						auto scrolled_wd = brock.wd_manager().find_window(reinterpret_cast<native_window_type>(pointer_wd), scr_pos.x, scr_pos.y);

						def_window_proc = true;
						auto evt_wd = scrolled_wd;
						while (evt_wd)
						{
							if (evt_wd->together.events_ptr->mouse_wheel.length() != 0)
							{
								def_window_proc = false;
								nana::point mspos{ scr_pos.x, scr_pos.y };
								brock.wd_manager().calc_window_point(evt_wd, mspos);

								arg_wheel arg;
								arg.which = (WM_MOUSEHWHEEL == message ? arg_wheel::wheel::horizontal : arg_wheel::wheel::vertical);
								assign_arg(arg, evt_wd, pmdec);
								brock.emit(event_code::mouse_wheel, evt_wd, arg, true, &context);
								break;
							}
							evt_wd = evt_wd->parent;
						}

						if (scrolled_wd && (nullptr == evt_wd))
						{
							nana::point mspos{ scr_pos.x, scr_pos.y };
							brock.wd_manager().calc_window_point(scrolled_wd, mspos);

							arg_wheel arg;
							arg.which = (WM_MOUSEHWHEEL == message ? arg_wheel::wheel::horizontal : arg_wheel::wheel::vertical);
							assign_arg(arg, scrolled_wd, pmdec);
							brock.emit_drawer(event_code::mouse_wheel, scrolled_wd, arg, &context);
							brock.wd_manager().do_lazy_refresh(scrolled_wd, false);
						}
					}
					else
					{
						DWORD pid = 0;
						::GetWindowThreadProcessId(pointer_wd, &pid);
						if (pid == ::GetCurrentProcessId())
							::PostMessage(pointer_wd, message, wParam, lParam);
					}
				}
				break;
			case WM_DROPFILES:
				{
					HDROP drop = reinterpret_cast<HDROP>(wParam);
					POINT pos;
					::DragQueryPoint(drop, &pos);

					msgwnd = brock.wd_manager().find_window(native_window, pos.x, pos.y);
					if(msgwnd)
					{
						arg_dropfiles dropfiles;

						std::unique_ptr<wchar_t[]> varbuf;
						std::size_t bufsize = 0;

						unsigned size = ::DragQueryFile(drop, 0xFFFFFFFF, 0, 0);
						for(unsigned i = 0; i < size; ++i)
						{
							unsigned reqlen = ::DragQueryFile(drop, i, 0, 0) + 1;
							if(bufsize < reqlen)
							{
								varbuf.reset(new wchar_t[reqlen]);
								bufsize = reqlen;
							}

							::DragQueryFile(drop, i, varbuf.get(), reqlen);

							dropfiles.files.emplace_back(to_utf8(varbuf.get()));
						}

						while(msgwnd && (msgwnd->flags.dropable == false))
							msgwnd = msgwnd->parent;

						if(msgwnd)
						{
							dropfiles.pos.x = pos.x;
							dropfiles.pos.y = pos.y;

							brock.wd_manager().calc_window_point(msgwnd, dropfiles.pos);
							dropfiles.window_handle = reinterpret_cast<window>(msgwnd);

							msgwnd->together.events_ptr->mouse_dropfiles.emit(dropfiles);
							brock.wd_manager().do_lazy_refresh(msgwnd, false);
						}
					}

					::DragFinish(drop);
				}
				break;
			case WM_SIZING:
				{
					::RECT* const r = reinterpret_cast<RECT*>(lParam);
					unsigned width = static_cast<unsigned>(r->right - r->left) - msgwnd->extra_width;
					unsigned height = static_cast<unsigned>(r->bottom - r->top) - msgwnd->extra_height;

					if(msgwnd->max_track_size.width || msgwnd->min_track_size.width)
					{
						if(wParam == WMSZ_LEFT || wParam == WMSZ_BOTTOMLEFT || wParam == WMSZ_TOPLEFT)
						{
							if(msgwnd->max_track_size.width && (width > msgwnd->max_track_size.width))
								r->left = r->right - static_cast<int>(msgwnd->max_track_size.width) - msgwnd->extra_width;
							if(msgwnd->min_track_size.width && (width < msgwnd->min_track_size.width))
								r->left = r->right - static_cast<int>(msgwnd->min_track_size.width) - msgwnd->extra_width;
						}
						else if(wParam == WMSZ_RIGHT || wParam == WMSZ_BOTTOMRIGHT || wParam == WMSZ_TOPRIGHT)
						{
							if(msgwnd->max_track_size.width && (width > msgwnd->max_track_size.width))
								r->right = r->left + static_cast<int>(msgwnd->max_track_size.width) + msgwnd->extra_width;
							if(msgwnd->min_track_size.width && (width < msgwnd->min_track_size.width))
								r->right = r->left + static_cast<int>(msgwnd->min_track_size.width) + msgwnd->extra_width;
						}
					}

					if(msgwnd->max_track_size.height || msgwnd->min_track_size.height)
					{
						if(wParam == WMSZ_TOP || wParam == WMSZ_TOPLEFT || wParam == WMSZ_TOPRIGHT)
						{
							if(msgwnd->max_track_size.height && (height > msgwnd->max_track_size.height))
								r->top = r->bottom - static_cast<int>(msgwnd->max_track_size.height) - msgwnd->extra_height;
							if(msgwnd->min_track_size.height && (height < msgwnd->min_track_size.height))
								r->top = r->bottom - static_cast<int>(msgwnd->min_track_size.height) - msgwnd->extra_height;
						}
						else if(wParam == WMSZ_BOTTOM || wParam == WMSZ_BOTTOMLEFT || wParam == WMSZ_BOTTOMRIGHT)
						{
							if(msgwnd->max_track_size.height && (height > msgwnd->max_track_size.height))
								r->bottom = r->top + static_cast<int>(msgwnd->max_track_size.height) + msgwnd->extra_height;
							if(msgwnd->min_track_size.height && (height < msgwnd->min_track_size.height))
								r->bottom = r->top + static_cast<int>(msgwnd->min_track_size.height) + msgwnd->extra_height;
						}
					}

					nana::size size_before(	static_cast<unsigned>(r->right - r->left - msgwnd->extra_width),
											static_cast<unsigned>(r->bottom - r->top - msgwnd->extra_height));

					arg_resizing arg;
					arg.window_handle = reinterpret_cast<window>(msgwnd);
					arg.width = size_before.width;
					arg.height = size_before.height;

					switch (wParam)
					{
					case WMSZ_LEFT:
						arg.border = window_border::left;		break;
					case WMSZ_RIGHT:
						arg.border = window_border::right;	break;
					case WMSZ_BOTTOM:
						arg.border = window_border::bottom;	break;
					case WMSZ_BOTTOMLEFT:
						arg.border = window_border::bottom_left;	break;
					case WMSZ_BOTTOMRIGHT:
						arg.border = window_border::bottom_right;	break;
					case WMSZ_TOP:
						arg.border = window_border::top;	break;
					case WMSZ_TOPLEFT:
						arg.border = window_border::top_left;	break;
					case WMSZ_TOPRIGHT:
						arg.border = window_border::top_right;	break;
					}

					brock.emit(event_code::resizing, msgwnd, arg, false, &context);

					if (arg.width != width || arg.height != height)
					{
						adjust_sizing(msgwnd, r, static_cast<int>(wParam), arg.width, arg.height);
						return TRUE;
					}
				}
				break;
			case WM_SIZE:
				if(wParam != SIZE_MINIMIZED)
					brock.wd_manager().size(msgwnd, size(pmdec.size.width, pmdec.size.height), true, true);
				break;
			case WM_MOVE:
				brock.event_move(msgwnd, (int)(short) LOWORD(lParam), (int)(short) HIWORD(lParam));
				break;
			case WM_PAINT:
				{
					::PAINTSTRUCT ps;
					::BeginPaint(root_window, &ps);
					if (msgwnd->is_draw_through())
					{
						msgwnd->other.attribute.root->draw_through();
					}
					else
					{
						//Don't copy root_graph to the window directly, otherwise the edge nimbus effect will be missed.
						::nana::rectangle update_area(ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top);
						if (!update_area.empty())
							msgwnd->drawer.map(reinterpret_cast<window>(msgwnd), true, &update_area);
					}
					::EndPaint(root_window, &ps);
			    }
				break;
			case WM_SYSCHAR:
				def_window_proc = true;
				brock.set_keyboard_shortkey(true);
				msgwnd = brock.wd_manager().find_shortkey(native_window, static_cast<unsigned long>(wParam));
				if(msgwnd)
				{
					arg_keyboard arg;
					arg.evt_code = event_code::shortkey;
					arg.key = static_cast<wchar_t>(wParam < 0x61 ? wParam + 0x61 - 0x41 : wParam);
					arg.ctrl = arg.shift = false;
					arg.window_handle = reinterpret_cast<window>(msgwnd);
					arg.ignore = false;
					brock.emit(event_code::shortkey, msgwnd, arg, true, &context);
					def_window_proc = false;
				}
				break;
			case WM_SYSKEYDOWN:
				def_window_proc = true;
				if (brock.whether_keyboard_shortkey() == false)
				{
					msgwnd = msgwnd->root_widget->other.attribute.root->menubar;
					if (msgwnd)
					{
						bool focused = (brock.focus() == msgwnd);
						arg_keyboard arg;
						arg.evt_code = event_code::key_press;
						arg.window_handle = reinterpret_cast<window>(msgwnd);
						arg.ignore = false;
						arg.key = static_cast<wchar_t>(wParam);
						brock.get_key_state(arg);
						brock.emit(event_code::key_press, msgwnd, arg, true, &context);

						msgwnd->root_widget->flags.ignore_menubar_focus = (focused && (brock.focus() != msgwnd));
					}
					else
						brock.erase_menu(true);
				}
				break;
			case WM_SYSKEYUP:
				def_window_proc = true;
				if(brock.set_keyboard_shortkey(false) == false)
				{
					msgwnd = msgwnd->root_widget->other.attribute.root->menubar;
					if(msgwnd)
					{
						//Don't call default window proc to avoid popuping system menu.
						def_window_proc = false;

						bool set_focus = (brock.focus() != msgwnd) && (!msgwnd->root_widget->flags.ignore_menubar_focus);
						if (set_focus)
							brock.wd_manager().set_focus(msgwnd, false);

						arg_keyboard arg;
						arg.evt_code = event_code::key_release;
						arg.window_handle = reinterpret_cast<window>(msgwnd);
						arg.ignore = false;
						arg.key = static_cast<wchar_t>(wParam);
						brock.get_key_state(arg);
						brock.emit(event_code::key_release, msgwnd, arg, true, &context);

						if (!set_focus)
						{
							brock.set_menubar_taken(nullptr);
							msgwnd->root_widget->flags.ignore_menubar_focus = false;
						}
					}
				}
				break;
			case WM_KEYDOWN:
				if(msgwnd->flags.enabled)
				{
					auto menu_wd = brock.get_menu();
					if (menu_wd)
						brock.delay_restore(0);	//Enable delay restore

					if (msgwnd->root != menu_wd)
						msgwnd = brock.focus();

					if(msgwnd)
					{
						auto & wd_manager = brock.wd_manager();
						if((VK_TAB == wParam) && (!msgwnd->visible || (false == (msgwnd->flags.tab & tab_type::eating)))) //Tab
						{
							bool is_forward = (::GetKeyState(VK_SHIFT) >= 0);

							auto tstop_wd = wd_manager.tabstop(msgwnd, is_forward);
							if (tstop_wd)
							{
								wd_manager.set_focus(tstop_wd, false);
								wd_manager.do_lazy_refresh(msgwnd, false);
								wd_manager.do_lazy_refresh(tstop_wd, true);
							}
						}
						else if ((VK_SPACE == wParam) && msgwnd->flags.space_click_enabled)
						{
							//Clicked by spacebar
							if (nullptr == pressed_wd && nullptr == pressed_wd_space)
							{
								arg_mouse arg;
								arg.alt = false;
								arg.button = ::nana::mouse::left_button;
								arg.ctrl = false;
								arg.evt_code = event_code::mouse_down;
								arg.left_button = true;
								arg.mid_button = false;
								arg.pos.x = 0;
								arg.pos.y = 0;
								arg.window_handle = reinterpret_cast<window>(msgwnd);

								msgwnd->flags.action = mouse_action::pressed;

								pressed_wd_space = msgwnd;
								auto retain = msgwnd->together.events_ptr;

								emit_drawer(&drawer::mouse_down, msgwnd, arg, &context);
								wd_manager.do_lazy_refresh(msgwnd, false);
							}
						}
						else
						{
							arg_keyboard arg;
							arg.evt_code = event_code::key_press;
							arg.window_handle = reinterpret_cast<window>(msgwnd);
							arg.ignore = false;
							arg.key = static_cast<wchar_t>(wParam);
							brock.get_key_state(arg);
							brock.emit(event_code::key_press, msgwnd, arg, true, &context);

							if (msgwnd->root_widget->other.attribute.root->menubar == msgwnd)
							{
								//In order to keep the focus on the menubar, cancel the delay_restore
								//when pressing ESC to close the menu which is popuped by the menubar.
								//If no menu popuped by the menubar, it should enable delay restore to
								//restore the focus for taken window.

								int cmd = (menu_wd && (keyboard::escape == static_cast<wchar_t>(wParam)) ? 1 : 0);
								brock.delay_restore(cmd);
							}
						}
					}
				}
				break;
			case WM_CHAR:
				msgwnd = brock.focus();
				if (msgwnd && msgwnd->flags.enabled)
				{
					// When tab is pressed, only tab-eating mode is allowed
					if ((9 != wParam) || (msgwnd->flags.tab & tab_type::eating))
					{
						arg_keyboard arg;
						arg.evt_code = event_code::key_char;
						arg.window_handle = reinterpret_cast<window>(msgwnd);
						arg.key = static_cast<wchar_t>(wParam);
						brock.get_key_state(arg);
						arg.ignore = false;

						msgwnd->together.events_ptr->key_char.emit(arg);
						if ((false == arg.ignore) && brock.wd_manager().available(msgwnd))
							brock.emit_drawer(event_code::key_char, msgwnd, arg, &context);

						brock.wd_manager().do_lazy_refresh(msgwnd, false);
					}
				}
				return 0;
			case WM_KEYUP:
				if(wParam != VK_MENU) //MUST NOT BE AN ALT
				{
					msgwnd = brock.focus();
					if(msgwnd)
					{
						if (msgwnd == pressed_wd_space)
						{
							msgwnd->flags.action = mouse_action::normal;

							arg_click click_arg;
							click_arg.mouse_args = nullptr;
							click_arg.window_handle = reinterpret_cast<window>(msgwnd);

							auto retain = msgwnd->together.events_ptr;
							if (brock.emit(event_code::click, msgwnd, click_arg, true, &context))
							{
								arg_mouse arg;
								arg.alt = false;
								arg.button = ::nana::mouse::left_button;
								arg.ctrl = false;
								arg.evt_code = event_code::mouse_up;
								arg.left_button = true;
								arg.mid_button = false;
								arg.pos.x = 0;
								arg.pos.y = 0;
								arg.window_handle = reinterpret_cast<window>(msgwnd);

								emit_drawer(&drawer::mouse_up, msgwnd, arg, &context);
								brock.wd_manager().do_lazy_refresh(msgwnd, false);
							}
							pressed_wd_space = nullptr;
						}
						else
						{
							arg_keyboard arg;
							arg.evt_code = event_code::key_release;
							arg.window_handle = reinterpret_cast<window>(msgwnd);
							arg.key = static_cast<wchar_t>(wParam);
							brock.get_key_state(arg);
							arg.ignore = false;
							brock.emit(event_code::key_release, msgwnd, arg, true, &context);
						}
					}
				}
				else
					brock.set_keyboard_shortkey(false);

				//Do delay restore if key is not arrow_left/right/up/down, otherwise
				//A menubar will be restored if the item is empty(not have a menu item)
				if (wParam < 37 || 40 < wParam)
					brock.delay_restore(2);	//Restores while key release
				break;
			case WM_CLOSE:
			{
				arg_unload arg;
				arg.window_handle = reinterpret_cast<window>(msgwnd);
				arg.cancel = false;
				brock.emit(event_code::unload, msgwnd, arg, true, &context);
				if (!arg.cancel)
				{
					def_window_proc = true;
					//Activate is owner, refer to the window_manager::close for the explaination
					if (msgwnd->flags.modal || (msgwnd->owner == 0) || msgwnd->owner->flags.take_active)
						native_interface::activate_owner(msgwnd->root);
				}
				break;
			}
			case WM_DESTROY:
				if (msgwnd->root == brock.get_menu())
				{
					brock.erase_menu(false);
					brock.delay_restore(3);	//Restores if delay_restore not decleared
				}
				brock.wd_manager().destroy(msgwnd);
				nana::detail::platform_spec::instance().release_window_icon(msgwnd->root);
				break;
			case WM_NCDESTROY:
				brock.manage_form_loader(msgwnd, false);
				brock.wd_manager().destroy_handle(msgwnd);

				if(--context.window_count <= 0)
				{
					::PostQuitMessage(0);
					def_window_proc = true;
				}
				break;
			default:
				def_window_proc = true;
			}

			root_runtime = brock.wd_manager().root_runtime(native_window);
			if(root_runtime)
			{
				root_runtime->condition.pressed = pressed_wd;
				root_runtime->condition.hovered = hovered_wd;
				root_runtime->condition.pressed_by_space = pressed_wd_space;
			}

			if (!def_window_proc)
				return 0;
		}

		return ::DefWindowProc(root_window, message, wParam, lParam);
	}

	::nana::category::flags bedrock::category(core_window_t* wd)
	{
		internal_scope_guard lock;
		return (wd_manager().available(wd) ? wd->other.category : ::nana::category::flags::super);
	}

	auto bedrock::focus() ->core_window_t*
	{
		core_window_t* wd = wd_manager().root(native_interface::get_focus_window());
		return (wd ? wd->other.attribute.root->focus : nullptr);
	}

	void bedrock::set_menubar_taken(core_window_t* wd)
	{
		auto pre = impl_->menu.taken_window;
		impl_->menu.taken_window = wd;

		//assigning of a nullptr taken window is to restore the focus of pre taken
		//don't restore the focus if pre is a menu.
		if ((!wd) && pre && (pre->root != get_menu()))
		{
			internal_scope_guard lock;
			wd_manager().set_focus(pre, false);
			wd_manager().update(pre, true, false);
		}
	}

	//0:Enable delay, 1:Cancel, 2:Restores, 3: Restores when menu is destroying
	void bedrock::delay_restore(int state)
	{
		switch (state)
		{
		case 0:	//Enable
			break;
		case 1: //Cancel
			break;
		case 2:	//Restore if key released
			//restores the focus when menu is closed by pressing keyboard
			if ((!impl_->menu.window) && impl_->menu.delay_restore)
				set_menubar_taken(nullptr);
			break;
		case 3:	//Restores if destroying
			//when the menu is destroying, restores the focus if delay restore is not declared
			if (!impl_->menu.delay_restore)
				set_menubar_taken(nullptr);
		}

		impl_->menu.delay_restore = (0 == state);
	}

	bool bedrock::close_menu_if_focus_other_window(native_window_type wd)
	{
		if(impl_->menu.window && (impl_->menu.window != wd))
		{
			wd = native_interface::get_owner_window(wd);
			while(wd)
			{
				if(wd != impl_->menu.window)
					wd = native_interface::get_owner_window(wd);
				else
					return false;
			}
			erase_menu(true);
			return true;
		}
		return false;
	}

	void bedrock::set_menu(native_window_type menu_wd, bool has_keyboard)
	{
		if(menu_wd && impl_->menu.window != menu_wd)
		{
			erase_menu(true);

			impl_->menu.window = menu_wd;
			impl_->menu.owner = native_interface::get_owner_window(menu_wd);
			impl_->menu.has_keyboard = has_keyboard;
		}
	}

	native_window_type bedrock::get_menu(native_window_type owner, bool is_keyboard_condition)
	{
		if(	(impl_->menu.owner == nullptr) ||
			(owner && (impl_->menu.owner == owner))
			)
		{
			return ((!is_keyboard_condition) || impl_->menu.has_keyboard ? impl_->menu.window : nullptr);
		}
		return nullptr;
	}

	native_window_type bedrock::get_menu()
	{
		return impl_->menu.window;
	}

	void bedrock::erase_menu(bool try_destroy)
	{
		if (impl_->menu.window)
		{
			if (try_destroy)
				native_interface::close_window(impl_->menu.window);

			impl_->menu.window = impl_->menu.owner = nullptr;
			impl_->menu.has_keyboard = false;
		}
	}

	void bedrock::get_key_state(arg_keyboard& kb)
	{
		kb.ctrl = (0 != (::GetKeyState(VK_CONTROL) & 0x80));
		kb.shift = (0 != (::GetKeyState(VK_SHIFT) & 0x80));
	}

	bool bedrock::set_keyboard_shortkey(bool yes)
	{
		bool ret = impl_->keyboard_tracking_state.has_shortkey_occured;
		impl_->keyboard_tracking_state.has_shortkey_occured = yes;
		return ret;
	}

	bool bedrock::whether_keyboard_shortkey() const
	{
		return impl_->keyboard_tracking_state.has_shortkey_occured;
	}

	element_store& bedrock::get_element_store() const
	{
		return impl_->estore;
	}

	void bedrock::map_through_widgets(core_window_t* wd, native_drawable_type drawable)
	{
#if defined(NANA_WINDOWS)
		auto graph_context = reinterpret_cast<HDC>(wd->root_graph->handle()->context);

		for (auto child : wd->children)
		{
			if (!child->visible) continue;

			if (::nana::category::flags::widget == child->other.category)
			{
				::BitBlt(reinterpret_cast<HDC>(drawable), child->pos_root.x, child->pos_root.y, static_cast<int>(child->dimension.width), static_cast<int>(child->dimension.height),
					graph_context, child->pos_root.x, child->pos_root.y, SRCCOPY);
			}
			else if (::nana::category::flags::lite_widget == child->other.category)
				map_through_widgets(child, drawable);
		}
#endif
	}

	bool bedrock::emit(event_code evt_code, core_window_t* wd, const ::nana::event_arg& arg, bool ask_update, thread_context* thrd)
	{
		if (wd_manager().available(wd) == false)
			return false;

		basic_window* prev_event_wd;
		if (thrd)
		{
			prev_event_wd = thrd->event_window;
			thrd->event_window = wd;
			_m_event_filter(evt_code, wd, thrd);
		}

		if (wd->other.upd_state == core_window_t::update_state::none)
			wd->other.upd_state = core_window_t::update_state::lazy;

		_m_emit_core(evt_code, wd, false, arg);

		if (ask_update)
			wd_manager().do_lazy_refresh(wd, false);
		else if (wd_manager().available(wd))
			wd->other.upd_state = basic_window::update_state::none;

		if (thrd)	thrd->event_window = prev_event_wd;
		return true;
	}

	bool bedrock::emit_drawer(event_code evt_code, core_window_t* wd, const ::nana::event_arg& arg, thread_context* thrd)
	{
		if (bedrock_object.wd_manager().available(wd) == false)
			return false;

		core_window_t* prev_event_wd;
		if (thrd)
		{
			prev_event_wd = thrd->event_window;
			thrd->event_window = wd;
		}

		if (wd->other.upd_state == core_window_t::update_state::none)
			wd->other.upd_state = core_window_t::update_state::lazy;

		_m_emit_core(evt_code, wd, true, arg);

		if (thrd) thrd->event_window = prev_event_wd;
		return true;
	}

	const wchar_t* translate(cursor id)
	{
		const wchar_t* name = IDC_ARROW;

		switch(id)
		{
		case cursor::arrow:
			name = IDC_ARROW;	break;
		case cursor::wait:
			name = IDC_WAIT;	break;
		case cursor::hand:
			name = IDC_HAND;	break;
		case cursor::size_we:
			name = IDC_SIZEWE;	break;
		case cursor::size_ns:
			name = IDC_SIZENS;	break;
		case cursor::size_bottom_left:
		case cursor::size_top_right:
			name = IDC_SIZENESW;	break;
		case cursor::size_top_left:
		case cursor::size_bottom_right:
			name = IDC_SIZENWSE;	break;
		case cursor::iterm:
			name = IDC_IBEAM;	break;
		}
		return name;
	}

	void bedrock::thread_context_destroy(core_window_t * wd)
	{
		auto * thr = get_thread_context(0);
		if (thr && thr->event_window == wd)
			thr->event_window = nullptr;
	}

	void bedrock::thread_context_lazy_refresh()
	{
		auto* thrd = get_thread_context(0);
		if (thrd && thrd->event_window)
		{
			//the state none should be tested, becuase in an event, there would be draw after an update,
			//if the none is not tested, the draw after update will not be refreshed.
			switch (thrd->event_window->other.upd_state)
			{
			case core_window_t::update_state::none:
			case core_window_t::update_state::lazy:
				thrd->event_window->other.upd_state = core_window_t::update_state::refresh;
			default:	break;
			}
		}
	}

	//Dynamically set a cursor for a window
	void bedrock::set_cursor(core_window_t* wd, nana::cursor cur, thread_context* thrd)
	{
		if (nullptr == thrd)
			thrd = get_thread_context(wd->thread_id);

		if ((cursor::arrow == cur) && !thrd->cursor.native_handle)
			return;

		thrd->cursor.window = wd;
		if ((thrd->cursor.native_handle == wd->root) && (cur == thrd->cursor.predef_cursor))
			return;

		thrd->cursor.native_handle = wd->root;
		if (thrd->cursor.predef_cursor != cur)
		{
			thrd->cursor.predef_cursor = cur;
			thrd->cursor.handle = ::LoadCursor(nullptr, translate(cur));
		}

		if (wd->root_widget->other.attribute.root->running_cursor != cur)
		{
			wd->root_widget->other.attribute.root->running_cursor = cur;
#ifdef _WIN64
			::SetClassLongPtr(reinterpret_cast<HWND>(wd->root), GCLP_HCURSOR,
				reinterpret_cast<LONG_PTR>(thrd->cursor.handle));
#else
			::SetClassLong(reinterpret_cast<HWND>(wd->root), GCL_HCURSOR,
				static_cast<unsigned long>(reinterpret_cast<size_t>(thrd->cursor.handle)));
#endif
		}
		if (cursor::arrow == thrd->cursor.predef_cursor)
		{
			thrd->cursor.window = nullptr;
			thrd->cursor.native_handle = nullptr;
		}
	}

	void bedrock::define_state_cursor(core_window_t* wd, nana::cursor cur, thread_context* thrd)
	{
		wd->root_widget->other.attribute.root->state_cursor = cur;
		wd->root_widget->other.attribute.root->state_cursor_window = wd;
		set_cursor(wd, cur, thrd);
		auto cur_handle = ::LoadCursor(nullptr, translate(cur));
		::SetCursor(cur_handle);
		::ShowCursor(TRUE);
	}

	void bedrock::undefine_state_cursor(core_window_t * wd, thread_context* thrd)
	{
		if (nullptr == thrd)
			thrd = get_thread_context(wd->thread_id);

		HCURSOR rev_handle = ::LoadCursor(nullptr, IDC_ARROW);
		if (!wd_manager().available(wd))
		{
			::ShowCursor(FALSE);
			::SetCursor(rev_handle);
			return;
		}

		wd->root_widget->other.attribute.root->state_cursor = nana::cursor::arrow;
		wd->root_widget->other.attribute.root->state_cursor_window = nullptr;

		auto pos = native_interface::cursor_position();
		auto native_handle = native_interface::find_window(pos.x, pos.y);

		if (!native_handle)
		{
			::ShowCursor(FALSE);
			::SetCursor(rev_handle);
			return;
		}

		native_interface::calc_window_point(native_handle, pos);
		auto rev_wd = wd_manager().find_window(native_handle, pos.x, pos.y);
		if (rev_wd)
		{
			set_cursor(rev_wd, rev_wd->predef_cursor, thrd);
			rev_handle = thrd->cursor.handle;
		}
		::ShowCursor(FALSE);
		::SetCursor(rev_handle);
	}

	void bedrock::_m_event_filter(event_code event_id, core_window_t * wd, thread_context * thrd)
	{
		auto not_state_cur = (wd->root_widget->other.attribute.root->state_cursor == nana::cursor::arrow);

		switch(event_id)
		{
		case event_code::mouse_enter:
			if (not_state_cur)
				set_cursor(wd, wd->predef_cursor, thrd);
			break;
		case event_code::mouse_leave:
			if (not_state_cur && (wd->predef_cursor != cursor::arrow))
				set_cursor(wd, cursor::arrow, thrd);
			break;
		case event_code::destroy:
			if (wd->root_widget->other.attribute.root->state_cursor_window == wd)
				undefine_state_cursor(wd, thrd);

			if(wd == thrd->cursor.window)
			{
				set_cursor(wd, cursor::arrow, thrd);
				wd->root_widget->other.attribute.root->running_cursor = cursor::arrow;
			}
			break;
        default:
            break;
		}
	}
}//end namespace detail
}//end namespace nana
#endif //NANA_WINDOWS
