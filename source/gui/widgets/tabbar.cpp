/*
 *	A Tabbar Implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/tabbar.hpp
 */
#include <nana/gui/widgets/tabbar.hpp>
#include <nana/gui/widgets/menu.hpp>
#include <nana/paint/text_renderer.hpp>
#include <stdexcept>
#include <list>

namespace nana
{
	namespace drawerbase
	{
		namespace tabbar
		{
			event_agent_interface::~event_agent_interface()
			{}

			struct item_t
			{
				window relative;
				paint::image img;
				nana::string text;
				any	value;

				color_t bgcolor;
				color_t fgcolor;

				item_t()
					:relative(nullptr), bgcolor(nana::null_color), fgcolor(nana::null_color)
				{}
			};

			class def_renderer
				: public item_renderer
			{
			public:
				def_renderer()
					: bgcolor_(0xFF000000)
				{}

			private:
				virtual void background(graph_reference graph, const nana::rectangle& r, nana::color_t bgcolor)
				{
					if(bgcolor_ != bgcolor)
					{
						bgcolor_ = bgcolor;
						dark_bgcolor_ = nana::paint::graphics::mix(bgcolor, 0, 0.9);
						blcolor_ = nana::paint::graphics::mix(bgcolor, 0, 0.5);
						ilcolor_ = nana::paint::graphics::mix(bgcolor, 0xFFFFFF, 0.5);
					}

					graph.rectangle(bgcolor, true);
				}

				virtual void item(graph_reference graph, const item_t& m, bool active, state_t sta)
				{
					//*
					const nana::rectangle & r = m.r;
					nana::color_t bgcolor;
					nana::color_t blcolor;
					nana::color_t dark_bgcolor;

					if(m.bgcolor == nana::null_color)
					{
						bgcolor = bgcolor_;
						blcolor = blcolor_;
						dark_bgcolor = dark_bgcolor_;
					}
					else
					{
						bgcolor = m.bgcolor;
						blcolor = graph.mix(m.bgcolor, 0, 0.5);
						dark_bgcolor = nana::paint::graphics::mix(m.bgcolor, 0, 0.9);
					}

					graph.round_rectangle(r.x, r.y, r.width, r.height + 2, 3, 3, blcolor, true, 0xFFFFFF);

					nana::color_t beg = bgcolor;
					nana::color_t end = dark_bgcolor;

					if(active)
					{
						if(m.bgcolor == nana::null_color)
							beg = ilcolor_;
						else
							beg = nana::paint::graphics::mix(m.bgcolor, 0xFFFFFF, 0.5);
						end = bgcolor;
					}

					if(sta == item_renderer::highlight)
						beg = nana::paint::graphics::mix(beg, 0xFFFFFF, 0.5);

					graph.shadow_rectangle(r.x + 2, r.y + 2, r.width - 4, r.height - 2, beg,  end, true);
				}

				virtual void add(graph_reference graph, const nana::rectangle& r, state_t sta)
				{
					int x = r.x + (static_cast<int>(r.width) - 14) / 2;
					int y = r.y + (static_cast<int>(r.height) - 14) / 2;
					nana::color_t color;

					switch(sta)
					{
					case item_renderer::highlight:
						color = 0xFFFFFF; break;
					case item_renderer::press:
						color = 0xA0A0A0; break;
					case item_renderer::disable:
						color = 0x808080; break;
					default:
						color = 0xF0F0F0;
					}
					graph.rectangle(r, bgcolor_, true);
					nana::paint::gadget::cross(graph, x, y, 14, 6, color);
				}

				virtual void close(graph_reference graph, const nana::rectangle& r, state_t sta)
				{
					nana::paint::gadget::close_16_pixels(graph, r.x + (r.width - 16) / 2, r.y + (r.height - 16) / 2, 1, 0x0);
					if(sta == item_renderer::highlight)
					{
						graph.rectangle(r, 0xA0A0A0, false);
					}
				}

				virtual void close_fly(graph_reference graph, const nana::rectangle& r, bool active, state_t sta)
				{
					using namespace nana::paint;
					nana::color_t color = (active ? 0x0 : 0x9299A4);

					if(item_renderer::highlight == sta)
					{
						nana::color_t bgcolor = 0xCCD2DD;
						graph.round_rectangle(r.x, r.y, r.width, r.height, 1, 1, 0x9DA3AB, false, 0);
						nana::rectangle draw_r(r);
						graph.rectangle(draw_r.pare_off(1), graph.mix(0x9DA3AB, bgcolor, 0.8), false);
						graph.rectangle(draw_r.pare_off(1), graph.mix(0x9DA3AB, bgcolor, 0.4), false);
						graph.rectangle(draw_r.pare_off(1), graph.mix(0x9DA3AB, bgcolor, 0.2), false);
						color = 0x0;
					}

					int x = r.x - (16 - r.width) / 2;
					int y = r.y - (16 - r.height) / 2;

					gadget::close_16_pixels(graph, x, y, 1, color);
				}

				virtual void back(graph_reference graph, const nana::rectangle& r, state_t sta)
				{
					using namespace nana::paint::gadget;
					_m_draw_arrow(graph, r, sta, directions::to_west);
				}

				virtual void next(graph_reference graph, const nana::rectangle& r, state_t sta)
				{
					using namespace nana::paint::gadget;
					_m_draw_arrow(graph, r, sta, directions::to_east);
				}

				virtual void list(graph_reference graph, const nana::rectangle& r, state_t sta)
				{
					using namespace nana::paint::gadget;
					_m_draw_arrow(graph, r, sta, directions::to_south);
				}
			private:
				void _m_draw_arrow(graph_reference graph, const nana::rectangle& r, state_t sta, nana::paint::gadget::directions::t dir)
				{
					using namespace nana::paint::gadget;

					nana::color_t fgcolor = 0x0;
					int style = 1;
					if(sta == item_renderer::disable)
					{
						style = 0;
						fgcolor = 0x808080;
					}
					arrow_16_pixels(graph, r.x + (r.width - 16) / 2, r.y + (r.height - 16) / 2, fgcolor, style, dir);
					if(sta == item_renderer::highlight)
					{
						graph.rectangle(r, 0xA0A0A0, false);
					}
				}
			private:
				nana::color_t bgcolor_;
				nana::color_t dark_bgcolor_;
				nana::color_t blcolor_;
				nana::color_t ilcolor_;
			};

