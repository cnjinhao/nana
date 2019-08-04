/*
 *	A Drawer Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/detail/drawer.cpp
 */

#include "basic_window.hpp"
#include "effects_renderer.hpp"
#include <nana/config.hpp>
#include <nana/gui/detail/bedrock.hpp>
#include <nana/gui/detail/drawer.hpp>
#include "dynamic_drawing_object.hpp"

#if defined(NANA_X11)
	#include "../../detail/posix/platform_spec.hpp"
#endif

namespace nana
{
	//class drawer_trigger
		void drawer_trigger::attached(widget_reference, graph_reference){}
		void drawer_trigger::detached(){}	//none-const
		void drawer_trigger::typeface_changed(graph_reference){}
		void drawer_trigger::refresh(graph_reference){}

		void drawer_trigger::resizing(graph_reference, const arg_resizing&)
		{
			overridden_ &= ~(1 << static_cast<int>(event_code::resizing));
		}

		void drawer_trigger::resized(graph_reference graph, const arg_resized&)
		{
			overridden_ |= (1 << static_cast<int>(event_code::resized));
			this->refresh(graph);
			detail::bedrock::instance().thread_context_lazy_refresh();
		}

		void drawer_trigger::move(graph_reference, const arg_move&)
		{
			overridden_ &= ~(1 << static_cast<int>(event_code::move));
		}

		void drawer_trigger::click(graph_reference, const arg_click&)
		{
			overridden_ &= ~(1 << static_cast<int>(event_code::click));
		}

		void drawer_trigger::dbl_click(graph_reference, const arg_mouse&)
		{
			overridden_ &= ~(1 << static_cast<int>(event_code::dbl_click));
		}

		void drawer_trigger::mouse_enter(graph_reference, const arg_mouse&)
		{
			overridden_ &= ~(1 << static_cast<int>(event_code::mouse_enter));
		}

		void drawer_trigger::mouse_move(graph_reference, const arg_mouse&)
		{
			overridden_ &= ~(1 << static_cast<int>(event_code::mouse_move));
		}

		void drawer_trigger::mouse_leave(graph_reference, const arg_mouse&)
		{
			overridden_ &= ~(1 << static_cast<int>(event_code::mouse_leave));
		}

		void drawer_trigger::mouse_down(graph_reference, const arg_mouse&)
		{
			overridden_ &= ~(1 << static_cast<int>(event_code::mouse_down));
		}

		void drawer_trigger::mouse_up(graph_reference, const arg_mouse&)
		{
			overridden_ &= ~(1 << static_cast<int>(event_code::mouse_up));
		}

		void drawer_trigger::mouse_wheel(graph_reference, const arg_wheel&)
		{
			overridden_ &= ~(1 << static_cast<int>(event_code::mouse_wheel));
		}

		void drawer_trigger::mouse_dropfiles(graph_reference, const arg_dropfiles&)
		{
			overridden_ &= ~(1 << static_cast<int>(event_code::mouse_drop));
		}

		void drawer_trigger::focus(graph_reference, const arg_focus&)
		{
			overridden_ &= ~(1 << static_cast<int>(event_code::focus));
		}

		void drawer_trigger::key_ime(graph_reference, const arg_ime&)
		{
			overridden_ &= ~(1 << static_cast<int>(event_code::key_ime));
		}

		void drawer_trigger::key_press(graph_reference, const arg_keyboard&)
		{
			overridden_ &= ~(1 << static_cast<int>(event_code::key_press));
		}

		void drawer_trigger::key_char(graph_reference, const arg_keyboard&)
		{
			overridden_ &= ~(1 << static_cast<int>(event_code::key_char));
		}

		void drawer_trigger::key_release(graph_reference, const arg_keyboard&)
		{
			overridden_ &= ~(1 << static_cast<int>(event_code::key_release));
		}

		void drawer_trigger::shortkey(graph_reference, const arg_keyboard&)
		{
			overridden_ &= ~(1 << static_cast<int>(event_code::shortkey));
		}

		void drawer_trigger::_m_reset_overridden()
		{
			overridden_ = 0xFFFFFFFF;
		}

		bool drawer_trigger::_m_overridden(event_code evt_code) const
		{
			return 0 != (overridden_ & (1 << static_cast<int>(evt_code)));
		}

		void drawer_trigger::filter_event(const event_code evt_code, const bool bDisabled)
		{
			if (bDisabled)
				evt_disabled_ |= 1 << static_cast<int>(evt_code); // set
			else
				evt_disabled_ &= ~(1 << static_cast<int>(evt_code)); // clear
		}

