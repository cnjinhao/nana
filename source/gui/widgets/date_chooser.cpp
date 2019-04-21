/*
 *	A date chooser Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2017 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/date_chooser.cpp
 */

#include <nana/gui/widgets/date_chooser.hpp>
#include <nana/gui/element.hpp>
#include <nana/system/platform.hpp>
#include <sstream>

namespace nana
{
	namespace drawerbase
	{
		namespace date_chooser
		{
			enum class transform_action{ none, to_left, to_right, to_enter, to_leave };

			void perf_transform_helper(window window_handle, transform_action tfid, trigger::graph_reference graph, trigger::graph_reference dirtybuf, trigger::graph_reference newbuf, const nana::point& refpos)
			{
				const int sleep_time = 15;
				const int count = 20;
				double delta = dirtybuf.width() / double(count);
				double delta_h = dirtybuf.height() / double(count);
				double fade = 1.0 / count;

				if (tfid == transform_action::to_right)
				{
					nana::rectangle dr(0, refpos.y, 0, dirtybuf.height());
					nana::rectangle nr(refpos.x, refpos.y, 0, newbuf.height());
					for (int i = 1; i < count; ++i)
					{
						int off_x = static_cast<int>(delta * i);
						dr.x = refpos.x + off_x;
						dr.width = dirtybuf.width() - off_x;

						graph.bitblt(dr, dirtybuf);

						nr.width = off_x;
						graph.bitblt(nr, newbuf, nana::point(static_cast<int>(dr.width), 0));

						API::update_window(window_handle);
						nana::system::sleep(sleep_time);
					}
				}
				else if (tfid == transform_action::to_left)
				{
					double delta = dirtybuf.width() / double(count);
					nana::rectangle dr(refpos.x, refpos.y, 0, dirtybuf.height());
					nana::rectangle nr(0, refpos.y, 0, newbuf.height());

					for (int i = 1; i < count; ++i)
					{
						int off_x = static_cast<int>(delta * i);
						dr.width = dirtybuf.width() - off_x;

						graph.bitblt(dr, dirtybuf, nana::point(off_x, 0));

						nr.x = refpos.x + static_cast<int>(dr.width);
						nr.width = off_x;
						graph.bitblt(nr, newbuf);

						API::update_window(window_handle);
						nana::system::sleep(sleep_time);
					}
				}
				else if (tfid == transform_action::to_leave)
				{
					nana::paint::graphics dzbuf(newbuf.size());
					nana::paint::graphics nzbuf(newbuf.size());

					nana::rectangle r;
					for (int i = 1; i < count; ++i)
					{
						r.width = static_cast<int>(newbuf.width() - delta * i);
						r.height = static_cast<int>(newbuf.height() - delta_h * i);
						r.x = static_cast<int>(newbuf.width() - r.width) / 2;
						r.y = static_cast<int>(newbuf.height() - r.height) / 2;

						dzbuf.rectangle(true, colors::white);
						dirtybuf.stretch(dzbuf, r);

						r.width = static_cast<int>(newbuf.width() + delta * (count - i));
						r.height = static_cast<int>(newbuf.height() + delta_h * (count - i));
						r.x = static_cast<int>(newbuf.width() - r.width) / 2;
						r.y = static_cast<int>(newbuf.height() - r.height) / 2;
						newbuf.stretch(nzbuf, r);

						dzbuf.blend(::nana::rectangle{ nzbuf.size() }, nzbuf, {}, 1 - fade * (count - i));

						graph.bitblt(refpos.x, refpos.y, dzbuf);

						API::update_window(window_handle);
						nana::system::sleep(sleep_time);
					}
				}
				else if (tfid == transform_action::to_enter)
				{
					nana::paint::graphics dzbuf(newbuf.size());
					nana::paint::graphics nzbuf(newbuf.size());

					nana::rectangle r;
					for (int i = 1; i < count; ++i)
					{
						r.width = static_cast<int>(newbuf.width() + delta * i);
						r.height = static_cast<int>(newbuf.height() + delta_h * i);
						r.x = static_cast<int>(newbuf.width() - r.width) / 2;
						r.y = static_cast<int>(newbuf.height() - r.height) / 2;
						dirtybuf.stretch(dzbuf, r);

						r.width = static_cast<int>(newbuf.width() - delta * (count - i));
						r.height = static_cast<int>(newbuf.height() - delta_h * (count - i));
						r.x = static_cast<int>(newbuf.width() - r.width) / 2;
						r.y = static_cast<int>(newbuf.height() - r.height) / 2;
						nzbuf.rectangle(true, colors::white);
						newbuf.stretch(nzbuf, r);

						dzbuf.blend(::nana::rectangle{ nzbuf.size() }, nzbuf, {}, 1.0 - fade * (count - i));

						graph.bitblt(refpos.x, refpos.y, dzbuf);

						API::update_window(window_handle);
						nana::system::sleep(sleep_time);
					}
				}

				graph.bitblt(nana::rectangle(refpos, newbuf.size()), newbuf);
			}


