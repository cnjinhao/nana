/*
 *	A Label Control Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: source/gui/widgets/label.cpp
 *	@author: Jinhao
 *	@contributors: Ariel Vina-Rodriguez
 */

#include <nana/gui/widgets/label.hpp>
#include <nana/unicode_bidi.hpp>
#include <nana/gui/widgets/skeletons/text_token_stream.hpp>
#include <nana/system/platform.hpp>
#include <stdexcept>
#include <sstream>

namespace nana
{
	namespace drawerbase
	{
		namespace label
		{
			class renderer
			{
				typedef widgets::skeletons::dstream::linecontainer::iterator iterator;

				struct pixel_tag
				{
					int x_base;				//The x position where this line starts.
					std::size_t pixels;
					std::size_t baseline;	//The baseline for drawing text.
					std::vector<iterator> values;	//line values
				};

				//this is a helper variable, it just keeps the status while drawing.
				struct render_status
				{
					unsigned allowed_width;
					align text_align;
					align_v text_align_v;

					nana::point pos;
					std::vector<pixel_tag> pixels;
					std::size_t index;
				};

				struct traceable
				{
					nana::rectangle r;
					std::wstring target;
					std::wstring url;
				};

			public:
				typedef nana::paint::graphics& graph_reference;
				typedef widgets::skeletons::dstream dstream;
				typedef widgets::skeletons::fblock fblock;
				typedef widgets::skeletons::data data;

				void parse(const std::wstring& s)
				{
					dstream_.parse(s, format_enabled_);
				}

				bool format(bool fm)
				{
					if (fm == format_enabled_)
						return false;
					
					format_enabled_ = fm;
					return true;
				}

				void render(graph_reference graph, const ::nana::color& fgcolor, align th, align_v tv)
				{
					traceable_.clear();

					nana::paint::font ft = graph.typeface();	//used for restoring the font

					const unsigned def_line_pixels = graph.text_extent_size(L" ", 1).height;

					font_ = ft;
					fblock_ = nullptr;

					_m_set_default(ft, fgcolor);

					_m_measure(graph);

					render_status rs;

					rs.allowed_width = graph.size().width;
					rs.text_align = th;
					rs.text_align_v = tv;

					std::deque<std::vector<pixel_tag> > pixel_lines;

					std::size_t extent_v_pixels = 0;	//the pixels, in height, that text will be painted.

					for (auto & line : dstream_)
					{
						_m_line_pixels(line, def_line_pixels, rs);

						for (auto & m : rs.pixels)
							extent_v_pixels += m.pixels;

						pixel_lines.emplace_back(std::move(rs.pixels));

						if(extent_v_pixels >= graph.height())
							break;
					}

					if((tv != align_v::top) && extent_v_pixels < graph.height())
					{
						if(align_v::center == tv)
							rs.pos.y = static_cast<int>(graph.height() - extent_v_pixels) >> 1;
						else if(align_v::bottom == tv)
							rs.pos.y = static_cast<int>(graph.height() - extent_v_pixels);
					}
					else
						rs.pos.y = 0;

					auto pixels_iterator = pixel_lines.begin();

					for (auto & line : dstream_)
					{
						if (rs.pos.y >= static_cast<int>(graph.height()))
							break;

						rs.index = 0;
						rs.pixels.clear();

						rs.pixels.swap(*pixels_iterator++);

						rs.pos.x = rs.pixels.front().x_base;

						//Stop drawing when it goes out of range.
						if(false == _m_each_line(graph, line, rs))
							break;

						rs.pos.y += static_cast<int>(rs.pixels.back().pixels);
					}

					graph.typeface(ft);
				}

				bool find(int x, int y, std::wstring& target, std::wstring& url) const
				{
					for (auto & t : traceable_)
					{
						if(t.r.is_hit(x, y))
						{
							target = t.target;
							url = t.url;
							return true;
						}
					}

					return false;
				}

