/*
 *	A date chooser Implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/date_chooser.cpp
 */

#include <nana/gui/widgets/date_chooser.hpp>
#include <nana/paint/gadget.hpp>
#include <nana/system/platform.hpp>
#include <sstream>

namespace nana
{
	namespace drawerbase
	{
		namespace date_chooser
		{
			//class trigger: public drawer_trigger

				trigger::trigger()
					: widget_(nullptr), chose_(false), page_(page::date), pos_(where::none)
				{
					const nana::string ws[] = {STR("S"), STR("M"), STR("T"), STR("W"), STR("T"), STR("F"), STR("S")};
					const nana::string ms[] = {STR("January"), STR("February"), STR("March"), STR("April"), STR("May"), STR("June"), STR("July"), STR("August"), STR("September"), STR("October"), STR("November"), STR("December")};

					for(int i = 0; i < 7; ++i)	weekstr_[i] = ws[i];
					for(int i = 0; i < 12; ++i) monthstr_[i] = ms[i];

					nana::date d;
					chdate_.year = chmonth_.year = d.read().year;
					chdate_.month = chmonth_.month = d.read().month;
					chdate_.day = d.read().day;
				}

				bool trigger::chose() const
				{
					return chose_;
				}

				nana::date trigger::read() const
				{
					return nana::date(chdate_.year, chdate_.month, chdate_.day);
				}

				void trigger::week_name(unsigned index, const nana::string& str)
				{
					if(0 <= index && index < 7)
						this->weekstr_[index] = str;
				}

				void trigger::month_name(unsigned index, const nana::string& str)
				{
					if(0 <= index && index < 12)
						this->monthstr_[index] = str;
				}

				void trigger::_m_init_color()
				{
					color_.selected = 0x2F3699;
					color_.highlight = 0x4D56C8;
					color_.normal = 0x0;
					color_.bkcolor = 0x88C4FF;
				}

				trigger::where trigger::_m_pos_where(graph_reference graph, int x, int y)
				{
					int xend = static_cast<int>(graph.width()) - 1;
					int yend = static_cast<int>(graph.height()) - 1;
					if(0 < y && y < static_cast<int>(topbar_height))
					{
						if(static_cast<int>(border_size) < x && x < xend)
						{
							if(x < border_size + 16)
								return where::left_button;
							else if(xend - border_size - 16 < x)
								return where::right_button;
							return where::topbar;
						}
					}
					else if(topbar_height < y && y < yend)
					{
						trace_pos_.x = x;
						trace_pos_.y = y;
						return where::textarea;
					}
					return where::none;
				}

				void trigger::_m_draw(graph_reference graph)
				{
					_m_init_color();

					const unsigned width = graph.width() - 2;

					graph.rectangle(0xB0B0B0, false);
					graph.rectangle(1, 1, width, topbar_height, 0xFFFFFF, true);

					_m_draw_topbar(graph);

					if(graph.height() > 2 + topbar_height)
					{
						nana::point refpos(1, static_cast<int>(topbar_height) + 1);

						nana::paint::graphics gbuf(width, graph.height() - 2 - topbar_height);
						gbuf.rectangle(0xF0F0F0, true);

						switch(page_)
						{
						case page::date:
							_m_draw_days(refpos, gbuf);
							break;
						case page::month:
							_m_draw_months(refpos, gbuf);
							break;
						default:	break;
						}

						graph.bitblt(refpos.x, refpos.y, gbuf);
					}
				}