			class trigger::model
			{
				friend class trigger;
			public:
				struct view_month_rep
				{
					int year;
					int month;
				};

				enum class where
				{
					none,
					left_button,
					right_button,
					topbar,
					textarea
				};

				enum class page_mode
				{
					date,
					month
				};

				struct drawing_basis
				{
					nana::point refpos;
					double line_s;
					double row_s;
				};

				model()
				{
					const std::string ws[] = { "S", "M", "T", "W", "T", "F", "S" };
					for (int i = 0; i < 7; ++i)
						weekstr_[i] = ws[i];

					nana::date d;
					date_.year = view_month_.year = d.read().year;
					date_.month = view_month_.month = d.read().month;
					date_.day = d.read().day;
				}

				void move(bool forward, graph_reference graph, window window_handle)
				{
					if (page_mode::date == page)
					{
						step_view_month(forward);
						
					}
					else
					{
						if (forward)
							view_month_.year++;
						else
							view_month_.year--;
					}

					perf_transform(graph, window_handle, (forward ? transform_action::to_left : transform_action::to_right));
				}

				void enter(graph_reference graph, window window_handle)
				{
					transform_action tfid;

					if (model::page_mode::date == page)
					{
						page = model::page_mode::month;
						tfid = transform_action::to_leave;

						if ((!trace_.is_by_mouse) && (date_.year != view_month_.year))
						{
							trace_.logic_pos.x = 0;
							trace_.logic_pos.y = 0;
						}
					}
					else
					{
						page = model::page_mode::date;
						tfid = transform_action::to_enter;

						if (!trace_.is_by_mouse)
						{
							if ((date_.year != view_month_.year) || (date_.month != view_month_.month))
							{
								trace_.logic_pos.x = 0;
								trace_.logic_pos.y = 1;
							}
						}
					}

					perf_transform(graph, window_handle, tfid);
				}

				bool respond_key(graph_reference graph, const arg_keyboard& arg, bool& redrawn)
				{
					redrawn = false;
					if (trace_.empty_logic_pos())
						return false;

					unsigned right = 7;
					unsigned top = 1;
					unsigned bottom = 7;
					if (page_mode::month == page)
					{
						right = 4;
						top = 0;
						bottom = 3;
					}

					switch (arg.key)
					{
					case keyboard::os_arrow_left:
						if (trace_.logic_pos.x > 0)
						{
							--trace_.logic_pos.x;
						}
						else if (trace_.logic_pos.y > top)
						{
							trace_.logic_pos.x = right - 1;
							--trace_.logic_pos.y;
						}
						else
						{
							move(false, graph, arg.window_handle);
							redrawn = true;
						}
						break;
					case keyboard::os_arrow_right:
						++trace_.logic_pos.x;
						if (trace_.logic_pos.x == right)
						{
							if (trace_.logic_pos.y + 1 < bottom)
							{
								++trace_.logic_pos.y;
								trace_.logic_pos.x = 0;
							}
							else
							{
								trace_.logic_pos.x = right - 1;
								move(true, graph, arg.window_handle);
								redrawn = true;
							}
						}

						break;
					case keyboard::os_arrow_up:
						if (trace_.logic_pos.y == top)
							trace_.logic_pos.y = bottom - 1;
						else
							--trace_.logic_pos.y;
						break;
					case keyboard::os_arrow_down:
						if (trace_.logic_pos.y + 1 == bottom)
							trace_.logic_pos.y = top;
						else
							++trace_.logic_pos.y;
						break;
					case keyboard::enter:
						if (page_mode::month == page)
						{
							int n = get_trace_logic_pos();
							view_month_.month = n;
							this->enter(graph, arg.window_handle);
							redrawn = true;
						}
						else
						{
							if (!trace_.empty_logic_pos())
							{
								auto value = get_trace_logic_pos();

								if (0 < value && value <= static_cast<int>(::nana::date::month_days(view_month_.year, view_month_.month)))
									this->choose(arg.window_handle, value);
								else
								{
									move(value > 0, graph, arg.window_handle);
									redrawn = true;
								}
							}
						}
						break;
					case keyboard::escape:
						if (page_mode::date == page)
						{
							enter(graph, arg.window_handle);
							redrawn = true;
						}
						break;
					default:
						return false;
					}

					trace_.is_by_mouse = false;
					return true;
				}

