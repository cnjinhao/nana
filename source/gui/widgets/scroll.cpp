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

				if(scale >= fixedsize * 2)
				{
					if(pos < static_cast<int>(fixedsize))
						return buttons::first;
					if(pos > static_cast<int>(scale - fixedsize))
						return buttons::second;
				}
				else
				{
					if(pos < static_cast<int>(scale / 2))
						return buttons::first;
					if(pos > static_cast<int>(scale / 2))
						return buttons::second;
				}

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
					metrics_.value = pos * value_max / scroll_area;
					if(metrics_.value < metrics_.peak - metrics_.range)
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
				if(_m_check())
				{
					if(buttons::forward == metrics_.what)
					{	//backward
						if(metrics_.value <= metrics_.range)
							metrics_.value = 0;
						else
							metrics_.value -= metrics_.range;
					}
					else if(buttons::backward == metrics_.what)
					{
						if(metrics_.peak - metrics_.range - metrics_.value <= metrics_.range)
							metrics_.value = metrics_.peak - metrics_.range;
						else
							metrics_.value += metrics_.range;
					}
				}
			}

			void drawer::draw(graph_reference graph, buttons what)
			{
				if(false == metrics_.pressed || metrics_.what != buttons::scroll)
					_m_adjust_scroll(graph);

				_m_background(graph);

				::nana::rectangle r(graph.size());
				if(vertical_)
				{
					r.y = r.height - fixedsize;
					r.height = fixedsize;
				}
				else
				{
					r.x = r.width - fixedsize;
					r.width = fixedsize;
				}

				int state = ((_m_check() == false || what == buttons::none) ? states::none : states::highlight);
				int moused_state = (_m_check() ? (metrics_.pressed ? states::selected : states::actived) : states::none);

				//draw first
				_m_draw_button(graph, { 0, 0, r.width, r.height }, buttons::first, (buttons::first == what ? moused_state : state));

				//draw second
				_m_draw_button(graph, r, buttons::second, (buttons::second == what ? moused_state : state));

				//draw scroll
				_m_draw_scroll(graph, (buttons::scroll == what ? moused_state : states::highlight));
				
			}
		//private:
			void drawer::_m_background(graph_reference graph)
			{
				graph.rectangle(true, {0xf0, 0xf0, 0xf0});

				if(metrics_.pressed && _m_check())
				{
					int x = 0, y = 0;
					unsigned width = graph.width(), height = graph.height();

					if(metrics_.what == buttons::forward)
					{
						*(vertical_ ? &y : &x) = fixedsize;
						*(vertical_ ? &height: &width) = metrics_.scroll_pos;
					}
					else if(buttons::backward == metrics_.what)
					{
						*(vertical_ ? &y : &x) = static_cast<int>(fixedsize + metrics_.scroll_pos + metrics_.scroll_length);
						*(vertical_ ? &height: &width) = static_cast<unsigned>((vertical_ ? graph.height() : graph.width()) - (fixedsize * 2 + metrics_.scroll_pos + metrics_.scroll_length));
					}
					else
						return;

					if(width && height)
						graph.rectangle({ x, y, width, height }, true, {0xDC, 0xDC, 0xDC});
				}
			}

			void drawer::_m_button_frame(graph_reference graph, rectangle r, int state)
			{
				if(state)
				{
					::nana::expr_color clr{0x97, 0x97, 0x97}; //highlight
					switch(state)
					{
					case states::actived:
						clr.from_rgb(0x86, 0xD5, 0xFD); break;
					case states::selected:
						clr.from_rgb(0x3C, 0x7F, 0xB1); break;
					}
					
					graph.rectangle(r, false, clr);

					graph.set_color(clr.blend(colors::white, 0.5));
					//unsigned color_x = graph.mix(color, 0xFFFFFF, 0.5);	//deprecated

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
					//graph.shadow_rectangle(x, y, width, height, 0xFFFFFF, color_x, !vertical_);
					graph.gradual_rectangle(r, colors::white, clr, !vertical_);
				}
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
					::nana::rectangle r(graph.size());

					if(vertical_)
					{
						r.y = fixedsize + metrics_.scroll_pos;
						r.height = static_cast<unsigned>(metrics_.scroll_length);
					}
					else
					{
						r.x = fixedsize + metrics_.scroll_pos;
						r.width = static_cast<unsigned>(metrics_.scroll_length);					
					}

					_m_button_frame(graph, r, state);
				}
			}

			void drawer::_m_draw_button(graph_reference graph, rectangle r, buttons what, int state)
			{
				if(_m_check())
					_m_button_frame(graph, r, state);
				
				using namespace nana::paint::gadget;

				if(buttons::first == what || buttons::second == what)
				{
					nana::size sz = graph.size();
					directions::t dir;
					if(buttons::second == what)
					{
						if(vertical_)
						{
							r.y = static_cast<int>(sz.height - fixedsize);
							dir = directions::to_south;
						}
						else
						{
							r.x = static_cast<int>(sz.width - fixedsize);
							dir = directions::to_east;
						}
					}
					else
						dir = vertical_ ? directions::to_north : directions::to_west;

					if(vertical_)
						r.x = (static_cast<int>(sz.width) - 16) / 2;
					else
						r.y = (static_cast<int>(sz.height) - 16) / 2;
					
					arrow_16_pixels(graph, r.x, r.y, (_m_check() ? expr_color(colors::black) : expr_color(0x80, 0x80, 0x80)), (states::none == state ? 0 : 1), dir);
				}
			}
		//end class drawer
		}//end namespace scroll
	}//end namespace drawerbase
}//end namespace nana