		void drawer_trigger::filter_event(const std::vector<event_code>& evt_codes, const bool bDisabled)
		{
			for (auto evt_code : evt_codes) 
			{
				filter_event(evt_code, bDisabled);
			}
		}

		void drawer_trigger::filter_event(const event_filter_status& evt_all_states)
		{
			evt_disabled_ = evt_all_states.evt_disabled_;
		}

		bool drawer_trigger::filter_event(const event_code evt_code)
		{
			return static_cast<bool>((evt_disabled_ >> static_cast<int>(evt_code)) & 1);
		}

		event_filter_status drawer_trigger::filter_event()
		{
			return event_filter_status(evt_disabled_);
		}

		void drawer_trigger::clear_filter()
		{
			for (int i = 0; i < static_cast<int>(nana::event_code::end); i++)
				filter_event(static_cast<nana::event_code>(i), false);
		}

	//end class drawer_trigger

	//class event_filter_status
	event_filter_status::event_filter_status()
	{
		evt_disabled_ = 0;
	}

	event_filter_status::event_filter_status(const event_filter_status& rOther)
	{
		this->evt_disabled_ = rOther.evt_disabled_;
	}

	event_filter_status::event_filter_status(const unsigned evt_disabled_)
	{
		this->evt_disabled_ = evt_disabled_;
	}

	bool event_filter_status::operator[](const nana::event_code evt_code) const
	{
		return static_cast<bool>((evt_disabled_ >> static_cast<int>(evt_code)) & 1);
	}

	bool event_filter_status::operator==(const event_filter_status& rOther) const
	{
		return evt_disabled_ == rOther.evt_disabled_;
	}

	bool event_filter_status::operator!=(const event_filter_status& rOther) const
	{
		return evt_disabled_ != rOther.evt_disabled_;
	}

	const event_filter_status& event_filter_status::operator=(const event_filter_status& rOther)
	{
		evt_disabled_ = rOther.evt_disabled_;
		return *this;
	}

	const event_filter_status& event_filter_status::operator=(const unsigned evt_disabled_)
	{
		this->evt_disabled_ = evt_disabled_;
		return *this;
	}
	//end of class event_filter_status

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

		void drawer::click(const arg_click& arg, const bool bForce__EmitInternal)
		{
			_m_emit(event_code::click, arg, &drawer_trigger::click, bForce__EmitInternal);
		}

		void drawer::dbl_click(const arg_mouse& arg, const bool bForce__EmitInternal)
		{
			_m_emit(event_code::dbl_click, arg, &drawer_trigger::dbl_click, bForce__EmitInternal);
		}

		void drawer::mouse_enter(const arg_mouse& arg, const bool bForce__EmitInternal)
		{
			_m_emit(event_code::mouse_enter, arg, &drawer_trigger::mouse_enter, bForce__EmitInternal);
		}

		void drawer::mouse_move(const arg_mouse& arg, const bool bForce__EmitInternal)
		{
			_m_emit(event_code::mouse_move, arg, &drawer_trigger::mouse_move, bForce__EmitInternal);
		}

		void drawer::mouse_leave(const arg_mouse& arg, const bool bForce__EmitInternal)
		{
			_m_emit(event_code::mouse_leave, arg, &drawer_trigger::mouse_leave, bForce__EmitInternal);
		}

		void drawer::mouse_down(const arg_mouse& arg, const bool bForce__EmitInternal)
		{
			_m_emit(event_code::mouse_down, arg, &drawer_trigger::mouse_down, bForce__EmitInternal);
		}

		void drawer::mouse_up(const arg_mouse& arg, const bool bForce__EmitInternal)
		{
			_m_emit(event_code::mouse_up, arg, &drawer_trigger::mouse_up, bForce__EmitInternal);
		}

		void drawer::mouse_wheel(const arg_wheel& arg, const bool bForce__EmitInternal)
		{
			_m_emit(event_code::mouse_wheel, arg, &drawer_trigger::mouse_wheel, bForce__EmitInternal);
		}

		void drawer::mouse_dropfiles(const arg_dropfiles& arg, const bool bForce__EmitInternal)
		{
			_m_emit(event_code::mouse_drop, arg, &drawer_trigger::mouse_dropfiles, bForce__EmitInternal);
		}

		void drawer::resizing(const arg_resizing& arg, const bool bForce__EmitInternal)
		{
			_m_emit(event_code::resizing, arg, &drawer_trigger::resizing, bForce__EmitInternal);
		}

		void drawer::resized(const arg_resized& arg, const bool bForce__EmitInternal)
		{
			_m_emit(event_code::resized, arg, &drawer_trigger::resized, bForce__EmitInternal);
		}