				::nana::size measure(graph_reference graph, unsigned limited, align th, align_v tv)
				{
					::nana::size retsize;

					auto ft = graph.typeface();	//used for restoring the font

					const unsigned def_line_pixels = graph.text_extent_size(L" ", 1).height;

					font_ = ft;
					fblock_ = nullptr;

					_m_set_default(ft, colors::black);
					_m_measure(graph);

					render_status rs;

					rs.allowed_width = limited;
					rs.text_align = th;
					rs.text_align_v = tv;

					for(auto i = dstream_.begin(), end = dstream_.end(); i != end; ++i)
					{
						rs.pixels.clear();
						unsigned w = _m_line_pixels(*i, def_line_pixels, rs);

						if(limited && (w > limited))
							w = limited;

						if(retsize.width < w)
							retsize.width = w;

						for (auto & px : rs.pixels)
							retsize.height += static_cast<unsigned>(px.pixels);
					}

					return retsize;
				}
			private:
				//Manage the fblock for a specified rectangle if it is a traceable fblock.
				void _m_inser_if_traceable(int x, int y, const nana::size& sz, widgets::skeletons::fblock* fbp)
				{
					if(fbp->target.size() || fbp->url.size())
					{
						traceable tr;
						tr.r.x = x;
						tr.r.y = y;
						tr.r.width = sz.width;
						tr.r.height = sz.height;
						tr.target = fbp->target;
						tr.url = fbp->url;

						traceable_.push_back(tr);
					}
				}

				void _m_set_default(const ::nana::paint::font& ft, const ::nana::color& fgcolor)
				{
					def_.font_name = ft.name();
					def_.font_size = ft.size();
					def_.font_bold = ft.bold();
					def_.fgcolor = fgcolor;
				}

				const ::nana::color& _m_fgcolor(nana::widgets::skeletons::fblock* fp)
				{
					while(fp->fgcolor.invisible())
					{
						fp = fp->parent;
						if(nullptr == fp)
							return def_.fgcolor;
					}
					return fp->fgcolor;
				}

				std::size_t _m_font_size(nana::widgets::skeletons::fblock* fp)
				{
					while(fp->font_size == 0xFFFFFFFF)
					{
						fp = fp->parent;
						if(nullptr == fp)
							return def_.font_size;
					}
					return fp->font_size;
				}

				bool _m_bold(nana::widgets::skeletons::fblock* fp)
				{
					while(fp->bold_empty)
					{
						fp = fp->parent;
						if(nullptr == fp)
							return def_.font_bold;
					}
					return fp->bold;
				}

				const std::string& _m_fontname(nana::widgets::skeletons::fblock* fp)
				{
					while(fp->font.empty())
					{
						fp = fp->parent;
						if(nullptr == fp)
							return def_.font_name;
					}
					return fp->font;
				}

				void _m_change_font(graph_reference graph, nana::widgets::skeletons::fblock* fp)
				{
					if(fp != fblock_)
					{
						auto& name = _m_fontname(fp);
						auto fontsize = static_cast<unsigned>(_m_font_size(fp));
						bool bold = _m_bold(fp);

						if((fontsize != font_.size()) || bold != font_.bold() || name != font_.name())
						{
							font_.make(name, fontsize, bold);
							graph.typeface(font_);
						}
						fblock_ = fp;
					}
				}

				void _m_measure(graph_reference graph)
				{
					nana::paint::font ft = font_;
					for (auto & line : dstream_)
					{
						for (auto & value : line)
						{
							_m_change_font(graph, value.fblock_ptr);
							value.data_ptr->measure(graph);
						}
					}
					if(font_ != ft)
					{
						font_ = ft;
						graph.typeface(ft);
						fblock_ = nullptr;
					}
				}

				void _m_align_x_base(const render_status& rs, pixel_tag & px, unsigned w)
				{
					switch(rs.text_align)
					{
					case align::left:
						px.x_base = 0;
						break;
					case align::center:
						px.x_base = (static_cast<int>(rs.allowed_width - w) >> 1);
						break;
					case align::right:
						px.x_base = static_cast<int>(rs.allowed_width - w);
						break;
					}
				}

