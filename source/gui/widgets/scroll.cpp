/*
 *	A Scroll Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2018 Jinhao(cnjinhao@hotmail.com)
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
			metrics_type::metrics_type():
				peak(1),
				range(1),
				step(1),
				value(0),
				what(buttons::none),
				pressed(false),
				scroll_length(0),
				scroll_pos(0)
			{}
		//end struct metrics_type

		//class drawer
			drawer::drawer(bool vert) :
				vert(vert)
			{}

			buttons drawer::what(graph_reference graph, const point& screen_pos)
			{
				unsigned scale;
				int pos;

				if(vert)
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

				if(metrics.scroll_length)
				{
					if(metrics.scroll_pos + static_cast<int>(fixedsize) <= pos && pos < metrics.scroll_pos + static_cast<int>(fixedsize + metrics.scroll_length))
						return buttons::scroll;
				}

				if(static_cast<int>(fixedsize) <= pos && pos < metrics.scroll_pos)
					return buttons::forward;
				else if(metrics.scroll_pos + static_cast<int>(metrics.scroll_length) <= pos && pos < static_cast<int>(scale - fixedsize))
					return buttons::backward;

				return buttons::none;
			}

			void drawer::scroll_delta_pos(graph_reference graph, int mouse_pos)
			{
				if(mouse_pos + metrics.scroll_mouse_offset == metrics.scroll_pos) return;

				unsigned scale = vert ? graph.height() : graph.width();

				if(scale > fixedsize * 2)
				{
					int pos = mouse_pos - metrics.scroll_mouse_offset;
					const unsigned scroll_area = static_cast<unsigned>(scale - fixedsize * 2 - metrics.scroll_length);

					if(pos < 0)
						pos = 0;
					else if(pos > static_cast<int>(scroll_area))
						pos = static_cast<int>(scroll_area);

					metrics.scroll_pos = pos;
					auto value_max = metrics.peak - metrics.range;

					//Check scroll_area to avoiding division by zero.
					if (scroll_area)
						metrics.value = static_cast<std::size_t>(pos * (static_cast<double>(value_max) / scroll_area));	//converting to double to avoid overflow.

					if (metrics.value < value_max)
					{
						//converting to double to avoid overflow.
						auto const px_per_value = static_cast<double>(scroll_area) / value_max;
						int selfpos = static_cast<int>(metrics.value * px_per_value);
						int nextpos = static_cast<int>((metrics.value + 1) * px_per_value);

						if(selfpos != nextpos && (pos - selfpos > nextpos - pos))
							++metrics.value;
					}
					else
						metrics.value = value_max;
				}
			}

			void drawer::auto_scroll()
			{
				if (!_m_check())
					return;
				
				if(buttons::forward == metrics.what)
				{	//backward
					if(metrics.value <= metrics.range)
						metrics.value = 0;
					else
						metrics.value -= (metrics.range-1);
				}
				else if(buttons::backward == metrics.what)
				{
					if(metrics.peak - metrics.range - metrics.value <= metrics.range)
						metrics.value = metrics.peak - metrics.range;
					else
						metrics.value += (metrics.range-1);
				}
			}

			void drawer::draw(graph_reference graph)
			{
				if(false == metrics.pressed || metrics.what != buttons::scroll)
					_m_adjust_scroll(graph);

				_m_background(graph);

				rectangle_rotator r(vert, ::nana::rectangle{ graph.size() });
				r.x_ref() = static_cast<int>(r.w() - fixedsize);
				r.w_ref() = fixedsize;

				auto state = ((_m_check() == false || metrics.what == buttons::none) ? states::none : states::highlight);
				auto moused_state = (_m_check() ? (metrics.pressed ? states::selected : states::actived) : states::none);

				auto result = r.result();

				//draw first
				_m_draw_button(graph, { 0, 0, result.width, result.height }, buttons::first, (buttons::first == metrics.what ? moused_state : state));

				//draw second
				_m_draw_button(graph, result, buttons::second, (buttons::second == metrics.what ? moused_state : state));

				//draw scroll
				_m_draw_scroll(graph, (buttons::scroll == metrics.what ? moused_state : states::highlight));
				
			}
		//private:
			void drawer::_m_background(graph_reference graph)
			{
				graph.rectangle(true, {0xf0, 0xf0, 0xf0});

				if (!metrics.pressed || !_m_check())
					return;
				
				nana::rectangle_rotator r(vert, ::nana::rectangle{ graph.size() });
				if(metrics.what == buttons::forward)
				{
					r.x_ref() = static_cast<int>(fixedsize);
					r.w_ref() = metrics.scroll_pos;
				}
				else if(buttons::backward == metrics.what)
				{
					r.x_ref() = static_cast<int>(fixedsize + metrics.scroll_pos + metrics.scroll_length);
					r.w_ref() = static_cast<unsigned>((vert ? graph.height() : graph.width()) - (fixedsize * 2 + metrics.scroll_pos + metrics.scroll_length));
				}
				else
					return;

				auto result = r.result();
				if (!result.empty())
					graph.rectangle(result, true, static_cast<color_rgb>(0xDCDCDC));
			}

			void drawer::_m_button_frame(graph_reference graph, rectangle r, states state)
			{
				if (states::none == state)
					return;
				
				::nana::color clr{0x97, 0x97, 0x97}; //highlight
				switch(state)
				{
				case states::actived:
					clr.from_rgb(0x86, 0xD5, 0xFD); break;
				case states::selected:
					clr.from_rgb(0x3C, 0x7F, 0xB1); break;
				default: break;
				}
				
				graph.rectangle(r, false, clr);

				clr = clr.blend(colors::white, 0.5);
				graph.palette(false, clr);

				r.pare_off(2);
				if(vert)
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
				graph.gradual_rectangle(r, colors::white, clr, !vert);
			}

			bool drawer::_m_check() const
			{
				return (metrics.scroll_length && metrics.range && (metrics.peak > metrics.range));
			}

			void drawer::_m_adjust_scroll(graph_reference graph)
			{
				if(metrics.range == 0 || metrics.peak <= metrics.range) return;

				unsigned pixels = vert ? graph.height() : graph.width();

				int pos = 0;
				unsigned len = 0;

				if(pixels > fixedsize * 2)
				{
					pixels -= (fixedsize * 2);
					len = static_cast<unsigned>(pixels * metrics.range / metrics.peak);
					
					if(len < fixedsize)
						len = fixedsize;

					if(metrics.value)
					{
						pos = static_cast<int>(pixels - len);
						if(metrics.value + metrics.range >= metrics.peak)
							metrics.value = metrics.peak - metrics.range;
						else
							pos = static_cast<int>((metrics.value * pos) /(metrics.peak - metrics.range));
					}
				}

				metrics.scroll_pos = pos;
				metrics.scroll_length = len;
			}

			void drawer::_m_draw_scroll(graph_reference graph, states state)
			{
				if(_m_check())
				{
					rectangle_rotator r(vert, rectangle{ graph.size() });
					r.x_ref() = static_cast<int>(fixedsize + metrics.scroll_pos);
					r.w_ref() = static_cast<unsigned>(metrics.scroll_length);

					_m_button_frame(graph, r.result(), state);
				}
			}

			void drawer::_m_draw_button(graph_reference graph, rectangle r, buttons what, states state)
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
						if (vert)
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
						dir = vert ? direction::north : direction::west;

					if (vert)
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

