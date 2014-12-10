/*
 *	A Scroll Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2014 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/scroll.hpp
 */
#ifndef NANA_GUI_WIDGET_SCROLL_HPP
#define NANA_GUI_WIDGET_SCROLL_HPP

#include "widget.hpp"
#include <nana/paint/gadget.hpp>
#include <nana/gui/timer.hpp>

namespace nana
{
	template<bool Vert> class scroll;

	template<bool Vert>
	struct arg_scroll
	{
		scroll<Vert> & widget;
	};

	namespace drawerbase
	{
		namespace scroll
		{
			template<bool Vert>
			struct scroll_events
				: public general_events
			{
				basic_event<arg_scroll<Vert>> value_changed;
			};

			enum class buttons
			{
				none, forward, backward, scroll, first, second
			};

			struct metrics_type
			{
				typedef std::size_t size_type;

				size_type peak;
				size_type range;
				size_type step;
				size_type value;

				buttons what;
				bool pressed;
				size_type	scroll_length;
				int			scroll_pos;
				int			scroll_mouse_offset;

				metrics_type();
			};

			class drawer
			{
			public:
				struct states
				{
					enum{none, highlight, actived, selected};
				};

				typedef nana::paint::graphics& graph_reference;
				const static unsigned fixedsize = 16;

				drawer(metrics_type& m);
				void set_vertical(bool);
				buttons what(graph_reference, const point&);
				void scroll_delta_pos(graph_reference, int);
				void auto_scroll();
				void draw(graph_reference, buttons);
			private:
				bool _m_check() const;
				void _m_adjust_scroll(graph_reference);
				void _m_background(graph_reference);
				void _m_button_frame(graph_reference, int x, int y, unsigned width, unsigned height, int state);
				void _m_draw_scroll(graph_reference, int state);
				void _m_draw_button(graph_reference, int x, int y, unsigned width, unsigned height, buttons what, int state);
			private:
				metrics_type &metrics_;
				bool	vertical_;
			};

			template<bool Vertical>
			class trigger
				: public drawer_trigger
			{
			public:
				typedef metrics_type::size_type size_type;

				trigger()
					: graph_(nullptr), drawer_(metrics_)
				{
					drawer_.set_vertical(Vertical);
				}

				const metrics_type& metrics() const
				{
					return metrics_;
				}

				void peak(size_type s)
				{
					if(graph_ && (metrics_.peak != s))
					{
						metrics_.peak = s;
						API::refresh_window(widget_->handle());
					}
				}

				void value(size_type s)
				{
					if(s + metrics_.range > metrics_.peak)
						s = metrics_.peak - metrics_.range;

					if(graph_ && (metrics_.value != s))
					{
						metrics_.value = s;
						_m_emit_value_changed();

						API::refresh_window(*widget_);
					}
				}

				void range(size_type s)
				{
					if(graph_ && (metrics_.range != s))
					{
						metrics_.range = s;
						API::refresh_window(widget_->handle());
					}
				}

				void step(size_type s)
				{
					metrics_.step = s;
				}

				bool make_step(bool forward, unsigned multiple)
				{
					if(graph_)
					{
						size_type step = (multiple > 1 ? metrics_.step * multiple : metrics_.step);
						size_type value = metrics_.value;
						if(forward)
						{
							size_type maxv = metrics_.peak - metrics_.range;
							if(metrics_.peak > metrics_.range && value < maxv)
							{
								if(maxv - value >= step)
									value += step;
								else
									value = maxv;
							}
						}
						else if(value)
						{
							if(value > step)
								value -= step;
							else
								value = 0;
						}
						size_type cmpvalue = metrics_.value;
						metrics_.value = value;
						if(value != cmpvalue)
						{
							_m_emit_value_changed();
							return true;
						}
						return false;
					}
					return false;
				}
			private:
				void attached(widget_reference widget, graph_reference graph)
				{
					graph_ = &graph;
					widget_ = static_cast< ::nana::scroll<Vertical>*>(&widget);
					widget.caption(STR("Nana Scroll"));

					timer_.stop();
					timer_.elapse(std::bind(&trigger::_m_tick, this));
				}

				void detached()
				{
					graph_ = nullptr;
				}

				void refresh(graph_reference graph)
				{
					drawer_.draw(graph, metrics_.what);
				}

				void resized(graph_reference graph, const ::nana::arg_resized&) override
				{
					drawer_.draw(graph, metrics_.what);
					API::lazy_refresh();
				}

				void mouse_enter(graph_reference graph, const ::nana::arg_mouse& arg) override
				{
					metrics_.what = drawer_.what(graph, arg.pos);
					drawer_.draw(graph, metrics_.what);
					API::lazy_refresh();
				}

				void mouse_move(graph_reference graph, const ::nana::arg_mouse& arg)
				{
					bool redraw = false;
					if(metrics_.pressed && (metrics_.what == buttons::scroll))
					{
						size_type cmpvalue = metrics_.value;
						drawer_.scroll_delta_pos(graph, (Vertical ? arg.pos.y : arg.pos.x));
						if(cmpvalue != metrics_.value)
							_m_emit_value_changed();
						redraw = true;
					}
					else
					{
						buttons what = drawer_.what(graph, arg.pos);
						if(metrics_.what != what)
						{
							redraw = true;
							metrics_.what = what;
						}
					}
					if(redraw)
					{
						drawer_.draw(graph, metrics_.what);
						API::lazy_refresh();
					}
				}

				void mouse_down(graph_reference graph, const arg_mouse& arg)
				{
					if(arg.left_button)
					{
						metrics_.pressed = true;
						metrics_.what = drawer_.what(graph, arg.pos);
						switch(metrics_.what)
						{
						case buttons::first:
						case buttons::second:
							make_step(metrics_.what == buttons::second, 1);
							timer_.interval(1000);
							timer_.start();
							break;
						case buttons::scroll:
							API::capture_window(widget_->handle(), true);
							metrics_.scroll_mouse_offset = (Vertical ? arg.pos.y : arg.pos.x) - metrics_.scroll_pos;
							break;
						case buttons::forward:
						case buttons::backward:
							{
								size_type cmpvalue = metrics_.value;
								drawer_.auto_scroll();
								if(cmpvalue != metrics_.value)
									_m_emit_value_changed();
							}
							break;
						default:	//Ignore buttons::none
							break;
						}
						drawer_.draw(graph, metrics_.what);
						API::lazy_refresh();
					}
				}

				void mouse_up(graph_reference graph, const arg_mouse& arg)
				{
					timer_.stop();

					API::capture_window(widget_->handle(), false);

					metrics_.pressed = false;
					metrics_.what = drawer_.what(graph, arg.pos);
					drawer_.draw(graph, metrics_.what);
					API::lazy_refresh();
				}

				void mouse_leave(graph_reference graph, const arg_mouse&)
				{
					if(metrics_.pressed) return;

					metrics_.what = buttons::none;
					drawer_.draw(graph, buttons::none);
					API::lazy_refresh();
				}

				void mouse_wheel(graph_reference graph, const arg_wheel& arg)
				{
					if(make_step(arg.upwards == false, 3))
					{
						drawer_.draw(graph, metrics_.what);
						API::lazy_refresh();
					}
				}
			private:
				void _m_emit_value_changed()
				{
					widget_->events().value_changed.emit(::nana::arg_scroll<Vertical>({*widget_}));
				}

				void _m_tick()
				{
					make_step(metrics_.what == buttons::second, 1);
					API::refresh_window(widget_->handle());
					timer_.interval(100);
				}
			private:
				::nana::scroll<Vertical> * widget_;
				nana::paint::graphics * graph_;
				metrics_type metrics_;
				drawer	drawer_;
				timer timer_;
			};
		}//end namespace scroll
	}//end namespace drawerbase

