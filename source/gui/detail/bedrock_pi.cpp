/*
*	A Bedrock Platform-Independent Implementation
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file: nana/gui/detail/bedrock_pi.cpp
*/

#include "../../detail/platform_spec_selector.hpp"
#include "basic_window.hpp"
#include "bedrock_types.hpp"
#include <nana/gui/compact.hpp>
#include <nana/gui/widgets/widget.hpp>
#include <nana/gui/detail/event_code.hpp>
#include <nana/system/platform.hpp>
#include <nana/system/timepiece.hpp>
#include <nana/gui/detail/native_window_interface.hpp>
#include <nana/gui/layout_utility.hpp>
#include <nana/gui/detail/element_store.hpp>

#include <sstream>
#include <algorithm>

namespace nana
{

	//class internal_scope_guard
		internal_scope_guard::internal_scope_guard()
		{
			detail::bedrock::instance().wd_manager().internal_lock().lock();
		}

		internal_scope_guard::~internal_scope_guard()
		{
			detail::bedrock::instance().wd_manager().internal_lock().unlock();
		}
	//end class internal_scope_guard

	//class internal_revert_guard
		internal_revert_guard::internal_revert_guard()
		{
			detail::bedrock::instance().wd_manager().internal_lock().revert();
		}

		internal_revert_guard::~internal_revert_guard()
		{
			detail::bedrock::instance().wd_manager().internal_lock().forward();
		}
		//end class internal_revert_guard

	//class event_arg
	void event_arg::stop_propagation() const
	{
		stop_propagation_ = true;
	}

	bool event_arg::propagation_stopped() const
	{
		return stop_propagation_;
	}
	//end class event_arg

	namespace detail
	{
		void events_operation_register(event_handle evt)
		{
			bedrock::instance().evt_operation().register_evt(evt);
		}

		class bedrock::flag_guard
		{
		public:
			flag_guard(bedrock* brock, basic_window * wd)
				: brock_{ brock }, wd_(wd)
			{
				wd_->flags.refreshing = true;
			}

			~flag_guard()
			{
				if (brock_->wd_manager().available((wd_)))
					wd_->flags.refreshing = false;
			}
		private:
			bedrock			*const brock_;
			basic_window	*const wd_;
		};

		//class root_guard
		bedrock::root_guard::root_guard(bedrock& brock, basic_window* root_wd):
			brock_(brock),
			root_wd_(root_wd)
		{
			root_wd_->other.attribute.root->lazy_update = true;
		}

		bedrock::root_guard::~root_guard()
		{
			if (!brock_.wd_manager().available(root_wd_))
				return;

			root_wd_->other.attribute.root->lazy_update = false;
			root_wd_->other.attribute.root->update_requesters.clear();
		}
		//end class root_guard

		basic_window* bedrock::focus()
		{
			auto wd = wd_manager().root(native_interface::get_focus_window());
			return (wd ? wd->other.attribute.root->focus : nullptr);
		}

		events_operation& bedrock::evt_operation()
		{
			return pi_data_->evt_operation;
		}

		window_manager& bedrock::wd_manager()
		{
			return pi_data_->wd_manager;
		}

		void bedrock::manage_form_loader(basic_window* wd, bool insert_or_remove)
		{
			if (insert_or_remove)
			{
				pi_data_->auto_form_set.insert(wd);
				return;
			}

			if (pi_data_->auto_form_set.erase(wd))
			{
				auto p = wd->widget_notifier->widget_ptr();
				delete p;
			}
		}

		void bedrock::close_thread_window(thread_t thread_id)
		{
			std::vector<basic_window*> v;
			wd_manager().all_handles(v);

			std::vector<native_window_type> roots;
			native_window_type root = nullptr;
			for (auto wd : v)
			{
				if (((0 == thread_id) || (wd->thread_id == thread_id)) && (wd->root != root))
				{
					root = wd->root;
					if (roots.end() == std::find(roots.begin(), roots.end(), root))
						roots.emplace_back(root);
				}
			}

			for (auto i : roots)
				native_interface::close_window(i);
		}