				void trigger::_m_draw_topbar(graph_reference graph)
				{
					int ypos = (topbar_height - 16) / 2 + 1;

					const nana::color_t color = color_.normal;

					nana::paint::gadget::arrow_16_pixels(graph, border_size, ypos, (pos_ == where::left_button ? color_.highlight : color), 1, nana::paint::gadget::directions::to_west);
					nana::paint::gadget::arrow_16_pixels(graph, graph.width() - (border_size + 16 + 1), ypos, (pos_ == where::right_button ? color_.highlight : color), 1, nana::paint::gadget::directions::to_east);

					if(graph.width() > 32 + border_size * 2)
					{
						std::stringstream ss;
						ss<<chmonth_.year;
						nana::string str;
						if(page_ == page::date)
						{
							str += monthstr_[chmonth_.month - 1];
							str += STR("  ");
						}
						str += nana::charset(ss.str());

						nana::size txt_s = graph.text_extent_size(str);

						ypos = (topbar_height - txt_s.height) / 2 + 1;

						int xpos = (graph.width() - txt_s.width) / 2;
						if(xpos < border_size + 16) xpos = 16 + border_size + 1;

						graph.string(xpos, ypos, (pos_ == where::topbar ? color_.highlight : color), str);
					}
				}

				void trigger::_m_make_drawing_basis(drawing_basis& dbasis, graph_reference graph, const nana::point& refpos)
				{
					dbasis.refpos = refpos;
					const unsigned width = graph.width();
					const unsigned height = graph.height();

					if(page::date == page_)
					{
						dbasis.line_s = height / 7.0;
						dbasis.row_s = width / 7.0;
					}
					else if(page::month == page_)
					{
						dbasis.line_s = height / 3.0;
						dbasis.row_s = width / 4.0;
					}

					dbasis_ = dbasis;
				}

				void trigger::_m_draw_pos(drawing_basis & dbasis, graph_reference graph, int x, int y, const nana::string& str, bool primary, bool sel)
				{
					nana::rectangle r(static_cast<int>(x * dbasis.row_s), static_cast<int>(y * dbasis.line_s),
						static_cast<int>(dbasis.row_s), static_cast<int>(dbasis.line_s));

					nana::color_t color{ color_.normal };

					nana::point tpos{ trace_pos_ - dbasis.refpos };

					if((pos_ == where::textarea)
						&& (r.x <= tpos.x)
						&& (tpos.x < r.x + static_cast<int>(r.width))
						&& (r.y <= tpos.y)
						&& (tpos.y < r.y + static_cast<int>(r.height)))
					{
						if((page_ != page::date) || y)
						{
							color = color_.highlight;
							graph.rectangle(r, color_.bkcolor, true);
						}
					}

					if(sel)
					{
						color = color_.highlight;
						graph.rectangle(r, color_.bkcolor, true);
						graph.rectangle(r, color_.selected, false);
					}

					if(primary == false)
						color = 0xB0B0B0;

					nana::size txt_s = graph.text_extent_size(str);
					graph.string(r.x + static_cast<int>(r.width - txt_s.width) / 2, r.y + static_cast<int>(r.height - txt_s.height) / 2, color, str);
				}

				void trigger::_m_draw_pos(drawing_basis & dbasis, graph_reference graph, int x, int y, int number, bool primary, bool sel)
				{
				    //The C++ library comes with MinGW does not provide std::to_wstring() conversion
				    std::wstringstream ss;
				    ss<<number;
					_m_draw_pos(dbasis, graph, x, y, nana::charset(ss.str()), primary, sel);
				}

				void trigger::_m_draw_ex_days(drawing_basis & dbasis, graph_reference graph, int begx, int begy, bool before)
				{
					int x = nana::date::day_of_week(chmonth_.year, chmonth_.month, 1);
					int y = (x ? 1 : 2);

					if(before)
					{
						int year = chmonth_.year;
						int month = chmonth_.month - 1;
						if(month == 0)
						{
							--year;
							month = 12;
						}
						bool same = (chdate_.year == year && chdate_.month == month);
						int days = nana::date::month_days(year, month);

						int size = (x ? x : 7);
						int beg = days - size + 1;

						for(int i = 0; i < size; ++i)
						{
							this->_m_draw_pos(dbasis, graph, i, 1, beg + i, false, same && (chdate_.day == beg + i));
						}
					}
					else
					{
						int year = chmonth_.year;
						int month = chmonth_.month + 1;
						if(month == 13)
						{
							++year;
							month = 1;
						}
						bool same = (chdate_.year == year && chdate_.month == month);

						int day = 1;
						x = begx;
						for(y = begy; y < 7; ++y)
						{
							for(; x < 7; ++x)
							{
								_m_draw_pos(dbasis, graph, x, y, day, false, same && (chdate_.day == day));
								++day;
							}
							x = 0;
						}
					}
				}

