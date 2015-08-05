/*
 *	A Drawer Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2014 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/detail/drawer.cpp
 */

#include <nana/config.hpp>
#include <nana/gui/detail/bedrock.hpp>
#include <nana/gui/detail/drawer.hpp>
#include <nana/gui/detail/dynamic_drawing_object.hpp>
#include <nana/gui/detail/effects_renderer.hpp>
#include <nana/gui/detail/basic_window.hpp>

#if defined(NANA_X11)
	#include <nana/detail/linux_X11/platform_spec.hpp>
#endif

namespace nana
{
	typedef detail::edge_nimbus_renderer<detail::bedrock::core_window_t> edge_nimbus_renderer_t;

	//class drawer_trigger
		drawer_trigger::~drawer_trigger(){}
		void drawer_trigger::attached(widget_reference, graph_reference){}
		void drawer_trigger::detached(){}	//none-const
		void drawer_trigger::typeface_changed(graph_reference){}
		void drawer_trigger::refresh(graph_reference){}

		void drawer_trigger::resizing(graph_reference, const arg_resizing&)
		{
			overrided_ &= ~(1 << static_cast<int>(event_code::resizing));
		}

		void drawer_trigger::resized(graph_reference graph, const arg_resized&)
		{
			overrided_ |= (1 << static_cast<int>(event_code::resized));
			this->refresh(graph);
			detail::bedrock::instance().thread_context_lazy_refresh();
		}

		void drawer_trigger::move(graph_reference, const arg_move&)
		{
			overrided_ &= ~(1 << static_cast<int>(event_code::move));
		}

		void drawer_trigger::click(graph_reference, const arg_click&)
		{
			overrided_ &= ~(1 << static_cast<int>(event_code::click));
		}

		void drawer_trigger::dbl_click(graph_reference, const arg_mouse&)
		{
			overrided_ &= ~(1 << static_cast<int>(event_code::dbl_click));
		}

		void drawer_trigger::mouse_enter(graph_reference, const arg_mouse&)
		{
			overrided_ &= ~(1 << static_cast<int>(event_code::mouse_enter));
		}

		void drawer_trigger::mouse_move(graph_reference, const arg_mouse&)
		{
			overrided_ &= ~(1 << static_cast<int>(event_code::mouse_move));
		}

		void drawer_trigger::mouse_leave(graph_reference, const arg_mouse&)
		{
			overrided_ &= ~(1 << static_cast<int>(event_code::mouse_leave));
		}

		void drawer_trigger::mouse_down(graph_reference, const arg_mouse&)
		{
			overrided_ &= ~(1 << static_cast<int>(event_code::mouse_down));
		}

		void drawer_trigger::mouse_up(graph_reference, const arg_mouse&)
		{
			overrided_ &= ~(1 << static_cast<int>(event_code::mouse_up));
		}

		void drawer_trigger::mouse_wheel(graph_reference, const arg_wheel&)
		{
			overrided_ &= ~(1 << static_cast<int>(event_code::mouse_wheel));
		}

		void drawer_trigger::mouse_dropfiles(graph_reference, const arg_dropfiles&)
		{
			overrided_ &= ~(1 << static_cast<int>(event_code::mouse_drop));
		}

		void drawer_trigger::focus(graph_reference, const arg_focus&)
		{
			overrided_ &= ~(1 << static_cast<int>(event_code::focus));
		}

		void drawer_trigger::key_press(graph_reference, const arg_keyboard&)
		{
			overrided_ &= ~(1 << static_cast<int>(event_code::key_press));
		}

		void drawer_trigger::key_char(graph_reference, const arg_keyboard&)
		{
			overrided_ &= ~(1 << static_cast<int>(event_code::key_char));
		}

		void drawer_trigger::key_release(graph_reference, const arg_keyboard&)
		{
			overrided_ &= ~(1 << static_cast<int>(event_code::key_release));
		}

		void drawer_trigger::shortkey(graph_reference, const arg_keyboard&)
		{
			overrided_ &= ~(1 << static_cast<int>(event_code::shortkey));
		}

		void drawer_trigger::_m_reset_overrided()
		{
			overrided_ = 0xFFFFFFFF;
		}

		bool drawer_trigger::_m_overrided(event_code evt_code) const
		{
			return 0 != (overrided_ & (1 << static_cast<int>(evt_code)));
		}

	//end class drawer_trigger

	namespace detail
	{
		typedef bedrock bedrock_type;

		//class drawer
		drawer::~drawer()
		{
			for(auto p : dynamic_drawing_objects_)
			{
				delete p;
			}
		}

		void drawer::bind(basic_window* cw)
		{
			core_window_ = cw;
		}

		void drawer::typeface_changed()
		{
			if(realizer_)
				realizer_->typeface_changed(graphics);
		}

		void drawer::click(const arg_click& arg)
		{
			_m_emit(event_code::click, arg, &drawer_trigger::click);
		}

		void drawer::dbl_click(const arg_mouse& arg)
		{
			_m_emit(event_code::dbl_click, arg, &drawer_trigger::dbl_click);
		}

		void drawer::mouse_enter(const arg_mouse& arg)
		{
			_m_emit(event_code::mouse_enter, arg, &drawer_trigger::mouse_enter);
		}

		void drawer::mouse_move(const arg_mouse& arg)
		{
			_m_emit(event_code::mouse_move, arg, &drawer_trigger::mouse_move);
		}

		void drawer::mouse_leave(const arg_mouse& arg)
		{
			_m_emit(event_code::mouse_leave, arg, &drawer_trigger::mouse_leave);
		}

		void drawer::mouse_down(const arg_mouse& arg)
		{
			_m_emit(event_code::mouse_down, arg, &drawer_trigger::mouse_down);
		}