				::nana::date read() const
				{
					return{date_.year, date_.month, date_.day};
				}

				void weekname(std::size_t pos, std::string&& name)
				{
					if (pos < 7)
						weekstr_[pos] = std::move(name);
				}

				where pos_where(const ::nana::size& area, const ::nana::point& pos)
				{
					int pos_right = static_cast<int>(area.width) - 1;
					int pos_bottom = static_cast<int>(area.height) - 1;
					if (0 < pos.y && pos.y < static_cast<int>(topbar_height))
					{
						if (static_cast<int>(border_size) < pos.x && pos.x < pos_right)
						{
							if (pos.x < border_size + 16)
								return where::left_button;
							else if (pos_right - border_size - 16 < pos.x)
								return where::right_button;
							return where::topbar;
						}
					}
					else if (topbar_height < pos.y && pos.y < pos_bottom)
					{
						trace_.ms_pos = pos;
						trace_.clear_logic_pos();
						trace_.is_by_mouse = true;

						return where::textarea;
					}
					return where::none;
				}

				bool set_where(where pos)
				{
					if (pos == pos_)
						return false;

					if (pos != where::textarea)
					{
						trace_.clear_logic_pos();
						trace_.is_by_mouse = false;
					}

					pos_ = pos;
					return true;
				}

				int get_trace_logic_pos() const
				{
					if (trace_.empty_logic_pos())
						return 0;

					const int rows = (page_mode::month == page ? 4 : 7);

					int n = trace_.logic_pos.y * rows + trace_.logic_pos.x + 1;
					if (page_mode::date == page)
					{
						if (n < 8) return 0; //Here is week title bar
						int dw = nana::date::day_of_week(view_month_.year, view_month_.month, 1);
						n -= (dw ? dw + 7 : 14);
					}
					return n;
				}

				bool get_trace(point pos, int & res) const
				{
					pos -= dwbasis_.refpos;

					int lines = 7, rows = 7;	//for page::date

					if (page_mode::month == page)
					{
						lines = 3;
						rows = 4;
					}

					int width = static_cast<int>(dwbasis_.row_s * rows);
					int height = static_cast<int>(dwbasis_.line_s * lines);

					if (0 <= pos.x && pos.x < width && 0 <= pos.y && pos.y < height)
					{
						pos.x = static_cast<int>(pos.x / dwbasis_.row_s);
						pos.y = static_cast<int>(pos.y / dwbasis_.line_s);

						int n = pos.y * rows + pos.x + 1;
						if (page_mode::date == page)
						{
							if (n < 8) return false; //Here is week title bar
							int dw = nana::date::day_of_week(view_month_.year, view_month_.month, 1);
							n -= (dw ? dw + 7 : 14);
						}
						res = n;
						return true;
					}
					return false;
				}

				int days_of_view_month() const
				{
					return ::nana::date::month_days(view_month_.year, view_month_.month);
				}

				void step_view_month(bool is_forward)
				{
					if (is_forward)
					{
						if (++view_month_.month == 13)
						{
							view_month_.month = 1;
							++view_month_.year;
						}
					}
					else if (--view_month_.month == 0)
					{
						view_month_.month = 12;
						--view_month_.year;
					}
				}

				view_month_rep& view_month()
				{
					return view_month_;
				}


				void set_view_month(int month)
				{
					view_month_.month = month;
				}

				void choose(window window_handle, int day)
				{
					date_.year = view_month_.year;
					date_.month = view_month_.month;
					date_.day = day;
					chose_ = true;

					arg_datechooser evt_arg{ static_cast<nana::date_chooser*>(API::get_widget(window_handle)) };
					API::events<nana::date_chooser>(window_handle).date_changed.emit(evt_arg, window_handle);
				}

				bool chose() const
				{
					return chose_;
				}

