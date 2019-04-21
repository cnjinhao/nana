/**
 *	A Scroll Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/scroll.hpp
 *	@contributors: Ariel Vina-Rodriguez
 */
#ifndef NANA_GUI_WIDGET_SCROLL_HPP
#define NANA_GUI_WIDGET_SCROLL_HPP

#include "widget.hpp"
#include <nana/gui/timer.hpp>
#include <nana/push_ignore_diagnostic>

namespace nana
{
	template<bool Vert> class scroll;	//forward declaration

	struct arg_scroll
		: public event_arg
	{
		window window_handle;

		arg_scroll(window wd)
			: window_handle{ wd }
		{}
	};

	namespace drawerbase
	{
		namespace scroll
		{
			struct scroll_events
				: public general_events
			{
				basic_event<arg_scroll> value_changed;
			};

			enum class buttons
			{
				none, forward, backward, scroll, first, second
			};

			struct metrics_type
			{
				using size_type = std::size_t;

				size_type peak;   ///< the whole total
				size_type range;  ///< how many is shown on a page, that is, How many to scroll after click on first or second
				size_type step;   ///< how many to scroll by click in forward  or backward
				size_type value;  ///< current offset calculated from the very beginning

				buttons what;
				bool pressed;
				size_type	scroll_length;       ///< the length in pixels of the central button show how many of the total (peak) is shown (range)
				int			scroll_pos;          ///< in pixels, and correspond to the offset from the very beginning (value)
				int			scroll_mouse_offset;

				metrics_type();
			};

			class drawer
			{
			public:
				enum class states
				{
					none, highlight, actived, selected
				};

				using graph_reference = paint::graphics&;
				const static unsigned fixedsize = 16; // make it part of a new "metric" in the widget_scheme

				drawer(bool vert);
				buttons what(graph_reference, const point&);
				void scroll_delta_pos(graph_reference, int);
				void auto_scroll();
				void draw(graph_reference);

			private:
				bool _m_check() const;
				void _m_adjust_scroll(graph_reference);
				void _m_background(graph_reference);
				void _m_button_frame(graph_reference, ::nana::rectangle, states state);
				void _m_draw_scroll(graph_reference, states state);
				void _m_draw_button(graph_reference, ::nana::rectangle, buttons what, states state);
			public:
				metrics_type	metrics;
				bool const		vert;
			};