		void drawer::mouse_up(const arg_mouse& arg)
		{
			_m_emit(event_code::mouse_up, arg, &drawer_trigger::mouse_up);
		}

		void drawer::mouse_wheel(const arg_wheel& arg)
		{
			_m_emit(event_code::mouse_wheel, arg, &drawer_trigger::mouse_wheel);
		}

		void drawer::mouse_dropfiles(const arg_dropfiles& arg)
		{
			_m_emit(event_code::mouse_drop, arg, &drawer_trigger::mouse_dropfiles);
		}

		void drawer::resizing(const arg_resizing& arg)
		{
			_m_emit(event_code::resizing, arg, &drawer_trigger::resizing);
		}

		void drawer::resized(const arg_resized& arg)
		{
			_m_emit(event_code::resized, arg, &drawer_trigger::resized);
		}

		void drawer::move(const arg_move& arg)
		{
			_m_emit(event_code::move, arg, &drawer_trigger::move);
		}

		void drawer::focus(const arg_focus& arg)
		{
			_m_emit(event_code::focus, arg, &drawer_trigger::focus);
		}

		void drawer::key_press(const arg_keyboard& arg)
		{
			_m_emit(event_code::key_press, arg, &drawer_trigger::key_press);
		}

		void drawer::key_char(const arg_keyboard& arg)
		{
			_m_emit(event_code::key_char, arg, &drawer_trigger::key_char);
		}

		void drawer::key_release(const arg_keyboard& arg)
		{
			_m_emit(event_code::key_release, arg, &drawer_trigger::key_release);
		}

		void drawer::shortkey(const arg_keyboard& arg)
		{
			_m_emit(event_code::shortkey, arg, &drawer_trigger::shortkey);
		}

		void drawer::map(window wd, bool forced, const rectangle* update_area)	//Copy the root buffer to screen
		{
			if(wd)
			{
				auto iwd = reinterpret_cast<bedrock_type::core_window_t*>(wd);
				auto caret_wd = iwd->root_widget->other.attribute.root->focus;

				bool owns_caret = (caret_wd && (caret_wd->together.caret) && (caret_wd->together.caret->visible()));

				//The caret in X11 is implemented by Nana, it is different from Windows'
				//the caret in X11 is asynchronous, it is hard to hide and show the caret
				//immediately, and therefore the caret always be flickering when the graphics
				//buffer is mapping to the window.
				if(owns_caret)
				{
#ifndef NANA_X11
					caret_wd->together.caret->visible(false);
#else
					owns_caret = nana::detail::platform_spec::instance().caret_update(iwd->root, *iwd->root_graph, false);
#endif
				}

				edge_nimbus_renderer_t::instance().render(iwd, forced, update_area);

				if(owns_caret)
				{
#ifndef NANA_X11
					caret_wd->together.caret->visible(true);
#else
					nana::detail::platform_spec::instance().caret_update(iwd->root, *iwd->root_graph, true);
#endif
				}
			}
		}

		void drawer::refresh()
		{
			if(realizer_ && (refreshing_ == false))
			{
				refreshing_ = true;
				_m_bground_pre();
				realizer_->refresh(graphics);
				_m_draw_dynamic_drawing_object();
				_m_bground_end();
				graphics.flush();
				refreshing_ = false;
			}
		}

		drawer_trigger* drawer::realizer() const
		{
			return realizer_;
		}

		void drawer::attached(widget& wd, drawer_trigger& realizer)
		{
			for (auto i = std::begin(mth_state_), end = std::end(mth_state_); i != end; ++i)
				*i = method_state::pending;

			realizer_ = &realizer;
			realizer._m_reset_overrided();
			realizer.attached(wd, graphics);
		}

		drawer_trigger* drawer::detached()
		{
			if(realizer_)
			{
				auto rmp = realizer_;
				realizer_ = nullptr;
				rmp->detached();
				return rmp;
			}
			return nullptr;
		}

		void drawer::clear()
		{
			std::vector<dynamic_drawing::object*> then;
			for(auto p : dynamic_drawing_objects_)
			{
				if(p->diehard())
					then.push_back(p);
				else
					delete p;
			}

			then.swap(dynamic_drawing_objects_);
		}

		void* drawer::draw(std::function<void(paint::graphics&)> && f, bool diehard)
		{
			if(f)
			{
				auto p = new dynamic_drawing::user_draw_function(std::move(f), diehard);
				dynamic_drawing_objects_.push_back(p);
				return (diehard ? p : nullptr);
			}
			return nullptr;
		}

		void drawer::erase(void * p)
		{
			if(p)
			{
				for (auto i = dynamic_drawing_objects_.begin(); i != dynamic_drawing_objects_.end(); ++i)
					if (*i == p)
					{
						delete (*i);
						dynamic_drawing_objects_.erase(i);
						break;
					}
			}
		}

		void drawer::_m_bground_pre()
		{
			if(core_window_->effect.bground && core_window_->effect.bground_fade_rate < 0.01)
				core_window_->other.glass_buffer.paste(graphics, 0, 0);
		}

		void drawer::_m_bground_end()
		{
			if(core_window_->effect.bground && core_window_->effect.bground_fade_rate >= 0.01)
				core_window_->other.glass_buffer.blend(::nana::rectangle{ core_window_->other.glass_buffer.size() }, graphics, nana::point(), core_window_->effect.bground_fade_rate);
		}

		void drawer::_m_draw_dynamic_drawing_object()
		{
			for(auto * dw : dynamic_drawing_objects_)
				dw->draw(graphics);
		}

		bool drawer::_m_lazy_decleared() const
		{
			return (basic_window::update_state::refresh == core_window_->other.upd_state);
		}
	}//end namespace detail
}//end namespace nana