				unsigned _m_line_pixels(dstream::linecontainer& line, unsigned def_line_pixels, render_status & rs)
				{
					if (line.empty())
					{
						pixel_tag px;
						px.baseline = 0;
						px.pixels = def_line_pixels;
						px.x_base = 0;

						rs.pixels.push_back(px);

						return 0;
					}

					unsigned total_w = 0;
					unsigned w = 0;
					unsigned max_ascent = 0;
					unsigned max_descent = 0;
					unsigned max_px = 0;

					//Bidi reorder is requried here

					std::vector<iterator> line_values;

					for(auto i = line.begin(), end = line.end(); i != end; ++i)
					{
						data * data_ptr = i->data_ptr;
						nana::size sz = data_ptr->size();
						total_w += sz.width;

						unsigned as = 0;	//ascent
						unsigned ds = 0;	//descent

						if(fblock::aligns::baseline == i->fblock_ptr->text_align)
						{
							as = static_cast<unsigned>(data_ptr->ascent());
							ds = static_cast<unsigned>(sz.height - as);

							if(max_descent < ds)
								max_descent = ds;

							if((false == data_ptr->is_text()) && (sz.height < max_ascent + max_descent))
								sz.height = max_ascent + max_descent;
						}

						if(w + sz.width <= rs.allowed_width)
						{
							w += sz.width;

							if(max_ascent < as)		max_ascent = as;
							if(max_descent < ds)	max_descent = ds;
							if(max_px < sz.height)	max_px = sz.height;
							line_values.push_back(i);
						}
						else
						{
							if(w)
							{
								pixel_tag px;

								_m_align_x_base(rs, px, w);

								if(max_ascent + max_descent > max_px)
									max_px = max_descent + max_ascent;
								else
									max_ascent = max_px - max_descent;

								px.pixels = max_px;
								px.baseline = max_ascent;
								px.values.swap(line_values);

								rs.pixels.push_back(px);

								w = sz.width;
								max_px = sz.height;
								max_ascent = as;
								max_descent = ds;
								line_values.push_back(i);
							}
							else
							{
								pixel_tag px;

								_m_align_x_base(rs, px, sz.width);
								px.pixels = sz.height;
								px.baseline = as;

								px.values.push_back(i);

								rs.pixels.push_back(px);
								max_px = 0;
								max_ascent = max_descent = 0;
							}
						}
					}

					if (max_px)
					{
						pixel_tag px;

						_m_align_x_base(rs, px, w);

						if (max_ascent + max_descent > max_px)
							max_px = max_descent + max_ascent;
						else
							max_ascent = max_px - max_descent;

						px.pixels = max_px;
						px.baseline = max_ascent;
						px.values.swap(line_values);
						rs.pixels.push_back(px);
					}
					return total_w;
				}

				bool _m_each_line(graph_reference graph, dstream::linecontainer& line, render_status& rs)
				{
					std::wstring text;
					iterator block_start;

					const int lastpos = static_cast<int>(graph.height()) - 1;

					for(auto i = rs.pixels.begin(), end = rs.pixels.end(); i != end; ++i)
					{
						for (auto & render_iterator : i->values)
						{
							auto & value = *render_iterator;
							if(false == value.data_ptr->is_text())
							{
								if(text.size())
								{
									_m_draw_block(graph, text, block_start, rs);
									if(lastpos <= rs.pos.y)
										return false;
									text.clear();
								}
								nana::size sz = value.data_ptr->size();

								pixel_tag px = rs.pixels[rs.index];
								if ((rs.allowed_width < rs.pos.x + sz.width) && (rs.pos.x != px.x_base))
								{
									//Change a line.
									rs.pos.y += static_cast<int>(px.pixels);
									px = rs.pixels[++rs.index];
									rs.pos.x = px.x_base;
								}

								int y = rs.pos.y + _m_text_top(px, value.fblock_ptr, value.data_ptr);

								value.data_ptr->nontext_render(graph, rs.pos.x, y);
								_m_inser_if_traceable(rs.pos.x, y, sz, value.fblock_ptr);
								rs.pos.x += static_cast<int>(sz.width);

								if(lastpos < y)
									return false;
							}
							else
							{
								//hold the block while the text is empty,
								//it stands for the first block
								if(text.empty())
									block_start = render_iterator;

								text += value.data_ptr->text();
							}
						}

						if(text.size())
						{
							_m_draw_block(graph, text, block_start, rs);
							text.clear();
						}
					}
					return (rs.pos.y <= lastpos);
				}

				static bool _m_overline(const render_status& rs, int right, bool equal_required)
				{
					if(align::left == rs.text_align)
						return (equal_required ? right >= static_cast<int>(rs.allowed_width) : right > static_cast<int>(rs.allowed_width));

					return (equal_required ? rs.pixels[rs.index].x_base <= 0 : rs.pixels[rs.index].x_base < 0);
				}

				static int _m_text_top(const pixel_tag& px, fblock* fblock_ptr, const data* data_ptr)
				{
					switch(fblock_ptr->text_align)
					{
					case fblock::aligns::center:
						return static_cast<int>(px.pixels - data_ptr->size().height) / 2;
					case fblock::aligns::bottom:
						return static_cast<int>(px.pixels - data_ptr->size().height);
					case fblock::aligns::baseline:
						return static_cast<int>(px.baseline - (data_ptr->is_text() ? data_ptr->ascent() : data_ptr->size().height));
					default:	break;
					}
					return 0;
				}

