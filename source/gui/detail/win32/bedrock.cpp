/*
 *	A Bedrock Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2014 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/detail/win32/bedrock.cpp
 */

#include <nana/config.hpp>

#include PLATFORM_SPEC_HPP
#include GUI_BEDROCK_HPP
#include <nana/gui/detail/event_code.hpp>
#include <nana/system/platform.hpp>
#include <sstream>
#include <nana/system/timepiece.hpp>
#include <nana/gui/wvl.hpp>
#include <nana/gui/detail/inner_fwd_implement.hpp>
#include <nana/gui/detail/native_window_interface.hpp>
#include <nana/gui/layout_utility.hpp>
#include <nana/gui/detail/element_store.hpp>

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

	//class internal_scope_guard
		internal_scope_guard::internal_scope_guard()
		{
			detail::bedrock::instance().wd_manager.internal_lock().lock();
		}
		internal_scope_guard::~internal_scope_guard()
		{
			detail::bedrock::instance().wd_manager.internal_lock().unlock();
		}
	//end class internal_scope_guard
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
			nana::char_t keychar;
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

		element_store estore;

		struct cache_type
		{
			struct thread_context_cache
			{
				unsigned tid = 0;
				thread_context *object = nullptr;
			}tcontext;
		}cache;

		struct menu_tag
		{
			core_window_t*	taken_window	= nullptr;
			native_window_type window		= nullptr;
			native_window_type owner		= nullptr;
			bool has_keyboard	= false;
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
		:impl_(new private_impl)
	{
		nana::detail::platform_spec::instance(); //to guaranty the platform_spec object is initialized before using.


		WNDCLASSEX wincl;
		wincl.hInstance = ::GetModuleHandle(0);
		wincl.lpszClassName = STR("NanaWindowInternal");
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
		if(wd_manager.number_of_core_window())
		{
			std::stringstream ss;
			ss<<"Nana.GUI detects a memory leaks in window_manager, "<<static_cast<unsigned>(wd_manager.number_of_core_window())<<" window(s) are not uninstalled.";
			::MessageBoxA(0, ss.str().c_str(), ("Nana C++ Library"), MB_OK);
		}

		if(evt_operation.size())
		{
			std::stringstream ss;
			ss<<"Nana.GUI detects a memory leaks in events operation, "<<static_cast<unsigned>(evt_operation.size())<<" event(s) are not uninstalled.";
			::MessageBoxA(0, ss.str().c_str(), ("Nana C++ Library"), MB_OK);
		}
		delete impl_;
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

	void bedrock::map_thread_root_buffer(core_window_t* wd)
	{
		::PostMessage(reinterpret_cast<HWND>(wd->root), nana::detail::messages::map_thread_root_buffer, reinterpret_cast<WPARAM>(wd), 0);
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
		thread_context * context = this->open_thread_context(tid);
		if(0 == context->window_count)
		{
			//test if there is not a window
			//GetMessage may block if there is not a window
			remove_thread_context();
			return;
		}

		++(context->event_pump_ref_count);

		auto & intr_locker = wd_manager.internal_lock();
		intr_locker.revert();

		try
		{
			MSG msg;
			if(modal_window)
			{
				HWND native_handle = reinterpret_cast<HWND>(
									root(reinterpret_cast<core_window_t*>(modal_window)));
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

							if ((msg.message == WM_CHAR || msg.message == WM_KEYDOWN || msg.message == WM_KEYUP) || !::IsDialogMessage(native_handle, &msg))
							{
								auto menu_wd = get_menu(reinterpret_cast<native_window_type>(msg.hwnd), true);
								if (menu_wd) interior_helper_for_menu(msg, menu_wd);

								::TranslateMessage(&msg);
								::DispatchMessage(&msg);

								wd_manager.remove_trash_handle(tid);
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

						wd_manager.remove_trash_handle(tid);
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

					wd_manager.remove_trash_handle(tid);
				}//end while

				//Empty these rest messages, there is not a window to process these messages.
				while(::PeekMessage(&msg, 0, 0, 0, PM_REMOVE));
			}
		}
        catch(std::exception& e)
        {
             (msgbox(modal_window, STR("An uncaptured std::exception during message pumping: ")).icon(msgbox::icon_information)
                                 <<STR("\n   in form: ") << API::window_caption(modal_window)
                                 <<STR("\n   exception : ") << e.what() 
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
			(msgbox(modal_window, STR("An exception during message pumping!")).icon(msgbox::icon_information)
				<< STR("An uncaptured non-std exception during message pumping!")
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
		//event code
		switch (msg)
		{
		case WM_LBUTTONUP: case WM_RBUTTONUP: case WM_MBUTTONUP:
			arg.evt_code = event_code::mouse_up;
			break;
		case WM_LBUTTONDBLCLK: case WM_MBUTTONDBLCLK: case WM_RBUTTONDBLCLK:
			arg.evt_code = (wd->flags.dbl_click ? event_code::dbl_click : event_code::mouse_down);
			break;
		case WM_MOUSEMOVE:
			arg.evt_code = event_code::mouse_move;
			break;
		}

		//event arguments
		switch (msg)
		{
		case WM_LBUTTONDOWN: case WM_RBUTTONDOWN: case WM_MBUTTONDOWN:
			arg.evt_code = event_code::mouse_down;
		case WM_LBUTTONUP: case WM_RBUTTONUP: case WM_MBUTTONUP:
		case WM_LBUTTONDBLCLK: case WM_MBUTTONDBLCLK: case WM_RBUTTONDBLCLK:
		case WM_MOUSEMOVE:
			arg.pos.x = pmdec.mouse.x - wd->pos_root.x;
			arg.pos.y = pmdec.mouse.y - wd->pos_root.y;
			arg.shift = pmdec.mouse.button.shift;
			arg.ctrl = pmdec.mouse.button.ctrl;

			switch (msg)
			{
			case WM_LBUTTONUP: case WM_RBUTTONUP: case WM_MBUTTONUP:
				arg.left_button = (WM_LBUTTONUP == msg);
				arg.right_button = (WM_RBUTTONUP == msg);
				arg.mid_button = (WM_MBUTTONUP == msg);
				break;
			default:
				arg.left_button = pmdec.mouse.button.left;
				arg.mid_button = pmdec.mouse.button.middle;
				arg.right_button = pmdec.mouse.button.right;
			}
			break;
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
		arg.distance = std::abs(pmdec.mouse.button.wheel_delta);

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
			bedrock.wd_manager.map(reinterpret_cast<bedrock::core_window_t*>(wParam));
			::UpdateWindow(wd);
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
		if (bedrock::instance().wd_manager.available(wd) == false)
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
		auto* root_runtime = brock.wd_manager.root_runtime(native_window);

		if(root_runtime)
		{
			bool def_window_proc = false;
			auto& context = *brock.get_thread_context();

			auto pressed_wd = root_runtime->condition.pressed;
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
				else if((reinterpret_cast<WINDOWPOS*>(lParam)->flags & SWP_HIDEWINDOW) && msgwnd->visible)
					brock.event_expose(msgwnd, false);

				def_window_proc = true;
				break;
			case WM_SETFOCUS:
				if(msgwnd->flags.enabled && msgwnd->flags.take_active)
				{
					auto focus = msgwnd->other.attribute.root->focus;

					if(focus && focus->together.caret)
						focus->together.caret->set_active(true);

					msgwnd->root_widget->other.attribute.root->context.focus_changed = true;

					arg_focus arg;
					assign_arg(arg, focus, native_window, true);
					if (!brock.emit(event_code::focus, focus, arg, true, &context))
						brock.wd_manager.set_focus(msgwnd, true);
				}
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
				//focus_changed means that during an event procedure if the focus is changed
				if(brock.wd_manager.available(msgwnd))
					msgwnd->root_widget->other.attribute.root->context.focus_changed = true;
				break;
			case WM_MOUSEACTIVATE:
				if(msgwnd->flags.take_active == false)
					return MA_NOACTIVATE;
				break;
			case WM_LBUTTONDBLCLK: case WM_MBUTTONDBLCLK: case WM_RBUTTONDBLCLK:
				pressed_wd = nullptr;
				msgwnd = brock.wd_manager.find_window(native_window, pmdec.mouse.x, pmdec.mouse.y);
				if(msgwnd && msgwnd->flags.enabled)
				{
					if(msgwnd->flags.take_active)
						brock.wd_manager.set_focus(msgwnd, false);

					arg_mouse arg;
					assign_arg(arg, msgwnd, message, pmdec);
					if (brock.emit(arg.evt_code, msgwnd, arg, true, &context))
					{
						if (brock.wd_manager.available(msgwnd))
							pressed_wd = msgwnd;
					}
				}
				break;
			case WM_NCLBUTTONDOWN: case WM_NCMBUTTONDOWN: case WM_NCRBUTTONDOWN:
				brock.close_menu_if_focus_other_window(native_window);
				def_window_proc = true;
				break;
			case WM_LBUTTONDOWN: case WM_MBUTTONDOWN: case WM_RBUTTONDOWN:
				msgwnd = brock.wd_manager.find_window(native_window, pmdec.mouse.x, pmdec.mouse.y);
				if(nullptr == msgwnd)	break;

				//if event on the menubar, just remove the menu if it is not associating with the menubar
				if((msgwnd == msgwnd->root_widget->other.attribute.root->menubar) && brock.get_menu(msgwnd->root, true))
					brock.remove_menu();
				else
					brock.close_menu_if_focus_other_window(msgwnd->root);

				if(msgwnd->flags.enabled)
				{
					pressed_wd = msgwnd;
					auto new_focus = (msgwnd->flags.take_active ? msgwnd : msgwnd->other.active_window);
					if(new_focus)
					{
						auto kill_focus = brock.wd_manager.set_focus(new_focus, false);
						if(kill_focus != new_focus)
						{
							brock.wd_manager.do_lazy_refresh(kill_focus, false);
							msgwnd->root_widget->other.attribute.root->context.focus_changed = false;
						}
					}

					arg_mouse arg;
					assign_arg(arg, msgwnd, message, pmdec);
					msgwnd->flags.action = mouse_action::pressed;
					if (brock.emit(event_code::mouse_down, msgwnd, arg, true, &context))
					{
						//If a root_window is created during the mouse_down event, Nana.GUI will ignore the mouse_up event.
						if(msgwnd->root_widget->other.attribute.root->context.focus_changed)
						{
							auto pos = native_interface::cursor_position();
							auto rootwd = native_interface::find_window(pos.x, pos.y);
							native_interface::calc_window_point(rootwd, pos);
							if(msgwnd != brock.wd_manager.find_window(rootwd, pos.x, pos.y))
							{
								//call the drawer mouse up event for restoring the surface graphics
								msgwnd->flags.action = mouse_action::normal;

								arg.evt_code = event_code::mouse_up;
								emit_drawer(&drawer::mouse_up, msgwnd, arg, &context);
								brock.wd_manager.do_lazy_refresh(msgwnd, false);
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
				msgwnd = brock.wd_manager.find_window(native_window, pmdec.mouse.x, pmdec.mouse.y);
				if(nullptr == msgwnd)
					break;

				msgwnd->flags.action = mouse_action::normal;
				if(msgwnd->flags.enabled)
				{
					nana::arg_mouse arg;
					assign_arg(arg, msgwnd, message, pmdec);

					const bool hit = msgwnd->dimension.is_hit(arg.pos);

					bool fire_click = false;
					if(msgwnd == pressed_wd)
					{
						if(msgwnd->flags.enabled && hit)
						{
							msgwnd->flags.action = mouse_action::over;
							arg.evt_code = event_code::click;
							emit_drawer(&drawer::click, msgwnd, arg, &context);
							fire_click = true;
						}
					}

					//Do mouse_up, this handle may be closed by click handler.
					if(brock.wd_manager.available(msgwnd) && msgwnd->flags.enabled)
					{
						if(hit)
							msgwnd->flags.action = mouse_action::over;

						arg.evt_code = event_code::mouse_up;
						emit_drawer(&drawer::mouse_up, msgwnd, arg, &context);

						auto evt_ptr = msgwnd->together.events_ptr;

						if (fire_click)
						{
							arg.evt_code = event_code::click;
							msgwnd->together.attached_events->click.emit(arg);
						}

						if (brock.wd_manager.available(msgwnd))
						{
							arg.evt_code = event_code::mouse_up;
							msgwnd->together.attached_events->mouse_up.emit(arg);
						}
					}
					else if (fire_click)
					{
						arg.evt_code = event_code::click;
						msgwnd->together.attached_events->click.emit(arg);
					}
					brock.wd_manager.do_lazy_refresh(msgwnd, false);
				}
				pressed_wd = nullptr;
				break;
			case WM_MOUSEMOVE:
				msgwnd = brock.wd_manager.find_window(native_window, pmdec.mouse.x, pmdec.mouse.y);
				if (brock.wd_manager.available(hovered_wd) && (msgwnd != hovered_wd))
				{
					brock.event_msleave(hovered_wd);
					hovered_wd->flags.action = mouse_action::normal;
					hovered_wd = nullptr;

					//if msgwnd is neither captured window nor the child of captured window,
					//redirect the msgwnd to the captured window.
					auto wd = brock.wd_manager.capture_redirect(msgwnd);
					if(wd)
						msgwnd = wd;
				}

				else if(msgwnd)
				{
					bool prev_captured_inside;
					if(brock.wd_manager.capture_window_entered(pmdec.mouse.x, pmdec.mouse.y, prev_captured_inside))
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
					msgwnd->flags.action = mouse_action::over;
					if (hovered_wd != msgwnd)
					{
						hovered_wd = msgwnd;
						arg.evt_code = event_code::mouse_enter;
						brock.emit(event_code::mouse_enter, msgwnd, arg, true, &context);
					}

					arg.evt_code = event_code::mouse_move;
					brock.emit(event_code::mouse_move, msgwnd, arg, true, &context);
					track.hwndTrack = native_window;
					restrict::track_mouse_event(&track);
				}
				if (!brock.wd_manager.available(hovered_wd))
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
					::POINT scr_pos{ int(LOWORD(lParam)), int(HIWORD(lParam)) };	//Screen position
					auto pointer_wd = ::WindowFromPoint(scr_pos);
					if (pointer_wd == root_window)
					{
						::ScreenToClient(pointer_wd, &scr_pos);
						auto scrolled_wd = brock.wd_manager.find_window(reinterpret_cast<native_window_type>(pointer_wd), scr_pos.x, scr_pos.y);

						def_window_proc = true;
						while (scrolled_wd)
						{
							if (scrolled_wd->together.attached_events->mouse_wheel.length() != 0)
							{
								def_window_proc = false;
								nana::point mspos{ scr_pos.x, scr_pos.y };
								brock.wd_manager.calc_window_point(scrolled_wd, mspos);

								arg_wheel arg;
								arg.which = (WM_MOUSEHWHEEL == message ? arg_wheel::wheel::horizontal : arg_wheel::wheel::vertical);
								assign_arg(arg, scrolled_wd, pmdec);
								brock.emit(event_code::mouse_wheel, scrolled_wd, arg, true, &context);
								break;
							}
							scrolled_wd = scrolled_wd->parent;
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

					msgwnd = brock.wd_manager.find_window(native_window, pos.x, pos.y);
					if(msgwnd)
					{
						arg_dropfiles dropfiles;

						std::unique_ptr<nana::char_t[]> varbuf;
						std::size_t bufsize = 0;

						unsigned size = ::DragQueryFile(drop, 0xFFFFFFFF, 0, 0);
						for(unsigned i = 0; i < size; ++i)
						{
							unsigned reqlen = ::DragQueryFile(drop, i, 0, 0) + 1;
							if(bufsize < reqlen)
							{
								varbuf.reset(new nana::char_t[reqlen]);
								bufsize = reqlen;
							}

							::DragQueryFile(drop, i, varbuf.get(), reqlen);
							dropfiles.files.emplace_back(varbuf.get());
						}

						while(msgwnd && (msgwnd->flags.dropable == false))
							msgwnd = msgwnd->parent;

						if(msgwnd)
						{
							dropfiles.pos.x = pos.x;
							dropfiles.pos.y = pos.y;

							brock.wd_manager.calc_window_point(msgwnd, dropfiles.pos);
							dropfiles.window_handle = reinterpret_cast<window>(msgwnd);

							msgwnd->together.attached_events->mouse_dropfiles.emit(dropfiles);
							brock.wd_manager.do_lazy_refresh(msgwnd, false);
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
					brock.wd_manager.size(msgwnd, size(pmdec.size.width, pmdec.size.height), true, true);
				break;
			case WM_MOVE:
				brock.event_move(msgwnd, (int)(short) LOWORD(lParam), (int)(short) HIWORD(lParam));
				break;
			case WM_PAINT:
				{
					::PAINTSTRUCT ps;
					::HDC dc = ::BeginPaint(root_window, &ps);

					if((ps.rcPaint.left != ps.rcPaint.right) && (ps.rcPaint.bottom != ps.rcPaint.top))
					{
						::BitBlt(dc,
								ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top,
								reinterpret_cast<HDC>(msgwnd->root_graph->handle()->context),
								ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);
					}
					::EndPaint(root_window, &ps);
				}
				break;
			case WM_SYSCHAR:
				brock.set_keyboard_shortkey(true);
				msgwnd = brock.wd_manager.find_shortkey(native_window, static_cast<unsigned long>(wParam));
				if(msgwnd)
				{
					arg_keyboard arg;
					arg.evt_code = event_code::shortkey;
					arg.key = static_cast<wchar_t>(wParam < 0x61 ? wParam + 0x61 - 0x41 : wParam);
					arg.ctrl = arg.shift = false;
					arg.window_handle = reinterpret_cast<window>(msgwnd);
					arg.ignore = false;
					brock.emit(event_code::shortkey, msgwnd, arg, true, &context);
				}
				break;
			case WM_SYSKEYDOWN:
				if(brock.whether_keyboard_shortkey() == false)
				{
					msgwnd = msgwnd->root_widget->other.attribute.root->menubar;
					if(msgwnd)
					{
						brock.wd_manager.set_focus(msgwnd, false);

						arg_keyboard arg;
						arg.evt_code = event_code::key_press;
						arg.window_handle = reinterpret_cast<window>(msgwnd);
						arg.ignore = false;
						arg.key = static_cast<nana::char_t>(wParam);
						brock.get_key_state(arg);
						brock.emit(event_code::key_press, msgwnd, arg, true, &context);
					}
					else if(brock.get_menu())
						brock.remove_menu();
				}
				break;
			case WM_SYSKEYUP:
				if(brock.set_keyboard_shortkey(false) == false)
				{
					msgwnd = msgwnd->root_widget->other.attribute.root->menubar;
					if(msgwnd)
					{
						arg_keyboard arg;
						arg.evt_code = event_code::key_release;
						arg.window_handle = reinterpret_cast<window>(msgwnd);
						arg.ignore = false;
						arg.key = static_cast<nana::char_t>(wParam);
						brock.get_key_state(arg);
						brock.emit(event_code::key_release, msgwnd, arg, true, &context);
					}
				}
				break;
			case WM_KEYDOWN:
				if(msgwnd->flags.enabled)
				{
					if(msgwnd->root != brock.get_menu())
						msgwnd = brock.focus();

					if(msgwnd)
					{
						if((wParam == 9) && (false == (msgwnd->flags.tab & tab_type::eating))) //Tab
						{
							auto the_next = brock.wd_manager.tabstop(msgwnd, true);
							if(the_next)
							{
								brock.wd_manager.set_focus(the_next, false);
								brock.wd_manager.do_lazy_refresh(msgwnd, false);
								brock.wd_manager.do_lazy_refresh(the_next, true);
								root_runtime->condition.tabstop_focus_changed = true;
							}
						}
						else
						{
							arg_keyboard arg;
							arg.evt_code = event_code::key_press;
							arg.window_handle = reinterpret_cast<window>(msgwnd);
							arg.ignore = false;
							arg.key = static_cast<nana::char_t>(wParam);
							brock.get_key_state(arg);
							brock.emit(event_code::key_press, msgwnd, arg, true, &context);
						}
					}
				}
				break;
			case WM_CHAR:
				msgwnd = brock.focus();
				if(false == root_runtime->condition.tabstop_focus_changed)
				{
					if(msgwnd && msgwnd->flags.enabled)
					{
						arg_keyboard arg;
						arg.evt_code = event_code::key_char;
						arg.window_handle = reinterpret_cast<window>(msgwnd);
						arg.key = static_cast<nana::char_t>(wParam);
						brock.get_key_state(arg);
						arg.ignore = false;

						msgwnd->together.attached_events->key_char.emit(arg);
						if ((false == arg.ignore) && brock.wd_manager.available(msgwnd))
							brock.emit_drawer(event_code::key_char, msgwnd, arg, &context);

						brock.wd_manager.do_lazy_refresh(msgwnd, false);
					}
				}
				else
					root_runtime->condition.tabstop_focus_changed = false;
				return 0;
			case WM_KEYUP:
				if(wParam != 18) //MUST NOT BE AN ALT
				{
					msgwnd = brock.focus();
					if(msgwnd)
					{
						arg_keyboard arg;
						arg.evt_code = event_code::key_release;
						arg.window_handle = reinterpret_cast<window>(msgwnd);
						arg.key = static_cast<nana::char_t>(wParam);
						brock.get_key_state(arg);
						arg.ignore = false;
						brock.emit(event_code::key_release, msgwnd, arg, true, &context);
					}
				}
				else
					brock.set_keyboard_shortkey(false);
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
				if(msgwnd->root == brock.get_menu())
					brock.empty_menu();
				brock.wd_manager.destroy(msgwnd);

				nana::detail::platform_spec::instance().release_window_icon(msgwnd->root);
				break;
			case WM_NCDESTROY:
				brock.rt_manager.remove_if_exists(msgwnd);
				brock.wd_manager.destroy_handle(msgwnd);

				if(--context.window_count <= 0)
				{
					::PostQuitMessage(0);
					def_window_proc = true;
				}
				break;
			default:
				def_window_proc = true;
			}

			root_runtime = brock.wd_manager.root_runtime(native_window);
			if(root_runtime)
			{
				root_runtime->condition.pressed = pressed_wd;
				root_runtime->condition.hovered = hovered_wd;
			}

			if (!def_window_proc)
				return 0;
		}

		return ::DefWindowProc(root_window, message, wParam, lParam);
	}

	nana::category::flags bedrock::category(core_window_t* wd)
	{
		if(wd)
		{
			internal_scope_guard isg;
			if(wd_manager.available(wd))
				return wd->other.category;
		}
		return nana::category::flags::super;
	}

	auto bedrock::focus() ->core_window_t*
	{
		core_window_t* wd = wd_manager.root(native_interface::get_focus_window());
		return (wd ? wd->other.attribute.root->focus : nullptr);
	}

	native_window_type bedrock::root(core_window_t* wd)
	{
		if(wd)
		{
			internal_scope_guard isg;
			if(wd_manager.available(wd))
				return wd->root;
		}
		return nullptr;
	}

	void bedrock::set_menubar_taken(core_window_t* wd)
	{
		impl_->menu.taken_window = wd;
	}

	bedrock::core_window_t* bedrock::get_menubar_taken()
	{
		core_window_t* wd = impl_->menu.taken_window;
		impl_->menu.taken_window = nullptr;
		return wd;
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
			remove_menu();
			return true;
		}
		return false;
	}

	void bedrock::set_menu(native_window_type menu_wd, bool has_keyboard)
	{
		if(menu_wd && impl_->menu.window != menu_wd)
		{
			remove_menu();

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

	void bedrock::remove_menu()
	{
		if(impl_->menu.window)
		{
			auto delwin = impl_->menu.window;
			impl_->menu.window = impl_->menu.owner = nullptr;
			impl_->menu.has_keyboard = false;
			native_interface::close_window(delwin);
		}
	}

	void bedrock::empty_menu()
	{
		if(impl_->menu.window)
		{
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

	bool bedrock::emit(event_code evt_code, core_window_t* wd, const arg_mouse& arg, bool ask_update, thread_context* thrd)
	{
		if (evt_code != arg.evt_code)
			throw std::runtime_error("Nana.bedrock: invalid event arg.");

		return emit(evt_code, wd, static_cast<const ::nana::detail::event_arg_interface&>(arg), ask_update, thrd);
	}

	bool bedrock::emit(event_code evt_code, core_window_t* wd, const ::nana::detail::event_arg_interface& arg, bool ask_update, thread_context* thrd)
	{
		if (wd_manager.available(wd) == false)
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
			wd_manager.do_lazy_refresh(wd, false);
		else if (wd_manager.available(wd))
			wd->other.upd_state = basic_window::update_state::none;

		if (thrd)	thrd->event_window = prev_event_wd;
		return true;
	}

	bool bedrock::emit_drawer(event_code evt_code, core_window_t* wd, const ::nana::detail::event_arg_interface& arg, thread_context* thrd)
	{
		if (bedrock_object.wd_manager.available(wd) == false)
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

	const nana::char_t* translate(cursor id)
	{
		const nana::char_t* name = IDC_ARROW;

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
		if (!wd_manager.available(wd))
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
		auto rev_wd = wd_manager.find_window(native_handle, pos.x, pos.y);
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