			class toolbox
			{
				struct button_tag
				{
					bool visible;
					bool enable;
				};
			public:
				enum button_t{ButtonAdd, ButtonScrollBack, ButtonScrollNext, ButtonList, ButtonClose, ButtonSize};

				toolbox()
					: close_fly_(false)
				{
					for(int i = ButtonAdd; i < ButtonSize; ++i)
					{
						buttons_[i].visible = true;
						buttons_[i].enable = true;
					}
					buttons_[0].enable = false;
					buttons_[3].enable = false;
					buttons_[4].enable = false;
				}

				nana::rectangle area(button_t btn, unsigned height) const
				{
					nana::rectangle r(-1, 0, 0, 0);
					if((btn == ButtonAdd) || (btn == ButtonClose && close_fly_))
						return r;

					int x = 0;
					for(int i = ButtonScrollBack; i < ButtonSize; ++i)
					{
						if(btn != i)
						{
							if(i != ButtonAdd && buttons_[i].visible && buttons_[i].enable)
								x += item_pixels();
						}
						else
						{
							r.x = x;
							r.width = item_pixels();
							r.height = height;
							return r;
						}
					}
					return r;
				}

				bool renderable(button_t btn) const
				{
					if(ButtonAdd <= btn && btn < ButtonSize)
					{
						if((btn == ButtonClose) && close_fly_)
							return false;
						return (buttons_[btn].visible && buttons_[btn].enable);
					}
					return false;
				}

				bool visible(button_t btn, bool vs)
				{
					if(buttons_[btn].visible != vs)
					{
						buttons_[btn].visible = vs;
						return true;
					}
					return false;
				}

				bool visible(button_t btn) const
				{
					return buttons_[btn].visible;
				}

				bool close_fly(bool fly)
				{
					if(close_fly_ != fly)
					{
						close_fly_ = fly;
						return true;
					}
					return false;
				}

				bool close_fly() const
				{
					return close_fly_;
				}

				bool enable(button_t btn) const
				{
					return buttons_[btn].enable;
				}

				bool enable(button_t btn, bool enb)
				{
					if(buttons_[btn].enable != enb)
					{
						buttons_[btn].enable = enb;
						return true;
					}
					return false;
				}

				unsigned width() const
				{
					unsigned pixels = 0;
					for(int i = ButtonScrollBack; i < ButtonSize; ++i)
					{
						if(renderable(static_cast<button_t>(i)))
						{
							if((i != ButtonClose) || (close_fly_ == false))
								pixels += item_pixels();
						}
					}
					return pixels;
				}