	/// Provides a way to display an object which is larger than the window's client area.
	template<bool Vertical>
	class scroll
		: public widget_object<category::widget_tag, drawerbase::scroll::trigger<Vertical>, drawerbase::scroll::scroll_events<Vertical>>
	{
		typedef widget_object<category::widget_tag, drawerbase::scroll::trigger<Vertical> > base_type;
	public:
		typedef std::size_t size_type;

		///  \brief The default constructor without creating the widget.
		scroll(){}

		/// \brief The construct that creates a widget.
		/// @param wd  A handle to the parent window of the widget being created.
		/// @param visible  specifying the visible after creating.
		scroll(window wd, bool visible)
		{
			this->create(wd, rectangle(), visible);
		}

		///  \brief The construct that creates a widget.
		/// @param wd  A handle to the parent window of the widget being created.
		/// @param r  the size and position of the widget in its parent window coordinate.
		/// @param visible  specifying the visible after creating.
		scroll(window wd, const rectangle& r, bool visible = true)
		{
			this->create(wd, r, visible);
		}

		///  \brief Determines whether it is scrollable.
		/// @param for_less  whether it can be scrolled for a less value.
		bool scrollable(bool for_less) const
		{
			auto & m = this->get_drawer_trigger().metrics();
			return (for_less ? (0 != m.value) : (m.value < m.peak - m.range));
		}

		size_type amount() const
		{
			return this->get_drawer_trigger().metrics().peak;
		}

		void amount(size_type Max)
		{
			return this->get_drawer_trigger().peak(Max);
		}

		/// Get the range of the widget.
		size_type range() const
		{
			return this->get_drawer_trigger().metrics().range;
		}

		/// Set the range of the widget.
		void range(size_type r)
		{
			return this->get_drawer_trigger().range(r);
		}

		///  \brief Get the value.
		/// @return the value.
		size_type value() const
		{
			return this->get_drawer_trigger().metrics().value;
		}

		///  \brief Set the value.
		/// @param s  a new value.
		void value(size_type s)
		{
			return this->get_drawer_trigger().value(s);
		}


		///  \brief Get the step of the sroll widget. The step indicates a variation of the value.
		/// @return the step.
		size_type step() const
		{
			return this->get_drawer_trigger().metrics().step;
		}

		///  \brief Set the step.
		/// @param s  a value for step.
		void step(size_type s)
		{
			return this->get_drawer_trigger().step(s);
		}

		///  \brief Increase/decrease values by a step.
		/// @param forward  it determines whether increase or decrease.
		/// @return true if the value is changed.
		bool make_step(bool forward)
		{
			if(this->get_drawer_trigger().make_step(forward, 1))
			{
				API::refresh_window(this->handle());
				return true;
			}
			return false;
		}

		///  \brief Increase/decrease values by steps as if it is scrolled through mouse wheel.
		/// @param forward  it determines whether increase or decrease.
		/// @return true if the vlaue is changed.
		bool make_scroll(bool forward)
		{
			if(this->get_drawer_trigger().make_step(forward, 3))
			{
				API::refresh_window(this->handle());
				return true;
			}
			return false;
		}
	};//end class scroll
}//end namespace nana
#endif