				void render(graph_reference graph)
				{
					const unsigned width = graph.width() - 2;

					graph.rectangle(false, static_cast<color_rgb>(0xb0b0b0));
					graph.rectangle({ 1, 1, width, static_cast<unsigned>(topbar_height) }, true, colors::white);

					_m_draw_topbar(graph);

					if (graph.height() > 2 + topbar_height)
					{
						const ::nana::point refpos(1, static_cast<int>(topbar_height)+1);

						_m_calc_basis(graph, refpos);

						nana::paint::graphics gbuf({ width, graph.height() - 2 - topbar_height });
						gbuf.rectangle(true, { 0xf0, 0xf0, 0xf0 });

						switch (page)
						{
						case page_mode::date:
							_m_draw_days(gbuf);
							break;
						case page_mode::month:
							_m_draw_months(gbuf);
							break;
						default:	break;
						}
						graph.bitblt(refpos.x, refpos.y, gbuf);
					}
				}

				void perf_transform(graph_reference graph, window window_handle, transform_action transf)
				{
					nana::point refpos(1, static_cast<int>(topbar_height)+1);
					nana::rectangle r(0, 0, graph.width() - 2, graph.height() - 2 - topbar_height);

					nana::paint::graphics dirtybuf({ r.width, r.height });
					dirtybuf.bitblt(r, graph, refpos);

					render(graph);

					nana::paint::graphics gbuf({ r.width, r.height });
					gbuf.bitblt(r, graph, refpos);

					perf_transform_helper(window_handle, transf, graph, dirtybuf, gbuf, refpos);
				}
			private:
				//rendering functions

				void _m_calc_basis(graph_reference graph, const nana::point& refpos)
				{
					dwbasis_.refpos = refpos;
					unsigned width = graph.width() - 2;
					unsigned height = graph.height() - 2;

					if (height < topbar_height)
						height = 0;
					else
						height -= topbar_height;

					if (page_mode::date == page)
					{
						dwbasis_.line_s = height / 7.0;
						dwbasis_.row_s = width / 7.0;
					}
					else if (page_mode::month == page)
					{
						dwbasis_.line_s = height / 3.0;
						dwbasis_.row_s = width / 4.0;
					}
				}

				void _m_draw_topbar(graph_reference graph)
				{
					::nana::color arrow_bgcolor;
					::nana::rectangle arrow_r{ static_cast<int>(border_size), (topbar_height - 16) / 2 + 1, 16, 16 };
					facade<element::arrow> arrow("solid_triangle");
					arrow.direction(::nana::direction::west);
					arrow.draw(graph, arrow_bgcolor, (pos_ == where::left_button ? colors_.highlighted : colors_.normal), arrow_r, element_state::normal);

					arrow_r.x = static_cast<int>(graph.width()) - static_cast<int>(border_size + 17);
					arrow.direction(::nana::direction::east);
					arrow.draw(graph, arrow_bgcolor, (pos_ == where::right_button ? colors_.highlighted : colors_.normal), arrow_r, element_state::normal);

					const char * monthstr[] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };

					if (graph.width() > 32 + border_size * 2)
					{
						std::string str;
						if (page_mode::date == page)
						{
							str = ::nana::internationalization()(monthstr[this->view_month_.month - 1]);
							str += "  ";
						}
						str += std::to_string(view_month_.year);

						nana::size txt_s = graph.text_extent_size(str);

						int top = (topbar_height - static_cast<int>(txt_s.height)) / 2 + 1;

						int xpos = static_cast<int>(graph.width() - txt_s.width) / 2;
						if (xpos < border_size + 16) xpos = 16 + border_size + 1;

						graph.string({ xpos, top }, str, (pos_ == where::topbar ? colors_.highlighted : colors_.normal));
					}
				}

