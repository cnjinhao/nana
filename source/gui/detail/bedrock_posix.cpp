/*
 *	A Bedrock Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://nanapro.sourceforge.net/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/detail/linux_X11/bedrock.cpp
 */

#include <nana/detail/platform_spec_selector.hpp>
#if defined(NANA_POSIX) && defined(NANA_X11)
#include <nana/gui/detail/bedrock_pi_data.hpp>
#include <nana/gui/detail/event_code.hpp>
#include <nana/system/platform.hpp>
#include <nana/gui/detail/inner_fwd_implement.hpp>
#include <nana/gui/detail/native_window_interface.hpp>
#include <nana/gui/layout_utility.hpp>
#include <nana/gui/detail/element_store.hpp>
#include <errno.h>
#include <algorithm>

namespace nana
{
namespace detail
{
#pragma pack(1)
		union event_mask
		{
			struct
			{
				short x;
				short y;
			}pos;

			struct
			{
				short width;
				short height;
			}size;

			struct
			{
				unsigned short vkey;
				short delta;
			}wheel;
		};
#pragma pack()

	struct bedrock::thread_context
	{
		unsigned event_pump_ref_count{0};

		int		window_count{0};	//The number of windows
		core_window_t* event_window{nullptr};
		bool	is_alt_pressed{false};
		bool	is_ctrl_pressed{false};

		struct platform_detail_tag
		{
			native_window_type	motion_window;
			nana::point		motion_pointer_pos;
		}platform;

		struct cursor_tag
		{
			core_window_t * window;
			native_window_type native_handle;
			nana::cursor predef_cursor;
			Cursor handle;
		}cursor;

		thread_context()
		{
			cursor.window = nullptr;
			cursor.native_handle = nullptr;
			cursor.predef_cursor = nana::cursor::arrow;
			cursor.handle = 0;
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
			bool	has_keyboard{ false };
		}menu;

		struct keyboard_tracking_state_tag
		{
			keyboard_tracking_state_tag()
				:has_shortkey_occured(false), has_keyup(true), alt(0)
			{}

			bool has_shortkey_occured;
			bool has_keyup;

			unsigned long alt : 2;
		}keyboard_tracking_state;
	};

	void timer_proc(unsigned);
	void window_proc_dispatcher(Display*, nana::detail::msg_packet_tag&);
	void window_proc_for_packet(Display *, nana::detail::msg_packet_tag&);
	void window_proc_for_xevent(Display*, XEvent&);

	//class bedrock defines a static object itself to implement a static singleton
	//here is the definition of this object
	bedrock bedrock::bedrock_object;

	Window event_window(const XEvent& event)
	{
		switch(event.type)
		{
		case MapNotify:
		case UnmapNotify:
		case DestroyNotify:
			return event.xmap.window;
		}
		return event.xkey.window;
	}

	bedrock::bedrock()
		: pi_data_(new pi_data), impl_(new private_impl)
	{
		nana::detail::platform_spec::instance().msg_set(timer_proc, window_proc_dispatcher);
	}

	bedrock::~bedrock()
	{
		delete pi_data_;
		delete impl_;
	}

	void bedrock::map_thread_root_buffer(core_window_t*, bool forced, const rectangle*)
	{
		//GUI in X11 is thread-independent, so no implementation.
	}

	//inc_window
	//@biref: increament the number of windows
	int bedrock::inc_window(unsigned tid)
	{
		private_impl * impl = instance().impl_;
		std::lock_guard<decltype(impl->mutex)> lock(impl->mutex);

		int & cnt = (impl->thr_contexts[tid ? tid : nana::system::this_thread_id()].window_count);
		return (cnt < 0 ? cnt = 1 : ++cnt);
	}

	bedrock::thread_context* bedrock::open_thread_context(unsigned tid)
	{
		if(0 == tid) tid = nana::system::this_thread_id();

		std::lock_guard<decltype(impl_->mutex)> lock(impl_->mutex);
		if(impl_->cache.tcontext.tid == tid)
			return impl_->cache.tcontext.object;

		bedrock::thread_context* context = nullptr;

		private_impl::thr_context_container::iterator i = impl_->thr_contexts.find(tid);
		if(i == impl_->thr_contexts.end())
			context = &(impl_->thr_contexts[tid]);
		else
			context = &(i->second);

		impl_->cache.tcontext.tid = tid;
		impl_->cache.tcontext.object = context;
		return context;
	}