				void trigger::_m_draw_days(const nana::point& refpos, graph_reference graph)
				{
					drawing_basis dbasis;
					_m_make_drawing_basis(dbasis, graph, refpos);

					for(int i = 0; i < 7; ++i)
						_m_draw_pos(dbasis, graph, i, 0, weekstr_[i], true, false);

					int day = 1;
					int x = nana::date::day_of_week(chmonth_.year, chmonth_.month, 1);
					int y = (x ? 1 : 2);

					//draw the days that before the first day of this month
					_m_draw_ex_days(dbasis, graph, 0, 0, true);
					//
					int days = static_cast<int>(nana::date::month_days(chmonth_.year, chmonth_.month));

					bool same = (chdate_.year == chmonth_.year && chdate_.month == chmonth_.month);
					while(day <= days)
					{
						for(; x < 7; ++x)
						{
							_m_draw_pos(dbasis, graph, x, y, day, true, (same && chdate_.day == day));
							if(++day > days) break;
						}
						if(day > days) break;
						y++;
						x = 0;
					}

					++x;
					if(x >= 7)
					{
						x = 0;
						++y;
					}

					_m_draw_ex_days(dbasis, graph, x, y, false);
				}

				void trigger::_m_draw_months(const nana::point& refpos, graph_reference graph)
				{
					drawing_basis dbasis;
					_m_make_drawing_basis(dbasis, graph, refpos);

					for(int y = 0; y < 3; ++y)
						for(int x = 0; x < 4; ++x)
						{
							int index = x + y * 4;
							_m_draw_pos(dbasis, graph, x, y, monthstr_[index], true, (chmonth_.year == chdate_.year) && (index + 1 == chdate_.month));
						}
				}

				bool trigger::_m_get_trace(point pos, int & res)
				{
					pos -= dbasis_.refpos;

					int lines = 7, rows = 7;	//defaultly for page::date

					if(page_ == page::month)
					{
						lines = 3;
						rows = 4;
					}

					int width = static_cast<int>(dbasis_.row_s * rows);
					int height = static_cast<int>(dbasis_.line_s * lines);

					if(0 <= pos.x && pos.x < width && 0 <= pos.y && pos.y < height)
					{
						pos.x = static_cast<int>(pos.x / dbasis_.row_s);
						pos.y = static_cast<int>(pos.y / dbasis_.line_s);

						int n = pos.y * rows + pos.x + 1;
						if(page_ == page::date)
						{
							if(n < 8) return false; //Here is week title bar
							int dw = nana::date::day_of_week(chmonth_.year, chmonth_.month, 1);
							n -= (dw ? dw + 7 : 14);
						}
						res = n;
						return true;
					}
					return false;
				}