			template<bool Vertical>
			class trigger
				: public drawer_trigger
			{
			public:
				typedef metrics_type::size_type size_type;

				trigger()
					: graph_(nullptr), drawer_(Vertical)
				{
				}

				const metrics_type& metrics() const
				{
					return drawer_.metrics;
				}

				void peak(size_type s)
				{
					if (graph_ && (drawer_.metrics.peak != s))
					{
						drawer_.metrics.peak = s;
						API::refresh_window(widget_->handle());
					}
				}

				void value(size_type s)
				{
					if (drawer_.metrics.range > drawer_.metrics.peak)
						s = 0;
					else if (s + drawer_.metrics.range > drawer_.metrics.peak)
						s = drawer_.metrics.peak - drawer_.metrics.range;

					if (graph_ && (drawer_.metrics.value != s))
					{
						drawer_.metrics.value = s;
						_m_emit_value_changed();

						API::refresh_window(*widget_);
					}
				}

				void range(size_type s)
				{
					if (graph_ && (drawer_.metrics.range != s))
					{
						drawer_.metrics.range = s;
						API::refresh_window(widget_->handle());
					}
				}

				void step(size_type s)
				{
					drawer_.metrics.step = (s ? s : 1);
				}

				bool make_step(bool forward, unsigned multiple)
				{
					if (!graph_)
						return false;
					
					size_type step = (multiple > 1 ? drawer_.metrics.step * multiple : drawer_.metrics.step);
					size_type value = drawer_.metrics.value;
					if (forward)
					{
						size_type maxv = drawer_.metrics.peak - drawer_.metrics.range;
						if (drawer_.metrics.peak > drawer_.metrics.range && value < maxv)
						{
							if (maxv - value >= step)
								value += step;
							else
								value = maxv;
						}
					}
					else if (value)
					{
						if (value > step)
							value -= step;
						else
							value = 0;
					}
					size_type cmpvalue = drawer_.metrics.value;
					drawer_.metrics.value = value;
					if (value != cmpvalue)
					{
						_m_emit_value_changed();
						return true;
					}
					return false;
				}
			private:
				void attached(widget_reference widget, graph_reference graph) override
				{
					graph_ = &graph;
					widget_ = static_cast< ::nana::scroll<Vertical>*>(&widget);
					widget.caption("nana scroll");

					//scroll doesn't want the keyboard focus.
					API::take_active(widget, false, widget.parent());

					timer_.stop();
					timer_.elapse(std::bind(&trigger::_m_tick, this));
				}

				void detached() override
				{
					graph_ = nullptr;
				}

				void refresh(graph_reference graph) override
				{
					drawer_.draw(graph);
				}

				void resized(graph_reference graph, const ::nana::arg_resized&) override
				{
					drawer_.draw(graph);
					API::dev::lazy_refresh();
				}

				void mouse_enter(graph_reference graph, const ::nana::arg_mouse& arg) override
				{
					drawer_.metrics.what = drawer_.what(graph, arg.pos);
					drawer_.draw(graph);
					API::dev::lazy_refresh();
				}

				void mouse_move(graph_reference graph, const ::nana::arg_mouse& arg) override
				{
					if (drawer_.metrics.pressed && (drawer_.metrics.what == buttons::scroll))
					{
						size_type cmpvalue = drawer_.metrics.value;
						drawer_.scroll_delta_pos(graph, (Vertical ? arg.pos.y : arg.pos.x));
						if (cmpvalue != drawer_.metrics.value)
							_m_emit_value_changed();
					}
					else
					{
						buttons what = drawer_.what(graph, arg.pos);
						if (drawer_.metrics.what == what)
							return; //no change, don't redraw

						drawer_.metrics.what = what;
					}

					drawer_.draw(graph);
					API::dev::lazy_refresh();
				}

				void dbl_click(graph_reference graph, const arg_mouse& arg) override
				{
					mouse_down(graph, arg);
				}

				void mouse_down(graph_reference graph, const arg_mouse& arg) override
				{
					if (arg.left_button)
					{
						drawer_.metrics.pressed = true;
						drawer_.metrics.what = drawer_.what(graph, arg.pos);
						switch (drawer_.metrics.what)
						{
						case buttons::first:
						case buttons::second:
							make_step(drawer_.metrics.what == buttons::second, 1);
							timer_.interval(std::chrono::seconds{1});
							timer_.start();
							break;
						case buttons::scroll:
							widget_->set_capture(true);
							drawer_.metrics.scroll_mouse_offset = (Vertical ? arg.pos.y : arg.pos.x) - drawer_.metrics.scroll_pos;
							break;
						case buttons::forward:
						case buttons::backward:
						{
							size_type cmpvalue = drawer_.metrics.value;
							drawer_.auto_scroll();
							if (cmpvalue != drawer_.metrics.value)
								_m_emit_value_changed();
						}
						break;
						default:	//Ignore buttons::none
							break;
						}
						drawer_.draw(graph);
						API::dev::lazy_refresh();
					}
				}

				void mouse_up(graph_reference graph, const arg_mouse& arg) override
				{
					timer_.stop();

					widget_->release_capture();

					drawer_.metrics.pressed = false;
					drawer_.metrics.what = drawer_.what(graph, arg.pos);
					drawer_.draw(graph);
					API::dev::lazy_refresh();
				}

				void mouse_leave(graph_reference graph, const arg_mouse&) override
				{
					if (drawer_.metrics.pressed) return;

					drawer_.metrics.what = buttons::none;
					drawer_.draw(graph);
					API::dev::lazy_refresh();
				}

				void mouse_wheel(graph_reference graph, const arg_wheel& arg) override
				{
					if (make_step(arg.upwards == false, 3))
					{
						drawer_.draw(graph);
						API::dev::lazy_refresh();
					}
				}
			private:
				void _m_emit_value_changed()
				{
					widget_->events().value_changed.emit({ widget_->handle() }, widget_->handle());
				}

				void _m_tick()
				{
					make_step(drawer_.metrics.what == buttons::second, 1);
					API::refresh_window(widget_->handle());
					timer_.interval(std::chrono::milliseconds{ 100 });
				}
			private:
				::nana::scroll<Vertical> * widget_;
				nana::paint::graphics * graph_;
				drawer	drawer_;
				timer timer_;
			};
		}//end namespace scroll
	}//end namespace drawerbase

	class scroll_interface
	{
	public:
		using size_type = std::size_t;

		virtual ~scroll_interface() = default;

		///  \brief Determines whether it is scrollable.
		/// @param for_less  whether it can be scrolled for a less value (backward or "up" if true, forward or "down" if false).
		virtual bool scrollable(bool for_less) const = 0;
		
		///  the whole total (peak)
		virtual size_type amount() const = 0;

		virtual void amount(size_type peak) = 0;

		/// Get the range of the widget (how many is shown on a page, that is, How many to scroll after click on first or second)
		virtual size_type range() const = 0;

		/// Set the range of the widget.
		virtual void range(size_type r) = 0;