		void drawer::move(const arg_move& arg, const bool bForce__EmitInternal)
		{
			_m_emit(event_code::move, arg, &drawer_trigger::move, bForce__EmitInternal);
		}

		void drawer::focus(const arg_focus& arg, const bool bForce__EmitInternal)
		{
			_m_emit(event_code::focus, arg, &drawer_trigger::focus, bForce__EmitInternal);
		}

		void drawer::key_ime(const arg_ime& arg, const bool bForce__EmitInternal)
		{
			_m_emit(event_code::key_ime, arg, &drawer_trigger::key_ime, bForce__EmitInternal);
		}

		void drawer::key_press(const arg_keyboard& arg, const bool bForce__EmitInternal)
		{
			_m_emit(event_code::key_press, arg, &drawer_trigger::key_press, bForce__EmitInternal);
		}

		void drawer::key_char(const arg_keyboard& arg, const bool bForce__EmitInternal)
		{
			_m_emit(event_code::key_char, arg, &drawer_trigger::key_char, bForce__EmitInternal);
		}

		void drawer::key_release(const arg_keyboard& arg, const bool bForce__EmitInternal)
		{
			_m_emit(event_code::key_release, arg, &drawer_trigger::key_release, bForce__EmitInternal);
		}

		void drawer::shortkey(const arg_keyboard& arg, const bool bForce__EmitInternal)
		{
			_m_emit(event_code::shortkey, arg, &drawer_trigger::shortkey, bForce__EmitInternal);
		}

		void drawer::map(window wd, bool forced, const rectangle* update_area)	//Copy the root buffer to screen
		{
			if(wd)
			{
				bool owns_caret = (wd->annex.caret_ptr) && (wd->annex.caret_ptr->visible());

				//The caret in X11 is implemented by Nana, it is different from Windows'
				//the caret in X11 is asynchronous, it is hard to hide and show the caret
				//immediately, and therefore the caret always be flickering when the graphics
				//buffer is mapping to the window.
				if(owns_caret)
				{
#ifndef NANA_X11
					wd->annex.caret_ptr->visible(false);
#else
					owns_caret = nana::detail::platform_spec::instance().caret_update(wd->root, *wd->root_graph, false);
#endif
				}

				edge_nimbus_renderer::instance().render(wd, forced, update_area);

				if(owns_caret)
				{
#ifndef NANA_X11
					wd->annex.caret_ptr->visible(true);
#else
					nana::detail::platform_spec::instance().caret_update(wd->root, *wd->root_graph, true);
#endif
				}
			}
		}