				void trigger::_m_perf_transform(transform_action tfid, graph_reference graph, graph_reference dirtybuf, graph_reference newbuf, const nana::point& refpos)
				{
					const int sleep_time = 15;
					const int count = 20;
					double delta = dirtybuf.width() / double(count);
					double delta_h = dirtybuf.height() / double(count);
					double fade = 1.0 / count;

					if(tfid == transform_action::to_right)
					{
						nana::rectangle dr(0, refpos.y, 0, dirtybuf.height());
						nana::rectangle nr(refpos.x, refpos.y, 0, newbuf.height());
						for(int i = 1; i < count; ++i)
						{
							int off_x = static_cast<int>(delta * i);
							dr.x = refpos.x + off_x;
							dr.width = dirtybuf.width() - off_x;

							graph.bitblt(dr, dirtybuf);

							nr.width = off_x;
							graph.bitblt(nr, newbuf, nana::point(static_cast<int>(dr.width), 0));

							API::update_window(*widget_);
							nana::system::sleep(sleep_time);
						}
					}
					else if(tfid == transform_action::to_left)
					{
						double delta = dirtybuf.width() / double(count);
						nana::rectangle dr(refpos.x, refpos.y, 0, dirtybuf.height());
						nana::rectangle nr(0, refpos.y, 0, newbuf.height());

						for(int i = 1; i < count; ++i)
						{
							int off_x = static_cast<int>(delta * i);
							dr.width = dirtybuf.width() - off_x;

							graph.bitblt(dr, dirtybuf, nana::point(off_x, 0));

							nr.x = refpos.x + static_cast<int>(dr.width);
							nr.width = off_x;
							graph.bitblt(nr, newbuf);

							API::update_window(*widget_);
							nana::system::sleep(sleep_time);
						}
					}
					else if(tfid == transform_action::to_leave)
					{
						nana::paint::graphics dzbuf(newbuf.size());
						nana::paint::graphics nzbuf(newbuf.size());

						nana::rectangle r;
						for(int i = 1; i < count; ++i)
						{
							r.width = static_cast<int>(newbuf.width() - delta * i);
							r.height = static_cast<int>(newbuf.height() - delta_h * i);
							r.x = static_cast<int>(newbuf.width() - r.width) / 2;
							r.y = static_cast<int>(newbuf.height() - r.height) / 2;

							dzbuf.rectangle(0xFFFFFF, true);
							dirtybuf.stretch(dzbuf, r);

							r.width = static_cast<int>(newbuf.width() + delta * (count - i));
							r.height = static_cast<int>(newbuf.height() + delta_h * (count - i));
							r.x = static_cast<int>(newbuf.width() - r.width) / 2;
							r.y = static_cast<int>(newbuf.height() - r.height) / 2;
							newbuf.stretch(nzbuf, r);

							nzbuf.blend(nzbuf.size(), dzbuf, nana::point(), fade * (count - i));
							graph.bitblt(refpos.x, refpos.y, dzbuf);

							API::update_window(*widget_);
							nana::system::sleep(sleep_time);
						}
					}
					else if(tfid == transform_action::to_enter)
					{
						nana::paint::graphics dzbuf(newbuf.size());
						nana::paint::graphics nzbuf(newbuf.size());

						nana::rectangle r;
						for(int i = 1; i < count; ++i)
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
							nzbuf.rectangle(0xFFFFFF, true);
							newbuf.stretch(nzbuf, r);

							nzbuf.blend(nzbuf.size(), dzbuf, nana::point(), fade * (count - i));
							graph.bitblt(refpos.x, refpos.y, dzbuf);

							API::update_window(*widget_);
							nana::system::sleep(sleep_time);
						}
					}

					graph.bitblt(nana::rectangle(refpos, newbuf.size()), newbuf);
				}

				void trigger::refresh(graph_reference graph)
				{
					_m_draw(graph);
				}

				void trigger::attached(widget_reference widget, graph_reference)
				{
					widget_ = &widget;
				}

				void trigger::mouse_move(graph_reference graph, const arg_mouse& arg)
				{
					where pos = _m_pos_where(graph, arg.pos.x, arg.pos.y);
					if(pos == pos_ && pos_ != where::textarea) return;
					pos_ = pos;
					_m_draw(graph);
					API::lazy_refresh();
				}

				void trigger::mouse_leave(graph_reference graph, const arg_mouse&)
				{
					if(where::none == pos_) return;
					pos_ = where::none;
					_m_draw(graph);
					API::lazy_refresh();
				}

