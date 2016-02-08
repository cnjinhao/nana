/*
 *	A Scroll Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2014 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/scroll.cpp
 */

#include <nana/gui/widgets/scroll.hpp>
#include <nana/gui/element.hpp>

namespace nana
{
	namespace drawerbase
	{
		namespace scroll
		{
		//struct metrics_type
			metrics_type::metrics_type()
				:peak(1), range(1), step(1), value(0),
				what(buttons::none), pressed(false), scroll_length(0), scroll_pos(0)
			{}
		//end struct metrics_type

		//class drawer
			drawer::drawer(metrics_type& m)
				:metrics_(m)
			{}

			void drawer::set_vertical(bool v)
			{
				vertical_ = v;
			}

			buttons drawer::what(graph_reference graph, const point& screen_pos)
			{
				unsigned scale;
				int pos;

				if(vertical_)
				{
					scale = graph.height();
					pos = screen_pos.y;
				}
				else
				{
					scale = graph.width();
					pos = screen_pos.x;
				}

				const auto bound_pos = static_cast<int>(scale >= fixedsize * 2 ? fixedsize : scale / 2);
				if (pos < bound_pos)
					return buttons::first;
				if (pos > static_cast<int>(scale) - bound_pos)
					return buttons::second;

				if(metrics_.scroll_length)
				{
					if(metrics_.scroll_pos + static_cast<int>(fixedsize) <= pos && pos < metrics_.scroll_pos + static_cast<int>(fixedsize + metrics_.scroll_length))
						return buttons::scroll;
				}

				if(static_cast<int>(fixedsize) <= pos && pos < metrics_.scroll_pos)
					return buttons::forward;
				else if(metrics_.scroll_pos + static_cast<int>(metrics_.scroll_length) <= pos && pos < static_cast<int>(scale - fixedsize))
					return buttons::backward;

				return buttons::none;
			}

			void drawer::scroll_delta_pos(graph_reference graph, int mouse_pos)
			{
				if(mouse_pos + metrics_.scroll_mouse_offset == metrics_.scroll_pos) return;

				unsigned scale = vertical_ ? graph.height() : graph.width();

				if(scale > fixedsize * 2)
				{
					int pos = mouse_pos - metrics_.scroll_mouse_offset;
					const unsigned scroll_area = static_cast<unsigned>(scale - fixedsize * 2 - metrics_.scroll_length);

					if(pos < 0)
						pos = 0;
					else if(pos > static_cast<int>(scroll_area))
						pos = static_cast<int>(scroll_area);

					metrics_.scroll_pos = pos;
					auto value_max = metrics_.peak - metrics_.range;

					//Check scroll_area to avoiding division by zero.
					if (scroll_area)
						metrics_.value = pos * value_max / scroll_area;

					if(metrics_.value < value_max)
					{
						int selfpos = static_cast<int>(metrics_.value * scroll_area / value_max);
						int nextpos = static_cast<int>((metrics_.value + 1) * scroll_area / value_max);

						if(selfpos != nextpos && (pos - selfpos > nextpos - pos))
							++metrics_.value;
					}
					else
						metrics_.value = value_max;
				}
			}

			void drawer::auto_scroll()
			{
				if (!_m_check())
					return;
				
				if(buttons::forward == metrics_.what)
				{	//backward
					if(metrics_.value <= metrics_.range)
						metrics_.value = 0;
					else
						metrics_.value -= (metrics_.range-1);
				}
				else if(buttons::backward == metrics_.what)
				{
					if(metrics_.peak - metrics_.range - metrics_.value <= metrics_.range)
						metrics_.value = metrics_.peak - metrics_.range;
					else
						metrics_.value += (metrics_.range-1);
				}
			}