				void _m_draw_pos(graph_reference graph, const upoint& logic_pos, const std::string& text_utf8, bool primary, bool sel)
				{
					nana::rectangle r(static_cast<int>(logic_pos.x * dwbasis_.row_s), static_cast<int>(logic_pos.y * dwbasis_.line_s),
						static_cast<int>(dwbasis_.row_s), static_cast<int>(dwbasis_.line_s));

					auto color = colors_.normal;

					if (trace_.is_by_mouse ? ((pos_ == where::textarea) && r.is_hit(trace_.ms_pos - dwbasis_.refpos)) : (trace_.logic_pos == logic_pos))
					{
						if ((page != page_mode::date) || logic_pos.y)
						{
							color = colors_.highlighted;
							graph.rectangle(r, true, colors_.bgcolor);

							if (trace_.is_by_mouse)
								trace_.logic_pos = logic_pos;
						}
					}

					if (sel)
					{
						color = colors_.highlighted;
						graph.rectangle(r, true, colors_.bgcolor);
						graph.rectangle(r, false, colors_.selected);

						if (trace_.empty_logic_pos())
							trace_.logic_pos = logic_pos;
					}

					if (false == primary)
						color = { 0xB0, 0xB0, 0xB0 };

					auto txt_s = graph.text_extent_size(text_utf8);
					graph.string({ r.x + static_cast<int>(r.width - txt_s.width) / 2, r.y + static_cast<int>(r.height - txt_s.height) / 2 }, text_utf8, color);
				}

				void _m_draw_ex_days(graph_reference graph, const upoint& begin_logic_pos, bool before)
				{
					int x = nana::date::day_of_week(view_month_.year, view_month_.month, 1);
					int year = view_month_.year;
					if (before)
					{
						
						int month = view_month_.month - 1;
						if (month == 0)
						{
							--year;
							month = 12;
						}
						bool same = (date_.year == year && date_.month == month);
						auto days = nana::date::month_days(year, month);

						unsigned size = (x ? x : 7);
						unsigned beg = days - size + 1;

						for (upoint logic_pos{0, 1}; logic_pos.x < size; ++ logic_pos.x)
						{
							this->_m_draw_pos(graph, logic_pos, std::to_string(beg + logic_pos.x), false, same && (static_cast<unsigned>(date_.day) == beg + logic_pos.x));
						}
					}
					else
					{
						int month = view_month_.month + 1;
						if (month == 13)
						{
							++year;
							month = 1;
						}
						bool same = (date_.year == year && date_.month == month);

						int day = 1;

						for (upoint logic_pos{begin_logic_pos}; logic_pos.y < 7; ++logic_pos.y)
						{
							for (; logic_pos.x < 7; ++logic_pos.x)
							{
								_m_draw_pos(graph, logic_pos, std::to_string(day), false, same && (date_.day == day));
								++day;
							}
							logic_pos.x = 0;
						}
					}
				}

				void _m_draw_days(graph_reference graph)
				{
					upoint logic_pos{ 0, 0 };
					for (; logic_pos.x < 7; ++logic_pos.x)
						_m_draw_pos(graph, logic_pos, weekstr_[logic_pos.x], true, false);

					//draw the days that before the first day of this month
					_m_draw_ex_days(graph, {}, true);

					int days = static_cast<int>(nana::date::month_days(view_month_.year, view_month_.month));

					bool same = (date_.year == view_month_.year && date_.month == view_month_.month);

					logic_pos.x = ::nana::date::day_of_week(view_month_.year, view_month_.month, 1);
					logic_pos.y = (logic_pos.x ? 1 : 2);
					
					int day = 1;
					while (day <= days)
					{
						for (; logic_pos.x < 7; ++logic_pos.x)
						{
							_m_draw_pos(graph, logic_pos, std::to_string(day), true, (same && date_.day == day));
							if (++day > days) break;
						}
						if (day > days) break;
						logic_pos.y++;
						logic_pos.x = 0;
					}

					++logic_pos.x;
					if (logic_pos.x >= 7)
					{
						logic_pos.x = 0;
						++logic_pos.y;
					}

					_m_draw_ex_days(graph, logic_pos, false);
				}

				void _m_draw_months(graph_reference graph)
				{
					const char * monthstr[] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
					::nana::internationalization i18n;

					for (unsigned y = 0; y < 3; ++y)
						for (upoint logic_pos{0, y}; logic_pos.x < 4; ++logic_pos.x)
						{
							int index = logic_pos.x + logic_pos.y * 4;
							_m_draw_pos(graph, logic_pos, i18n(monthstr[index]), true, (view_month_.year == date_.year) && (index + 1 == date_.month));
						}
				}
			private:
				page_mode page{ page_mode::date };
			private:
				::std::string weekstr_[7];

				bool chose_{ false };	//indicates whether the date is chose
				where pos_{ where::none };

				struct
				{
					bool is_by_mouse{ true };
					point ms_pos;				//the mouse position
					upoint logic_pos{ 8, 8 };	//pos(8, 8) indicates the end position.