		void drawer::refresh()
		{
			if (data_impl_->realizer && (!(data_impl_->refreshing || graphics.size().empty())))
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
			realizer._m_reset_overridden();
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

	namespace detail
	{
		//class edge_nimbus_renderer
		edge_nimbus_renderer& edge_nimbus_renderer::instance()
		{
			static edge_nimbus_renderer object;
			return object;
		}

		void edge_nimbus_renderer::erase(basic_window* wd)
		{
			if (effects::edge_nimbus::none == wd->effect.edge_nimbus)
				return;

			auto root_wd = wd->root_widget;
			auto & nimbus = root_wd->other.attribute.root->effects_edge_nimbus;

			for (auto i = nimbus.begin(); i != nimbus.end(); ++i)
			{
				if (i->window == wd)
				{
					auto pixels = weight();
					rectangle r{ wd->pos_root, wd->dimension };
					r.x -= static_cast<int>(pixels);
					r.y -= static_cast<int>(pixels);
					r.width += static_cast<unsigned>(pixels << 1);
					r.height += static_cast<unsigned>(pixels << 1);

					root_wd->root_graph->paste(root_wd->root, r, r.x, r.y);

					nimbus.erase(i);
					break;
				}
			}
		}

		void edge_nimbus_renderer::render(basic_window* wd, bool forced, const rectangle* update_area)
		{
			bool copy_separately = true;
			std::vector<std::pair<rectangle, basic_window*>> rd_set;

			if (wd->root_widget->other.attribute.root->effects_edge_nimbus.size())
			{
				auto root_wd = wd->root_widget;

				auto & nimbus = root_wd->other.attribute.root->effects_edge_nimbus;

				auto focused = root_wd->other.attribute.root->focus;

				const unsigned pixels = weight();

				auto graph = root_wd->root_graph;

				nana::rectangle r;
				for (auto & action : nimbus)
				{
					if (_m_edge_nimbus(action.window, focused) && window_layer::read_visual_rectangle(action.window, r))
					{
						if (action.window == wd)
						{
							if (update_area)
								::nana::overlap(*update_area, rectangle(r), r);
							copy_separately = false;
						}

						//Avoiding duplicated rendering. If the window is declared to lazy refresh, it should be rendered.
						if ((forced && (action.window == wd)) || (focused == action.window) || !action.rendered || (action.window->other.upd_state == basic_window::update_state::refreshed))
						{
							rd_set.emplace_back(r, action.window);
							action.rendered = true;
						}
					}
					else if (action.rendered)
					{
						action.rendered = false;

						if (action.window == wd)
							copy_separately = false;

						::nana::rectangle erase_r(
							action.window->pos_root.x - static_cast<int>(pixels),
							action.window->pos_root.y - static_cast<int>(pixels),
							static_cast<unsigned>(action.window->dimension.width + (pixels << 1)),
							static_cast<unsigned>(action.window->dimension.height + (pixels << 1))
						);

						graph->paste(root_wd->root, erase_r, erase_r.x, erase_r.y);
					}
				}
			}

			if (copy_separately)
			{
				rectangle vr;
				if (window_layer::read_visual_rectangle(wd, vr))
				{
					if (update_area)
						::nana::overlap(*update_area, rectangle(vr), vr);
					wd->root_graph->paste(wd->root, vr, vr.x, vr.y);
				}
			}

			rectangle wd_r{ wd->pos_root, wd->dimension };
			wd_r.pare_off(-static_cast<int>(this->weight()));
			//Render
			for (auto & rd : rd_set)
			{
				auto other_wd = rd.second;

				if (other_wd != wd)
				{
					rectangle other_r{ other_wd->pos_root, other_wd->dimension };
					other_r.pare_off(-static_cast<int>(this->weight()));
					if (!overlapped(wd_r, other_r))
						continue;
				}
				_m_render_edge_nimbus(other_wd, rd.first);
			}
		}
		
		/// Determines whether the effect will be rendered for the given window.
		bool edge_nimbus_renderer::_m_edge_nimbus(basic_window * const wd, basic_window * const focused_wd)
		{
			// Don't render the effect if the window is disabled.
			if (wd->flags.enabled)
			{
				if ((focused_wd == wd) && (static_cast<unsigned>(wd->effect.edge_nimbus) & static_cast<unsigned>(effects::edge_nimbus::active)))
					return true;
				else if ((static_cast<unsigned>(wd->effect.edge_nimbus) & static_cast<unsigned>(effects::edge_nimbus::over)) && (wd->flags.action == mouse_action::hovered))
					return true;
			}
			return false;
		}

		void edge_nimbus_renderer::_m_render_edge_nimbus(basic_window* wd, const nana::rectangle & visual)
		{
			wd->flags.action_before = wd->flags.action;

			auto r = visual;
			r.pare_off(-static_cast<int>(weight()));
			rectangle good_r;
			if (overlap(r, rectangle{ wd->root_graph->size() }, good_r))
			{
				if ((good_r.x < wd->pos_root.x) || (good_r.y < wd->pos_root.y) ||
					(good_r.right() > visual.right()) || (good_r.bottom() > visual.bottom()))
				{
					auto graph = wd->root_graph;
					nana::paint::pixel_buffer pixbuf(graph->handle(), r);

					pixel_argb_t px0, px1, px2, px3;

					px0 = pixbuf.pixel(0, 0);
					px1 = pixbuf.pixel(r.width - 1, 0);
					px2 = pixbuf.pixel(0, r.height - 1);
					px3 = pixbuf.pixel(r.width - 1, r.height - 1);

					good_r.x = good_r.y = 1;
					good_r.width = r.width - 2;
					good_r.height = r.height - 2;
					pixbuf.rectangle(good_r, wd->annex.scheme->activated.get_color(), 0.95, false);

					good_r.x = good_r.y = 0;
					good_r.width = r.width;
					good_r.height = r.height;
					pixbuf.rectangle(good_r, wd->annex.scheme->activated.get_color(), 0.4, false);

					pixbuf.pixel(0, 0, px0);
					pixbuf.pixel(r.width - 1, 0, px1);
					pixbuf.pixel(0, r.height - 1, px2);
					pixbuf.pixel(r.width - 1, r.height - 1, px3);

					pixbuf.paste(wd->root, { r.x, r.y });

					std::vector<typename window_layer::wd_rectangle> overlaps;
					if (window_layer::read_overlaps(wd, visual, overlaps))
					{
						for (auto & wdr : overlaps)
							graph->paste(wd->root, wdr.r, wdr.r.x, wdr.r.y);
					}
				}
				else
					wd->root_graph->paste(wd->root, visual, visual.x, visual.y);
			}
		}

		//end class edge_nimbus_renderer
	}//end namespace detail
}//end namespace nana