			void drawer::draw(graph_reference graph, buttons what)
			{
				if(false == metrics_.pressed || metrics_.what != buttons::scroll)
					_m_adjust_scroll(graph);

				_m_background(graph);

				rectangle_rotator r(vertical_, ::nana::rectangle{ graph.size() });
				r.x_ref() = static_cast<int>(r.w() - fixedsize);
				r.w_ref() = fixedsize;

				int state = ((_m_check() == false || what == buttons::none) ? states::none : states::highlight);
				int moused_state = (_m_check() ? (metrics_.pressed ? states::selected : states::actived) : states::none);

				auto result = r.result();

				//draw first
				_m_draw_button(graph, { 0, 0, result.width, result.height }, buttons::first, (buttons::first == what ? moused_state : state));

				//draw second
				_m_draw_button(graph, result, buttons::second, (buttons::second == what ? moused_state : state));

				//draw scroll
				_m_draw_scroll(graph, (buttons::scroll == what ? moused_state : states::highlight));
				
			}
		//private:
			void drawer::_m_background(graph_reference graph)
			{
				graph.rectangle(true, {0xf0, 0xf0, 0xf0});

				if (!metrics_.pressed || !_m_check())
					return;
				
				nana::rectangle_rotator r(vertical_, ::nana::rectangle{ graph.size() });
				if(metrics_.what == buttons::forward)
				{
					r.x_ref() = static_cast<int>(fixedsize);
					r.w_ref() = metrics_.scroll_pos;
				}
				else if(buttons::backward == metrics_.what)
				{
					r.x_ref() = static_cast<int>(fixedsize + metrics_.scroll_pos + metrics_.scroll_length);
					r.w_ref() = static_cast<unsigned>((vertical_ ? graph.height() : graph.width()) - (fixedsize * 2 + metrics_.scroll_pos + metrics_.scroll_length));
				}
				else
					return;

				auto result = r.result();
				if (!result.empty())
					graph.rectangle(result, true, static_cast<color_rgb>(0xDCDCDC));
			}

			void drawer::_m_button_frame(graph_reference graph, rectangle r, int state)
			{
				if (!state)
					return;
				
				::nana::color clr{0x97, 0x97, 0x97}; //highlight
				switch(state)
				{
				case states::actived:
					clr.from_rgb(0x86, 0xD5, 0xFD); break;
				case states::selected:
					clr.from_rgb(0x3C, 0x7F, 0xB1); break;
				}
				
				graph.rectangle(r, false, clr);

				clr = clr.blend(colors::white, 0.5);
				graph.palette(false, clr);

				r.pare_off(2);
				if(vertical_)
				{
					unsigned half = r.width / 2;
					graph.rectangle({ r.x + static_cast<int>(r.width - half), r.y, half, r.height }, true);
					r.width -= half;
				}
				else
				{
					unsigned half = r.height / 2;
					graph.rectangle({r.x, r.y + static_cast<int>(r.height - half), r.width, half}, true);
					r.height -= half;
				}
				graph.gradual_rectangle(r, colors::white, clr, !vertical_);
			}

			bool drawer::_m_check() const
			{
				return (metrics_.scroll_length && metrics_.range && (metrics_.peak > metrics_.range));
			}

			void drawer::_m_adjust_scroll(graph_reference graph)
			{
				if(metrics_.range == 0 || metrics_.peak <= metrics_.range) return;

				unsigned pixels = vertical_ ? graph.height() : graph.width();

				int pos = 0;
				unsigned len = 0;

				if(pixels > fixedsize * 2)
				{
					pixels -= (fixedsize * 2);
					len = static_cast<unsigned>(pixels * metrics_.range / metrics_.peak);
					
					if(len < fixedsize)
						len = fixedsize;

					if(metrics_.value)
					{
						pos = static_cast<int>(pixels - len);
						if(metrics_.value + metrics_.range >= metrics_.peak)
							metrics_.value = metrics_.peak - metrics_.range;
						else
							pos = static_cast<int>((metrics_.value * pos) /(metrics_.peak - metrics_.range));
					}
				}

				metrics_.scroll_pos = pos;
				metrics_.scroll_length = len;
			}

			void drawer::_m_draw_scroll(graph_reference graph, int state)
			{
				if(_m_check())
				{
					rectangle_rotator r(vertical_, rectangle{ graph.size() });
					r.x_ref() = static_cast<int>(fixedsize + metrics_.scroll_pos);
					r.w_ref() = static_cast<unsigned>(metrics_.scroll_length);

					_m_button_frame(graph, r.result(), state);
				}
			}

			void drawer::_m_draw_button(graph_reference graph, rectangle r, buttons what, int state)
			{
				if(_m_check())
					_m_button_frame(graph, r, state);

				if(buttons::first == what || buttons::second == what)
				{
					auto sz = graph.size();
					int top = static_cast<int>(sz.height - fixedsize);
					int left = static_cast<int>(sz.width - fixedsize);

					direction dir;
					if (buttons::second == what)
					{
						if (vertical_)
						{
							r.y = top;
							dir = direction::south;
						}
						else
						{
							r.x = left;
							dir = direction::east;
						}
					}
					else
						dir = vertical_ ? direction::north : direction::west;

					if (vertical_)
						r.x = left / 2;
					else
						r.y = top / 2;

					r.width = r.height = 16;

					facade<element::arrow> arrow(states::none == state ? "hollow_triangle" : "solid_triangle");
					arrow.direction(dir);
					arrow.draw(graph, {}, (_m_check() ? colors::black : colors::gray), r, element_state::normal);
				}
			}
		//end class drawer
		}//end namespace scroll
	}//end namespace drawerbase
}//end namespace nana