	bedrock::thread_context* bedrock::get_thread_context(unsigned tid)
	{
		if(0 == tid) tid = nana::system::this_thread_id();

		std::lock_guard<decltype(impl_->mutex)> lock(impl_->mutex);
		if(impl_->cache.tcontext.tid == tid)
			return impl_->cache.tcontext.object;

		private_impl::thr_context_container::iterator i = impl_->thr_contexts.find(tid);
		if(i != impl_->thr_contexts.end())
		{
			impl_->cache.tcontext.tid = tid;
			return (impl_->cache.tcontext.object = &(i->second));
		}

		impl_->cache.tcontext.tid = 0;
		return 0;
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

	category::flags bedrock::category(bedrock::core_window_t* wd)
	{
		if(wd)
		{
			internal_scope_guard lock;
			if(wd_manager().available(wd))
				return wd->other.category;
		}
		return category::flags::super;
	}

	bedrock::core_window_t* bedrock::focus()
	{
		core_window_t* wd = wd_manager().root(native_interface::get_focus_window());
		return (wd ? wd->other.attribute.root->focus : 0);
	}

	void bedrock::set_menubar_taken(core_window_t* wd)
	{
		auto pre = impl_->menu.taken_window;
		impl_->menu.taken_window = wd;

		//assigning of a nullptr taken window is to restore the focus of pre taken
		if ((!wd) && pre)
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

	void bedrock::set_menu(native_window_type menu_window, bool has_keyboard)
	{
		if(menu_window && impl_->menu.window != menu_window)
		{
			erase_menu(true);
			impl_->menu.window = menu_window;
			impl_->menu.owner = native_interface::get_owner_window(menu_window);
			impl_->menu.has_keyboard = has_keyboard;
		}
	}

	native_window_type bedrock::get_menu(native_window_type owner, bool is_keyboard_condition)
	{
		if(	(impl_->menu.owner == nullptr) ||
			(owner && (impl_->menu.owner == owner))
			)
		{
			return ( is_keyboard_condition ? (impl_->menu.has_keyboard ? impl_->menu.window : nullptr) : impl_->menu.window);
		}

		return 0;
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

	void bedrock::get_key_state(arg_keyboard& arg)
	{
		XKeyEvent xkey;
		nana::detail::platform_spec::instance().read_keystate(xkey);
		arg.ctrl = (xkey.state & ControlMask);
		arg.shift = (xkey.state & ShiftMask);
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
		//No implementation for Linux
	}

	bool bedrock::emit(event_code evt_code, core_window_t* wd, const ::nana::event_arg& arg, bool ask_update, thread_context* thrd)
	{
		if(wd_manager().available(wd) == false)
			return false;

		core_window_t * prev_wd;
		if(thrd)
		{
			prev_wd = thrd->event_window;
			thrd->event_window = wd;
			_m_event_filter(evt_code, wd, thrd);
		}

		if(wd->other.upd_state == core_window_t::update_state::none)
			wd->other.upd_state = core_window_t::update_state::lazy;

		_m_emit_core(evt_code, wd, false, arg);

		if(ask_update)
			wd_manager().do_lazy_refresh(wd, false);
		else
			wd->other.upd_state = core_window_t::update_state::none;

		if(thrd) thrd->event_window = prev_wd;
		return true;
	}

	bool bedrock::emit_drawer(event_code evt_code, core_window_t* wd, const ::nana::event_arg& arg, thread_context* thrd)
	{
		if(wd_manager().available(wd) == false)
			return false;

		core_window_t * prev_wd;
		if(thrd)
		{
			prev_wd = thrd->event_window;
			thrd->event_window = wd;
			_m_event_filter(evt_code, wd, thrd);
		}

		if(wd->other.upd_state == core_window_t::update_state::none)
			wd->other.upd_state = core_window_t::update_state::lazy;

		_m_emit_core(evt_code, wd, true, arg);

		if(thrd) thrd->event_window = prev_wd;
		return true;

	}

	void assign_arg(arg_mouse& arg, basic_window* wd, unsigned msg, const XEvent& evt)
	{
		arg.window_handle = reinterpret_cast<window>(wd);
		arg.button = ::nana::mouse::any_button;

		int mask_state = 0;
		if (ButtonPress == msg || ButtonRelease == msg)
		{
			mask_state = evt.xbutton.state;
			if (evt.xbutton.button != Button4 && evt.xbutton.button != Button5)
			{
				arg.evt_code = (ButtonPress == msg ? event_code::mouse_down : event_code::mouse_up);
				arg.pos.x = evt.xbutton.x - wd->pos_root.x;
				arg.pos.y = evt.xbutton.y - wd->pos_root.y;

				switch (evt.xbutton.button)
				{
				case Button1:
					arg.button = ::nana::mouse::left_button;
					break;
				case Button2:
					arg.button = ::nana::mouse::middle_button;
					break;
				case Button3:
					arg.button = ::nana::mouse::right_button;
					break;
				}
			}
		}
		else if (msg == MotionNotify)
		{
			mask_state = evt.xmotion.state;
			arg.evt_code = event_code::mouse_move;
			arg.pos.x = evt.xmotion.x - wd->pos_root.x;
			arg.pos.y = evt.xmotion.y - wd->pos_root.y;
		}
		else if (EnterNotify == msg)
		{
			mask_state = evt.xcrossing.state;
			arg.evt_code = event_code::mouse_enter;
			arg.pos.x = evt.xcrossing.x - wd->pos_root.x;
			arg.pos.y = evt.xcrossing.y - wd->pos_root.y;
		}

		arg.left_button		= ((Button1Mask & mask_state) != 0) || (::nana::mouse::left_button == arg.button) ;
		arg.right_button	= ((Button2Mask & mask_state) != 0) || (::nana::mouse::right_button == arg.button);
		arg.mid_button		= ((Button3Mask & mask_state) != 0) || (::nana::mouse::middle_button == arg.button);
		arg.alt		= ((Mod1Mask & mask_state) != 0);
		arg.shift	= ((ShiftMask & mask_state) != 0);
		arg.ctrl	= ((ControlMask & mask_state) != 0);

	}

	void assign_arg(arg_focus& arg, basic_window* wd, native_window_type recv, bool getting)
	{
		arg.window_handle = reinterpret_cast<window>(wd);
		arg.receiver = recv;
		arg.getting = getting;
	}

	void assign_arg(arg_wheel& arg, basic_window* wd, const XEvent& evt)
	{
		arg.evt_code = event_code::mouse_wheel;
		arg.window_handle = reinterpret_cast<window>(wd);
		if (ButtonRelease == evt.type && (evt.xbutton.button == Button4 || evt.xbutton.button == Button5))
		{
			arg.evt_code = event_code::mouse_wheel;
			arg.pos.x = evt.xbutton.x - wd->pos_root.x;
			arg.pos.y = evt.xbutton.y - wd->pos_root.y;

			arg.upwards = (evt.xbutton.button == Button4);
			arg.left_button = arg.mid_button = arg.right_button = false;
			arg.shift = arg.ctrl = false;
			arg.distance = 120;
			arg.which = arg_wheel::wheel::vertical;
		}
		
	}

	void timer_proc(unsigned tid)
	{
		nana::detail::platform_spec::instance().timer_proc(tid);
	}

	void window_proc_dispatcher(Display* display, nana::detail::msg_packet_tag& msg)
	{
		switch(msg.kind)
		{
		case nana::detail::msg_packet_tag::kind_xevent:
			window_proc_for_xevent(display, msg.u.xevent);
			break;
		case nana::detail::msg_packet_tag::kind_mouse_drop:
			window_proc_for_packet(display, msg);
			break;
		default: break;
		}
	}

	void window_proc_for_packet(Display * display, nana::detail::msg_packet_tag& msg)
	{
		static auto& brock = detail::bedrock::instance();

		auto native_window = reinterpret_cast<native_window_type>(msg.u.packet_window);
		auto root_runtime = brock.wd_manager().root_runtime(native_window);

		if(root_runtime)
		{
			auto msgwd = root_runtime->window;

			switch(msg.kind)
			{
			case nana::detail::msg_packet_tag::kind_mouse_drop:
				msgwd = brock.wd_manager().find_window(native_window, msg.u.mouse_drop.x, msg.u.mouse_drop.y);
				if(msgwd)
				{
					arg_dropfiles arg;
					arg.window_handle = reinterpret_cast<window>(msgwd);
					arg.files.swap(*msg.u.mouse_drop.files);
					delete msg.u.mouse_drop.files;
					arg.pos.x = msg.u.mouse_drop.x - msgwd->pos_root.x;
					arg.pos.y = msg.u.mouse_drop.y - msgwd->pos_root.y;
					msgwd->together.events_ptr->mouse_dropfiles.emit(arg);
					brock.wd_manager().do_lazy_refresh(msgwd, false);
				}
				break;
			default:
				throw std::runtime_error("Nana.GUI.Bedrock: Undefined message packet");
			}
		}
	}

	template<typename Arg>
	void emit_drawer(void(::nana::detail::drawer::*event_ptr)(const Arg&), basic_window* wd, const Arg& arg, bedrock::thread_context* thrd)
	{
		if(bedrock::instance().wd_manager().available(wd) == false)
			return;
		basic_window * pre_wd;
		if(thrd)
		{
			pre_wd = thrd->event_window;
			thrd->event_window = wd;
		}

		if(wd->other.upd_state == basic_window::update_state::none)
			wd->other.upd_state = basic_window::update_state::lazy;

		(wd->drawer.*event_ptr)(arg);
		if(thrd) thrd->event_window = pre_wd;
	}

	wchar_t os_code_from_keysym(KeySym keysym)
	{
		switch(keysym)
		{
		case XK_Alt_L: case XK_Alt_R:
			keysym = keyboard::alt;	break;
		case XK_BackSpace:
			keysym = keyboard::backspace;	break;
		case XK_Tab:
			keysym = keyboard::tab;		break;
		case XK_Escape:
			keysym = keyboard::escape;	break;
		case XK_Return:
			keysym = keyboard::enter;	break;
		case XK_Cancel:
			keysym = keyboard::end_of_text;	break;	//Ctrl+C
		case XK_Page_Up:
			keysym = keyboard::os_pageup;	break;
		case XK_Page_Down:
			keysym = keyboard::os_pagedown; break;
		case XK_Left: case XK_Up: case XK_Right: case XK_Down:
			keysym = keyboard::os_arrow_left + (keysym - XK_Left); break;
		case XK_Insert:
			keysym = keyboard::os_insert; break;
		case XK_Delete:
			keysym = keyboard::os_del; break;
		case XK_Shift_L: case XK_Shift_R:	//shift
			keysym = keyboard::os_shift; break;
		case XK_Control_L: case XK_Control_R: //ctrl
			keysym = keyboard::os_ctrl;	break;
		default:
			do
			{
				//Transfer the keysym for key_press and key_release event to the same behavior with Windows
				if('a' <= keysym && keysym <= 'z')
				{
					keysym = keysym - 'a' + 'A';
					break;
				}
				//reverts the key
				static const char shift_num[]=")!@#$%^&*(";
				auto p = std::find(shift_num, shift_num + sizeof shift_num, char(keysym));
				if(p != shift_num + sizeof shift_num)
				{
					keysym = (p - shift_num + '0');
					break;
				}

				switch(keysym)
				{
				case '~': keysym = '`'; break;
				case '_': keysym = '-'; break;
				case '+': keysym = '='; break;
				case '{': keysym = '['; break;
				case '}': keysym = ']'; break;
				case '|': keysym = '\\'; break;
				case ':': keysym = ';'; break;
				case '"': keysym = '\''; break;
				case '<': keysym = ','; break;
				case '>': keysym = '.'; break;
				case '?': keysym = '/'; break;
				}
			}while(false);
		}
		return wchar_t(keysym);
	}

	void window_proc_for_xevent(Display* display, XEvent& xevent)
	{
		typedef detail::bedrock::core_window_t core_window_t;

		static auto& brock = detail::bedrock::instance();
		static unsigned long	last_mouse_down_time;
		static core_window_t*	last_mouse_down_window;

		auto native_window = reinterpret_cast<native_window_type>(event_window(xevent));
		auto root_runtime = brock.wd_manager().root_runtime(native_window);

		if(root_runtime)
		{
			auto msgwnd = root_runtime->window;
			auto& context = *brock.get_thread_context(msgwnd->thread_id);

			auto pre_event_window = context.event_window;
			auto pressed_wd = root_runtime->condition.pressed;
			auto pressed_wd_space = root_runtime->condition.pressed_by_space;
			auto hovered_wd = root_runtime->condition.hovered;

			const int message = xevent.type;
			switch(xevent.type)
			{
			case EnterNotify:
				//Ignore mouse events when a window has been pressed by pressing spacebar.
				if(pressed_wd_space)
					break;

				msgwnd = brock.wd_manager().find_window(native_window, xevent.xcrossing.x, xevent.xcrossing.y);
				if(msgwnd)
				{
					if (mouse_action::pressed != msgwnd->flags.action)
						msgwnd->flags.action = mouse_action::over;
					hovered_wd = msgwnd;

					arg_mouse arg;
					assign_arg(arg, msgwnd, message, xevent);
					brock.emit(event_code::mouse_enter, msgwnd, arg, true, &context);

					arg.evt_code = event_code::mouse_move;
					brock.emit(event_code::mouse_move, msgwnd, arg, true, &context);
					
					if (!brock.wd_manager().available(hovered_wd))
						hovered_wd = nullptr;
				}
				break;
			case LeaveNotify:
				brock.event_msleave(hovered_wd);
				hovered_wd = nullptr;
				break;
			case FocusIn:
				if(msgwnd->flags.enabled && msgwnd->flags.take_active)
				{
					auto focus = msgwnd->other.attribute.root->focus;
					if(focus && focus->together.caret)
						focus->together.caret->set_active(true);

					arg_focus arg;
					arg.window_handle = reinterpret_cast<window>(focus);
					arg.receiver = native_window;
					arg.getting = true;
					if(!brock.emit(event_code::focus, focus, arg, true, &context))
						brock.wd_manager().set_focus(msgwnd, true);
				}
				break;
			case FocusOut:
				if(msgwnd->other.attribute.root->focus && native_interface::is_window(msgwnd->root))
				{
					nana::point pos = native_interface::cursor_position();
					auto recv = native_interface::find_window(pos.x, pos.y);

					auto focus = msgwnd->other.attribute.root->focus;
					arg_focus arg;
					arg.window_handle = reinterpret_cast<window>(focus);
					arg.getting = false;
					arg.receiver = recv;
					if(brock.emit(event_code::focus, focus, arg, true, &context))
					{
						if(focus->together.caret)
							focus->together.caret->set_active(false);
					}
					brock.close_menu_if_focus_other_window(recv);
				}
				break;
			case ConfigureNotify:
				if(msgwnd->dimension.width != static_cast<unsigned>(xevent.xconfigure.width) || msgwnd->dimension.height != static_cast<unsigned>(xevent.xconfigure.height))
				{
					auto & cf = xevent.xconfigure;
					brock.wd_manager().size(msgwnd, nana::size{static_cast<unsigned>(cf.width), static_cast<unsigned>(cf.height)}, true, true);
				}
				
				if(msgwnd->pos_native.x != xevent.xconfigure.x || msgwnd->pos_native.y != xevent.xconfigure.y)
				{
					msgwnd->pos_native.x = xevent.xconfigure.x;
					msgwnd->pos_native.y = xevent.xconfigure.y;
					brock.event_move(msgwnd, xevent.xconfigure.x, xevent.xconfigure.y);
				}
				break;
			case ButtonPress:
				//Ignore mouse events when a window has been pressed by pressing spacebar
				if(pressed_wd_space)
					break;

				if(xevent.xbutton.button == Button4 || xevent.xbutton.button == Button5)
					break;
					
				msgwnd = brock.wd_manager().find_window(native_window, xevent.xbutton.x, xevent.xbutton.y);
				if(nullptr == msgwnd) break;
					
				if ((msgwnd == msgwnd->root_widget->other.attribute.root->menubar) && brock.get_menu(msgwnd->root, true))
					brock.erase_menu(true);
				else
					brock.close_menu_if_focus_other_window(msgwnd->root);

				if(msgwnd->flags.enabled)
				{
					bool dbl_click = (last_mouse_down_window == msgwnd) && (xevent.xbutton.time - last_mouse_down_time <= 400);
					last_mouse_down_time = xevent.xbutton.time;
					last_mouse_down_window = msgwnd;

					if (Button1 == xevent.xbutton.button)	//Sets the focus only if left button is pressed
					{
						auto new_focus = (msgwnd->flags.take_active ? msgwnd : msgwnd->other.active_window);
						if (new_focus && !new_focus->flags.ignore_mouse_focus)
						{
							context.event_window = new_focus;
							auto kill_focus = brock.wd_manager().set_focus(new_focus, false);
							if (kill_focus != new_focus)
								brock.wd_manager().do_lazy_refresh(kill_focus, false);
						}
					}

					auto retain = msgwnd->together.events_ptr;
					context.event_window = msgwnd;

					pressed_wd = nullptr;
					msgwnd->flags.action = mouse_action::pressed;
					arg_mouse arg;
					assign_arg(arg, msgwnd, ButtonPress, xevent);
					arg.evt_code = dbl_click ? event_code::dbl_click : event_code::mouse_down;
					if(brock.emit(arg.evt_code, msgwnd, arg, true, &context))
					{
						if (brock.wd_manager().available(msgwnd))
						{
							pressed_wd = msgwnd;
							//If a root window is created during the mouse_down event, Nana.GUI will ignore the mouse_up event.
							if (msgwnd->root != native_interface::get_focus_window())
							{
								//call the drawer mouse up event for restoring the surface graphics
								msgwnd->flags.action = mouse_action::normal;
								emit_drawer(&drawer::mouse_up, msgwnd, arg, &context);
								brock.wd_manager().do_lazy_refresh(msgwnd, false);
							}
						}
					}
				}
				break;
			case ButtonRelease:
				//Ignore mouse events when a window has been pressed by pressing spacebar
				if(pressed_wd_space)
					break;

				if(xevent.xbutton.button == Button4 || xevent.xbutton.button == Button5)
				{
					//The hovered window receives the message, unlike in Windows, no redirection is required.
					nana::point mspos{xevent.xbutton.x, xevent.xbutton.y};
					while(msgwnd)
					{
						if(msgwnd->together.events_ptr->mouse_wheel.length() != 0)
						{
							mspos -= msgwnd->pos_root;
							arg_wheel arg;
							arg.which = arg_wheel::wheel::vertical;
							assign_arg(arg, msgwnd, xevent);
							brock.emit(event_code::mouse_wheel, msgwnd, arg, true, &context);
							break;
						}
						msgwnd = msgwnd->parent;
					}
				}
				else
				{
					msgwnd = brock.wd_manager().find_window(native_window, xevent.xbutton.x, xevent.xbutton.y);
					if(nullptr == msgwnd)
						break;

					msgwnd->flags.action = mouse_action::normal;
					if(msgwnd->flags.enabled)
					{
						auto retain = msgwnd->together.events_ptr;

						::nana::arg_mouse arg;
						assign_arg(arg, msgwnd, message, xevent);

						::nana::arg_click click_arg;

						//the window_handle of click_arg is used as a flag to determinate whether to emit click event.
						click_arg.window_handle = nullptr;
						click_arg.mouse_args = &arg;

						const bool hit = msgwnd->dimension.is_hit(arg.pos);
						if(msgwnd == pressed_wd)
						{
							if((arg.button == ::nana::mouse::left_button) && hit)
							{
								msgwnd->flags.action = mouse_action::over;

								click_arg.window_handle = reinterpret_cast<window>(msgwnd);
								emit_drawer(&drawer::click, msgwnd, click_arg, &context);
							}
						}
					
						//Do mouse_up, this handle may be closed by click handler.
						if(brock.wd_manager().available(msgwnd) && msgwnd->flags.enabled)
						{
							if(hit)
								msgwnd->flags.action = mouse_action::over;
							
							auto retain = msgwnd->together.events_ptr;
							auto evt_ptr = retain.get();

							arg.evt_code = event_code::mouse_up;
							emit_drawer(&drawer::mouse_up, msgwnd, arg, &context);

							if(click_arg.window_handle)
								evt_ptr->click.emit(click_arg);

							if (brock.wd_manager().available(msgwnd))
							{
								arg.evt_code = event_code::mouse_up;
								evt_ptr->mouse_up.emit(arg);
							}
						}
						else if(click_arg.window_handle)
							msgwnd->together.events_ptr->click.emit(click_arg);

						brock.wd_manager().do_lazy_refresh(msgwnd, false);
					}
					pressed_wd = nullptr;
				}
				break;
			case DestroyNotify:
				{
					auto & spec = nana::detail::platform_spec::instance();
					if(brock.wd_manager().available(msgwnd))
					{
						//The msgwnd may be destroyed if the window is destroyed by calling native interface of close_window().
						if (msgwnd->root == brock.get_menu())
						{
							brock.erase_menu(false);
							brock.delay_restore(3);	//Restores if delay_restore not decleared
						}

						spec.remove(native_window);
						brock.wd_manager().destroy(msgwnd);

						brock.manage_form_loader(msgwnd, false);
						brock.wd_manager().destroy_handle(msgwnd);
					}
				}
				break;
			case MotionNotify:
				//X may send the MotionNotify with same information repeatly.
				//Nana should ignore the repeated notify.
				if(context.platform.motion_window != native_window || context.platform.motion_pointer_pos != nana::point(xevent.xmotion.x, xevent.xmotion.y))
				{
					context.platform.motion_window = native_window;
					context.platform.motion_pointer_pos = nana::point(xevent.xmotion.x, xevent.xmotion.y);
				}
				else
					break;

				//Ignore mouse events when a window has been pressed by pressing spacebar
				if(pressed_wd_space)
					break;

				msgwnd = brock.wd_manager().find_window(native_window, xevent.xmotion.x, xevent.xmotion.y);
				if (brock.wd_manager().available(hovered_wd) && (msgwnd != hovered_wd))
				{
					brock.event_msleave(hovered_wd);
					hovered_wd->flags.action = mouse_action::normal;
					hovered_wd = nullptr;

					//if msgwnd is neither a captured window nor a child of captured window,
					//redirect the msgwnd to the captured window.
					auto cap_wd = brock.wd_manager().capture_redirect(msgwnd);
					if(cap_wd)
						msgwnd = cap_wd;
				}
				else if(msgwnd)
				{
					bool prev_captured_inside;
					if(brock.wd_manager().capture_window_entered(xevent.xmotion.x, xevent.xmotion.y, prev_captured_inside))
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
							if (mouse_action::pressed != msgwnd->flags.action)
								msgwnd->flags.action = mouse_action::over;
						}
						arg_mouse arg;
						assign_arg(arg, msgwnd, message, xevent);
						arg.evt_code = evt_code;
						brock.emit(evt_code, msgwnd, arg, true, &context);
					}
				}

				if(msgwnd)
				{
					arg_mouse arg;
					assign_arg(arg, msgwnd, message, xevent);
					
					if (mouse_action::pressed != msgwnd->flags.action)
						msgwnd->flags.action = mouse_action::over;

					if (hovered_wd != msgwnd)
					{
						hovered_wd = msgwnd;
						arg.evt_code = event_code::mouse_enter;
						brock.emit(event_code::mouse_enter, msgwnd, arg, true, &context);
					}

					arg.evt_code = event_code::mouse_move;
					brock.emit(event_code::mouse_move, msgwnd, arg, true, &context);
				}
				if (!brock.wd_manager().available(hovered_wd))
					hovered_wd = nullptr;
				break;
			case MapNotify:
			case UnmapNotify:
				brock.event_expose(msgwnd, (xevent.type == MapNotify));
				context.platform.motion_window = nullptr;
				break;
			case Expose:
				if(msgwnd->visible && (msgwnd->root_graph->empty() == false))
				{
					nana::detail::platform_scope_guard psg;
					//Don't copy root_graph to the window directly, otherwise the edge nimbus effect will be missed.
					::nana::rectangle update_area(xevent.xexpose.x, xevent.xexpose.y, xevent.xexpose.width, xevent.xexpose.height);
					if (!update_area.empty())
						msgwnd->drawer.map(reinterpret_cast<window>(msgwnd), true, &update_area);
				}
				break;
			case KeyPress:
				nana::detail::platform_spec::instance().write_keystate(xevent.xkey);
				if(msgwnd->flags.enabled)
				{
					auto menu_wd = brock.get_menu();
					if (menu_wd)
						brock.delay_restore(0);	//Enable delay restore

					if(msgwnd->root != menu_wd)
						msgwnd = brock.focus();

					if(msgwnd)
					{
						KeySym keysym;
						Status status;
						char fixbuf[33];
						char * keybuf = fixbuf;
						int len = 0;
						XIC input_context = nana::detail::platform_spec::instance().caret_input_context(native_window);
						if(input_context)
						{
							nana::detail::platform_scope_guard psg;
#if 1	//Utf8
							len = ::Xutf8LookupString(input_context, &xevent.xkey, keybuf, 32, &keysym, &status);
							if(status == XBufferOverflow)
							{
								keybuf = new char[len + 1];
								len = ::Xutf8LookupString(input_context, &xevent.xkey, keybuf, len, &keysym, &status);
							}
#else
							len = ::XmbLookupString(input_context, &xevent.xkey, keybuf, 32, &keysym, &status);
							if(status == XBufferOverflow)
							{
								keybuf = new char[len + 1];
								len = ::XmbLookupString(input_context, &xevent.xkey, keybuf, len, &keysym, &status);
							}
#endif
						}
						else
						{
							nana::detail::platform_scope_guard psg;
							status = XLookupKeySym;
							keysym = ::XLookupKeysym(&xevent.xkey, 0);
						}

						keybuf[len] = 0;
						wchar_t os_code = 0;
						auto & wd_manager = brock.wd_manager();
						switch(status)
						{
						case XLookupKeySym:
						case XLookupBoth:
							os_code = os_code_from_keysym(keysym);
							if(os_code == keyboard::tab && (false == (msgwnd->flags.tab & detail::tab_type::eating))) //Tab
							{
								arg_keyboard argkey;
								brock.get_key_state(argkey);
								auto tstop_wd = wd_manager.tabstop(msgwnd, !argkey.shift);
								if (tstop_wd)
								{
									wd_manager.set_focus(tstop_wd, false);
									wd_manager.do_lazy_refresh(msgwnd, false);
									wd_manager.do_lazy_refresh(tstop_wd, true);
								}
							}
							else if((keyboard::space == os_code) && msgwnd->flags.space_click_enabled)
							{
								//Clicked by spacebar
								if((nullptr == pressed_wd) && (nullptr == pressed_wd_space))
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
							else if(keyboard::alt == os_code)
							{
								context.is_alt_pressed = true;
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
										arg.key = os_code;
										brock.get_key_state(arg);

										brock.emit(event_code::key_press, msgwnd, arg, true, &context);

										msgwnd->root_widget->flags.ignore_menubar_focus = (focused && (brock.focus() != msgwnd));
									}
									else
										brock.erase_menu(true);
								}
							}
							else
							{
								if(keyboard::os_ctrl == os_code)
									context.is_ctrl_pressed = true;

								arg_keyboard arg;
								arg.ignore = false;
								arg.key = os_code;
								arg.evt_code = event_code::key_press;
								brock.get_key_state(arg);
								arg.window_handle = reinterpret_cast<window>(msgwnd);

								brock.emit(event_code::key_press, msgwnd, arg, true, &context);

								if(brock.wd_manager().available(msgwnd) && (msgwnd->root_widget->other.attribute.root->menubar == msgwnd))
								{
									int cmd = (menu_wd && (keyboard::escape == static_cast<wchar_t>(arg.key)) ? 1 : 0 );
									brock.delay_restore(cmd);
								}
							}

							if(XLookupKeySym == status)
							{
								wd_manager.do_lazy_refresh(msgwnd, false);
								break;
							}
						case XLookupChars:
							if (msgwnd->flags.enabled)
							{
								const wchar_t* charbuf;

								nana::detail::charset_conv charset("UTF-32", "UTF-8");
								const std::string& str = charset.charset(std::string(keybuf, keybuf + len));
								charbuf = reinterpret_cast<const wchar_t*>(str.c_str()) + 1;
								len = str.size() / sizeof(wchar_t) - 1;

								for(int i = 0; i < len; ++i)
								{
									arg_keyboard arg;
									arg.ignore = false;
									arg.key = charbuf[i];

									// When tab is pressed, only tab-eating mode is allowed
									if ((keyboard::tab == arg.key) && !(msgwnd->flags.tab & tab_type::eating))
										continue;

									if(context.is_alt_pressed)
									{
										arg.ctrl = arg.shift = false;
										arg.evt_code = event_code::shortkey;
										brock.set_keyboard_shortkey(true);
										auto shr_wd = wd_manager.find_shortkey(native_window, arg.key);
										if(shr_wd)
										{
											arg.window_handle = reinterpret_cast<window>(shr_wd);
											brock.emit(event_code::shortkey, shr_wd, arg, true, &context);
										}
										continue;
									}
									arg.evt_code = event_code::key_char;
									arg.window_handle = reinterpret_cast<window>(msgwnd);
									brock.get_key_state(arg);
									msgwnd->together.events_ptr->key_char.emit(arg);
									if(arg.ignore == false && wd_manager.available(msgwnd))
										brock.emit_drawer(event_code::key_char, msgwnd, arg, &context);
								}

								if(brock.set_keyboard_shortkey(false))
									context.is_alt_pressed = false;
							}
							break;
						}

						wd_manager.do_lazy_refresh(msgwnd, false);
						if(keybuf != fixbuf)
							delete [] keybuf;
					}
				}
				break;
			case KeyRelease:
				nana::detail::platform_spec::instance().write_keystate(xevent.xkey);
				{
					auto os_code = os_code_from_keysym(::XLookupKeysym(&xevent.xkey, 0));
					if(keyboard::alt != os_code)
					{
						if(0x11 == os_code)
							context.is_ctrl_pressed = false;

						msgwnd = brock.focus();
						if(msgwnd)
						{
							if(msgwnd == pressed_wd_space)
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
								arg.ignore = false;
								arg.key = os_code;
								brock.get_key_state(arg);
								brock.emit(event_code::key_release, msgwnd, arg, true, &context);
							}
						}

						if(os_code < keyboard::os_arrow_left || keyboard::os_arrow_down < os_code)
							brock.delay_restore(2);	//Restores while key release
					}
					else
					{
						context.is_alt_pressed = false;
						if (brock.set_keyboard_shortkey(false) == false)
						{
							msgwnd = msgwnd->root_widget->other.attribute.root->menubar;
							if (msgwnd)
							{
								bool set_focus = (brock.focus() != msgwnd) && (!msgwnd->root_widget->flags.ignore_menubar_focus);
								if (set_focus)
									brock.wd_manager().set_focus(msgwnd, false);

								arg_keyboard arg;
								arg.evt_code = event_code::key_release;
								arg.window_handle = reinterpret_cast<window>(msgwnd);
								arg.ignore = false;
								arg.key = os_code;
								brock.get_key_state(arg);
								brock.emit(event_code::key_release, msgwnd, arg, true, &context);

								if (!set_focus)
								{
									brock.set_menubar_taken(nullptr);
									msgwnd->root_widget->flags.ignore_menubar_focus = false;
								}
							}
						}
					}
				}
				break;
			default:
				if(message == ClientMessage)
				{
					auto & atoms = nana::detail::platform_spec::instance().atombase();
					if(atoms.wm_protocols == xevent.xclient.message_type)
					{
						if(msgwnd->flags.enabled && (atoms.wm_delete_window == static_cast<Atom>(xevent.xclient.data.l[0])))
						{
							arg_unload arg;
							arg.window_handle = reinterpret_cast<window>(msgwnd);
							arg.cancel = false;
							brock.emit(event_code::unload, msgwnd, arg, true, &context);
							if(false == arg.cancel)
								native_interface::close_window(native_window);
						}
					}
				}
			}

			root_runtime = brock.wd_manager().root_runtime(native_window);
			if(root_runtime)
			{
				context.event_window = pre_event_window;
				root_runtime->condition.pressed	= pressed_wd;
				root_runtime->condition.hovered = hovered_wd;
				root_runtime->condition.pressed_by_space = pressed_wd_space;
			}
			else
			{
				auto context = brock.get_thread_context();
				if(context) context->event_window = pre_event_window;
			}

			auto thread_id = ::nana::system::this_thread_id();
			brock.wd_manager().call_safe_place(thread_id);

			if(msgwnd)
				brock.wd_manager().remove_trash_handle(thread_id);
		}
	}

