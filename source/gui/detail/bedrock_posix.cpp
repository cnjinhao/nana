/*
 *	A Bedrock Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://nanapro.sourceforge.net/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/detail/linux_X11/bedrock.cpp
 */

#include "../../detail/platform_spec_selector.hpp"
#if defined(NANA_POSIX) && defined(NANA_X11)
#include <nana/gui/detail/event_code.hpp>
#include <nana/system/platform.hpp>
#include <nana/gui/detail/native_window_interface.hpp>
#include <nana/gui/layout_utility.hpp>
#include <nana/gui/detail/window_layout.hpp>
#include <nana/gui/detail/element_store.hpp>
#include "inner_fwd_implement.hpp"
#include <errno.h>
#include <algorithm>

#include "bedrock_types.hpp"

namespace nana
{
namespace detail
{
	//Declarations of helper functions defined in native_window_interface.cpp
	void x11_apply_exposed_position(native_window_type wd);

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
				thread_t tid{ 0 };
				thread_context *object{ nullptr };
			}tcontext;
		}cache;
	};

	void timer_proc(thread_t);
	void window_proc_dispatcher(Display*, nana::detail::msg_packet_tag&);
	void window_proc_for_packet(Display *, nana::detail::msg_packet_tag&);
	void window_proc_for_xevent(Display*, XEvent&);

	class accel_key_comparer
	{
	public:
		bool operator()(const accel_key& a, const accel_key& b) const
		{
			auto va = a.case_sensitive ? a.key : std::tolower(a.key);
			auto vb = b.case_sensitive ? b.key : std::tolower(b.key);

			if(va < vb)
				return true;
			else if(va > vb)
				return false;

			if (a.case_sensitive != b.case_sensitive)
				return b.case_sensitive;

			if (a.alt != b.alt)
				return b.alt;

			if (a.ctrl != b.ctrl)
				return b.ctrl;

			return ((a.shift != b.shift) && b.shift);
		}
	};

	struct accel_key_value
	{
		std::function<void()> command;
	};

	struct window_platform_assoc
	{
		std::map<accel_key, accel_key_value, accel_key_comparer> accel_commands;
	};

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

	void bedrock::flush_surface(basic_window* wd, bool forced, const rectangle* update_area)
	{
		wd->drawer.map(wd, forced, update_area);
	}

	//inc_window
	//@biref: increament the number of windows
	int bedrock::inc_window(thread_t tid)
	{
		private_impl * impl = instance().impl_;
		std::lock_guard<decltype(impl->mutex)> lock(impl->mutex);

		int & cnt = (impl->thr_contexts[tid ? tid : nana::system::this_thread_id()].window_count);
		return (cnt < 0 ? cnt = 1 : ++cnt);
	}

	bedrock::thread_context* bedrock::open_thread_context(thread_t tid)
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

	bedrock::thread_context* bedrock::get_thread_context(thread_t tid)
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
	
	void bedrock::get_key_state(arg_keyboard& arg)
	{
		XKeyEvent xkey;
		nana::detail::platform_spec::instance().read_keystate(xkey);
		arg.alt = (xkey.state & Mod1Mask);
		arg.ctrl = (xkey.state & ControlMask);
		arg.shift = (xkey.state & ShiftMask);
	}

	void bedrock::delete_platform_assoc(window_platform_assoc* passoc)
	{
		delete passoc;
	}

	void bedrock::keyboard_accelerator(native_window_type wd, const accel_key& ackey, const std::function<void()>& fn)
	{
		auto misc = wd_manager().root_runtime(wd);
		if (nullptr == misc)
			return;

		if (!misc->wpassoc)
			misc->wpassoc = new window_platform_assoc;

		misc->wpassoc->accel_commands[ackey].command = fn;
	}

	element_store& bedrock::get_element_store() const
	{
		return impl_->estore;
	}

	void bedrock::map_through_widgets(basic_window*, native_drawable_type)
	{
		//No implementation for Linux
	}

	void assign_arg(arg_mouse& arg, basic_window* wd, unsigned msg, const XEvent& evt)
	{
		arg.window_handle = wd;
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
		arg.window_handle = wd;
		arg.receiver = recv;
		arg.getting = getting;
        arg.focus_reason = arg_focus::reason::general;
	}

	void assign_arg(arg_wheel& arg, basic_window* wd, const XEvent& evt)
	{
		arg.evt_code = event_code::mouse_wheel;
		arg.window_handle = wd;
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

	void timer_proc(thread_t tid)
	{
		nana::detail::platform_spec::instance().timer_proc(tid);
	}

	void window_proc_dispatcher(Display* display, nana::detail::msg_packet_tag& msg)
	{
		switch(msg.kind)
		{
		case nana::detail::msg_packet_tag::pkt_family::xevent:
			window_proc_for_xevent(display, msg.u.xevent);
			break;
		case nana::detail::msg_packet_tag::pkt_family::mouse_drop:
			window_proc_for_packet(display, msg);
			break;
		default: break;
		}
	}

	void window_proc_for_packet(Display * /*display*/, nana::detail::msg_packet_tag& msg)
	{
		static auto& brock = detail::bedrock::instance();

		auto native_window = reinterpret_cast<native_window_type>(msg.u.packet_window);
		auto root_runtime = brock.wd_manager().root_runtime(native_window);

		if(root_runtime)
		{
			auto msgwd = root_runtime->window;

			switch(msg.kind)
			{
			case nana::detail::msg_packet_tag::pkt_family::mouse_drop:
				msgwd = brock.wd_manager().find_window(native_window, {msg.u.mouse_drop.x, msg.u.mouse_drop.y});

				if(msgwd)
				{
					arg_dropfiles arg;
					arg.window_handle = msgwd;
					arg.files.swap(*msg.u.mouse_drop.files);
					delete msg.u.mouse_drop.files;
					arg.pos.x = msg.u.mouse_drop.x - msgwd->pos_root.x;
					arg.pos.y = msg.u.mouse_drop.y - msgwd->pos_root.y;
					msgwd->annex.events_ptr->mouse_dropfiles.emit(arg, msgwd);
					brock.wd_manager().do_lazy_refresh(msgwd, false);
				}
				break;
			default:
				throw std::runtime_error("Nana.GUI.Bedrock: Undefined message packet");
			}
		}
	}

	template<typename Arg>
	void draw_invoker(void(::nana::detail::drawer::*event_ptr)(const Arg&, const bool), basic_window* wd, const Arg& arg, bedrock::thread_context* thrd)
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

		(wd->drawer.*event_ptr)(arg, false);
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
		case XK_Delete: case XK_KP_Delete:
			keysym = keyboard::del; break;
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

	bool translate_keyboard_accelerator(root_misc* misc, char os_code, const arg_keyboard& modifiers)
	{
		if(!misc->wpassoc)
			return false;

		auto lower_oc = std::tolower(os_code);

		std::function<void()> command;

		for(auto & accel : misc->wpassoc->accel_commands)
		{
			if(accel.first.key != (accel.first.case_sensitive ? os_code : lower_oc))
				continue;

			if(accel.first.alt == modifiers.alt && accel.first.ctrl == modifiers.ctrl && accel.first.shift == modifiers.shift)
			{
				command = accel.second.command;
				break;
			}
		}

		if(!command)
			return false;
		
		command();
		return true;
	}

	void x_lookup_chars(const root_misc* rruntime, basic_window * msgwd, char* keybuf, std::size_t keybuf_len, const arg_keyboard& modifiers_status)
	{
		if (!msgwd->flags.enabled)
			return;

		static auto& brock = detail::bedrock::instance();
		auto & wd_manager = brock.wd_manager();

		auto& context = *brock.get_thread_context(msgwd->thread_id);

		auto const native_window = rruntime->window->root;
		
		auto wstr = nana::to_wstring(std::string{keybuf, keybuf + keybuf_len});
		auto const charbuf = wstr.c_str();
		auto const len = wstr.length();

		for(std::size_t i = 0; i < len; ++i)
		{
			arg_keyboard arg = modifiers_status;
			arg.ignore = false;
			arg.key = charbuf[i];

			// ignore Unicode BOM (it may or may not appear)
			if (arg.key == 0xFEFF) continue;

			//Only accept tab when it is not ignored.
			if ((keyboard::tab == arg.key) && rruntime->condition.ignore_tab)
				continue;

			if(context.is_alt_pressed)
			{
				arg.ctrl = arg.shift = false;
				arg.evt_code = event_code::shortkey;
				brock.shortkey_occurred(true);
				auto shr_wd = wd_manager.find_shortkey(native_window, arg.key);
				if(shr_wd)
				{
					arg.window_handle = shr_wd;
					brock.emit(event_code::shortkey, shr_wd, arg, true, &context);
				}
				continue;
			}
			
			arg.evt_code = event_code::key_char;
			arg.window_handle = msgwd;
			msgwd->annex.events_ptr->key_char.emit(arg, msgwd);
			if(arg.ignore == false && wd_manager.available(msgwd))
				draw_invoker(&drawer::key_char, msgwd, arg, &context);
		}

		if(brock.shortkey_occurred(false))
			context.is_alt_pressed = false;
	}

	void window_proc_for_xevent(Display* /*display*/, XEvent& xevent)
	{
		static auto& brock = detail::bedrock::instance();
		static unsigned long	last_mouse_down_time;
		static basic_window*	last_mouse_down_window;

		auto native_window = reinterpret_cast<native_window_type>(event_window(xevent));
		auto & wd_manager = brock.wd_manager();
		auto root_runtime = wd_manager.root_runtime(native_window);

		if(root_runtime)
		{
			auto const root_wd = root_runtime->window;
			auto msgwnd = root_wd;

			detail::bedrock::root_guard rw_guard{ brock, root_wd };

			auto& context = *brock.get_thread_context(msgwnd->thread_id);

			auto const pre_event_window = context.event_window;
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

				msgwnd = wd_manager.find_window(native_window, {xevent.xcrossing.x, xevent.xcrossing.y});
				if(msgwnd)
				{
					if (mouse_action::pressed != msgwnd->flags.action)
						msgwnd->set_action(mouse_action::hovered);

					hovered_wd = msgwnd;

					arg_mouse arg;
					assign_arg(arg, msgwnd, message, xevent);
					brock.emit(event_code::mouse_enter, msgwnd, arg, true, &context);

					arg.evt_code = event_code::mouse_move;
					brock.emit(event_code::mouse_move, msgwnd, arg, true, &context);

					if (!wd_manager.available(hovered_wd))
						hovered_wd = nullptr;
				}
				break;
			case LeaveNotify:
				brock.event_msleave(hovered_wd);
				hovered_wd = nullptr;
				break;
			case FocusIn:
				brock.event_focus_changed(msgwnd, native_window, true);
				break;
			case FocusOut:
				if(native_interface::is_window(msgwnd->root))
				{
					nana::point pos = native_interface::cursor_position();
					auto recv = native_interface::find_window(pos.x, pos.y);

					brock.event_focus_changed(msgwnd, recv, false);
				}
				break;
			case ConfigureNotify:
				if(msgwnd->dimension.width != static_cast<unsigned>(xevent.xconfigure.width) || msgwnd->dimension.height != static_cast<unsigned>(xevent.xconfigure.height))
				{
					auto & cf = xevent.xconfigure;
					wd_manager.size(msgwnd, nana::size{static_cast<unsigned>(cf.width), static_cast<unsigned>(cf.height)}, true, true);
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

				msgwnd = wd_manager.find_window(native_window, {xevent.xbutton.x, xevent.xbutton.y});

				pressed_wd = nullptr;
				if(nullptr == msgwnd)
					break;

				if ((msgwnd == msgwnd->root_widget->other.attribute.root->menubar) && brock.get_menu(msgwnd->root, true))
					brock.erase_menu(true);
				else
					brock.close_menu_if_focus_other_window(msgwnd->root);

				if(msgwnd->flags.enabled)
				{
					pressed_wd = msgwnd;

					const bool dbl_click = (last_mouse_down_window == msgwnd) && (xevent.xbutton.time - last_mouse_down_time <= 400);
					last_mouse_down_time = xevent.xbutton.time;
					last_mouse_down_window = msgwnd;

					if (Button1 == xevent.xbutton.button)	//Sets the focus only if left button is pressed
					{
						auto new_focus = (msgwnd->flags.take_active ? msgwnd : msgwnd->other.active_window);
						if (new_focus && !new_focus->flags.ignore_mouse_focus)
						{
							context.event_window = new_focus;
							auto kill_focus = brock.wd_manager().set_focus(new_focus, false, arg_focus::reason::mouse_press);
							if (kill_focus != new_focus)
								wd_manager.do_lazy_refresh(kill_focus, false);
						}
					}

					auto retain = msgwnd->annex.events_ptr;
					context.event_window = msgwnd;

					msgwnd->set_action(mouse_action::pressed);
					arg_mouse arg;
					assign_arg(arg, msgwnd, ButtonPress, xevent);
					arg.evt_code = dbl_click ? event_code::dbl_click : event_code::mouse_down;
					if (brock.emit(arg.evt_code, msgwnd, arg, true, &context))
					{
						//If a root window is created during the mouse_down event, Nana.GUI will ignore the mouse_up event.
						if (msgwnd->root != native_interface::get_focus_window())
						{
							auto pos = native_interface::cursor_position();
							auto rootwd = native_interface::find_window(pos.x, pos.y);
							native_interface::calc_window_point(rootwd, pos);
							if(msgwnd != wd_manager.find_window(rootwd, pos))
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
			case ButtonRelease:
				//Ignore mouse events when a window has been pressed by pressing spacebar
				if(pressed_wd_space)
					break;

				msgwnd = wd_manager.find_window(native_window, {xevent.xbutton.x, xevent.xbutton.y});
				if(nullptr == msgwnd)
					break;

				if(xevent.xbutton.button == Button4 || xevent.xbutton.button == Button5)
				{
					//The hovered window receives the message, unlike in Windows, no redirection is required.
					auto evt_wd = msgwnd;
					while(evt_wd)
					{
						if(evt_wd->annex.events_ptr->mouse_wheel.length() != 0)
						{
							arg_wheel arg;
							arg.which = arg_wheel::wheel::vertical;
							assign_arg(arg, evt_wd, xevent);
							brock.emit(event_code::mouse_wheel, evt_wd, arg, true, &context);
							break;
						}
						evt_wd = evt_wd->parent;
					}

					if(msgwnd && (nullptr == evt_wd))
					{
						arg_wheel arg;
						arg.which = arg_wheel::wheel::vertical;
						assign_arg(arg, msgwnd, xevent);
						draw_invoker(&drawer::mouse_wheel, msgwnd, arg, &context);
						wd_manager.do_lazy_refresh(msgwnd, false);
					}
				}
				else
				{
					msgwnd->set_action(mouse_action::normal);
					if(msgwnd->flags.enabled)
					{
						auto retain = msgwnd->annex.events_ptr;

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
								msgwnd->set_action(mouse_action::hovered);

								click_arg.window_handle = msgwnd;
								draw_invoker(&drawer::click, msgwnd, click_arg, &context);
							}
						}

						//Do mouse_up, this handle may be closed by click handler.
						if(wd_manager.available(msgwnd) && msgwnd->flags.enabled)
						{
							if(hit)
								msgwnd->set_action(mouse_action::hovered);

							auto retain = msgwnd->annex.events_ptr;
							auto evt_ptr = retain.get();

							arg.evt_code = event_code::mouse_up;
							draw_invoker(&drawer::mouse_up, msgwnd, arg, &context);

							if(click_arg.window_handle)
								evt_ptr->click.emit(click_arg, msgwnd);

							if (wd_manager.available(msgwnd))
							{
								arg.evt_code = event_code::mouse_up;
								evt_ptr->mouse_up.emit(arg, msgwnd);
							}
						}
						else if(click_arg.window_handle)
							msgwnd->annex.events_ptr->click.emit(click_arg, msgwnd);

						wd_manager.do_lazy_refresh(msgwnd, false);
					}
					pressed_wd = nullptr;
				}
				break;
			case DestroyNotify:
				if(wd_manager.available(msgwnd))
				{
					//The msgwnd may be destroyed if the window is destroyed by calling native interface of close_window().
					if (msgwnd->root == brock.get_menu())
					{
						brock.erase_menu(false);
						brock.delay_restore(3);	//Restores if delay_restore not decleared
					}

					auto & spec = ::nana::detail::platform_spec::instance();
					spec.remove(native_window);
					wd_manager.destroy(msgwnd);

					brock.manage_form_loader(msgwnd, false);
					wd_manager.destroy_handle(msgwnd);
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

				msgwnd = wd_manager.find_window(native_window, {xevent.xmotion.x, xevent.xmotion.y});
				if (wd_manager.available(hovered_wd) && (msgwnd != hovered_wd))
				{
					brock.event_msleave(hovered_wd);
					hovered_wd->set_action(mouse_action::normal);
					hovered_wd = nullptr;

					//if msgwnd is neither a captured window nor a child of captured window,
					//redirect the msgwnd to the captured window.
					auto cap_wd = wd_manager.capture_redirect(msgwnd);
					if(cap_wd)
						msgwnd = cap_wd;
				}
				else if(msgwnd)
				{
					bool prev_captured_inside;
					if(wd_manager.capture_window_entered(xevent.xmotion.x, xevent.xmotion.y, prev_captured_inside))
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
							if (mouse_action::pressed != msgwnd->flags.action)
								msgwnd->set_action(mouse_action::hovered);
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
						msgwnd->set_action(mouse_action::hovered);

					if (hovered_wd != msgwnd)
					{
						hovered_wd = msgwnd;
						arg.evt_code = event_code::mouse_enter;
						brock.emit(event_code::mouse_enter, msgwnd, arg, true, &context);
					}

					arg.evt_code = event_code::mouse_move;
					brock.emit(event_code::mouse_move, msgwnd, arg, true, &context);
				}
				if (!wd_manager.available(hovered_wd))
					hovered_wd = nullptr;
				break;
			case MapNotify:
			case UnmapNotify:
				if(xevent.type == MapNotify)
					x11_apply_exposed_position(native_window);

				brock.event_expose(msgwnd, (xevent.type == MapNotify));
				context.platform.motion_window = nullptr;
				break;
			case Expose:
				if(msgwnd->visible && (msgwnd->root_graph->empty() == false))
				{
					nana::internal_scope_guard lock;
					//Don't lock this scope using platform-scope-guard. Because it would cause the platform-scope-lock to be locked
					//before the internal-scope-guard, and the order of locking would cause dead-lock.
					//
					//Locks this scope using internal-scope-guard is correct and safe. In the scope, the Xlib functions aren't called
					//directly.
					if(msgwnd->is_draw_through())
					{
						msgwnd->other.attribute.root->draw_through();
					}
					else
					{
						//Don't copy root_graph to the window directly, otherwise the edge nimbus effect will be missed.
						::nana::rectangle update_area(xevent.xexpose.x, xevent.xexpose.y, xevent.xexpose.width, xevent.xexpose.height);
						if (!update_area.empty())
							msgwnd->drawer.map(msgwnd, true, &update_area);
					}
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
						arg_keyboard modifiers_status;
						brock.get_key_state(modifiers_status);

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
						bool accel_translated = false;

						switch(status)
						{
						case XLookupKeySym:
						case XLookupBoth:
							os_code = os_code_from_keysym(keysym);
							accel_translated = translate_keyboard_accelerator(root_runtime, os_code, modifiers_status);
							if(accel_translated)
								break;

							if(os_code == keyboard::tab && (false == (msgwnd->flags.tab & detail::tab_type::eating))) //Tab
							{
								auto tstop_wd = wd_manager.tabstop(msgwnd, !modifiers_status.shift);
								if (tstop_wd)
								{
                                    root_runtime->condition.ignore_tab = true;
									wd_manager.set_focus(tstop_wd, false, arg_focus::reason::tabstop);
									wd_manager.do_lazy_refresh(msgwnd, false);
									wd_manager.do_lazy_refresh(tstop_wd, true);
								}
							}
							else if((keyboard::space == os_code) && msgwnd->flags.space_click_enabled)
							{
								//Clicked by spacebar
								if((nullptr == pressed_wd) && (nullptr == pressed_wd_space) && msgwnd->flags.enabled)
								{
									arg_mouse arg;
									arg.alt = modifiers_status.alt;
									arg.button = ::nana::mouse::left_button;
									arg.ctrl = modifiers_status.ctrl;
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
							else if(keyboard::alt == os_code)
							{
								context.is_alt_pressed = true;
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

								arg_keyboard arg = modifiers_status;
								arg.ignore = false;
								arg.key = os_code;
								arg.evt_code = event_code::key_press;
								arg.window_handle = msgwnd;

								brock.emit(event_code::key_press, msgwnd, arg, true, &context);

								if(wd_manager.available(msgwnd) && (msgwnd->root_widget->other.attribute.root->menubar == msgwnd))
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
							x_lookup_chars(root_runtime, msgwnd, keybuf, len, modifiers_status);
							break;
						case XLookupChars:
							x_lookup_chars(root_runtime, msgwnd, keybuf, len, modifiers_status);
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
					if(keyboard::alt != os_code) //MUST NOT BE AN ALT
					{
						if(0x11 == os_code)
							context.is_ctrl_pressed = false;

                        if (('\t' == os_code) && root_runtime->condition.ignore_tab)
                        {
                            root_runtime->condition.ignore_tab = false;
                        }
                        else
                        {

						    msgwnd = brock.focus();
						    if(msgwnd)
						    {
							    if((msgwnd == pressed_wd_space) && msgwnd->flags.enabled)
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
								    arg_keyboard arg;

								    arg.evt_code = event_code::key_release;
								    arg.window_handle = msgwnd;
								    arg.ignore = false;
								    arg.key = os_code;
								    brock.get_key_state(arg);
							    	brock.emit(event_code::key_release, msgwnd, arg, true, &context);
				   			    }
						    }
                        }

						if(os_code < keyboard::os_arrow_left || keyboard::os_arrow_down < os_code)
							brock.delay_restore(2);	//Restores while key release
					}
					else
					{
						context.is_alt_pressed = false;
						if (brock.shortkey_occurred(false) == false)
						{
							msgwnd = msgwnd->root_widget->other.attribute.root->menubar;
							if (msgwnd)
							{
								bool set_focus = (brock.focus() != msgwnd) && (!msgwnd->root_widget->flags.ignore_menubar_focus);
								if (set_focus)
									wd_manager.set_focus(msgwnd, false, arg_focus::reason::general);

								arg_keyboard arg;
								arg.evt_code = event_code::key_release;
								arg.window_handle = msgwnd;
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
					auto & spec = ::nana::detail::platform_spec::instance();
					auto & xclient = xevent.xclient;
					auto & atoms = spec.atombase();
					if(atoms.wm_protocols == xclient.message_type)
					{
						if(msgwnd->flags.enabled && (atoms.wm_delete_window == static_cast<Atom>(xclient.data.l[0])))
						{
							arg_unload arg;
							arg.window_handle = msgwnd;
							arg.cancel = false;
							brock.emit(event_code::unload, msgwnd, arg, true, &context);
							if(false == arg.cancel)
								native_interface::close_window(native_window);
						}
					}
				}
			}


			wd_manager.update_requesters(root_wd);

			root_runtime = wd_manager.root_runtime(native_window);
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
			wd_manager.call_safe_place(thread_id);

			if(msgwnd)
				wd_manager.remove_trash_handle(thread_id);
		}
	}

	void bedrock::pump_event(window condition_wd, bool is_modal)
	{
		thread_context * context = open_thread_context();
		if(0 == context->window_count)
		{
			//test if there is not a window
			remove_thread_context();
			return;
		}

		++(context->event_pump_ref_count);

		auto & lock = wd_manager().internal_lock();
		lock.revert();

		native_window_type owner_native{};
		basic_window * owner = nullptr;
		if(condition_wd && is_modal)
		{
			native_window_type modal = condition_wd->root;
			owner_native = native_interface::get_window(modal, window_relationship::owner);
			if(owner_native)
			{
				native_interface::enable_window(owner_native, false);
				owner = wd_manager().root(owner_native);
				if(owner)
					owner->flags.enabled = false;
			}
		}

		nana::detail::platform_spec::instance().msg_dispatch(condition_wd ? condition_wd->root : 0);

		if(owner_native)
		{
			if(owner)
				owner->flags.enabled = true;
			native_interface::enable_window(owner_native, true);
		}

		//Before exit of pump_event, it should call the remove_trash_handle.
		//Under Linux, if the windows are closed in other threads, all the widgets handles
		//will be marked as deleted after exit of the event loop and in other threads. So the
		//handle should be deleted from trash before exit the pump_event.
		auto thread_id = ::nana::system::this_thread_id();
		wd_manager().call_safe_place(thread_id);
		wd_manager().remove_trash_handle(thread_id);

		lock.forward();

		if(0 == --(context->event_pump_ref_count))
		{
			if(0 == condition_wd || 0 == context->window_count)
				remove_thread_context();
		}

	}//end bedrock::event_loop

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

	void bedrock::define_state_cursor(basic_window* wd, nana::cursor cur, thread_context* thrd)
	{
		wd->root_widget->other.attribute.root->state_cursor = cur;
		wd->root_widget->other.attribute.root->state_cursor_window = wd;
		set_cursor(wd, cur, thrd);
	}

	void bedrock::undefine_state_cursor(basic_window * wd, thread_context* thrd)
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
		auto rev_wd = wd_manager().find_window(native_handle, pos);
		if (rev_wd)
			set_cursor(rev_wd, rev_wd->predef_cursor, thrd);
	}
}//end namespace detail
}//end namespace nana
#endif //NANA_POSIX && NANA_X11