				void _m_draw_block(graph_reference graph, const std::wstring& s, dstream::linecontainer::iterator block_start, render_status& rs)
				{
					nana::unicode_bidi bidi;
					std::vector<nana::unicode_bidi::entity> reordered;
					bidi.linestr(s.data(), s.length(), reordered);

					pixel_tag px = rs.pixels[rs.index];

					for(auto & bidi : reordered)
					{
						std::size_t pos = bidi.begin - s.data();
						std::size_t len = bidi.end - bidi.begin;

						while (true)
						{
							auto i = block_start;

							//Text range indicates the position of text where begin to output
							//The output length is the min between len and the second of text range.
							auto text_range = _m_locate(i, pos);

							if (text_range.second > len)
								text_range.second = len;

							fblock * fblock_ptr = i->fblock_ptr;
							data * data_ptr = i->data_ptr;

							const int w = static_cast<int>(rs.allowed_width) - rs.pos.x;
							nana::size sz = data_ptr->size();
							if ((static_cast<int>(sz.width) > w) && (rs.pos.x != px.x_base))
							{
								//Change a new line
								rs.pos.y += static_cast<int>(px.pixels);
								px = rs.pixels[++rs.index];
								rs.pos.x = px.x_base;
							}

							const int y = rs.pos.y + _m_text_top(px, fblock_ptr, data_ptr);

							_m_change_font(graph, fblock_ptr);

							if (text_range.second == data_ptr->text().length())
							{
								graph.string({ rs.pos.x, y }, data_ptr->text(), _m_fgcolor(fblock_ptr));
							}
							else
							{
								auto str = data_ptr->text().substr(text_range.first, text_range.second);
								graph.string({ rs.pos.x, y }, str, _m_fgcolor(fblock_ptr));
								sz = graph.text_extent_size(str);
							}
							_m_inser_if_traceable(rs.pos.x, y, sz, fblock_ptr);
							rs.pos.x += static_cast<int>(sz.width);

							if(text_range.second < len)
							{
								len -= text_range.second;
								pos += text_range.second;
							}
							else
								break;
						}
					}
				}

				std::pair<std::size_t, std::size_t> _m_locate(dstream::linecontainer::iterator& i, std::size_t pos)
				{
					std::size_t n = i->data_ptr->text().length();
					while(pos >= n)
					{
						pos -= n;
						n = (++i)->data_ptr->text().length();
					}

					return{ pos, n - pos };
				}
			private:
				dstream dstream_;
				bool format_enabled_ = false;
				::nana::widgets::skeletons::fblock * fblock_ = nullptr;
				std::deque<traceable> traceable_;

				::nana::paint::font font_;
				struct def_font_tag
				{
					::std::string font_name;
					std::size_t font_size;
					bool	font_bold;
					::nana::color fgcolor;
				}def_;
			};

			//class trigger
			//@brief: Draw the label
				struct trigger::impl_t
				{
					widget * wd{nullptr};
					paint::graphics * graph{nullptr};

					align	text_align{align::left};
					align_v	text_align_v;

					class renderer renderer;

					std::wstring target;	//It indicates which target is tracing.
					std::wstring url;

					window for_associated_wd{ nullptr };

					void add_listener(std::function<void(command, const std::string&)>&& fn)
					{
						listener_.emplace_back(std::move(fn));
					}

					void call_listener(command cmd, const std::wstring& tar)
					{
						auto str = to_utf8(tar);
						for (auto & fn : listener_)
							fn(cmd, str);
					}
				private:
					std::vector<std::function<void(command, const std::string&)>> listener_;
				};

				trigger::trigger()
					:impl_(new impl_t)
				{}

				trigger::~trigger()
				{
					delete impl_;
				}

				trigger::impl_t * trigger::impl() const
				{
					return impl_;
				}

				void trigger::attached(widget_reference widget, graph_reference graph)
				{
					impl_->graph = &graph;
					impl_->wd = &widget;
				}

				void trigger::mouse_move(graph_reference, const arg_mouse& arg)
				{
					std::wstring target, url;

					if(impl_->renderer.find(arg.pos.x, arg.pos.y, target, url))
					{
						int cur_state = 0;
						if(target != impl_->target)
						{
							if(impl_->target.size())
							{
								impl_->call_listener(command::leave, impl_->target);
								cur_state = 1;	//Set arrow
							}

							impl_->target = target;

							if(target.size())
							{
								impl_->call_listener(command::enter, impl_->target);
								cur_state = 2;	//Set hand
							}
						}
						if (url != impl_->url)
						{
							if (impl_->url.size())
								cur_state = 1;	//Set arrow

							impl_->url = url;

							if (url.size())
								cur_state = 2;	//Set hand
						}

						if (cur_state)
							impl_->wd->cursor(1 == cur_state ? cursor::arrow : cursor::hand);
					}
					else
					{
						bool restore = false;
						if (impl_->target.size())
						{
							impl_->call_listener(command::leave, impl_->target);
							impl_->target.clear();
							restore = true;
						}

						if (impl_->url.size())
						{
							impl_->url.clear();
							restore = true;
						}

						if(restore)
							impl_->wd->cursor(cursor::arrow);
					}
				}