	void bedrock::pump_event(window modal_window, bool is_modal)
	{
		thread_context * context = open_thread_context();
		if(0 == context->window_count)
		{
			//test if there is not a window
			remove_thread_context();
			return;
		}

		++(context->event_pump_ref_count);
		wd_manager().internal_lock().revert();
		
		native_window_type owner_native = 0;
		core_window_t * owner = 0;
		if(modal_window)
		{
			native_window_type modal = reinterpret_cast<core_window_t*>(modal_window)->root;
			owner_native = native_interface::get_owner_window(modal);
			if(owner_native)
			{
				native_interface::enable_window(owner_native, false);
				owner = wd_manager().root(owner_native);
				if(owner)
					owner->flags.enabled = false;
			}	
		}
		
		nana::detail::platform_spec::instance().msg_dispatch(modal_window ? reinterpret_cast<core_window_t*>(modal_window)->root : 0);

		if(owner_native)
		{
			if(owner)
				owner->flags.enabled = true;
			native_interface::enable_window(owner_native, true);
		}
		
		wd_manager().internal_lock().forward();
		if(0 == --(context->event_pump_ref_count))
		{
			if(0 == modal_window || 0 == context->window_count)
				remove_thread_context();
		}

	}//end bedrock::event_loop

	void bedrock::thread_context_destroy(core_window_t * wd)
	{
		bedrock::thread_context * thr = get_thread_context(0);
		if(thr && thr->event_window == wd)
			thr->event_window = nullptr;
	}

