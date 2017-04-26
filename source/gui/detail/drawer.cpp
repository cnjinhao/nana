/*
 *	A Drawer Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2017 Jinhao(cnjinhao@hotmail.com)
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
#include <nana/gui/detail/effects_renderer.hpp>
#include <nana/gui/detail/basic_window.hpp>
#include "dynamic_drawing_object.hpp"

#if defined(NANA_X11)
	#include "../../detail/posix/platform_spec.hpp"
#endif

namespace nana
{
	typedef detail::edge_nimbus_renderer<detail::bedrock::core_window_t> edge_nimbus_renderer_t;

	//class drawer_trigger
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

		enum{
			event_size = static_cast<int>(event_code::end)
		};

		struct drawer::data_implement
		{
			bool			refreshing{ false };
			basic_window*	window_handle{ nullptr };
			drawer_trigger*	realizer{ nullptr };
			method_state	mth_state[event_size];
			std::vector<dynamic_drawing::object*>	draws;
		};

		drawer::drawer()
			: data_impl_{ new data_implement }
		{}

		drawer::~drawer()
		{
			for(auto p : data_impl_->draws)
			{
				delete p;
			}

			delete data_impl_;
		}

		void drawer::bind(basic_window* cw)
		{
			data_impl_->window_handle = cw;
		}

		void drawer::typeface_changed()
		{
			if(data_impl_->realizer)
				data_impl_->realizer->typeface_changed(graphics);
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
				bool owns_caret = (iwd->annex.caret_ptr) && (iwd->annex.caret_ptr->visible());

				//The caret in X11 is implemented by Nana, it is different from Windows'
				//the caret in X11 is asynchronous, it is hard to hide and show the caret
				//immediately, and therefore the caret always be flickering when the graphics
				//buffer is mapping to the window.
				if(owns_caret)
				{
#ifndef NANA_X11
					iwd->annex.caret_ptr->visible(false);
#else
					owns_caret = nana::detail::platform_spec::instance().caret_update(iwd->root, *iwd->root_graph, false);
#endif
				}

				edge_nimbus_renderer_t::instance().render(iwd, forced, update_area);

				if(owns_caret)
				{
#ifndef NANA_X11
					iwd->annex.caret_ptr->visible(true);
#else
					nana::detail::platform_spec::instance().caret_update(iwd->root, *iwd->root_graph, true);
#endif
				}
			}
		}

		void drawer::refresh()
		{
			if (data_impl_->realizer && !data_impl_->refreshing)
			{
				data_impl_->refreshing = true;
				data_impl_->realizer->refresh(graphics);
				_m_effect_bground_subsequent();
				graphics.flush();
				data_impl_->refreshing = false;
			}
		}

		drawer_trigger* drawer::realizer() const
		{
			return data_impl_->realizer;
		}

		void drawer::attached(widget& wd, drawer_trigger& realizer)
		{
			for (auto i = std::begin(data_impl_->mth_state), end = std::end(data_impl_->mth_state); i != end; ++i)
				*i = method_state::pending;

			data_impl_->realizer = &realizer;
			realizer._m_reset_overrided();
			realizer.attached(wd, graphics);
			realizer.typeface_changed(graphics);
		}

		drawer_trigger* drawer::detached()
		{
			if (data_impl_->realizer)
			{
				auto rmp = data_impl_->realizer;
				data_impl_->realizer = nullptr;
				rmp->detached();
				return rmp;
			}
			return nullptr;
		}

		void drawer::clear()
		{
			std::vector<dynamic_drawing::object*> then;
			for (auto p : data_impl_->draws)
			{
				if(p->diehard())
					then.emplace_back(p);
				else
					delete p;
			}

			then.swap(data_impl_->draws);
		}

		void* drawer::draw(std::function<void(paint::graphics&)> && f, bool diehard)
		{
			if(f)
			{
				auto p = new dynamic_drawing::user_draw_function(std::move(f), diehard);
				data_impl_->draws.emplace_back(p);
				return (diehard ? p : nullptr);
			}
			return nullptr;
		}

		void drawer::erase(void * p)
		{
			if(p)
			{
				for (auto i = data_impl_->draws.begin(); i != data_impl_->draws.end(); ++i)
					if (*i == p)
					{
						delete (*i);
						data_impl_->draws.erase(i);
						break;
					}
			}
		}

		void drawer::_m_effect_bground_subsequent()
		{
			auto & effect = data_impl_->window_handle->effect;

			for (auto * dw : data_impl_->draws)
				dw->draw(graphics);


			if (effect.bground)
			{
				if (effect.bground_fade_rate >= 0.01)
				{
					graphics.blend(::nana::rectangle{ data_impl_->window_handle->other.glass_buffer.size() }, data_impl_->window_handle->other.glass_buffer, {}, 1 - effect.bground_fade_rate);
				}
			}
			
		}

		drawer::method_state& drawer::_m_mth_state(int pos)
		{
			return data_impl_->mth_state[pos];
		}
	}//end namespace detail
}//end namespace nana