				void trigger::mouse_leave(graph_reference, const arg_mouse&)
				{
					if(impl_->target.size())
					{
						impl_->call_listener(command::leave, impl_->target);
						impl_->target.clear();
						impl_->wd->cursor(cursor::arrow);
					}
				}

				void trigger::click(graph_reference, const arg_click&)
				{
					//make a copy, because the listener may popup a window, and then
					//user moves the mouse. it will reset the url when the mouse is moving out from the element.
					auto url = impl_->url;

					if(impl_->target.size())
						impl_->call_listener(command::click, impl_->target);

					system::open_url(to_utf8(url));

					API::focus_window(impl_->for_associated_wd);
				}

				void trigger::refresh(graph_reference graph)
				{
					if(nullptr == impl_->wd) return;

					window wd = impl_->wd->handle();
					if(bground_mode::basic != API::effects_bground_mode(wd))
						graph.rectangle(true, API::bgcolor(wd));

					impl_->renderer.render(graph, API::fgcolor(wd), impl_->text_align, impl_->text_align_v);
				}

			//end class label_drawer
		}//end namespace label
	}//end namespace drawerbase


	//
	//class label
		label::label(){}

		label::label(window wd, bool visible)
		{
			create(wd, rectangle(), visible);
            bgcolor(API::bgcolor(wd));
		}

		label::label(window wd, const std::string& text, bool visible)
		{
			throw_not_utf8(text);
			create(wd, rectangle(), visible);
			bgcolor(API::bgcolor(wd));
			caption(text);
		}

		label::label(window wd, const rectangle& r, bool visible)
		{
			create(wd, r, visible);
			bgcolor(API::bgcolor(wd));
		}

		label& label::transparent(bool enabled)
		{
			if(enabled)
				API::effects_bground(*this, effects::bground_transparent(0), 0.0);
			else
				API::effects_bground_remove(*this);
			return *this;
		}

		bool label::transparent() const throw()
		{
			return (bground_mode::basic == API::effects_bground_mode(*this));
		}

		label& label::format(bool f)
		{
			auto impl = get_drawer_trigger().impl();

			if(impl->renderer.format(f))
			{
				window wd = *this;
				impl->renderer.parse(::nana::to_wstring(API::dev::window_caption(wd)));
				API::refresh_window(wd);
			}
			return *this;
		}

		label& label::add_format_listener(std::function<void(command, const std::string&)> f)
		{
			get_drawer_trigger().impl()->add_listener(std::move(f));
			return *this;
		}

		label& label::click_for(window associated_window) throw()
		{
			get_drawer_trigger().impl()->for_associated_wd = associated_window;
			return *this;
		}

		nana::size label::measure(unsigned limited) const
		{
			if(empty())
				return nana::size();

			auto impl = get_drawer_trigger().impl();
			
			//First Check the graph of label
			//Then take a substitute for graph when the graph of label is zero-sized.
			nana::paint::graphics * graph_ptr = impl->graph;
			nana::paint::graphics substitute;
			if(graph_ptr->empty())
			{
				graph_ptr = &substitute;
				graph_ptr->make({ 10, 10 });
			}

			return impl->renderer.measure(*graph_ptr, limited, impl->text_align, impl->text_align_v);
		}

		::nana::size label::measure(paint::graphics& graph, const ::std::string& str, unsigned allowed_width_in_pixel, bool format_enabled, align h_align, align_v v_align)
		{
			throw_not_utf8(str);
			drawerbase::label::renderer rd;
			rd.format(format_enabled);
			rd.parse(to_wstring(str));
			return rd.measure(graph, allowed_width_in_pixel, h_align, v_align);
		}

		label& label::text_align(align th, align_v tv)
		{
			internal_scope_guard lock;
			auto impl = get_drawer_trigger().impl();

			if (th != impl->text_align || tv != impl->text_align_v)
			{
				impl->text_align = th;
				impl->text_align_v = tv;
				API::refresh_window(*this);
			}

			return *this;
		}

		void label::_m_caption(native_string_type&& str)
		{
			internal_scope_guard lock;
			window wd = *this;
			get_drawer_trigger().impl()->renderer.parse(to_wstring(str));
			API::dev::window_caption(wd, std::move(str));
			API::refresh_window(wd);
		}
	//end class label
}//end namespace nana