				unsigned item_pixels() const
				{
					return 18;
				}

				button_t which(int x) const
				{
					for(int i = ButtonAdd; i < ButtonSize; ++i)
					{
						if((i == ButtonClose && close_fly_) || (i == ButtonAdd))
							continue;

						if(renderable(static_cast<button_t>(i)))
						{
							if(0 <= x && x < static_cast<int>(item_pixels()))
								return static_cast<button_t>(i);
							else
								x -= static_cast<int>(item_pixels());
						}
					}
					return ButtonSize;
				}
			private:
				bool close_fly_;
				button_tag buttons_[ButtonSize];
			};

			//
			class layouter
			{
			public:
				typedef std::list<item_t>::iterator iterator;
				typedef std::list<item_t>::const_iterator const_iterator;

				nana::any& at(std::size_t i)
				{
					if(i < list_.size())
						return at_no_bound_check(i);
					throw std::out_of_range("Nana.GUI.tabbar::at() is out of range");
				}

				iterator iterator_at(std::size_t pos)
				{
					auto i = list_.begin();
					std::advance(i, pos);
					return i;
				}

				const_iterator iterator_at(std::size_t pos) const
				{
					auto i = list_.cbegin();
					std::advance(i, pos);
					return i;
				}

				nana::any& at_no_bound_check(std::size_t pos)
				{
					return iterator_at(pos)->value;
				}

				const nana::any& at(std::size_t pos) const
				{
					if(pos < list_.size())
						return at_no_bound_check(pos);
					throw std::out_of_range("Nana.GUI.tabbar::at() const is out of range");
				}

				const nana::any& at_no_bound_check(std::size_t pos) const
				{
					return iterator_at(pos)->value;
				}

				toolbox & toolbox_object()
				{
					return toolbox_;
				}

				window widget_handle() const
				{
					return basis_.wd;
				}

				void attach(window wd, nana::paint::graphics& graph)
				{
					basis_.wd = wd;
					basis_.graph = &graph;
				}

				void detach()
				{
					basis_.graph = 0;
				}

				const pat::cloneable<item_renderer> & ext_renderer() const
				{
					return basis_.renderer;
				}

				void ext_renderer(const pat::cloneable<item_renderer>& ir)
				{
					basis_.renderer = ir;
				}

				void event_agent(event_agent_interface* evt)
				{
					evt_agent_ = evt;
				}

				void push_back(const nana::string& text, const nana::any & value)
				{
					item_t m;
					m.text = text;
					m.value = value;
					list_.push_back(m);
					activate(static_cast<size_t>(list_.size() - 1));
					render();
				}

				std::size_t length() const
				{
					return list_.size();
				}

				bool erase(std::size_t pos)
				{
					if(pos < list_.size())
					{
						if ((nullptr == evt_agent_) || evt_agent_->removed(pos))
						{
							list_.erase(iterator_at(pos));
							_m_adjust();

							if(pos < basis_.active)
							{
								--basis_.active;
								if(basis_.scroll_pixels > basis_.item_pixels)
									basis_.scroll_pixels -= basis_.item_pixels;
								else
									basis_.scroll_pixels = 0;
							}
							else
							{
								auto count = list_.size();
								if (pos == count)
									basis_.active = pos = count - 1;

								count *= basis_.item_pixels;
								if (count > static_cast<unsigned>(_m_itembar_right()))
									basis_.scroll_pixels = static_cast<unsigned>(count)-static_cast<unsigned>(_m_itembar_right());
								else
									basis_.scroll_pixels = 0;
							}

							if(evt_agent_)
								evt_agent_->activated(basis_.active);
							return true;
						}
					}
					return false;
				}

				void render()
				{
					_m_adjust();
					_m_render();
				}

				bool press()
				{
					trace_.state = item_renderer::press;
					return (trace_.what != trace_.null);
				}

				bool active_by_trace()
				{
					return ((trace_.what == trace_.item) && (trace_.item_part != trace_.close)? activate(trace_.u.index) : false);
				}

				bool release()
				{
					trace_.state = item_renderer::highlight;
					return true;
				}

				bool leave()
				{
					trace_.state = item_renderer::normal;
					if(trace_.what != trace_.null)
					{
						trace_.what = trace_.null;
						return true;
					}
					return false;
				}

				void track()
				{
					int left, right;
					if(_m_item_pos(basis_.active, left, right))
					{
						if(left < 0)
							basis_.scroll_pixels -= static_cast<unsigned>(-left);
						else if(right > _m_itembar_right())
							basis_.scroll_pixels += static_cast<unsigned>(right - _m_itembar_right());
					}
				}

