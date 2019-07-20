/*
 *	A Tabbar Implementation
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
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
#include <nana/gui/element.hpp>
#include <stdexcept>
#include <list>

namespace nana
{
	namespace drawerbase
	{
		namespace tabbar
		{
			using native_string_type = ::nana::detail::native_string_type;
			struct item_t
			{
				window relative{nullptr};
				paint::image img;
				native_string_type text;
				any	value;

				::nana::color bgcolor;
				::nana::color fgcolor;

				item_t() = default;
				
				item_t(native_string_type&& text, any && value)
					: text(std::move(text)), value(std::move(value))
				{}
			};

			class def_renderer
				: public item_renderer
			{
			private:
				virtual void background(graph_reference graph, const nana::rectangle&, const ::nana::color& bgcolor)
				{
					if(bgcolor_ != bgcolor)
					{
						bgcolor_ = bgcolor;

						dark_bgcolor_ = bgcolor.blend(colors::black, 0.1);
						blcolor_ = bgcolor.blend(colors::black, 0.5);
						ilcolor_ = bgcolor.blend(colors::white, 0.1);
					}

					graph.rectangle(true, bgcolor);
				}

				virtual void item(graph_reference graph, const item_t& m, bool active, state_t sta)
				{
					const nana::rectangle & r = m.r;
					color bgcolor;
					color blcolor;
					color dark_bgcolor;

					if(m.bgcolor.invisible())
					{
						bgcolor = bgcolor_;
						blcolor = blcolor_;
						dark_bgcolor = dark_bgcolor_;
					}
					else
					{
						bgcolor = m.bgcolor;
						blcolor = m.bgcolor.blend(colors::black, 0.5);
						dark_bgcolor = m.bgcolor.blend(colors::black, 0.1);
					}

					auto round_r = r;
					round_r.height += 2;
					graph.round_rectangle(round_r, 3, 3, blcolor, true, colors::white);

					auto beg = bgcolor;
					auto end = dark_bgcolor;

					if(active)
					{
						if (m.bgcolor.invisible())
							beg = ilcolor_;
						else
							beg = m.bgcolor.blend(colors::white, 0.5);
						end = bgcolor;
					}

					if (sta == item_renderer::highlight)
						beg = beg.blend(colors::white, 0.5);

					graph.gradual_rectangle(round_r.pare_off(2), beg, end, true);
				}

				virtual void add(graph_reference graph, const nana::rectangle& r, state_t sta)
				{
					int x = r.x + (static_cast<int>(r.width) - 14) / 2;
					int y = r.y + (static_cast<int>(r.height) - 14) / 2;

					::nana::color clr;

					switch(sta)
					{
					case item_renderer::highlight:
						clr = colors::white; break;
					case item_renderer::press:
						clr = static_cast<color_rgb>(0xA0A0A0); break;
					case item_renderer::disable:
						clr = static_cast<color_rgb>(0x808080); break;
					default:
						clr = static_cast<color_rgb>(0xF0F0F0);
					}
					graph.rectangle(r, true, bgcolor_);
					facade<element::cross> cross;
					cross.draw(graph, {}, clr, { x, y, 14, 6 }, element_state::normal);
				}

				virtual void close(graph_reference graph, const nana::rectangle& r, state_t sta)
				{
					facade<element::x_icon> x_icon;
					x_icon.draw(graph, {}, colors::black, { r.x + static_cast<int>(r.width - 16) / 2, r.y + static_cast<int>(r.height - 16) / 2, 16, 16 }, element_state::normal);
					if(item_renderer::highlight == sta)
						graph.rectangle(r, false, static_cast<color_rgb>(0xa0a0a0));
				}

				virtual void close_fly(graph_reference graph, const nana::rectangle& r, bool active, state_t sta)
				{
					using namespace nana::paint;
					::nana::color clr{ colors::black };

					if (sta == item_renderer::highlight)
					{
						::nana::color bgcolor{ static_cast<color_rgb>(0xCCD2DD) };
						::nana::color rect_clr{ static_cast<color_rgb>(0x9da3ab) };
						graph.round_rectangle(r, 1, 1, rect_clr, false, {});
						nana::rectangle draw_r(r);
						graph.rectangle(draw_r.pare_off(1), false, rect_clr.blend(bgcolor, 0.2));
						graph.rectangle(draw_r.pare_off(1), false, rect_clr.blend(bgcolor, 0.6));
						graph.rectangle(draw_r.pare_off(1), false, rect_clr.blend(bgcolor, 0.8));
					}
					else if (!active)
						clr = static_cast<color_rgb>(0x9299a4);

					facade<element::x_icon> x_icon;
					x_icon.draw(graph, {}, colors::black, { r.x + static_cast<int>(r.width - 16) / 2, r.y + static_cast<int>(r.height - 16) / 2, 16, 16 }, element_state::normal);

				}

				virtual void back(graph_reference graph, const nana::rectangle& r, state_t sta)
				{
					_m_draw_arrow(graph, r, sta, direction::west);
				}

				virtual void next(graph_reference graph, const nana::rectangle& r, state_t sta)
				{
					_m_draw_arrow(graph, r, sta, direction::east);
				}

				virtual void list(graph_reference graph, const nana::rectangle& r, state_t sta)
				{
					_m_draw_arrow(graph, r, sta, direction::south);
				}
			private:
				void _m_draw_arrow(graph_reference graph, const nana::rectangle& r, state_t sta, ::nana::direction dir)
				{
					facade<element::arrow> arrow("solid_triangle");
					arrow.direction(dir);
					colors fgcolor = colors::black;
					if (item_renderer::disable == sta)
					{
						arrow.switch_to("hollow_triangle");
						fgcolor = colors::gray;
					}
					auto arrow_r = r;
					arrow_r.x += static_cast<int>(arrow_r.width - 16) / 2;
					arrow_r.y += static_cast<int>(arrow_r.height - 16) / 2;
					arrow_r.width = arrow_r.height = 16;
					arrow.draw(graph, bgcolor_, fgcolor, arrow_r, element_state::normal);

					if(item_renderer::highlight == sta)
						graph.rectangle(r, false, colors::dark_gray);
				}
			private:
				::nana::color bgcolor_;
				::nana::color dark_bgcolor_;
				::nana::color blcolor_;
				::nana::color ilcolor_;
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
					throw std::out_of_range("invalid position of tabbar");
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
					throw std::out_of_range("invalid position of tabbar");
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

				void insert(std::size_t pos, native_string_type&& text, nana::any&& value)
				{
					if (pos >= list_.size())
						pos = list_.size();

					list_.emplace(iterator_at(pos), std::move(text), std::move(value));
					this->activate(pos);
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
						bool close_attach = true;
						if ((nullptr == evt_agent_) || evt_agent_->removed(pos, close_attach))
						{
							if (close_attach)
								API::close_window(iterator_at(pos)->relative);
							else
								API::show_window(iterator_at(pos)->relative, false);
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

							if (basis_.active != ::nana::npos)
								API::show_window(iterator_at(basis_.active)->relative, true);
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

				bool active_by_trace(const arg_mouse& arg)
				{
					if((trace_.what == trace_.item) && (trace_.item_part != trace_.close))
					{
						if(false == evt_agent_->click(arg, trace_.u.index))
							return activate(trace_.u.index);

						return true;
					}
					
					return false;
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
						auto & tab_act = *iterator_at(pos);
						API::show_window(tab_act.relative, true);
						if (basis_.active < list_.size())
						{
							auto & tab_deact = *iterator_at(basis_.active);

							//Don't hide the relative window if it is equal to active relative window.
							//The tabbar allows a window to be attached to multiple tabs(pass false to DropOther of attach method)
							if (tab_deact.relative != tab_act.relative)
								API::show_window(tab_deact.relative, false);
						}

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

				window attach(std::size_t pos, window wd, bool drop_other)
				{
					if (pos >= list_.size())
						throw std::out_of_range("tabbar: invalid position");

					auto & tab = *iterator_at(pos);
					auto old = tab.relative;

					if (drop_other)
					{
						//Drop the relative windows which are equal to wd.
						for (auto & t : list_)
						{
							if (wd == t.relative)
								t.relative = nullptr;
						}
					}
					
					tab.relative = wd;
					API::show_window(wd, basis_.active == pos);
					return old;
				}

				bool tab_color(std::size_t pos, bool is_bgcolor, const ::nana::color& clr)
				{
					if (pos >= list_.size())
						throw std::out_of_range("tabbar: invalid position");

					auto & m = *iterator_at(pos);
					auto & m_clr = (is_bgcolor ? m.bgcolor : m.fgcolor);
					if (m_clr != clr)
					{
						m_clr = clr;
						return true;
					}
					return false;
				}

				void tab_image(std::size_t pos, const nana::paint::image& img)
				{
					if (pos >= list_.size())
						throw std::out_of_range("tabbar: invalid position");

					auto & m = *iterator_at(pos);
					if(img)
						m.img = img;
					else
						m.img.close();
				}

				bool text(std::size_t pos, const native_string_type& str)
				{
					if (pos >= list_.size())
						throw std::out_of_range("tabbar: invalid position");

					auto & m = *iterator_at(pos);
					if(m.text != str)
					{
						m.text = str;
						return true;
					}

					return false;
				}

				native_string_type text(std::size_t pos) const
				{
					if (pos >= list_.size())
						throw std::out_of_range("tabbar: invalid position");

					return iterator_at(pos)->text;
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
			private: //Foundation
				bool _m_nextable() const
				{
					return (basis_.scroll_pixels + _m_itembar_right() < basis_.item_pixels * list_.size());
				}

				bool _m_add_tab(std::size_t pos)
				{
					if((pos == npos) || (pos >= list_.size()))
					{
						pos = list_.size();
						
						if(evt_agent_)
							if(!evt_agent_->adding(pos))
								return false;

						this->list_.emplace_back();
					}
					else
					{
						if(evt_agent_)
							if(!evt_agent_->adding(pos))
								return false;

						list_.emplace(iterator_at(pos));
					}

					basis_.active = pos;
					if(evt_agent_)
					{
						evt_agent_->added(pos);
						erase(pos);
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
					auto fn = [this](::nana::menu::item_proxy& ipx)
					{
						if (this->activate(ipx.index()))
							API::refresh_window(basis_.wd);
					};

					for(auto & m : list_)
						menulister_.append(to_utf8(m.text), fn);

					auto r = toolbox_.area(toolbox_.ButtonList, basis_.graph->height());
					r.x += _m_toolbox_pos();
					menulister_.popup(basis_.wd, r.x, r.bottom());
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
					if(!basis_.renderer || (nullptr == basis_.graph))
						return;

					auto bgcolor = API::bgcolor(basis_.wd);
					auto fgcolor = API::fgcolor(basis_.wd);

					item_renderer::item_t m;
					m.r = ::nana::rectangle{ basis_.graph->size() };

					basis_.renderer->background(*basis_.graph, m.r, bgcolor);

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
								item.img.stretch(::nana::rectangle{ item.img.size() }, *basis_.graph, nana::rectangle(m.r.x + 4, (m.r.height - 16) / 2, 16, 16));

							if(item.text.size())
							{
								nana::size ts = basis_.graph->text_extent_size(item.text);
								basis_.graph->palette(true, m.fgcolor.invisible() ? fgcolor : m.fgcolor);
								nana::paint::text_renderer tr(*basis_.graph);

								std::wstring wtext = to_wstring(item.text);
								tr.render({ m.r.x + 24, m.r.y + static_cast<int>(m.r.height - ts.height) / 2 },
											wtext.c_str(), wtext.length(), basis_.item_pixels - 24 - 18, paint::text_renderer::mode::truncate_with_ellipsis);
							}
						}

						m.r.x += static_cast<int>(basis_.item_pixels);
					}

					_m_render_toolbox(bgcolor);

					int bottom = static_cast<int>(basis_.graph->height()) - 1;

					if(_m_nextable())
					{
						int x = _m_itembar_right();
						if (x > 0)
						{
							basis_.graph->line({ x - 2, 0 }, { x - 2, bottom }, static_cast<color_rgb>(0x808080));
							basis_.graph->line({ x - 1, 0 }, { x - 1, bottom }, static_cast<color_rgb>(0xf0f0f0));
						}
					}

					basis_.graph->palette(false, static_cast<color_rgb>(0x808080));

					int right = static_cast<int>(basis_.graph->width());
					int end = active_m.r.x + static_cast<int>(active_m.r.width);
					if(0 < active_m.r.x && active_m.r.x < right)
						basis_.graph->line({ 0, bottom }, { active_m.r.x, bottom });
					if(0 <= end && end < right)
						basis_.graph->line({ end, bottom }, { right, bottom });
				}

				void _m_render_toolbox(const ::nana::color& bgcolor)
				{
					bool backable = (basis_.scroll_pixels != 0);
					int xbase = _m_toolbox_pos();
					basis_.graph->rectangle({ xbase, 0, _m_toolbox_pixels(), basis_.graph->height() }, true, bgcolor);
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
					window wd{nullptr};
					nana::paint::graphics * graph{nullptr};
					pat::cloneable<item_renderer> renderer;
					unsigned max_pixels{250};
					unsigned min_pixels{100};
					unsigned item_pixels{max_pixels};
					unsigned scroll_pixels{0};
					std::size_t active{npos};

					basis_tag():renderer{ def_renderer() }
					{}
				}basis_;
			};

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

				void trigger::insert(std::size_t pos, native_string_type&& text, nana::any&& value)
				{
					layouter_->insert(pos, std::move(text), std::move(value));
				}

				std::size_t trigger::length() const
				{
					return layouter_->length();
				}

				bool trigger::close_fly(bool fly)
				{
					return layouter_->toolbox_object().close_fly(fly);
				}

				window trigger::attach(std::size_t pos, window wd, bool drop_other)
				{
					return layouter_->attach(pos, wd, drop_other);
				}

				void trigger::erase(std::size_t pos)
				{
					layouter_->erase(pos);
				}

				void trigger::tab_color(std::size_t i, bool is_bgcolor, const ::nana::color& clr)
				{
					if(layouter_->tab_color(i, is_bgcolor, clr))
						API::refresh_window(layouter_->widget_handle());
				}

				void trigger::tab_image(std::size_t i, const nana::paint::image& img)
				{
					layouter_->tab_image(i, img);
					API::refresh_window(layouter_->widget_handle());
				}

				void trigger::text(std::size_t i, const native_string_type& str)
				{
					if(layouter_->text(i, str))
						API::refresh_window(layouter_->widget_handle());
				}

				native_string_type trigger::text(std::size_t i) const
				{
					return layouter_->text(i);
				}

				bool trigger::toolbox(kits btn, bool enable)
				{
					auto tb = toolbox::ButtonSize;
					auto& tbox = layouter_->toolbox_object();
					switch(btn)
					{
					case kits::add:
						tb = toolbox::ButtonAdd; break;
					case kits::list:
						tb = toolbox::ButtonList; break;
					case kits::close:
						tb = toolbox::ButtonClose; break;
					case kits::scroll:
						tbox.enable(toolbox::ButtonScrollBack, enable);
						return tbox.enable(tbox.ButtonScrollNext, enable);
					}
					return (tb != toolbox::ButtonSize ? tbox.enable(tb, enable) : false);
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
					//Activates the tab only if left button is clicked.
					if(arg.is_left_button() && layouter_->press())
					{
						if(false == layouter_->active_by_trace(arg))
							layouter_->toolbox_answer(arg);
						layouter_->render();
						API::dev::lazy_refresh();
					}
				}

				void trigger::mouse_up(graph_reference, const arg_mouse& arg)
				{
					bool rd = layouter_->release();
					rd |= layouter_->toolbox_answer(arg);
					if(rd)
					{
						layouter_->render();
						API::dev::lazy_refresh();
					}
				}

				void trigger::mouse_move(graph_reference, const arg_mouse& arg)
				{
					if(layouter_->trace(arg.pos.x, arg.pos.y))
					{
						layouter_->render();
						API::dev::lazy_refresh();
					}
				}

				void trigger::mouse_leave(graph_reference, const arg_mouse&)
				{
					if(layouter_->leave())
					{
						layouter_->render();
						API::dev::lazy_refresh();
					}
				}
			//end class trigger
		}//end namespace tabbar
	}//end namespace drawerbase
}//end namespace nana

#include <forward_list>
namespace nana
{
		namespace drawerbase
		{
			namespace tabbar_lite
			{
				struct item
				{
					::std::string text;
					::nana::any	value;
					::std::pair<int, int> pos_ends;
					::nana::window attached_window{ nullptr };

					item(std::string t, ::nana::any v)
						: text(std::move(t)), value(std::move(v))
					{}
				};

				void calc_px(std::vector<unsigned>& values, unsigned limit)
				{
					unsigned	base = 0;
					auto		count = values.size();

					while (count)
					{
						unsigned minv = static_cast<unsigned>(-1);

						unsigned minv_count = 0;
						for (auto u : values)
						{
							if (u >= base && u < minv)
							{
								minv_count = 1;
								minv = u;
							}
							else if (minv == u)
								minv_count++;
						}

						count -= minv_count;
						if (minv * count >= limit)
						{
							auto piece = limit / double(count);
							double deviation = 0;
							for (auto & u : values)
							{
								if (minv >= u)
								{
									auto px = piece + deviation;
									u = static_cast<unsigned>(px);
									deviation = px - u;
								}
							}
							return;
						}

						base = minv;
						limit -= minv_count * minv;
					}
				}

				class model
				{
					struct indexes
					{
						std::size_t hovered_pos{ npos };
						std::size_t active_pos{ 0 };
					};
				public:
					using graph_reference = ::nana::paint::graphics&;
					static const std::size_t npos = static_cast<std::size_t>(-1);

					void set_widget(::nana::tabbar_lite& wdg)
					{
						widget_ = &wdg;
					}

					::nana::tabbar_lite* widget_ptr() const
					{
						return widget_;
					}

					std::forward_list<item>& items()
					{
						return items_;
					}

					void show_attached_window()
					{
						if (indexes_.active_pos != npos)
						{
							auto i = items_.cbegin();
							std::advance(i, indexes_.active_pos);
							API::show_window(i->attached_window, true);

							std::size_t pos = 0;
							for (auto & m : items_)
							{
								if (pos++ != indexes_.active_pos)
									API::show_window(m.attached_window, false);
							}
						}
					}

					bool track_pointer(const point& pos)
					{
						std::size_t item_pos = 0;
						bool matched = false;
						for (auto & m : items_)
						{
							if (m.pos_ends.first <= pos.x && pos.x < m.pos_ends.second)
							{
								matched = true;
								break;
							}

							++item_pos;
						}

						if (!matched)
							item_pos = npos;

						if (indexes_.hovered_pos == item_pos)
							return false;

						indexes_.hovered_pos = item_pos;
						return true;
					}

					indexes& get_indexes()
					{
						return indexes_;
					}
				private:
					::nana::tabbar_lite * widget_{ nullptr };
					std::forward_list<item> items_;
					indexes indexes_;
				};

				class renderer
				{
				public:
					using graph_reference = ::nana::paint::graphics&;

					void render(graph_reference graph, model& model)
					{
						_m_calc_metrics(graph, model.items());

						auto & scheme = model.widget_ptr()->scheme();

						//draw background
						graph.rectangle(true, scheme.background);

						auto & indexes = model.get_indexes();
						std::size_t pos = 0;
						for (auto & m : model.items())
						{
							rectangle r{ m.pos_ends.first, 0, static_cast<unsigned>(m.pos_ends.second - m.pos_ends.first), graph.height() };
							if (indexes.active_pos == pos)
							{
								graph.palette(false, colors::white);
								graph.palette(true, colors::black);
							}
							else
							{
								::nana::color bgcolor(colors::coral);

								if (pos == indexes.hovered_pos)
									bgcolor = bgcolor.blend(colors::white, 0.5);

								graph.palette(false, bgcolor);
								graph.palette(true, colors::white);
							}

							graph.rectangle(r, true);
#ifdef _nana_std_has_string_view
							graph.bidi_string({ m.pos_ends.first + 5, 0 }, m.text);

#else
							graph.bidi_string({ m.pos_ends.first + 5, 0 }, m.text.data(), m.text.size());
#endif

							++pos;
						}

					}
				private:
					void _m_calc_metrics(graph_reference graph, std::forward_list<item>& items)
					{
						std::vector<unsigned> pxs;

						unsigned pixels = 0;
						for (auto & m : items)
						{
							auto ts = graph.bidi_extent_size(m.text);
							pxs.emplace_back(ts.width + 12);
							pixels += ts.width + 12;
						}

						if (pixels > graph.width())
							calc_px(pxs, graph.width());

						auto i = pxs.cbegin();
						int pos = 0;
						for (auto & m : items)
						{
							m.pos_ends.first = pos;
							m.pos_ends.second = (pos += static_cast<int>(*i++));
						}
					}
				};


				//class driver
					driver::driver()
						: model_(new model)
					{
					}

					driver::~driver()
					{
						delete model_;
					}

					model* driver::get_model() const noexcept
					{
						return model_;
					}

					void driver::attached(widget_reference wdg, graph_reference)
					{
						model_->set_widget(dynamic_cast<nana::tabbar_lite&>(wdg));
					}

					//Overrides drawer_trigger's method
					void driver::refresh(graph_reference graph)
					{
						renderer rd;
						rd.render(graph, *model_);
					}

					void driver::mouse_move(graph_reference graph, const arg_mouse& arg)
					{
						if (!model_->track_pointer(arg.pos))
							return;

						refresh(graph);
						API::dev::lazy_refresh();
					}

					void driver::mouse_leave(graph_reference graph, const arg_mouse&)
					{
						if (model_->get_indexes().hovered_pos == model_->npos)
							return;

						refresh(graph);
						API::dev::lazy_refresh();
					}

					void driver::mouse_down(graph_reference graph, const arg_mouse& arg)
					{
						auto & indexes = model_->get_indexes();
						if ((indexes.hovered_pos == model_->npos) || (indexes.active_pos == indexes.hovered_pos) || !arg.is_left_button())
							return;

						if (indexes.active_pos != indexes.hovered_pos)
						{
							indexes.active_pos = indexes.hovered_pos;
							model_->show_attached_window();

							refresh(graph);
							API::dev::lazy_refresh();

							event_arg arg;
							model_->widget_ptr()->events().selected.emit(arg, model_->widget_ptr()->handle());
						}
					}
				//end class driver
			}
		}

		//class tabbar

		tabbar_lite::tabbar_lite(window parent_wd, bool visible, const ::nana::rectangle& r)
		{
			this->create(parent_wd, r, visible);
		}

		//capacity
		std::size_t tabbar_lite::length() const
		{
			auto& items = get_drawer_trigger().get_model()->items();
			internal_scope_guard lock;
			return static_cast<std::size_t>(std::distance(items.cbegin(), items.cend()));
		}

		//modifiers
		void tabbar_lite::attach(std::size_t pos_set, window wd)
		{
			auto model = get_drawer_trigger().get_model();
			internal_scope_guard lock;

			for (auto & m : model->items())
			{
				if (0 == pos_set--)
				{
					m.attached_window = wd;
					model->show_attached_window();
					return;
				}
			}

			throw std::out_of_range("invalid position of tabbar_lite");
		}

		window tabbar_lite::attach(std::size_t pos_set) const
		{
			auto model = get_drawer_trigger().get_model();
			internal_scope_guard lock;

			for (auto & m : model->items())
			{
				if (0 == pos_set--)
					return m.attached_window;
			}

			throw std::out_of_range("invalid position of tabbar_lite");
		}

		void tabbar_lite::push_back(std::string text, ::nana::any any)
		{
			auto & items = get_drawer_trigger().get_model()->items();
			internal_scope_guard lock;

			auto i = items.cbefore_begin();
			while (true)
			{
				auto next = i++;
				if (i == items.cend())
				{
					items.emplace_after(next, std::move(text), std::move(any));
					break;
				}

			}

			API::refresh_window(handle());
		}

		void tabbar_lite::push_front(std::string text, ::nana::any any)
		{
			auto & items = get_drawer_trigger().get_model()->items();
			internal_scope_guard lock;

			items.emplace_front(std::move(text), std::move(any));
			API::refresh_window(handle());
		}

		std::size_t tabbar_lite::selected() const
		{
			auto model = get_drawer_trigger().get_model();
			internal_scope_guard lock;

			return model->get_indexes().active_pos;
		}

		void tabbar_lite::erase(std::size_t pos, bool close_attached)
		{
			auto model = get_drawer_trigger().get_model();
			internal_scope_guard lock;

			const auto len = length();

			if (len <= pos)
				throw std::out_of_range("invalid position of tabbar_lite");

			auto active_pos = model->get_indexes().active_pos;

			//selection_changed is used to determine whether the title will be updated
			bool selection_changed = true;
			if (pos == active_pos)
			{
				if (active_pos + 1 == len)
				{
					if (active_pos)
						--active_pos;
					else
						active_pos = npos;
				}
			}
			else if (pos < active_pos)
				--active_pos;
			else
				selection_changed = false;

			model->get_indexes().active_pos = active_pos;

			auto i = model->items().cbefore_begin();
			std::advance(i, pos);

			auto attached_wd = std::next(i)->attached_window;

			model->items().erase_after(i);

			model->show_attached_window();
			API::refresh_window(handle());

			if (close_attached && attached_wd)
				API::close_window(attached_wd);

			if (selection_changed && (active_pos != npos))
			{
				event_arg arg;
				events().selected.emit(arg, handle());
			}
		}
		//end class tabbar
}//end namespace nana
