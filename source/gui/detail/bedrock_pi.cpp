/*
*	A Bedrock Platform-Independent Implementation
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file: nana/gui/detail/bedrock_pi.cpp
*/

#include <nana/detail/platform_spec_selector.hpp>
#include <nana/gui/detail/bedrock_pi_data.hpp>
#include <nana/gui/detail/event_code.hpp>
#include <nana/system/platform.hpp>
#include <sstream>
#include <nana/system/timepiece.hpp>
#include <nana/gui/wvl.hpp>
#include <nana/gui/detail/inner_fwd_implement.hpp>
#include <nana/gui/detail/native_window_interface.hpp>
#include <nana/gui/layout_utility.hpp>
#include <nana/gui/detail/element_store.hpp>
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

		void events_operation_cancel(event_handle evt)
		{
			bedrock::instance().evt_operation().cancel(evt);
		}

		class bedrock::flag_guard
		{
		public:
			flag_guard(bedrock* brock, core_window_t * wd)
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
			core_window_t	*const wd_;

		};

		events_operation& bedrock::evt_operation()
		{
			return pi_data_->evt_operation;
		}

		window_manager& bedrock::wd_manager()
		{
			return pi_data_->wd_manager;
		}

		void bedrock::manage_form_loader(core_window_t* wd, bool insert_or_remove)
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

		void bedrock::event_expose(core_window_t * wd, bool exposed)
		{
			if (nullptr == wd) return;

			wd->visible = exposed;

			arg_expose arg;
			arg.exposed = exposed;
			arg.window_handle = reinterpret_cast<window>(wd);
			if (emit(event_code::expose, wd, arg, false, get_thread_context()))
			{
				const core_window_t * caret_wd = (wd->together.caret ? wd : wd->child_caret());
				if (caret_wd)
				{
					if (exposed)
					{
						if (wd->root_widget->other.attribute.root->focus == caret_wd)
							caret_wd->together.caret->visible(true);
					}
					else
						caret_wd->together.caret->visible(false);
				}

				if (!exposed)
				{
					if (category::flags::root != wd->other.category)
					{
						//find an ancestor until it is not a lite_widget
						wd = wd->seek_non_lite_widget_ancestor();
					}
					else if (category::flags::frame == wd->other.category)
						wd = wd_manager().find_window(wd->root, wd->pos_root.x, wd->pos_root.y);
				}

				wd_manager().refresh_tree(wd);
				wd_manager().map(wd, false);
			}
		}

		void bedrock::event_move(core_window_t* wd, int x, int y)
		{
			if (wd)
			{
				arg_move arg;
				arg.window_handle = reinterpret_cast<window>(wd);
				arg.x = x;
				arg.y = y;
				if (emit(event_code::move, wd, arg, false, get_thread_context()))
					wd_manager().update(wd, false, true);
			}
		}

		bool bedrock::event_msleave(core_window_t* hovered)
		{
			if (wd_manager().available(hovered) && hovered->flags.enabled)
			{
				hovered->flags.action = mouse_action::normal;

				arg_mouse arg;
				arg.evt_code = event_code::mouse_leave;
				arg.window_handle = reinterpret_cast<window>(hovered);
				arg.pos.x = arg.pos.y = 0;
				arg.left_button = arg.right_button = arg.mid_button = false;
				arg.ctrl = arg.shift = false;
				emit(event_code::mouse_leave, hovered, arg, true, get_thread_context());
				return true;
			}
			return false;
		}

		void bedrock::update_cursor(core_window_t * wd)
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
				if (wd != wd_manager().find_window(native_handle, pos.x, pos.y))
					return;

				set_cursor(wd, wd->predef_cursor, thrd);
			}
		}

		widget_colors& bedrock::get_scheme_template(scheme_factory_base&& factory)
		{
			return pi_data_->scheme.scheme_template(std::move(factory));
		}

		widget_colors* bedrock::make_scheme(scheme_factory_base&& factory)
		{
			return pi_data_->scheme.create(std::move(factory));
		}

		void bedrock::_m_emit_core(event_code evt_code, core_window_t* wd, bool draw_only, const ::nana::event_arg& event_arg)
		{
			auto retain = wd->together.events_ptr;
			auto evts_ptr = retain.get();


			switch (evt_code)
			{
			case event_code::click:
				{
					auto arg = dynamic_cast<const arg_click*>(&event_arg);
					if (nullptr == arg)
						return;
					{
						//enable refreshing flag, this is a RAII class for exception-safe
						flag_guard fguard(this, wd);
						wd->drawer.click(*arg);
					}
					if (!draw_only)
						evts_ptr->click.emit(*arg);
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

				void(::nana::detail::drawer::*drawer_event_fn)(const arg_mouse&);
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
					(wd->drawer.*drawer_event_fn)(*arg);
				}
				
				if (!draw_only)
					evt_addr->emit(*arg);
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
						wd->drawer.mouse_wheel(*arg);
					}

					if (!draw_only)
						evts_ptr->mouse_wheel.emit(*arg);
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

				void(::nana::detail::drawer::*drawer_event_fn)(const arg_keyboard&);
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
					(wd->drawer.*drawer_event_fn)(*arg);
				}

				if (!draw_only)
					evt_addr->emit(*arg);
				break;
			}
			case event_code::expose:
				if (!draw_only)
				{
					auto arg = dynamic_cast<const arg_expose*>(&event_arg);
					if (arg)
						evts_ptr->expose.emit(*arg);
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
						wd->drawer.focus(*arg);
					}
					if (!draw_only)
						evts_ptr->focus.emit(*arg);
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
						wd->drawer.move(*arg);
					}
					if (!draw_only)
						evts_ptr->move.emit(*arg);
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
						wd->drawer.resizing(*arg);
					}
					if (!draw_only)
						evts_ptr->resizing.emit(*arg);
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
						wd->drawer.resized(*arg);
					}
					if (!draw_only)
						evts_ptr->resized.emit(*arg);
				}
				break;
			}
			case event_code::unload:
				if (!draw_only)
				{
					auto arg = dynamic_cast<const arg_unload*>(&event_arg);
					if (arg && (wd->other.category == category::flags::root))
					{
						auto evt_root = dynamic_cast<events_root_extension*>(evts_ptr);
						if (evt_root)
							evt_root->unload.emit(*arg);
					}
				}
				break;
			case event_code::destroy:
				if (!draw_only)
				{
					auto arg = dynamic_cast<const arg_destroy*>(&event_arg);
					if (arg)
						evts_ptr->destroy.emit(*arg);
				}
				break;
			default:
				throw std::runtime_error("Invalid event code");
			}
		}

		void bedrock::_m_except_handler()
		{
			std::vector<core_window_t*> v;
			wd_manager().all_handles(v);
			if (v.size())
			{
				std::vector<native_window_type> roots;
				native_window_type root = nullptr;
				unsigned tid = nana::system::this_thread_id();
				for (auto wd : v)
				{
					if ((wd->thread_id == tid) && (wd->root != root))
					{
						root = wd->root;
						if (roots.cend() == std::find(roots.cbegin(), roots.cend(), root))
							roots.push_back(root);
					}
				}

				for (auto i : roots)
					native_interface::close_window(i);
			}
		}
	}//end namespace detail
}//end namespace nana