				void trigger::mouse_up(graph_reference graph, const arg_mouse& arg)
				{
					bool redraw = true;
					where pos = _m_pos_where(graph, arg.pos.x, arg.pos.y);
					transform_action tfid = transform_action::none;

					if(pos == where::topbar)
					{
						switch(page_)
						{
						case page::date:
							page_ = page::month;
							tfid = transform_action::to_leave;
							break;
						default:
							redraw = false;
						}
					}
					else if(pos == where::textarea)
					{
						int ret = 0;
						switch(page_)
						{
						case page::date:
							if(_m_get_trace(arg.pos, ret))
							{
								if(ret < 1)
								{
									if(--chmonth_.month == 0)
									{
										--chmonth_.year;
										chmonth_.month = 12;
									}
									tfid = transform_action::to_right;
								}
								else
								{
									int days = nana::date::month_days(chmonth_.year, chmonth_.month);
									if(ret > days)
									{
										if(++chmonth_.month == 13)
										{
											++chmonth_.year;
											chmonth_.month = 1;
										}
										tfid = transform_action::to_left;
									}
									else //Selecting a day in this month
									{
										chdate_.year = chmonth_.year;
										chdate_.month = chmonth_.month;
										chdate_.day = ret;
										chose_ = true;
									}
								}
							}
							break;
						case page::month:
							if(_m_get_trace(arg.pos, ret))
								chmonth_.month = ret;
							page_ = page::date;
							tfid = transform_action::to_enter;
							break;
						default:
							redraw = false;
						}
					}
					else if(pos == where::left_button || pos == where::right_button)
					{
						int end_m;
						int beg_m;
						int step;
						if(pos == where::left_button)
						{
							end_m = 1;
							beg_m = 12;
							step = -1;
							tfid = transform_action::to_right;
						}
						else
						{
							end_m = 12;
							beg_m = 1;
							step = 1;
							tfid = transform_action::to_left;
						}
						switch(page_)
						{
						case page::date:
							if(chmonth_.month == end_m)
							{
								chmonth_.month = beg_m;
								chmonth_.year += step;
							}
							else
								chmonth_.month += step;
							break;
						case page::month:
							chmonth_.year += step;
							break;
						default:
							redraw = false;
						}
					}

					if(redraw)
					{
						if(tfid != transform_action::none)
						{
							nana::point refpos(1, static_cast<int>(topbar_height) + 1);
							nana::rectangle r(0, 0, graph.width() - 2, graph.height() - 2 - topbar_height);

							nana::paint::graphics dirtybuf(r.width, r.height);
							dirtybuf.bitblt(r, graph, refpos);

							_m_draw(graph);

							nana::paint::graphics gbuf(r.width, r.height);
							gbuf.bitblt(r, graph, refpos);

							_m_perf_transform(tfid, graph, dirtybuf, gbuf, refpos);
						}
						else
							_m_draw(graph);

						API::lazy_refresh();
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

		date_chooser::date_chooser(window wd, const nana::string& text, bool visible)
		{
			create(wd, rectangle(), visible);
			caption(text);
		}

		date_chooser::date_chooser(window wd, const nana::char_t* text, bool visible)
		{
			create(wd, rectangle(), visible);
			caption(text);		
		}

		date_chooser::date_chooser(window wd, const rectangle& r, bool visible)
		{
			create(wd, r, visible);
		}

		bool date_chooser::chose() const
		{
			return get_drawer_trigger().chose();
		}

		nana::date date_chooser::read() const
		{
			return get_drawer_trigger().read();
		}

		void date_chooser::weekstr(unsigned index, const nana::string& str)
		{
			get_drawer_trigger().week_name(index, str);
			API::refresh_window(*this);
		}

		void date_chooser::monthstr(unsigned index, const nana::string& str)
		{
			get_drawer_trigger().month_name(index, str);
			API::refresh_window(*this);
		}
	//end class date_chooser
}//end namespace nana