				bool trace(int x, int y)
				{
					trace_.state = item_renderer::highlight;
					if(basis_.graph == 0) return false;

					int ibar_end = _m_itembar_right();
					trace_tag::item_t item_part = trace_.item_part;
					std::size_t index = _m_where_itembar(x, y, ibar_end);
					if(index != npos)
					{
						if((trace_.what != trace_.item) || (trace_.u.index != index) || (item_part != trace_.item_part))
						{
							trace_.what = trace_.item;
							trace_.u.index = index;
							return true;
						}
						return false;
					}

					if(toolbox_.renderable(toolbox_.ButtonAdd))
					{
						if(ibar_end <= x && x < ibar_end + static_cast<int>(toolbox_.item_pixels()))
						{
							if((trace_.what != trace_.toolbox) || (trace_.u.button != toolbox::ButtonAdd))
							{
								trace_.what = trace_.toolbox;
								trace_.u.button = toolbox::ButtonAdd;
								return true;
							}
							return false;
						}
					}

					int tbpos = _m_toolbox_pos();
					if(tbpos <= x)
					{
						toolbox::button_t t = toolbox_.which(x - tbpos);

						if(trace_.what != trace_.toolbox || trace_.u.button != t)
						{
							trace_.what = trace_.toolbox;
							trace_.u.button = t;
							return true;
						}
						return false;
					}

					if(trace_.what != trace_.null)
					{
						trace_.what = trace_.null;
						return true;
					}
					return false;
				}

				bool activate(std::size_t pos)
				{
					if(pos < list_.size() && (pos != basis_.active))
					{
						API::show_window(iterator_at(pos)->relative, true);
						if(basis_.active < list_.size())
							API::show_window(iterator_at(basis_.active)->relative, false);

						basis_.active = pos;
						track();
						if(evt_agent_)
							evt_agent_->activated(pos);
						return true;
					}
					return false;
				}

				std::size_t active() const
				{
					return basis_.active;
				}

				void relate(std::size_t pos, window wd)
				{
					if(pos < list_.size())
					{
						iterator_at(pos)->relative = wd;
						API::show_window(wd, basis_.active == pos);
					}
				}

				bool tab_color(std::size_t pos, bool is_bgcolor, nana::color_t color)
				{
					if(pos < list_.size())
					{
						auto & m = *iterator_at(pos);
						if(is_bgcolor)
						{
							if(m.bgcolor != color)
							{
								m.bgcolor = color;
								return true;
							}
						}
						else
						{
							if(m.fgcolor != color)
							{
								m.fgcolor = color;
								return true;
							}
						}
					}
					return false;
				}

				bool tab_image(std::size_t pos, const nana::paint::image& img)
				{
					if(pos > list_.size()) return false;

					auto & m = *iterator_at(pos);
					if(img)
						m.img = img;
					else
						m.img.close();
					return true;
				}

				bool text(std::size_t pos, const nana::string& str)
				{
					if(pos < list_.size())
					{
						auto & m = *iterator_at(pos);
						if(m.text != str)
						{
							m.text = str;
							return true;
						}
					}
					return false;
				}

				nana::string text(std::size_t pos) const
				{
					if(pos < list_.size())
						return iterator_at(pos)->text;

					return nana::string();
				}

				bool toolbox_answer(const arg_mouse& arg)
				{
					if(trace_.what == trace_.toolbox)
					{
						if(toolbox_.renderable(trace_.u.button))
						{
							switch(trace_.u.button)
							{
							case toolbox::ButtonAdd:
								if(event_code::mouse_up == arg.evt_code)
									return _m_add_tab(npos);
								break;
							case toolbox::ButtonScrollBack:
								if(event_code::mouse_down == arg.evt_code)
									return _m_scroll(true);
								break;
							case toolbox::ButtonScrollNext:
								if(event_code::mouse_down == arg.evt_code)
									return _m_scroll(false);
								break;
							case toolbox::ButtonList:
								if (event_code::mouse_down == arg.evt_code)
								{
									_m_open_menulister();
									return true;
								}
								break;
							case toolbox::ButtonClose:
								if (event_code::mouse_up == arg.evt_code)
								{
									if(this->erase(basis_.active))
									{
										track();
										return true;
									}
								}
								break;
							default:
								break;
							}
						}
					}
					else if((trace_.what == trace_.item) && (trace_.item_part == trace_.close))
					{
						if(event_code::mouse_up == arg.evt_code)
						{
							if(this->erase(trace_.u.index))
							{
								track();
								trace(arg.pos.x, arg.pos.y);
								return true;
							}
						}
					}
					return false;
				}
			private: //Fundation
				bool _m_nextable() const
				{
					return (basis_.scroll_pixels + _m_itembar_right() < basis_.item_pixels * list_.size());
				}