					void clear_logic_pos()
					{
						logic_pos.x = logic_pos.y = 8;
					}

					bool empty_logic_pos() const
					{
						return (logic_pos.x == 8);
					}
				}trace_;

				drawing_basis dwbasis_;

				struct color_rep
				{
					::nana::color highlighted{ static_cast<color_rgb>(0x4d56c8) };
					::nana::color selected{ static_cast<color_rgb>(0x2f3699) };
					::nana::color normal{ colors::black };
					::nana::color bgcolor{ static_cast<color_rgb>(0x88c4ff) };
				}colors_;

				view_month_rep view_month_;

				struct date_mode
				{
					int year;
					int month;
					int day;
				}date_;
			};
			//class trigger: public drawer_trigger

				trigger::trigger()
					: model_(new model)
				{
				}

				trigger::~trigger()
				{
					delete model_;
				}

				auto trigger::get_model() const -> model*
				{
					return model_;
				}

				void trigger::refresh(graph_reference graph)
				{
					model_->render(graph);
				}

				void trigger::mouse_move(graph_reference graph, const arg_mouse& arg)
				{
					auto pos = model_->pos_where(graph.size(), arg.pos);
					if (model_->set_where(pos) || (model::where::textarea == pos))
					{
						model_->render(graph);
						API::dev::lazy_refresh();
					}
				}

				void trigger::mouse_leave(graph_reference graph, const arg_mouse&)
				{
					if (model_->set_where(model::where::none))
					{
						model_->render(graph);
						API::dev::lazy_refresh();
					}
				}

				void trigger::mouse_up(graph_reference graph, const arg_mouse& arg)
				{
					bool redraw = true;
					auto pos = model_->pos_where(graph.size(), arg.pos);
					//transform_action tfid = transform_action::none;
					bool transformed = false;

					if (pos == model::where::topbar)
					{
						if (model::page_mode::date == model_->page)
						{
							model_->enter(graph, arg.window_handle);
							transformed = true;
						}
						else
							redraw = false;
					}
					else if (pos == model::where::textarea)
					{
						int ret = 0;

						bool good_trace = model_->get_trace(arg.pos, ret);

						switch (model_->page)
						{
						case model::page_mode::date:
							if (good_trace)
							{
								if (ret < 1)
								{
									model_->move(false, graph, arg.window_handle);
									transformed = true;
								}
								else
								{
									auto days = model_->days_of_view_month();
									if (ret > days)
									{
										model_->move(true, graph, arg.window_handle);
										transformed = true;
									}
									else //Selecting a day in this month
									{
										model_->choose(arg.window_handle, ret);
									}
								}
							}
							break;
						case model::page_mode::month:
							if (good_trace)
								model_->view_month().month = ret;

							model_->enter(graph, arg.window_handle);
							transformed = true;
							break;
						default:
							redraw = false;
						}
					}
					else if (model::where::left_button == pos || model::where::right_button == pos)
					{
						model_->move((model::where::right_button == pos), graph, arg.window_handle);
						transformed = true;
					}
					else
						redraw = false;

					if (redraw)
					{
						if (!transformed)
							model_->render(graph);

						API::dev::lazy_refresh();
					}
				}

				void trigger::key_press(graph_reference graph, const arg_keyboard& arg)
				{
					bool redrawn = false;
					if (model_->respond_key(graph, arg, redrawn))
					{
						if (!redrawn)
							refresh(graph);

						API::dev::lazy_refresh();
					}
				}
			//end class trigger
		}//end namespace date_chooser
	}//end namespace drawerbase

	//class date_chooser
		date_chooser::date_chooser()
		{}

		date_chooser::date_chooser(window wd, bool visible)
		{
			create(wd, rectangle(), visible);
		}

		date_chooser::date_chooser(window wd, const rectangle& r, bool visible)
		{
			create(wd, r, visible);
		}

		bool date_chooser::chose() const
		{
			return get_drawer_trigger().get_model()->chose();
		}

		nana::date date_chooser::read() const
		{
			return get_drawer_trigger().get_model()->read();
		}

		void date_chooser::weekstr(unsigned index, ::std::string str)
		{
			get_drawer_trigger().get_model()->weekname(index, std::move(str));
			API::refresh_window(*this);
		}
	//end class date_chooser
}//end namespace nana