		void bedrock::event_expose(basic_window * wd, bool exposed)
		{
			if (nullptr == wd) return;

			wd->visible = exposed;

			arg_expose arg;
			arg.exposed = exposed;
			arg.window_handle = wd;
			if (emit(event_code::expose, wd, arg, false, get_thread_context()))
			{
				//Get the window who has the activated caret
				auto const caret_wd = ((wd->annex.caret_ptr && wd->annex.caret_ptr->activated()) ? wd : wd->child_caret());
				if (caret_wd)
				{
					if (exposed)
					{
						if (wd->root_widget->other.attribute.root->focus == caret_wd)
							caret_wd->annex.caret_ptr->visible(true);
					}
					else
						caret_wd->annex.caret_ptr->visible(false);
				}

				if ((!exposed) && (category::flags::root != wd->other.category))
				{
					//find an ancestor until it is not a lite_widget
					wd = wd->seek_non_lite_widget_ancestor();
				}

				wd_manager().refresh_tree(wd);
				wd_manager().map(wd, false);
			}
		}

		void bedrock::event_move(basic_window* wd, int x, int y)
		{
			if (wd)
			{
				arg_move arg;
				arg.window_handle = wd;
				arg.x = x;
				arg.y = y;
				emit(event_code::move, wd, arg, true, get_thread_context());
			}
		}

		bool bedrock::event_msleave(basic_window* hovered)
		{
			if (wd_manager().available(hovered) && hovered->flags.enabled)
			{
				hovered->set_action(mouse_action::normal);

				arg_mouse arg;
				arg.evt_code = event_code::mouse_leave;
				arg.window_handle = hovered;
				arg.pos.x = arg.pos.y = 0;
				arg.left_button = arg.right_button = arg.mid_button = false;
				arg.ctrl = arg.shift = false;
				emit(event_code::mouse_leave, hovered, arg, true, get_thread_context());
				return true;
			}
			return false;
		}

		//The wd must be a root window
		void bedrock::event_focus_changed(basic_window* root_wd, native_window_type receiver, bool getting)
		{
			auto focused = root_wd->other.attribute.root->focus;

			arg_focus arg;
			arg.window_handle = focused;
			arg.getting = getting;
			arg.receiver = receiver;

			if (getting)
			{
				if (root_wd->flags.enabled && root_wd->flags.take_active)
				{
					if (focused && focused->annex.caret_ptr)
						focused->annex.caret_ptr->activate(true);

					if (!emit(event_code::focus, focused, arg, true, get_thread_context()))
						this->wd_manager().set_focus(root_wd, true, arg_focus::reason::general);
				}
			}
			else if (root_wd->other.attribute.root->focus)
			{
				if (emit(event_code::focus, focused, arg, true, get_thread_context()))
				{
					if (focused->annex.caret_ptr)
						focused->annex.caret_ptr->activate(false);
				}
				close_menu_if_focus_other_window(receiver);
			}
		}

		void bedrock::update_cursor(basic_window * wd)
		{
			internal_scope_guard isg;
			if (wd_manager().available(wd))
			{
				auto * thrd = get_thread_context(wd->thread_id);
				if (nullptr == thrd)
					return;

				auto pos = native_interface::cursor_position();
				auto native_handle = native_interface::find_window(pos.x, pos.y);
				if (!native_handle)
					return;

				native_interface::calc_window_point(native_handle, pos);
				if (wd != wd_manager().find_window(native_handle, pos))
					return;

				set_cursor(wd, wd->predef_cursor, thrd);
			}
		}

