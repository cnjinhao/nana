/**
 *	A Bedrock Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file nana/gui/detail/win32/bedrock.cpp
 *  @brief A Bedrock Implementation
 *	@contributors: Ariel Vina-Rodriguez
 */

#include "../../detail/platform_spec_selector.hpp"
#if defined(NANA_WINDOWS)
#include "bedrock_types.hpp"
#include <nana/gui/detail/event_code.hpp>
#include <nana/system/platform.hpp>
#include <nana/system/timepiece.hpp>
#include <nana/gui/compact.hpp>
#include <nana/gui/msgbox.hpp>
#include <nana/gui/detail/native_window_interface.hpp>
#include <nana/gui/layout_utility.hpp>
#include <nana/gui/detail/window_layout.hpp>
#include <nana/gui/detail/element_store.hpp>
#include <nana/gui/detail/color_schemes.hpp>
#include "inner_fwd_implement.hpp"

#include <iostream>	//use std::cerr

#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL	0x020A
#endif

#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL	0x020E
#endif

#include "bedrock_types.hpp"


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
				thread_t tid{ 0 };
				thread_context *object{ nullptr };
			}tcontext;
		}cache;
	};

	struct window_platform_assoc
	{
		HACCEL accel{ nullptr };	///< A handle to a Windows keyboard accelerator object.
		std::map<int, std::function<void()>> accel_commands;
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
		if(wd_manager().window_count())
		{
			std::string msg = "Nana.GUI detects a memory leaks in window_manager, " + std::to_string(wd_manager().window_count()) + " window(s) are not uninstalled.";
			std::cerr << msg;  /// \todo add list of cations of opening windows and if auto testing GUI do auto OK after 2 seconds.
			::MessageBoxA(0, msg.c_str(), ("Nana C++ Library"), MB_OK);
		}

		delete impl_;
		delete pi_data_;
	}


	/// @brief increment the number of windows in the thread id
	int bedrock::inc_window(thread_t tid)
	{
		//impl refers to the object of private_impl, the object is created when bedrock is creating.
		private_impl * impl = instance().impl_;
		std::lock_guard<decltype(impl->mutex)> lock(impl->mutex);

		int & cnt = (impl->thr_contexts[tid ? tid : nana::system::this_thread_id()].window_count);
		return (cnt < 0 ? cnt = 1 : ++cnt);
	}

	auto bedrock::open_thread_context(thread_t tid) -> thread_context*
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

	auto bedrock::get_thread_context(thread_t tid) -> thread_context *
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

	void bedrock::remove_thread_context(thread_t tid)
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

	void bedrock::flush_surface(basic_window* wd, bool forced, const rectangle* update_area)
	{
		if (nana::system::this_thread_id() != wd->thread_id)
		{
			auto stru = reinterpret_cast<detail::messages::map_thread*>(::HeapAlloc(::GetProcessHeap(), 0, sizeof(detail::messages::map_thread)));
			if (stru)
			{
				stru->forced = forced;
				stru->ignore_update_area = true;

				if (update_area)
				{
					stru->ignore_update_area = false;
					stru->update_area = *update_area;
				}

				if (FALSE == ::PostMessage(reinterpret_cast<HWND>(wd->root), nana::detail::messages::remote_flush_surface, reinterpret_cast<WPARAM>(wd), reinterpret_cast<LPARAM>(stru)))
					::HeapFree(::GetProcessHeap(), 0, stru);
			}
		}
		else
			wd->drawer.map(wd, forced, update_area);
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

	void process_msg(bedrock* brock, MSG& msg)
	{
		if (WM_KEYFIRST <= msg.message && msg.message <= WM_KEYLAST)
		{
			auto misc = brock->wd_manager().root_runtime(reinterpret_cast<native_window_type>(msg.hwnd));
			if (misc && misc->wpassoc && misc->wpassoc->accel)
			{
				if (::TranslateAccelerator(msg.hwnd, misc->wpassoc->accel, &msg))
					return;
			}
		}

		auto menu_wd = brock->get_menu(reinterpret_cast<native_window_type>(msg.hwnd), true);
		if (menu_wd) interior_helper_for_menu(msg, menu_wd);

		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}

	void bedrock::pump_event(window condition_wd, bool is_modal)
	{
		thread_t tid = ::GetCurrentThreadId();
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
			if (condition_wd)
			{
				HWND native_handle = reinterpret_cast<HWND>(condition_wd->root);
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
								process_msg(this, msg);
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
							process_msg(this, msg);
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
						process_msg(this, msg);

					wd_manager().call_safe_place(tid);
					wd_manager().remove_trash_handle(tid);
				}//end while

				//Empty these rest messages, there is not a window to process these messages.
				while(::PeekMessage(&msg, 0, 0, 0, PM_REMOVE));
			}
		}
        catch(std::exception& e)
        {
			(msgbox(condition_wd, "An uncaptured std::exception during message pumping: ").icon(msgbox::icon_information)
								<< "\n   in form: " << API::window_caption(condition_wd)
								<<"\n   exception : "<< e.what()
			).show();

			internal_scope_guard lock;
			this->close_thread_window(nana::system::this_thread_id());

			intr_locker.forward();
			if (0 == --(context->event_pump_ref_count))
			{
				if ((nullptr == condition_wd) || (0 == context->window_count))
					remove_thread_context();
			}
			throw;
        }
		catch(...)
		{
			(msgbox(condition_wd, "An exception during message pumping!").icon(msgbox::icon_information)
				<<"An uncaptured non-std exception during message pumping!"
				<< "\n   in form: " << API::window_caption(condition_wd)
				).show();
			internal_scope_guard lock;
			this->close_thread_window(nana::system::this_thread_id());

			intr_locker.forward();
			if(0 == --(context->event_pump_ref_count))
			{
				if ((nullptr == condition_wd) || (0 == context->window_count))
					remove_thread_context();
			}
			throw;
		}

		intr_locker.forward();
		if(0 == --(context->event_pump_ref_count))
		{
			if ((nullptr == condition_wd) || (0 == context->window_count))
				remove_thread_context();
		}
	}//end pump_event

	void assign_arg(nana::arg_mouse& arg, basic_window* wd, unsigned msg, const parameter_decoder& pmdec)
	{
		arg.window_handle = wd;

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

	void assign_arg(arg_wheel& arg, basic_window* wd, const parameter_decoder& pmdec)
	{
		arg.window_handle = wd;
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
		case nana::detail::messages::remote_flush_surface:
			{
				auto stru = reinterpret_cast<detail::messages::map_thread*>(lParam);
				bedrock.wd_manager().map(reinterpret_cast<basic_window*>(wParam), stru->forced, (stru->ignore_update_area ? nullptr : &stru->update_area));
				::UpdateWindow(wd);
				::HeapFree(::GetProcessHeap(), 0, stru);
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
			detail::native_interface::close_window(reinterpret_cast<native_window_type>(wd));	//The owner would be activated before the message has posted in current thread.
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
		case nana::detail::messages::affinity_execute:
			if (wParam)
			{
				auto arg = reinterpret_cast<detail::messages::arg_affinity_execute*>(wParam);
				if (arg->function_ptr)
					(*arg->function_ptr)();
			}
			break;
		default:
			break;
		}

		switch(msg)
		{
		case WM_COMMAND:
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

	void adjust_sizing(basic_window* wd, ::RECT * const r, int edge, unsigned req_width, unsigned req_height)
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
	void draw_invoker(void (::nana::detail::drawer::*event_ptr)(const Arg&, const bool), basic_window* wd, const Arg& arg, bedrock::thread_context* thrd)
	{
		if (bedrock::instance().wd_manager().available(wd) == false)
			return;

		basic_window* prev_event_wd = nullptr;
		if (thrd)
		{
			prev_event_wd = thrd->event_window;
			thrd->event_window = wd;
		}

		if (wd->other.upd_state == basic_window::update_state::none)
			wd->other.upd_state = basic_window::update_state::lazy;

		(wd->drawer.*event_ptr)(arg, false);

		if (thrd) thrd->event_window = prev_event_wd;
	}

	//Translate OS Virtual-Key into ASCII code
	wchar_t translate_virtual_key(WPARAM vkey)
	{
		switch (vkey)
		{
		case VK_DELETE:
			return 127;
		case VK_DECIMAL:
			return 46;
		}
		return static_cast<wchar_t>(vkey);
	}

	LRESULT CALLBACK Bedrock_WIN32_WindowProc(HWND root_window, UINT message, WPARAM wParam, LPARAM lParam)
	{
		LRESULT window_proc_value = 0;
		if(trivial_message(root_window, message, wParam, lParam, window_proc_value))
			return window_proc_value;

		static auto& brock = bedrock::instance();
		static restrict::TRACKMOUSEEVENT track = {sizeof track, 0x00000002};

		auto const native_window = reinterpret_cast<native_window_type>(root_window);

		auto & wd_manager = brock.wd_manager();
		auto* root_runtime = wd_manager.root_runtime(native_window);

		if(root_runtime)
		{
			bool def_window_proc = false;
			auto& context = *brock.get_thread_context();

			auto const pre_event_window = context.event_window;
			auto pressed_wd = root_runtime->condition.pressed;
			auto pressed_wd_space = root_runtime->condition.pressed_by_space;
			auto hovered_wd = root_runtime->condition.hovered;

			parameter_decoder pmdec;
			pmdec.raw_param.lparam = lParam;
			pmdec.raw_param.wparam = wParam;

			internal_scope_guard lock;
			auto const root_wd = root_runtime->window;
			auto msgwnd = root_wd;

			detail::bedrock::root_guard rw_guard{ brock, root_wd };

			switch (message)
			{
			case WM_COMMAND:
				if ((1 == HIWORD(wParam)) && root_runtime->wpassoc)
				{
					auto i = root_runtime->wpassoc->accel_commands.find(LOWORD(wParam));
					if (i != root_runtime->wpassoc->accel_commands.end())
						i->second();
				}
				break;
			case WM_IME_STARTCOMPOSITION:
				if (msgwnd->other.attribute.root->ime_enabled)
				{
					auto native_font = msgwnd->drawer.graphics.typeface().handle();
					LOGFONTW logfont;
					::GetObjectW(reinterpret_cast<HFONT>(native_font), sizeof logfont, &logfont);

					HIMC imc = restrict::imm_get_context(root_window);
					restrict::imm_set_composition_font(imc, &logfont);

					POINT pos;
					::GetCaretPos(&pos);

					COMPOSITIONFORM cf = { CFS_POINT };
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

				if (!msgwnd->min_track_size.empty())
				{
					mmi->ptMinTrackSize.x = static_cast<LONG>(msgwnd->min_track_size.width + msgwnd->extra_width);
					mmi->ptMinTrackSize.y = static_cast<LONG>(msgwnd->min_track_size.height + msgwnd->extra_height);
					take_over = true;
				}

				if (false == msgwnd->flags.fullscreen)
				{
					if (msgwnd->max_track_size.width && msgwnd->max_track_size.height)
					{
						mmi->ptMaxTrackSize.x = static_cast<LONG>(msgwnd->max_track_size.width + msgwnd->extra_width);
						mmi->ptMaxTrackSize.y = static_cast<LONG>(msgwnd->max_track_size.height + msgwnd->extra_height);
						if (mmi->ptMaxSize.x > mmi->ptMaxTrackSize.x)
							mmi->ptMaxSize.x = mmi->ptMaxTrackSize.x;
						if (mmi->ptMaxSize.y > mmi->ptMaxTrackSize.y)
							mmi->ptMaxSize.y = mmi->ptMaxTrackSize.y;

						take_over = true;
					}
				}

				if (take_over)
					return 0;
			}
			break;
			case WM_SHOWWINDOW:
				if (msgwnd->visible == (FALSE == wParam))
					brock.event_expose(msgwnd, !msgwnd->visible);

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
				brock.event_focus_changed(msgwnd, native_window, true);
				def_window_proc = true;
				break;
			case WM_KILLFOCUS:
				//wParam indicates a handle of window that receives the focus.
				brock.event_focus_changed(msgwnd, reinterpret_cast<native_window_type>(wParam), false);
				def_window_proc = true;
				break;
			case WM_MOUSEACTIVATE:
				if (msgwnd->flags.take_active == false)
					return MA_NOACTIVATE;

				def_window_proc = true;
				break;
			case WM_LBUTTONDBLCLK: case WM_MBUTTONDBLCLK: case WM_RBUTTONDBLCLK:
				//Ignore mouse events when a window has been pressed by pressing spacebar
				if (pressed_wd_space)
					break;

				pressed_wd = nullptr;
				msgwnd = wd_manager.find_window(native_window, { pmdec.mouse.x, pmdec.mouse.y });
				if (msgwnd && msgwnd->flags.enabled)
				{
					if (msgwnd->flags.take_active && !msgwnd->flags.ignore_mouse_focus)
					{
						auto killed = brock.wd_manager().set_focus(msgwnd, false, arg_focus::reason::mouse_press);
						if (killed != msgwnd)
							wd_manager.do_lazy_refresh(killed, false);
					}

					arg_mouse arg;
					assign_arg(arg, msgwnd, message, pmdec);
					if (brock.emit(arg.evt_code, msgwnd, arg, true, &context))
						pressed_wd = msgwnd;
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

				msgwnd = wd_manager.find_window(native_window, { pmdec.mouse.x, pmdec.mouse.y });

				//Don't take care about whether msgwnd is equal to the pressed_wd.
				//
				//pressed_wd will remain when opens a non-activated window in an mouse_down event(like combox popups the drop-list).
				//After the non-activated window is closed, the window doesn't respond to the mouse click other than pressed_wd.
				pressed_wd = nullptr;
				if (nullptr == msgwnd)
					break;

				//if event on the menubar, just remove the menu if it is not associating with the menubar
				if ((msgwnd == msgwnd->root_widget->other.attribute.root->menubar) && brock.get_menu(msgwnd->root, true))
					brock.erase_menu(true);
				else
					brock.close_menu_if_focus_other_window(msgwnd->root);

				if (msgwnd->flags.enabled)
				{
					pressed_wd = msgwnd;

					if (WM_LBUTTONDOWN == message)	//Sets focus only if left button is pressed
					{
						auto new_focus = (msgwnd->flags.take_active ? msgwnd : msgwnd->other.active_window);
						if (new_focus && (!new_focus->flags.ignore_mouse_focus))
						{
							auto kill_focus = brock.wd_manager().set_focus(new_focus, false, arg_focus::reason::mouse_press);
							if (kill_focus != new_focus)
								wd_manager.do_lazy_refresh(kill_focus, false);
						}
					}

					arg_mouse arg;
					assign_arg(arg, msgwnd, message, pmdec);
					msgwnd->set_action(mouse_action::pressed);

					auto retain = msgwnd->annex.events_ptr;
					if (brock.emit(event_code::mouse_down, msgwnd, arg, true, &context))
					{
						//If a root_window is created during the mouse_down event, Nana.GUI will ignore the mouse_up event.
						if (msgwnd->root != native_interface::get_focus_window())
						{
							auto pos = native_interface::cursor_position();
							auto rootwd = native_interface::find_window(pos.x, pos.y);
							native_interface::calc_window_point(rootwd, pos);
							if (msgwnd != wd_manager.find_window(rootwd, pos))
							{
								//call the drawer mouse up event for restoring the surface graphics
								msgwnd->set_action(mouse_action::normal);

								arg.evt_code = event_code::mouse_up;
								draw_invoker(&drawer::mouse_up, msgwnd, arg, &context);
								wd_manager.do_lazy_refresh(msgwnd, false);
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

				msgwnd = wd_manager.find_window(native_window, { pmdec.mouse.x, pmdec.mouse.y });
				if (nullptr == msgwnd)
					break;

				msgwnd->set_action(mouse_action::normal);
				if (msgwnd->flags.enabled)
				{
					auto retain = msgwnd->annex.events_ptr;

					::nana::arg_mouse arg;
					assign_arg(arg, msgwnd, message, pmdec);

					::nana::arg_click click_arg;

					//the window_handle of click_arg is used as a flag to determinate whether to emit click event
					click_arg.window_handle = nullptr;
					click_arg.mouse_args = &arg;

					if (msgwnd->dimension.is_hit(arg.pos))
					{
						msgwnd->set_action(mouse_action::hovered);
						if ((::nana::mouse::left_button == arg.button) && (pressed_wd == msgwnd))
						{
							click_arg.window_handle = msgwnd;
							draw_invoker(&drawer::click, msgwnd, click_arg, &context);
						}
					}

					//Do mouse_up, this handle may be closed by click handler.
					if (wd_manager.available(msgwnd) && msgwnd->flags.enabled)
					{
						arg.evt_code = event_code::mouse_up;
						draw_invoker(&drawer::mouse_up, msgwnd, arg, &context);

						if (click_arg.window_handle)
							retain->click.emit(click_arg, msgwnd);

						if (wd_manager.available(msgwnd))
						{
							arg.evt_code = event_code::mouse_up;
							retain->mouse_up.emit(arg, msgwnd);
						}
					}
					else if (click_arg.window_handle)
						retain->click.emit(click_arg, msgwnd);

					wd_manager.do_lazy_refresh(msgwnd, false);
				}
				pressed_wd = nullptr;
				break;
			case WM_MOUSEMOVE:
				//Ignore mouse events when a window has been pressed by pressing spacebar
				if (pressed_wd_space)
					break;

				msgwnd = wd_manager.find_window(native_window, {pmdec.mouse.x, pmdec.mouse.y});
				if (wd_manager.available(hovered_wd) && (msgwnd != hovered_wd))
				{
					brock.event_msleave(hovered_wd);
					hovered_wd->set_action(mouse_action::normal);
					hovered_wd = nullptr;

					//if msgwnd is neither captured window nor the child of captured window,
					//redirect the msgwnd to the captured window.
					auto wd = wd_manager.capture_redirect(msgwnd);
					if(wd)
						msgwnd = wd;
				}

				else if(msgwnd)
				{
					bool prev_captured_inside;
					if(wd_manager.capture_window_entered(pmdec.mouse.x, pmdec.mouse.y, prev_captured_inside))
					{
						event_code evt_code;
						if(prev_captured_inside)
						{
							evt_code = event_code::mouse_leave;
							msgwnd->set_action(mouse_action::normal_captured);
						}
						else
						{
							evt_code = event_code::mouse_enter;
							if (pressed_wd == msgwnd)
								msgwnd->set_action(mouse_action::pressed);
							else if (mouse_action::pressed != msgwnd->flags.action)
								msgwnd->set_action(mouse_action::hovered);
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
							msgwnd->set_action(mouse_action::pressed);
						else if (mouse_action::pressed != msgwnd->flags.action)
							msgwnd->set_action(mouse_action::hovered);

						hovered_wd = msgwnd;
						arg.evt_code = event_code::mouse_enter;
						brock.emit(event_code::mouse_enter, msgwnd, arg, true, &context);
					}

					if (hovered_wd)
					{
						arg.evt_code = event_code::mouse_move;
						brock.emit(event_code::mouse_move, msgwnd, arg, true, &context);
					}
					track.hwndTrack = native_window;
					restrict::track_mouse_event(&track);
				}
				if (!wd_manager.available(hovered_wd))
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

					//Ignore the message if the window is disabled.
					if ((pointer_wd == root_window) && ::IsWindowEnabled(root_window))
					{
						::ScreenToClient(pointer_wd, &scr_pos);
						auto scrolled_wd = wd_manager.find_window(reinterpret_cast<native_window_type>(pointer_wd), { scr_pos.x, scr_pos.y });

						def_window_proc = true;
						auto evt_wd = scrolled_wd;
						while (evt_wd)
						{
							if (evt_wd->annex.events_ptr->mouse_wheel.length() != 0)
							{
								def_window_proc = false;

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
							arg_wheel arg;
							arg.which = (WM_MOUSEHWHEEL == message ? arg_wheel::wheel::horizontal : arg_wheel::wheel::vertical);
							assign_arg(arg, scrolled_wd, pmdec);

							draw_invoker(&drawer::mouse_wheel, scrolled_wd, arg, &context);
							wd_manager.do_lazy_refresh(scrolled_wd, false);
						}
					}
					else if (pointer_wd != root_window)
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
					POINT mswin_pos;
					::DragQueryPoint(drop, &mswin_pos);

					const point pos{ mswin_pos.x, mswin_pos.y };

					msgwnd = wd_manager.find_window(native_window, pos);
					if(msgwnd)
					{
						arg_dropfiles dropfiles;

						std::unique_ptr<wchar_t[]> varbuf;
						std::size_t bufsize = 0;

						unsigned size = ::DragQueryFile(drop, 0xFFFFFFFF, nullptr, 0);
						for(unsigned i = 0; i < size; ++i)
						{
							unsigned reqlen = ::DragQueryFile(drop, i, nullptr, 0) + 1;
							if(bufsize < reqlen)
							{
								varbuf.reset(new wchar_t[reqlen]);
								bufsize = reqlen;
							}

							::DragQueryFile(drop, i, varbuf.get(), reqlen);
							dropfiles.files.emplace_back(varbuf.get());
						}

						while(msgwnd && (msgwnd->flags.dropable == false))
							msgwnd = msgwnd->parent;

						if(msgwnd)
						{
							dropfiles.pos = pos;

							wd_manager.calc_window_point(msgwnd, dropfiles.pos);
							dropfiles.window_handle = msgwnd;

							msgwnd->annex.events_ptr->mouse_dropfiles.emit(dropfiles, msgwnd);
							wd_manager.do_lazy_refresh(msgwnd, false);
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
					arg.window_handle = msgwnd;
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
					wd_manager.size(msgwnd, size(pmdec.size.width, pmdec.size.height), true, true);
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
							msgwnd->drawer.map(msgwnd, true, &update_area);
					}
					::EndPaint(root_window, &ps);
			    }
				break;
			case WM_SYSCHAR:
				def_window_proc = true;
				brock.shortkey_occurred(true);
				msgwnd = wd_manager.find_shortkey(native_window, static_cast<unsigned long>(wParam));
				if(msgwnd)
				{
					arg_keyboard arg;
					arg.evt_code = event_code::shortkey;
					arg.key = static_cast<wchar_t>(wParam < 0x61 ? wParam + 0x61 - 0x41 : wParam);
					arg.ctrl = arg.shift = false;
					arg.window_handle = msgwnd;
					arg.ignore = false;
					brock.emit(event_code::shortkey, msgwnd, arg, true, &context);
					def_window_proc = false;
				}
				break;
			case WM_SYSKEYDOWN:
				def_window_proc = true;
				if (brock.shortkey_occurred() == false)
				{
					msgwnd = msgwnd->root_widget->other.attribute.root->menubar;
					if (msgwnd)
					{
						bool focused = (brock.focus() == msgwnd);
						arg_keyboard arg;
						arg.evt_code = event_code::key_press;
						arg.window_handle = msgwnd;
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
				if (brock.shortkey_occurred(false) == false)
				{
					msgwnd = msgwnd->root_widget->other.attribute.root->menubar;
					if(msgwnd)
					{
						//Don't call default window proc to avoid pop-upping system menu.
						def_window_proc = false;

						bool set_focus = (brock.focus() != msgwnd) && (!msgwnd->root_widget->flags.ignore_menubar_focus);
						if (set_focus)
							wd_manager.set_focus(msgwnd, false, arg_focus::reason::general);

						arg_keyboard arg;
						arg.evt_code = event_code::key_release;
						arg.window_handle = msgwnd;
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
						if((VK_TAB == wParam) && (!msgwnd->visible || (false == (msgwnd->flags.tab & tab_type::eating)))) //Tab
						{
							bool is_forward = (::GetKeyState(VK_SHIFT) >= 0);

							auto tstop_wd = wd_manager.tabstop(msgwnd, is_forward);
							if (tstop_wd)
							{
								root_runtime->condition.ignore_tab = true;
								wd_manager.set_focus(tstop_wd, false, arg_focus::reason::tabstop);
								wd_manager.do_lazy_refresh(msgwnd, false);
								wd_manager.do_lazy_refresh(tstop_wd, true);
							}
						}
						else if ((VK_SPACE == wParam) && msgwnd->flags.space_click_enabled)
						{
							//Clicked by spacebar
							if ((nullptr == pressed_wd) && (nullptr == pressed_wd_space) && msgwnd->flags.enabled)
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
								arg.window_handle = msgwnd;

								msgwnd->set_action(mouse_action::pressed);

								pressed_wd_space = msgwnd;
								auto retain = msgwnd->annex.events_ptr;

								draw_invoker(&drawer::mouse_down, msgwnd, arg, &context);
								wd_manager.do_lazy_refresh(msgwnd, false);
							}
						}
						else
						{
							arg_keyboard arg;
							arg.evt_code = event_code::key_press;
							arg.window_handle = msgwnd;
							arg.ignore = false;
							arg.key = translate_virtual_key(wParam);
							brock.get_key_state(arg);
							brock.emit(event_code::key_press, msgwnd, arg, true, &context);

							if (msgwnd->root_widget->other.attribute.root->menubar == msgwnd)
							{
								//In order to keep the focus on the menubar, cancel the delay_restore
								//when pressing ESC to close the menu which is pop-upped by the menubar.
								//If no menu pop-upped by the menubar, it should enable delay restore to
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
					auto & wd_manager = brock.wd_manager();

					//Only accept tab when it is not ignored.
					if (VK_TAB != wParam || !root_runtime->condition.ignore_tab)
					{
						arg_keyboard arg;
						arg.evt_code = event_code::key_char;
						arg.window_handle = msgwnd;
						arg.key = static_cast<wchar_t>(wParam);
						brock.get_key_state(arg);
						arg.ignore = false;

						msgwnd->annex.events_ptr->key_char.emit(arg, msgwnd);
						if ((false == arg.ignore) && wd_manager.available(msgwnd))
							draw_invoker(&drawer::key_char, msgwnd, arg, &context);

						wd_manager.do_lazy_refresh(msgwnd, false);
					}
				}
				break;
			case WM_KEYUP:
				if(wParam != VK_MENU) //MUST NOT BE AN ALT
				{
					if (VK_TAB == wParam && root_runtime->condition.ignore_tab)
					{
						root_runtime->condition.ignore_tab = false;
					}
					else
					{
						msgwnd = brock.focus();
						if (msgwnd)
						{
							if ((msgwnd == pressed_wd_space) && msgwnd->flags.enabled)
							{
								msgwnd->set_action(mouse_action::normal);

								auto retain = msgwnd->annex.events_ptr;

								arg_click click_arg;
								click_arg.mouse_args = nullptr;
								click_arg.window_handle = msgwnd;

								arg_mouse arg;
								arg.alt = false;
								arg.button = ::nana::mouse::left_button;
								arg.ctrl = false;
								arg.evt_code = event_code::mouse_up;
								arg.left_button = true;
								arg.mid_button = false;
								arg.pos.x = 0;
								arg.pos.y = 0;
								arg.window_handle = msgwnd;

								draw_invoker(&drawer::mouse_up, msgwnd, arg, &context);

								if (brock.emit(event_code::click, msgwnd, click_arg, true, &context))
									wd_manager.do_lazy_refresh(msgwnd, false);
								
								pressed_wd_space = nullptr;
							}
							else
							{
								arg_keyboard keyboard_arg;
								keyboard_arg.evt_code = event_code::key_release;
								keyboard_arg.window_handle = msgwnd;
								keyboard_arg.key = translate_virtual_key(wParam);
								brock.get_key_state(keyboard_arg);
								keyboard_arg.ignore = false;

								brock.emit(event_code::key_release, msgwnd, keyboard_arg, true, &context);
							}
						}
					}
				}
				else
					brock.shortkey_occurred(false);

				//Do delay restore if key is not arrow_left/right/up/down, otherwise
				//A menubar will be restored if the item is empty(not have a menu item)
				if (wParam < 37 || 40 < wParam)
					brock.delay_restore(2);	//Restores while key release
				break;
			case WM_CLOSE:
			{
				arg_unload arg;
				arg.window_handle = msgwnd;
				arg.cancel = false;
				brock.emit(event_code::unload, msgwnd, arg, true, &context);
				if (!arg.cancel)
				{
					def_window_proc = true;
					//Activates its owner, refer to the window_manager::close for the explanation
					if (msgwnd->flags.modal || (msgwnd->owner == 0) || msgwnd->owner->flags.take_active)
						native_interface::activate_owner(msgwnd->root);
				}
				break;
			}
			case WM_DESTROY:
				if (msgwnd->root == brock.get_menu())
				{
					brock.erase_menu(false);
					brock.delay_restore(3);	//Restores if delay_restore not declared
				}
				wd_manager.destroy(msgwnd);
				nana::detail::platform_spec::instance().release_window_icon(msgwnd->root);
				break;
			case WM_NCDESTROY:
				brock.manage_form_loader(msgwnd, false);
				wd_manager.destroy_handle(msgwnd);

				if(--context.window_count <= 0)
				{
					::PostQuitMessage(0);
					def_window_proc = true;
				}
				break;
			default:
				def_window_proc = true;
			}

			wd_manager.update_requesters(root_wd);

			root_runtime = wd_manager.root_runtime(native_window);
			if(root_runtime)
			{
				context.event_window = pre_event_window;
				root_runtime->condition.pressed = pressed_wd;
				root_runtime->condition.hovered = hovered_wd;
				root_runtime->condition.pressed_by_space = pressed_wd_space;
			}
			else
			{
				auto context = brock.get_thread_context();
				if(context) context->event_window = pre_event_window;
			}

			if (!def_window_proc)
				return 0;
		}

		return ::DefWindowProc(root_window, message, wParam, lParam);
	}

	void bedrock::get_key_state(arg_keyboard& kb)
	{
		kb.alt = (0 != (::GetKeyState(VK_MENU) & 0x80));
		kb.ctrl = (0 != (::GetKeyState(VK_CONTROL) & 0x80));
		kb.shift = (0 != (::GetKeyState(VK_SHIFT) & 0x80));
	}

	void bedrock::delete_platform_assoc(window_platform_assoc* passoc)
	{
		delete passoc;
	}

	//Generates an identifier for an accel key.
	std::pair<int, WORD> id_accel_key(const accel_key& key)
	{
		std::pair<int, WORD> ret;

		//Use virt-key for non-case sensitive
		if (!key.case_sensitive)
			ret.second = static_cast<WORD>(std::tolower(key.key) - 'a' + 0x41);

		ret.first = ret.second | int(key.case_sensitive ? (1 << 8) : 0) | int(key.alt ? (1 << 9) : 0) | int(key.ctrl ? (1 << 10) : 0) | int(key.shift ? (1 << 11) : 0);
		return ret;
	}

	void bedrock::keyboard_accelerator(native_window_type wd, const accel_key& key, const std::function<void()>& fn)
	{
		auto misc = wd_manager().root_runtime(wd);
		if (nullptr == misc)
			return;
		
		if (!misc->wpassoc)
			misc->wpassoc = new window_platform_assoc;

		auto idkey = id_accel_key(key);

		misc->wpassoc->accel_commands[idkey.first] = fn;

		auto accel_size = ::CopyAcceleratorTable(misc->wpassoc->accel, nullptr, 0);

		std::unique_ptr<ACCEL[]> accels(new ACCEL[accel_size + 1]);
		
		if (accel_size)
			::CopyAcceleratorTable(misc->wpassoc->accel, accels.get(), accel_size);

		auto p = accels.get() + accel_size;
		p->cmd = idkey.first;
		p->fVirt = (key.case_sensitive ? 0 : FVIRTKEY) | (key.alt ? FALT : 0) | (key.ctrl ? FCONTROL : 0) | (key.shift ? FSHIFT : 0);
		p->key = idkey.second;

		::DestroyAcceleratorTable(misc->wpassoc->accel);
		misc->wpassoc->accel = ::CreateAcceleratorTable(accels.get(), accel_size + 1);
	}

	element_store& bedrock::get_element_store() const
	{
		return impl_->estore;
	}

	void bedrock::map_through_widgets(basic_window* wd, native_drawable_type drawable)
	{
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

	//Dynamically set a cursor for a window
	void bedrock::set_cursor(basic_window* wd, nana::cursor cur, thread_context* thrd)
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

		auto this_cur = reinterpret_cast<HCURSOR>(
#ifdef _WIN64
			::GetClassLongPtr(reinterpret_cast<HWND>(wd->root), GCLP_HCURSOR)
#else
			::GetClassLong(reinterpret_cast<HWND>(wd->root), GCL_HCURSOR)
#endif
			);

		if(this_cur != thrd->cursor.handle)
		{
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

	void bedrock::define_state_cursor(basic_window* wd, nana::cursor cur, thread_context* thrd)
	{
		wd->root_widget->other.attribute.root->state_cursor = cur;
		wd->root_widget->other.attribute.root->state_cursor_window = wd;
		set_cursor(wd, cur, thrd);
		auto cur_handle = ::LoadCursor(nullptr, translate(cur));
		::SetCursor(cur_handle);
		::ShowCursor(TRUE);
	}

	void bedrock::undefine_state_cursor(basic_window * wd, thread_context* thrd)
	{
		HCURSOR rev_handle = ::LoadCursor(nullptr, IDC_ARROW);
		if (!wd_manager().available(wd))
		{
			::ShowCursor(FALSE);
			::SetCursor(rev_handle);
			return;
		}

		if (nullptr == thrd)
			thrd = get_thread_context(wd->thread_id);

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
		auto rev_wd = wd_manager().find_window(native_handle, pos);
		if (rev_wd)
		{
			set_cursor(rev_wd, rev_wd->predef_cursor, thrd);
			rev_handle = thrd->cursor.handle;
		}
		::ShowCursor(FALSE);
		::SetCursor(rev_handle);
	}
}//end namespace detail
}//end namespace nana
#endif //NANA_WINDOWS