				bool _m_add_tab(std::size_t pos)
				{
					item_t m;
					if((pos == npos) || (pos >= list_.size()))
					{
						this->list_.push_back(m);
						pos = static_cast<unsigned>(list_.size() - 1);
					}
					else
						list_.insert(iterator_at(pos), m);

					basis_.active = pos;
					if(evt_agent_)
					{
						evt_agent_->added(pos);
						evt_agent_->activated(pos);
					}
					return true;
				}

				bool _m_scroll(bool left)
				{
					if(left)
					{
						if(basis_.scroll_pixels)
						{
							unsigned i = basis_.scroll_pixels / basis_.item_pixels;
							if(i > 1)
								basis_.scroll_pixels = (i - 1) * basis_.item_pixels;
							else
								basis_.scroll_pixels = 0;
							return true;
						}
					}
					else
					{
						auto scale = static_cast<unsigned>(_m_itembar_right());
						auto take = static_cast<unsigned>(list_.size() * basis_.item_pixels);
						if(take > scale)
						{
							auto i = (basis_.scroll_pixels + scale) / basis_.item_pixels;
							i += (basis_.scroll_pixels % basis_.item_pixels ? 2 : 1);
							auto px = i * basis_.item_pixels;

							if(px > take) px = take;

							basis_.scroll_pixels = px - scale;
							return true;
						}
					}
					return false;
				}

				void _m_open_menulister()
				{
					menulister_.clear();

					auto f = std::bind(&layouter::_m_click_menulister, this, std::placeholders::_1);
					for(auto & m : list_)
						menulister_.append(m.text, f);

					auto r = toolbox_.area(toolbox_.ButtonList, basis_.graph->height());
					r.x += _m_toolbox_pos();
					menulister_.popup(basis_.wd, r.x, r.bottom());
				}

				void _m_click_menulister(nana::menu::item_proxy& ip)
				{
					if(this->activate(ip.index()))
						API::refresh_window(basis_.wd);
				}

				//the begin pos of toolbox
				int _m_toolbox_pos() const
				{
					int tbpos = static_cast<int>(basis_.graph->width()) - static_cast<int>(_m_toolbox_pixels());
					return (tbpos < 0 ? 0 : tbpos);
				}

				unsigned _m_toolbox_pixels() const
				{
					return toolbox_.width();
				}

				int _m_itembar_right() const
				{
					int right = _m_toolbox_pos();
					if(toolbox_.renderable(toolbox_.ButtonAdd))
						right -= static_cast<int>(toolbox_.item_pixels());

					int end = static_cast<int>(list_.size() * basis_.item_pixels);
					return (end < right ? end : right);
				}

				nana::rectangle _m_close_fly_area(int x)
				{
					return nana::rectangle(x + basis_.item_pixels - 18, (basis_.graph->height() - 14) / 2, 14, 14);
				}

				bool _m_item_pos(std::size_t index, int &left, int &right) const
				{
					if(index < list_.size())
					{
						left = static_cast<int>(index * basis_.item_pixels);
						left -= static_cast<int>(basis_.scroll_pixels);
						right = left + basis_.item_pixels;
						return true;
					}
					return false;
				}

				std::size_t _m_where_itembar(int x, int y, int end)
				{
					if(x < 0 || x >= end) return npos;

					int left = -static_cast<int>(basis_.scroll_pixels);
					std::size_t i = 0, size = list_.size();

					for(; i != size; ++i)
					{
						if(left >= end)
						{
							i = npos;
							break;
						}

						if(left <= x && x < left + static_cast<int>(basis_.item_pixels))
							break;

						left += basis_.item_pixels;
					}

					if(i < list_.size())
					{
						trace_.item_part = trace_.body;
						if(toolbox_.close_fly())
						{
							nana::rectangle r = _m_close_fly_area(left);
							if((r.x <= x && x < r.x + static_cast<int>(r.width)) && (r.y <= y && y < r.y + static_cast<int>(r.height)))
								trace_.item_part = trace_.close;
						}
						return i;
					}
					return npos;
				}