		///  \brief Get the value (current offset calculated from the very beginning)
		/// @return the value.
		virtual size_type value() const = 0;

		///  \brief Set the value.
		/// @param s  a new value.
		virtual void value(size_type s) = 0;


		///  \brief Get the step of the sroll widget. The step indicates a variation of the value.
		/// @return the step.
		virtual size_type step() const = 0;

		///  \brief Set the step.
		/// @param s  a value for step.
		virtual void step(size_type s) = 0;

		///  \brief Increase/decrease values by a step (alternatively by some number of steps).
		/// @param forward  it determines whether increase or decrease.
		/// @return true if the value is changed.
		virtual bool make_step(bool forward, unsigned steps = 1) = 0;

		virtual window window_handle() const = 0;
	};

	/// Provides a way to display an object which is larger than the window's client area.
	template<bool Vertical>
	class scroll    // add a widget scheme?
		:	public widget_object<category::widget_tag, drawerbase::scroll::trigger<Vertical>, drawerbase::scroll::scroll_events>,
			public scroll_interface
	{
		typedef widget_object<category::widget_tag, drawerbase::scroll::trigger<Vertical> > base_type;
	public:

		///  \brief The default constructor without creating the widget.
		scroll(){}

		/// \brief The construct that creates a widget.
		/// @param wd  A handle to the parent window of the widget being created.
		/// @param visible  specify the visibility after creation.
		scroll(window wd, bool visible = true)
		{
			this->create(wd, rectangle(), visible);   // add a widget scheme? and take some colors from these wd?
		}

		///  \brief The construct that creates a widget.
		/// @param wd  A handle to the parent window of the widget being created.
		/// @param r  the size and position of the widget in its parent window coordinate.
		/// @param visible  specify the visibility after creation.
		scroll(window wd, const rectangle& r, bool visible = true)
		{
			this->create(wd, r, visible);
		}

		///  \brief Determines whether it is scrollable.
		/// @param for_less  whether it can be scrolled for a less value (backward or "up" if true, forward or "down" if false).
		bool scrollable(bool for_less) const override
		{
			auto & m = this->get_drawer_trigger().metrics();
			return (for_less ? (0 != m.value) : (m.value < m.peak - m.range));
		}
		///  the whole total (peak)
		size_type amount() const override
		{
			return this->get_drawer_trigger().metrics().peak;
		}

		void amount(size_type peak) override
		{
			return this->get_drawer_trigger().peak(peak);
		}

		/// Get the range of the widget (how many is shown on a page, that is, How many to scroll after click on first or second)
		size_type range() const override
		{
			return this->get_drawer_trigger().metrics().range;
		}

		/// Set the range of the widget.
		void range(size_type r) override
		{
			return this->get_drawer_trigger().range(r);
		}

		///  \brief Get the value (current offset calculated from the very beginning)
		/// @return the value.
		size_type value() const override
		{
			return this->get_drawer_trigger().metrics().value;
		}

		///  \brief Set the value.
		/// @param s  a new value.
		void value(size_type s) override
		{
			return this->get_drawer_trigger().value(s);
		}

		///  \brief Get the step of the sroll widget. The step indicates a variation of the value.
		/// @return the step.
		size_type step() const override
		{
			return this->get_drawer_trigger().metrics().step;
		}

		///  \brief Set the step.
		/// @param s  a value for step.
		void step(size_type s) override
		{
			return this->get_drawer_trigger().step(s);
		}

		///  \brief Increase/decrease values by a step (alternatively by some number of steps).
		/// @param forward  it determines whether increase or decrease.
		/// @return true if the value is changed.
		bool make_step(bool forward, unsigned steps = 1) override
		{
			if (this->get_drawer_trigger().make_step(forward, steps))
			{
				API::refresh_window(this->handle());
				return true;
			}
			return false;
		}

		window window_handle() const override
		{
			return this->handle();
		}

		///  \brief Increase/decrease values by steps as if it is scrolled through mouse wheel.
		/// @param forward  it determines whether increase or decrease.
		/// @return true if the value is changed.
		bool make_scroll(bool forward)
		{
			return this->make_step(forward, 3);	// set this 3 in the metrics of the widget scheme ?
		}

		///  \brief Increase/decrease values by a page as if it is scrolled page up.
		/// @param forward  it determines whether increase or decrease.
		/// @return true if the value is changed.
		bool make_page_scroll(bool forward)
		{
			auto const count = range() / step();
			return this->make_step(forward, static_cast<unsigned>(count > 2 ? count - 1 : 1));
		}
	};//end class scroll
}//end namespace nana
#include <nana/pop_ignore_diagnostic>
#endif