	void bedrock::thread_context_lazy_refresh()
	{
		thread_context* thrd = get_thread_context(0);
		if(thrd && thrd->event_window)
		{
			//the state none should be tested, becuase in an event, there would be draw after an update,
			//if the none is not tested, the draw after update will not be refreshed.
			switch(thrd->event_window->other.upd_state)
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

		auto & spec = nana::detail::platform_spec::instance();
		Display * disp = spec.open_display();

		if (thrd->cursor.native_handle && (thrd->cursor.native_handle != wd->root))
			::XUndefineCursor(disp, reinterpret_cast<Window>(thrd->cursor.native_handle));

		thrd->cursor.native_handle = wd->root;
		if (thrd->cursor.predef_cursor != cur)
		{
			thrd->cursor.predef_cursor = cur;
			if (thrd->cursor.handle)
			{
				::XFreeCursor(disp, thrd->cursor.handle);
				thrd->cursor.handle = 0;
			}
		}

		if (nana::cursor::arrow == cur)
		{
			thrd->cursor.native_handle = nullptr;
			thrd->cursor.window = nullptr;
			::XUndefineCursor(disp, reinterpret_cast<Window>(wd->root));
		}
		else
		{
			if (!thrd->cursor.handle)
				thrd->cursor.handle = ::XCreateFontCursor(disp, static_cast<unsigned>(cur));
			::XDefineCursor(disp, reinterpret_cast<Window>(wd->root), thrd->cursor.handle);
		}
	}

	void bedrock::define_state_cursor(core_window_t* wd, nana::cursor cur, thread_context* thrd)
	{
		wd->root_widget->other.attribute.root->state_cursor = cur;
		wd->root_widget->other.attribute.root->state_cursor_window = wd;
		set_cursor(wd, cur, thrd);
	}

	void bedrock::undefine_state_cursor(core_window_t * wd, thread_context* thrd)
	{
		if (!wd_manager().available(wd))
			return;
	
		wd->root_widget->other.attribute.root->state_cursor = nana::cursor::arrow;
		wd->root_widget->other.attribute.root->state_cursor_window = nullptr;

		auto pos = native_interface::cursor_position();
		auto native_handle = native_interface::find_window(pos.x, pos.y);
		if (!native_handle)
			return;

		native_interface::calc_window_point(native_handle, pos);
		auto rev_wd = wd_manager().find_window(native_handle, pos.x, pos.y);
		if (rev_wd)
			set_cursor(rev_wd, rev_wd->predef_cursor, thrd);
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
				set_cursor(wd, nana::cursor::arrow, thrd);
			break;
		case event_code::destroy:
			if (wd->root_widget->other.attribute.root->state_cursor_window == wd)
				undefine_state_cursor(wd, thrd);

			if(wd == thrd->cursor.window)
				set_cursor(wd, cursor::arrow, thrd);
			break;
		default:
			break;
		}
	}
}//end namespace detail
}//end namespace nana
#endif //NANA_POSIX && NANA_X11