				nana::rectangle _m_toolbox_area(toolbox::button_t btn) const
				{
					nana::rectangle r(0, 0, toolbox_.item_pixels(), basis_.graph->height());
					if(btn == toolbox_.ButtonAdd)
					{
						int end = _m_itembar_right();
						if(static_cast<int>(list_.size() * basis_.item_pixels) < end)
							r.x = static_cast<int>(list_.size() * basis_.item_pixels);
						else
							r.x = end;
					}
					else if(toolbox_.ButtonClose == btn && toolbox_.close_fly())
					{
						r.x = -1;
						r.width = r.height = 0;
					}
					else
						r = toolbox_.area(btn, basis_.graph->height());
					return r;
				}

				void _m_adjust()
				{
					if((nullptr == basis_.graph) || (0 == list_.size())) return;

					//adjust the number of pixels of item.
					bool scrollable = toolbox_.renderable(toolbox_.ButtonScrollBack);
					if(scrollable)
					{
						toolbox_.visible(toolbox_.ButtonScrollBack, false);
						toolbox_.visible(toolbox_.ButtonScrollNext, false);
					}
					unsigned beside = _m_toolbox_pixels();
					if(toolbox_.renderable(toolbox_.ButtonAdd))
						beside += toolbox_.item_pixels();

					unsigned pixels = basis_.graph->width();
					if(pixels <= beside)
						return;
					unsigned each_pixels = static_cast<unsigned>((pixels - beside) / list_.size());
					if(each_pixels > basis_.max_pixels)
						each_pixels = basis_.max_pixels;
					else if(each_pixels < basis_.min_pixels)
						each_pixels = basis_.min_pixels;

					unsigned total = static_cast<unsigned>(each_pixels * list_.size());
					if(total > pixels - beside && toolbox_.enable(toolbox_.ButtonScrollBack))
					{
						toolbox_.visible(toolbox_.ButtonScrollBack, true);
						toolbox_.visible(toolbox_.ButtonScrollNext, true);

						beside = _m_toolbox_pixels();
						if(toolbox_.renderable(toolbox_.ButtonAdd))
							beside += toolbox_.item_pixels();

						if(pixels <= beside)
							return;

						each_pixels = static_cast<unsigned>((pixels - beside) / list_.size());
						if(each_pixels > basis_.max_pixels)
							each_pixels = basis_.max_pixels;
						else if(each_pixels < basis_.min_pixels)
							each_pixels = basis_.min_pixels;
					}
					else
						basis_.scroll_pixels = 0;

					if(each_pixels != basis_.item_pixels)
						basis_.item_pixels = each_pixels;

					if(scrollable != toolbox_.renderable(toolbox_.ButtonScrollBack))
						basis_.scroll_pixels = static_cast<unsigned>(list_.size() * basis_.item_pixels) - _m_itembar_right();
				}

				item_renderer::state_t _m_state(unsigned index) const
				{
					return ((trace_.what == trace_.item) && (trace_.u.index == index) ? item_renderer::highlight : item_renderer::normal);
				}

				item_renderer::state_t _m_state(toolbox::button_t kind) const
				{
					return ((trace_.what == trace_.toolbox && trace_.u.button == kind) ? item_renderer::highlight : item_renderer::normal);
				}