		void bedrock::set_menubar_taken(basic_window* wd)
		{
			auto pre = pi_data_->menu.taken_window;
			pi_data_->menu.taken_window = wd;

			//assigning of a nullptr taken window is to restore the focus of pre taken
			//don't restore the focus if pre is a menu.
			if ((!wd) && pre && (pre->root != get_menu()))
			{
				internal_scope_guard lock;
				wd_manager().set_focus(pre, false, arg_focus::reason::general);
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
				if ((!pi_data_->menu.window) && pi_data_->menu.delay_restore)
					set_menubar_taken(nullptr);
				break;
			case 3:	//Restores if destroying
				//when the menu is destroying, restores the focus if delay restore is not declared
				if (!pi_data_->menu.delay_restore)
					set_menubar_taken(nullptr);
			}

			pi_data_->menu.delay_restore = (0 == state);
		}

		bool bedrock::close_menu_if_focus_other_window(native_window_type wd)
		{
			if (pi_data_->menu.window && (pi_data_->menu.window != wd))
			{
				wd = native_interface::get_window(wd, window_relationship::owner);
				while (wd)
				{
					if (wd != pi_data_->menu.window)
						wd = native_interface::get_window(wd, window_relationship::owner);
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
			if(menu_wd && pi_data_->menu.window != menu_wd)
			{
				erase_menu(true);

				pi_data_->menu.window = menu_wd;
				pi_data_->menu.owner = native_interface::get_window(menu_wd, window_relationship::owner);
				pi_data_->menu.has_keyboard = has_keyboard;
			}
		}

		native_window_type bedrock::get_menu(native_window_type owner, bool is_keyboard_condition)
		{
			if ((pi_data_->menu.owner == nullptr) ||
				(owner && (pi_data_->menu.owner == owner))
				)
			{
				return (is_keyboard_condition ? (pi_data_->menu.has_keyboard ? pi_data_->menu.window : nullptr) : pi_data_->menu.window);
			}

			return nullptr;
		}

		native_window_type bedrock::get_menu()
		{
			return pi_data_->menu.window;
		}

		void bedrock::erase_menu(bool try_destroy)
		{
			if (pi_data_->menu.window)
			{
				if (try_destroy)
					native_interface::close_window(pi_data_->menu.window);

				pi_data_->menu.window = pi_data_->menu.owner = nullptr;
				pi_data_->menu.has_keyboard = false;
			}
		}

		bool bedrock::shortkey_occurred(bool status)
		{
			auto last_status = pi_data_->shortkey_occurred;
			pi_data_->shortkey_occurred = status;
			return last_status;
		}

		bool bedrock::shortkey_occurred() const
		{
			return pi_data_->shortkey_occurred;
		}

		color_schemes& bedrock::scheme()
		{
			return pi_data_->scheme;
		}

		void bedrock::_m_emit_core(event_code evt_code, basic_window* wd, bool draw_only, const ::nana::event_arg& event_arg, const bool bForce__EmitInternal)
		{
			auto retain = wd->annex.events_ptr;
			auto evts_ptr = retain.get();

			// if 'bForce__EmitInternal', omit user defined events
			const bool bProcess__External_event = !draw_only && !bForce__EmitInternal;
			switch (evt_code)
			{
			case event_code::click:
				{
					auto arg = dynamic_cast<const arg_click*>(&event_arg);
					if (arg)
					{
						{
							//enable refreshing flag, this is a RAII class for exception-safe
							flag_guard fguard(this, wd);
							wd->drawer.click(*arg, bForce__EmitInternal);
						}
						if (bProcess__External_event)
							evts_ptr->click.emit(*arg, wd);
					}
				}
				break;
			case event_code::dbl_click:
			case event_code::mouse_enter:
			case event_code::mouse_move:
			case event_code::mouse_leave:
			case event_code::mouse_down:
			case event_code::mouse_up:
			{
				auto arg = dynamic_cast<const arg_mouse*>(&event_arg);
				if (nullptr == arg)
					return;

				void(::nana::detail::drawer::*drawer_event_fn)(const arg_mouse&, const bool);
				::nana::basic_event<arg_mouse>* evt_addr;

				switch (evt_code)
				{
				case event_code::dbl_click:
					drawer_event_fn = &drawer::dbl_click;
					evt_addr = &evts_ptr->dbl_click;
					break;
				case event_code::mouse_enter:
					drawer_event_fn = &drawer::mouse_enter;
					evt_addr = &evts_ptr->mouse_enter;
					break;
				case event_code::mouse_move:
					drawer_event_fn = &drawer::mouse_move;
					evt_addr = &evts_ptr->mouse_move;
					break;
				case event_code::mouse_leave:
					drawer_event_fn = &drawer::mouse_leave;
					evt_addr = &evts_ptr->mouse_leave;
					break;
				case event_code::mouse_down:
					drawer_event_fn = &drawer::mouse_down;
					evt_addr = &evts_ptr->mouse_down;
					break;
				case event_code::mouse_up:
					drawer_event_fn = &drawer::mouse_up;
					evt_addr = &evts_ptr->mouse_up;
					break;
				default:
					throw std::runtime_error("Invalid mouse event code");
				}

				{
					//enable refreshing flag, this is a RAII class for exception-safe
					flag_guard fguard(this, wd);
					(wd->drawer.*drawer_event_fn)(*arg, bForce__EmitInternal);
				}

				if (bProcess__External_event)
					evt_addr->emit(*arg, wd);
				break;
			}
			case event_code::mouse_wheel:
			{
				auto arg = dynamic_cast<const arg_wheel*>(&event_arg);
				if (arg)
				{
					{
						//enable refreshing flag, this is a RAII class for exception-safe
						flag_guard fguard(this, wd);
						wd->drawer.mouse_wheel(*arg, bForce__EmitInternal);
					}

					if (bProcess__External_event)
						evts_ptr->mouse_wheel.emit(*arg, wd);
				}
				break;
			}
			case event_code::key_press:
			case event_code::key_char:
			case event_code::key_release:
			case event_code::shortkey:
			{
				auto arg = dynamic_cast<const arg_keyboard*>(&event_arg);
				if (nullptr == arg)
					return;

				void(::nana::detail::drawer::*drawer_event_fn)(const arg_keyboard&, const bool);
				::nana::basic_event<arg_keyboard>* evt_addr;

				switch (evt_code)
				{
				case event_code::key_press:
					drawer_event_fn = &drawer::key_press;
					evt_addr = &evts_ptr->key_press;
					break;
				case event_code::key_char:
					drawer_event_fn = &drawer::key_char;
					evt_addr = &evts_ptr->key_char;
					break;
				case event_code::key_release:
					drawer_event_fn = &drawer::key_release;
					evt_addr = &evts_ptr->key_release;
					break;
				case event_code::shortkey:
					drawer_event_fn = &drawer::shortkey;
					evt_addr = &evts_ptr->shortkey;
					break;
				default:
					throw std::runtime_error("Invalid keyboard event code");
				}
				{
					//enable refreshing flag, this is a RAII class for exception-safe
					flag_guard fguard(this, wd);
					(wd->drawer.*drawer_event_fn)(*arg, bForce__EmitInternal);
				}

				if (bProcess__External_event)
					evt_addr->emit(*arg, wd);
				break;
			}
			case event_code::expose:
				if (bProcess__External_event)
				{
					auto arg = dynamic_cast<const arg_expose*>(&event_arg);
					if (arg)
						evts_ptr->expose.emit(*arg, wd);
				}
				break;
			case event_code::focus:
			{
				auto arg = dynamic_cast<const arg_focus*>(&event_arg);
				if (arg)
				{
					{
						//enable refreshing flag, this is a RAII class for exception-safe
						flag_guard fguard(this, wd);
						wd->drawer.focus(*arg, bForce__EmitInternal);
					}
					if (bProcess__External_event)
						evts_ptr->focus.emit(*arg, wd);
				}
				break;
			}
			case event_code::move:
			{
				auto arg = dynamic_cast<const arg_move*>(&event_arg);
				if (arg)
				{
					{
						//enable refreshing flag, this is a RAII class for exception-safe
						flag_guard fguard(this, wd);
						wd->drawer.move(*arg, bForce__EmitInternal);
					}
					if (bProcess__External_event)
						evts_ptr->move.emit(*arg, wd);
				}
				break;
			}
			case event_code::resizing:
			{
				auto arg = dynamic_cast<const arg_resizing*>(&event_arg);
				if (arg)
				{
					{
						//enable refreshing flag, this is a RAII class for exception-safe
						flag_guard fguard(this, wd);
						wd->drawer.resizing(*arg, bForce__EmitInternal);
					}
					if (bProcess__External_event)
						evts_ptr->resizing.emit(*arg, wd);
				}
				break;
			}
			case event_code::resized:
			{
				auto arg = dynamic_cast<const arg_resized*>(&event_arg);
				if (arg)
				{
					{
						//enable refreshing flag, this is a RAII class for exception-safe
						flag_guard fguard(this, wd);
						wd->drawer.resized(*arg, bForce__EmitInternal);
					}
					if (bProcess__External_event)
						evts_ptr->resized.emit(*arg, wd);
				}
				break;
			}
			case event_code::unload:
				if (bProcess__External_event)
				{
					auto arg = dynamic_cast<const arg_unload*>(&event_arg);
					if (arg && (wd->other.category == category::flags::root))
					{
						auto evt_root = dynamic_cast<events_root_extension*>(evts_ptr);
						if (evt_root)
							evt_root->unload.emit(*arg, wd);
					}
				}
				break;
			case event_code::destroy:
				if (bProcess__External_event)
				{
					auto arg = dynamic_cast<const arg_destroy*>(&event_arg);
					if (arg)
						evts_ptr->destroy.emit(*arg, wd);
				}
				break;
			default:
				throw std::runtime_error("Invalid event code");
			}
		}

		void bedrock::thread_context_destroy(basic_window * wd)
		{
			auto ctx = get_thread_context(0);
			if(ctx && ctx->event_window == wd)
				ctx->event_window = nullptr;
		}

		void bedrock::thread_context_lazy_refresh()
		{
			auto ctx = get_thread_context(0);
			if(ctx && ctx->event_window)
				ctx->event_window->other.upd_state = basic_window::update_state::refreshed;
		}

		bool bedrock::emit(event_code evt_code, basic_window* wd, const ::nana::event_arg& arg, bool ask_update, thread_context* thrd, const bool bForce__EmitInternal)
		{
			if(wd_manager().available(wd) == false)
				return false;

			basic_window * prev_wd = nullptr;
			if(thrd)
			{
				prev_wd = thrd->event_window;
				thrd->event_window = wd;
				_m_event_filter(evt_code, wd, thrd);
			}

			using update_state = basic_window::update_state;

			if (update_state::none == wd->other.upd_state)
				wd->other.upd_state = update_state::lazy;

			_m_emit_core(evt_code, wd, false, arg, bForce__EmitInternal);

			bool good_wd = false;
			if(wd_manager().available(wd))
			{
				//A child of wd may not be drawn if it was out of wd's range before wd resized,
				//so refresh all children of wd when a resized occurs.
				if(ask_update || (event_code::resized == evt_code) || (update_state::refreshed == wd->other.upd_state))
				{
					wd_manager().do_lazy_refresh(wd, false, (event_code::resized == evt_code));
				}
				else
					wd->other.upd_state = update_state::none;

				good_wd = true;
			}


			if(thrd) thrd->event_window = prev_wd;
			return good_wd;
		}

		void bedrock::_m_event_filter(event_code event_id, basic_window * wd, thread_context * thrd)
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