				void _m_render()
				{
					if(basis_.renderer == 0 || basis_.graph == 0) return;
					nana::color_t bgcolor = API::background(basis_.wd);

					item_renderer::item_t m;
					m.r.width = basis_.graph->width();
					m.r.height = basis_.graph->height();

					basis_.renderer->background(*basis_.graph, m.r, bgcolor);
					nana::color_t fgcolor = API::foreground(basis_.wd);

					//the max number of pixels of tabs.
					int pixels = static_cast<int>(m.r.width - _m_toolbox_pixels());

					m.r.x = -static_cast<int>(basis_.scroll_pixels);

					m.r.width = basis_.item_pixels;

					unsigned index = 0;
					bool is_close_fly = toolbox_.visible(toolbox_.ButtonClose) && toolbox_.enable(toolbox_.ButtonClose) && toolbox_.close_fly();

					item_renderer::item_t active_m;

					for(auto i = list_.cbegin(); i != list_.cend(); ++i, ++index)
					{
						if(m.r.x >= pixels) break;

						if(m.r.x + static_cast<int>(basis_.item_pixels) > 0)
						{
							m.bgcolor = i->bgcolor;
							m.fgcolor = i->fgcolor;
							if(index == this->basis_.active)
								active_m = m;

							const item_t & item = *i;
							basis_.renderer->item(*basis_.graph, m, (index == basis_.active), _m_state(index));
							if(is_close_fly)
							{
								item_renderer::state_t sta = item_renderer::normal;
								if(trace_.what == trace_.item && trace_.item_part == trace_.close && index == trace_.u.index)
									sta = item_renderer::highlight;
								basis_.renderer->close_fly(*basis_.graph, _m_close_fly_area(m.r.x), (index == basis_.active), sta);
							}

							if(false == item.img.empty())
								item.img.stretch(item.img.size(), *basis_.graph, nana::rectangle(m.r.x + 4, (m.r.height - 16) / 2, 16, 16));

							if(item.text.size())
							{
								nana::size ts = basis_.graph->text_extent_size(item.text);
								nana::paint::text_renderer tr(*basis_.graph);
								tr.render(m.r.x + 24, m.r.y + (m.r.height - ts.height) / 2, (m.fgcolor == nana::null_color ? fgcolor : m.fgcolor), item.text.c_str(), item.text.length(), basis_.item_pixels - 24 - 18, true);
							}
						}

						m.r.x += static_cast<int>(basis_.item_pixels);
					}

					_m_render_toolbox(bgcolor);

					int bottom = static_cast<int>(basis_.graph->height()) - 1;

					if(_m_nextable())
					{
						int x = _m_itembar_right();
						if(x > 0)
						{
							basis_.graph->line(x - 2, 0, x - 2, bottom, 0x808080);
							basis_.graph->line(x - 1, 0, x - 1, bottom, 0xF0F0F0);
						}
					}

					int right = static_cast<int>(basis_.graph->width());
					int end = active_m.r.x + static_cast<int>(active_m.r.width);
					if(0 < active_m.r.x && active_m.r.x < right)
						basis_.graph->line(0, bottom, active_m.r.x, bottom, 0x808080);
					if(0 <= end && end < right)
						basis_.graph->line(end, bottom, right, bottom, 0x808080);
				}

				void _m_render_toolbox(nana::color_t bgcolor)
				{
					bool backable = (basis_.scroll_pixels != 0);
					int xbase = _m_toolbox_pos();
					basis_.graph->rectangle(xbase, 0, _m_toolbox_pixels(), basis_.graph->height(), bgcolor, true);
					for(int i = toolbox::ButtonAdd; i < toolbox::ButtonSize; ++i)
					{
						toolbox::button_t btn = static_cast<toolbox::button_t>(i);

						if(toolbox_.renderable(btn) == false) continue;
						nana::rectangle r = _m_toolbox_area(btn);
						if(toolbox_.ButtonAdd != btn)
							r.x += xbase;

						item_renderer::state_t state = item_renderer::normal;
						if((trace_.what == trace_.toolbox) && (trace_.u.button == btn))
							state = trace_.state;

						switch(btn)
						{
						case toolbox::ButtonScrollBack:
							basis_.renderer->back(*basis_.graph, r, (backable ? state : item_renderer::disable));
							break;
						case toolbox::ButtonScrollNext:
							basis_.renderer->next(*basis_.graph, r, (_m_nextable() ? state : item_renderer::disable));
							break;
						case toolbox::ButtonList:
							basis_.renderer->list(*basis_.graph, r, state);
							break;
						case toolbox::ButtonClose:
							basis_.renderer->close(*basis_.graph, r, state);
							break;
						case toolbox::ButtonAdd:
							basis_.renderer->add(*basis_.graph, r, state);
							break;
						default:
							break;
						}
					}
				}
			private:
				std::list<item_t>		list_;
				event_agent_interface*	evt_agent_ = nullptr;
				toolbox toolbox_;
				::nana::menu menulister_;

				struct trace_tag
				{
					enum t{null, item, toolbox};
					enum item_t{body, close};
					t what;
					item_t item_part;	//it is valid while "what" is item.
					item_renderer::state_t state;
					union
					{
						std::size_t index;
						toolbox::button_t button;
					}u;

					trace_tag():what(null), state(item_renderer::normal)
					{}

				}trace_;

				struct basis_tag
				{
					window wd;
					nana::paint::graphics * graph;
					pat::cloneable<item_renderer> renderer;
					unsigned max_pixels;
					unsigned min_pixels;
					unsigned item_pixels;
					unsigned scroll_pixels;
					std::size_t active;

					basis_tag()
						:	wd(nullptr), graph(nullptr),
							renderer(def_renderer()),
							max_pixels(250), min_pixels(100), item_pixels(max_pixels), scroll_pixels(0),
							active(npos)
					{
					}
				}basis_;
			};

			item_renderer::~item_renderer(){}

			//class trigger
				trigger::trigger()
					: layouter_(new layouter)
				{}

				trigger::~trigger()
				{
					delete layouter_;
				}

				void trigger::activate(std::size_t pos)
				{
					if(layouter_->activate(pos))
						API::refresh_window(layouter_->widget_handle());
				}

				std::size_t trigger::activated() const
				{
					return layouter_->active();
				}

				nana::any& trigger::at(std::size_t i) const
				{
					return layouter_->at(i);
				}

				nana::any& trigger::at_no_bound_check(std::size_t i) const
				{
					return layouter_->at_no_bound_check(i);
				}

				const pat::cloneable<item_renderer> & trigger::ext_renderer() const
				{
					return layouter_->ext_renderer();
				}

				void trigger::ext_renderer(const pat::cloneable<item_renderer>& ir)
				{
					layouter_->ext_renderer(ir);
				}

				void trigger::set_event_agent(event_agent_interface* evt)
				{
					layouter_->event_agent(evt);
				}

				void trigger::push_back(const nana::string& text, const nana::any& value)
				{
					layouter_->push_back(text, value);
				}

				std::size_t trigger::length() const
				{
					return layouter_->length();
				}

				bool trigger::close_fly(bool fly)
				{
					return layouter_->toolbox_object().close_fly(fly);
				}

				void trigger::relate(std::size_t i, window wd)
				{
					layouter_->relate(i, wd);
				}

				void trigger::tab_color(std::size_t i, bool is_bgcolor, nana::color_t color)
				{
					if(layouter_->tab_color(i, is_bgcolor, color))
						API::refresh_window(layouter_->widget_handle());
				}

				void trigger::tab_image(std::size_t i, const nana::paint::image& img)
				{
					if(layouter_->tab_image(i, img))
						API::refresh_window(layouter_->widget_handle());
				}

				void trigger::text(std::size_t i, const nana::string& str)
				{
					if(layouter_->text(i, str))
						API::refresh_window(layouter_->widget_handle());
				}

				nana::string trigger::text(std::size_t i) const
				{
					return layouter_->text(i);
				}

				bool trigger::toolbox_button(toolbox_button_t btn, bool enable)
				{
					toolbox::button_t tb = toolbox::ButtonSize;
					toolbox & tbobj = layouter_->toolbox_object();
					switch(btn)
					{
					case trigger::ButtonAdd:
						tb = toolbox::ButtonAdd; break;
					case trigger::ButtonList:
						tb = toolbox::ButtonList; break;
					case trigger::ButtonClose:
						tb = toolbox::ButtonClose; break;
					case trigger::ButtonScroll:
						tbobj.enable(toolbox::ButtonScrollBack, enable);
						return tbobj.enable(tbobj.ButtonScrollNext, enable);
					}
					return (tb != toolbox::ButtonSize ? tbobj.enable(tb, enable) : false);
				}

				void trigger::attached(widget_reference widget, graph_reference graph)
				{
					layouter_->attach(widget, graph);
				}

				void trigger::detached()
				{
					layouter_->detach();
				}

				void trigger::refresh(graph_reference)
				{
					layouter_->render();
				}

				void trigger::mouse_down(graph_reference, const arg_mouse& arg)
				{
					if(layouter_->press())
					{
						if(false == layouter_->active_by_trace())
							layouter_->toolbox_answer(arg);
						layouter_->render();
						API::lazy_refresh();
					}
				}

				void trigger::mouse_up(graph_reference, const arg_mouse& arg)
				{
					bool rd = layouter_->release();
					rd |= layouter_->toolbox_answer(arg);
					if(rd)
					{
						layouter_->render();
						API::lazy_refresh();
					}
				}

				void trigger::mouse_move(graph_reference, const arg_mouse& arg)
				{
					if(layouter_->trace(arg.pos.x, arg.pos.y))
					{
						layouter_->render();
						API::lazy_refresh();
					}
				}

				void trigger::mouse_leave(graph_reference, const arg_mouse&)
				{
					if(layouter_->leave())
					{
						layouter_->render();
						API::lazy_refresh();
					}
				}
			//end class trigger
		}//end namespace tabbar
	}//end namespace drawerbase
}//end namespace nana
