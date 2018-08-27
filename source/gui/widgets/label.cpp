/*
 *	A Label Control Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2018 Jinhao(cnjinhao@hotmail.com)
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
#include <nana/gui/widgets/skeletons/text_token_stream.hpp>
#include <nana/gui/detail/widget_content_measurer_interface.hpp>
#include <nana/unicode_bidi.hpp>
#include <nana/system/platform.hpp>
#include <stdexcept>
#include <sstream>

#define VISUAL_LINES

namespace nana
{
	namespace drawerbase
	{
		namespace label
		{
			class renderer
			{
#ifndef VISUAL_LINES
				typedef widgets::skeletons::dstream::linecontainer::iterator iterator;

				struct pixel_tag
				{
					int x_base;				//The x position where this line starts.
					std::size_t pixels;
					std::size_t baseline;	//The baseline for drawing text.
					std::vector<iterator> values;	//line values
				};
#else
				//Iterator of content element in a line.
				using content_element_iterator = widgets::skeletons::dstream::linecontainer::const_iterator; //subsitute for member type iterator

				struct visual_line //subsitute of pixel_tag
				{
					struct element
					{
						content_element_iterator content_element;
						std::pair<std::size_t, std::size_t> range; //A part of text in a text element. first: text begin, second: text length

						element(const content_element_iterator& iterator, std::size_t range_begin, std::size_t range_end):
							content_element(iterator),
							range(range_begin, range_end)
						{
						}
					};

					int x_base;	//The x position where this line starts.

					std::size_t extent_height_px;
					std::size_t baseline; //The baseline for rendering text.
					std::vector<element> elements; //description of text element in this rendering line.
				};

#endif

				//this is a helper variable, it just keeps the status while drawing.
				struct render_status
				{
					unsigned allowed_width;
					align text_align;
					align_v text_align_v;

					nana::point pos;
#ifndef VISUAL_LINES
					std::vector<pixel_tag> pixels;
#else
					std::vector<visual_line> vslines; //The lines description of a line of text. substitute of member pixels.
#endif
					std::size_t index;	//indicates the current rendering visual line.
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

					auto pre_font = graph.typeface();	//used for restoring the font

#ifdef _nana_std_has_string_view
					const unsigned def_line_pixels = graph.text_extent_size(std::wstring_view{ L" ", 1 }).height;
#else
					const unsigned def_line_pixels = graph.text_extent_size(L" ", 1).height;
#endif

					font_ = pre_font;
					fblock_ = nullptr;

					_m_set_default(pre_font, fgcolor);

					_m_measure(graph);

					render_status rs;

					rs.allowed_width = graph.size().width;
					rs.text_align = th;
					rs.text_align_v = tv;

#ifndef VISUAL_LINES
					std::deque<std::vector<pixel_tag> > pixel_lines;
#else
					//All visual lines data of whole text.
					std::deque<std::vector<visual_line>> content_lines;
#endif

					std::size_t extent_v_pixels = 0;	//the pixels, in height, that text will be painted.

					for (auto & line : dstream_)
					{
#ifndef VISUAL_LINES
						_m_line_pixels(line, def_line_pixels, rs);

						for (auto & m : rs.pixels)
							extent_v_pixels += m.pixels;

						pixel_lines.emplace_back(std::move(rs.pixels));
#else
						_m_prepare_visual_lines(graph, line, def_line_pixels, rs);

						for (auto & vsline : rs.vslines)
							extent_v_pixels += vsline.extent_height_px;

						content_lines.emplace_back(std::move(rs.vslines));
#endif

						if(extent_v_pixels >= graph.height())
							break;
					}

					if((tv != align_v::top) && extent_v_pixels < graph.height())
					{
						rs.pos.y = static_cast<int>(graph.height() - extent_v_pixels);

						if(align_v::center == tv)
							rs.pos.y >>= 1;
					}
					else
						rs.pos.y = 0;

#ifndef VISUAL_LINES
					auto pixels_iterator = pixel_lines.begin();
#else
					auto vsline_iterator = content_lines.begin();
#endif
					for (auto & line : dstream_)
					{
						if (rs.pos.y >= static_cast<int>(graph.height()))
							break;

#ifndef VISUAL_LINES
						rs.index = 0;
						rs.pixels.clear();

						rs.pixels.swap(*pixels_iterator++);

						rs.pos.x = rs.pixels.front().x_base;

						//Stop drawing when it goes out of range.
						if(false == _m_each_line(graph, line, rs))
							break;

						rs.pos.y += static_cast<int>(rs.pixels.back().pixels);
#else
						rs.index = 0;
						rs.vslines.clear();
						rs.vslines.swap(*vsline_iterator++);
						rs.pos.x = rs.vslines.front().x_base;

						if (!_m_foreach_visual_line(graph, rs))
							break;

						rs.pos.y += static_cast<int>(rs.vslines.back().extent_height_px);
#endif
					}

					graph.typeface(pre_font);
				}

				bool find(int x, int y, std::wstring& target, std::wstring& url) const noexcept
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

#ifdef _nana_std_has_string_view
					const unsigned def_line_pixels = graph.text_extent_size(std::wstring_view(L" ", 1)).height;
#else
					const unsigned def_line_pixels = graph.text_extent_size(L" ", 1).height;
#endif

					font_ = ft;
					fblock_ = nullptr;

					_m_set_default(ft, colors::black);
					_m_measure(graph);

					render_status rs;

					rs.allowed_width = limited;
					rs.text_align = th;
					rs.text_align_v = tv;

					for(auto & line: dstream_)
					{
#ifndef VISUAL_LINES
						rs.pixels.clear();
						unsigned w = _m_line_pixels(line, def_line_pixels, rs);
#else
						rs.vslines.clear();
						auto w = _m_prepare_visual_lines(graph, line, def_line_pixels, rs);
#endif

						if(limited && (w > limited))
							w = limited;

						if(retsize.width < w)
							retsize.width = w;

#ifndef VISUAL_LINES
						for (auto & px : rs.pixels)
							retsize.height += static_cast<unsigned>(px.pixels);
#else
						for (auto& vsline : rs.vslines)
							retsize.height += static_cast<unsigned>(vsline.extent_height_px);
#endif
					}

					return retsize;
				}
			private:
				//Manage the fblock for a specified rectangle if it is a traceable fblock.
				void _m_insert_if_traceable(int x, int y, const nana::size& sz, widgets::skeletons::fblock* fbp)
				{
					if(fbp->target.size() || fbp->url.size())
					{
						traceable_.emplace_back();
						auto & tr = traceable_.back();
						tr.r.x = x;
						tr.r.y = y;
						tr.r.dimension(sz);
						tr.target = fbp->target;
						tr.url = fbp->url;
					}
				}

				void _m_set_default(const ::nana::paint::font& ft, const ::nana::color& fgcolor)
				{
					def_.font_name = ft.name();
					def_.font_size = ft.size();
					def_.font_bold = ft.bold();
					def_.fgcolor = fgcolor;
				}

				const ::nana::color& _m_fgcolor(nana::widgets::skeletons::fblock* fp) noexcept
				{
					while(fp->fgcolor.invisible())
					{
						fp = fp->parent;
						if(nullptr == fp)
							return def_.fgcolor;
					}
					return fp->fgcolor;
				}

				double _m_font_size(nana::widgets::skeletons::fblock* fp) noexcept
				{
					while(fp->font_size < 0)
					{
						fp = fp->parent;
						if(nullptr == fp)
							return def_.font_size;
					}
					return fp->font_size;
				}

				bool _m_bold(nana::widgets::skeletons::fblock* fp) noexcept
				{
					while(fp->bold_empty)
					{
						fp = fp->parent;
						if(nullptr == fp)
							return def_.font_bold;
					}
					return fp->bold;
				}

				const std::string& _m_fontname(nana::widgets::skeletons::fblock* fp) noexcept
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
						auto fontsize = _m_font_size(fp);
						bool bold = _m_bold(fp);

						if((fontsize != font_.size()) || bold != font_.bold() || name != font_.name())
						{
							paint::font::font_style fs;
							fs.weight = (bold ? 800 : 400);
							font_ = paint::font{ name, fontsize, fs };
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

#ifndef VISUAL_LINES
				void _m_align_x_base(const render_status& rs, pixel_tag & px, unsigned w) noexcept
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
#else
				void _m_prepare_x(const render_status& rs, visual_line & vsline, unsigned w) noexcept
				{
					switch (rs.text_align)
					{
					case align::left:
						vsline.x_base = 0;
						break;
					case align::center:
						vsline.x_base = (static_cast<int>(rs.allowed_width - w) >> 1);
						break;
					case align::right:
						vsline.x_base = static_cast<int>(rs.allowed_width - w);
						break;
					}
				}
#endif

#ifdef VISUAL_LINES

				/**
				 * prepare data for rendering a line of text.
				 */
				unsigned _m_prepare_visual_lines(graph_reference graph, dstream::linecontainer& line, unsigned def_line_px, render_status& rs)
				{
					unsigned abs_text_px = 0;
					unsigned max_ascent = 0;
					unsigned max_descent = 0;
					unsigned max_content_height = 0;

					int text_pos = 0;

					std::vector<visual_line::element> vsline_elements;

					for (auto i = line.cbegin(); i != line.cend(); ++i)
					{
						auto const data = i->data_ptr;
						auto fblock = i->fblock_ptr;

						abs_text_px += data->size().width;

						unsigned ascent = 0;
						unsigned descent = 0;


						auto extent_size = data->size();

						//Check if the content is displayed in current line.
						if ((0 == rs.allowed_width) || (text_pos + extent_size.width <= rs.allowed_width))
						{
							text_pos += static_cast<int>(extent_size.width);

							//Adjust height of extent_size for special text alignement.
							if (fblock::aligns::baseline == fblock->text_align)
							{
								ascent = static_cast<unsigned>(data->ascent());
								descent = static_cast<unsigned>(extent_size.height - ascent);

								if (max_descent < descent)
									max_descent = descent;

								if ((false == data->is_text()) && (extent_size.height < max_ascent + max_descent))
									extent_size.height = max_ascent + max_descent;
							}

							if (max_ascent < ascent)	max_ascent = ascent;
							if (max_descent < descent)	max_descent = descent;
							if (max_content_height < extent_size.height)	max_content_height = extent_size.height;
							vsline_elements.emplace_back(i, 0, data->text().size());

							continue;
						}

						//make a visual line for existing vsline elements
						if (text_pos)
						{
#ifdef _nana_std_has_returnable_emplace_back
							auto & vsline = rs.vslines.emplace_back();
#else
							rs.vslines.emplace_back();
							auto & vsline = rs.vslines.back();
#endif
							_m_prepare_x(rs, vsline, static_cast<unsigned>(text_pos));

							if (max_ascent + max_descent > max_content_height)
								max_content_height = max_descent + max_ascent;
							else
								max_ascent = max_content_height - max_descent;

							vsline.extent_height_px = max_content_height;
							vsline.baseline = max_ascent;
							vsline.elements.swap(vsline_elements);
						}

						text_pos = 0;
						max_content_height = max_ascent = max_descent = 0;
						//Adjust height of extent_size for special text alignement.
						if (fblock::aligns::baseline == fblock->text_align)
						{
							ascent = static_cast<unsigned>(data->ascent());
							descent = static_cast<unsigned>(extent_size.height - ascent);

							if (max_descent < descent)
								max_descent = descent;

							if ((false == data->is_text()) && (extent_size.height < max_ascent + max_descent))
								extent_size.height = max_ascent + max_descent;
						}

						if (max_ascent < ascent)	max_ascent = ascent;
						if (max_descent < descent)	max_descent = descent;
						if (max_content_height < extent_size.height)	max_content_height = extent_size.height;

						if (data->is_text())
						{
							_m_change_font(graph, fblock);
							//Split a text into multiple lines
							auto rest_extent_size = extent_size.width;
							std::size_t text_begin = 0;
							while (text_begin < data->text().size())
							{
								unsigned sub_text_px = 0;
								auto sub_text_len = _m_fit_text(graph, data->text().substr(text_begin), rs.allowed_width, sub_text_px);

								if (text_begin + sub_text_len < data->text().size())
								{
									//make a new visual line
#ifdef _nana_std_has_returnable_emplace_back
									auto & vsline = rs.vslines.emplace_back();
#else
									rs.vslines.emplace_back();
									auto & vsline = rs.vslines.back();
#endif
									_m_prepare_x(rs, vsline, sub_text_px);

									vsline.extent_height_px = max_content_height;
									vsline.baseline = max_ascent;
									vsline.elements.emplace_back(i, text_begin, sub_text_len);
								}
								else
								{
									//the last part, write it to vsline_elements to keep the status for next line element(next i)
									vsline_elements.emplace_back(i, text_begin, sub_text_len);

									text_pos = sub_text_px;
								}

								text_begin += sub_text_len;
							}
						}
						else
						{
							//the last part, write it to vsline_elements to keep the status for next line element(next i)
							vsline_elements.emplace_back(i, 0, 0);

							text_pos = static_cast<int>(i->data_ptr->size().width);
						}
					}

					if (!vsline_elements.empty())
					{
#ifdef _nana_std_has_returnable_emplace_back
						auto & vsline = rs.vslines.emplace_back();
#else
						rs.vslines.emplace_back();
						auto & vsline = rs.vslines.back();
#endif
						_m_prepare_x(rs, vsline, static_cast<unsigned>(text_pos));

						if (max_ascent + max_descent > max_content_height)
							max_content_height = max_descent + max_ascent;
						else
							max_ascent = max_content_height - max_descent;

						vsline.extent_height_px = max_content_height;
						vsline.baseline = max_ascent;
						vsline.elements.swap(vsline_elements);
					}

					return abs_text_px;
				}

				//Get the length of characters in a text whose length in pixels doesn't beyond the limited width.
				static unsigned _m_fit_text(graph_reference graph, const std::wstring& text, unsigned limited_width_px, unsigned& text_px) noexcept
				{
#ifdef _nana_std_has_string_view
					auto pxbuf = graph.glyph_pixels(text);
#else
					std::unique_ptr<unsigned[]> pxbuf(new unsigned[text.size()]);
					graph.glyph_pixels(text.c_str(), text.size(), pxbuf.get());
#endif

					text_px = 0;
					for (std::size_t i = 0; i < text.size(); ++i)
					{
						if (text_px + pxbuf[i] > limited_width_px)
							return i;

						text_px += pxbuf[i];
					}
					return text.size();
				}
#else
				unsigned _m_line_pixels(dstream::linecontainer& line, unsigned def_line_pixels, render_status & rs)
				{
					if (line.empty())
					{
						pixel_tag px;
						px.baseline = 0;
						px.pixels = def_line_pixels;
						px.x_base = 0;

						rs.pixels.emplace_back(px);

						return 0;
					}

					unsigned total_w = 0;
					unsigned w = 0;
					unsigned max_ascent = 0;
					unsigned max_descent = 0;
					unsigned max_px = 0;

					//Bidi reorder is requried here

					std::vector<iterator> line_values;

					for(auto i = line.begin(); i != line.end(); ++i)
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

						//Check if the content is displayed in a new line.
						if((0 == rs.allowed_width) || (w + sz.width <= rs.allowed_width))
						{
							w += sz.width;

							if(max_ascent < as)		max_ascent = as;
							if(max_descent < ds)	max_descent = ds;
							if(max_px < sz.height)	max_px = sz.height;
							line_values.emplace_back(i);
						}
						else
						{
							pixel_tag px;
							_m_align_x_base(rs, px, (w ? w : sz.width));

							if(w)
							{
								if(max_ascent + max_descent > max_px)
									max_px = max_descent + max_ascent;
								else
									max_ascent = max_px - max_descent;

								px.pixels = max_px;
								px.baseline = max_ascent;
								px.values.swap(line_values);

								w = sz.width;
								max_px = sz.height;
								max_ascent = as;
								max_descent = ds;
								line_values.emplace_back(i);
							}
							else
							{
								px.pixels = sz.height;
								px.baseline = as;

								px.values.emplace_back(i);

								max_px = 0;
								max_ascent = max_descent = 0;
							}

							rs.pixels.emplace_back(px);
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
						rs.pixels.emplace_back(px);
					}
					return total_w;
				}
#endif

#ifndef VISUAL_LINES
				bool _m_each_line(graph_reference graph, dstream::linecontainer&, render_status& rs)
				{
					std::wstring text;
					iterator block_start;

					const int lastpos = static_cast<int>(graph.height()) - 1;

					for(auto & px : rs.pixels)
					{
						for(auto & render_iterator: px.values)
						{
							auto & value = *render_iterator;
							if (value.data_ptr->is_text())
							{
								//hold the block while the text is empty,
								//it stands for the first block
								if (text.empty())
									block_start = render_iterator;

								text += value.data_ptr->text();
								continue;
							}
							
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
							_m_insert_if_traceable(rs.pos.x, y, sz, value.fblock_ptr);
							rs.pos.x += static_cast<int>(sz.width);

							if(lastpos < y)
								return false;
						}

						if(text.size())
						{
							_m_draw_block(graph, text, block_start, rs);
							text.clear();
						}
					}
					return (rs.pos.y <= lastpos);
				}
#else
				bool _m_foreach_visual_line(graph_reference graph, render_status& rs)
				{
					std::wstring text;
					
					content_element_iterator block_start;

					auto const bottom = static_cast<int>(graph.height()) - 1;

					for (auto & vsline : rs.vslines)
					{
						rs.pos.x = vsline.x_base;
						for (auto& content_elm : vsline.elements)
						{
							_m_draw_vsline_element(graph, content_elm, rs);
						}

						++rs.index;	//next line index
						rs.pos.y += vsline.extent_height_px;

						if (rs.pos.y > bottom)
							return false;
					}

					return (rs.pos.y <= bottom);
				}
#endif


#if 0 //deprecated
				static bool _m_overline(const render_status& rs, int right, bool equal_required) noexcept
				{
					if(align::left == rs.text_align)
						return (equal_required ? right >= static_cast<int>(rs.allowed_width) : right > static_cast<int>(rs.allowed_width));

					return (equal_required ? rs.pixels[rs.index].x_base <= 0 : rs.pixels[rs.index].x_base < 0);
				}
#endif

#ifdef VISUAL_LINES
				static int _m_vsline_element_top(const visual_line& vsline, fblock* fblock_ptr, const data* data_ptr) noexcept
				{
					switch (fblock_ptr->text_align)
					{
					case fblock::aligns::center:
						return static_cast<int>(vsline.extent_height_px - data_ptr->size().height) / 2;
					case fblock::aligns::bottom:
						return static_cast<int>(vsline.extent_height_px - data_ptr->size().height);
					case fblock::aligns::baseline:
						return static_cast<int>(vsline.baseline - (data_ptr->is_text() ? data_ptr->ascent() : data_ptr->size().height));
					default:	break;
					}
					return 0;
				}
#else
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
#endif

#ifdef VISUAL_LINES
				void _m_draw_vsline_element(graph_reference graph, const visual_line::element& vsline_elm, render_status& rs)
				{
					auto data = vsline_elm.content_element->data_ptr;
					auto fblock = vsline_elm.content_element->fblock_ptr;

					if (data->is_text())
					{
						auto const text = data->text().c_str() + vsline_elm.range.first;
						auto const reordered = unicode_reorder(text, vsline_elm.range.second);

						_m_change_font(graph, fblock);
						for (auto & bidi : reordered)
						{
							auto extent_size = data->size();
#ifdef _nana_std_has_string_view
							std::wstring_view text_sv{ bidi.begin, static_cast<std::size_t>(bidi.end - bidi.begin) };
							if (data->text().size() != text_sv.size())
								extent_size = graph.text_extent_size(text_sv);

							const int y = rs.pos.y + _m_vsline_element_top(rs.vslines[rs.index], fblock, data);
							graph.string({ rs.pos.x, y }, text_sv, _m_fgcolor(fblock));
#else
							std::wstring text{ bidi.begin, static_cast<std::size_t>(bidi.end - bidi.begin) };
							if (data->text().size() != text.size())
								extent_size = graph.text_extent_size(text);

							const int y = rs.pos.y + _m_vsline_element_top(rs.vslines[rs.index], fblock, data);
							graph.string({ rs.pos.x, y }, text, _m_fgcolor(fblock));
#endif
							_m_insert_if_traceable(rs.pos.x, y, extent_size, fblock);
							rs.pos.x += static_cast<int>(extent_size.width);
						}
					}
					else
					{
						int y = rs.pos.y + _m_vsline_element_top(rs.vslines[rs.index], fblock, data);

						data->nontext_render(graph, rs.pos.x, y);
						_m_insert_if_traceable(rs.pos.x, y, data->size(), fblock);
						rs.pos.x += static_cast<int>(data->size().width);
					}
				}
#else
				void _m_draw_block(graph_reference graph, const std::wstring& s, dstream::linecontainer::iterator block_start, render_status& rs)
				{
					auto const reordered = unicode_reorder(s.data(), s.length());

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

#if 1
							const int range_text_area = static_cast<int>(rs.allowed_width) - rs.pos.x;

							_m_change_font(graph, fblock_ptr);

							auto text_extent_size = data_ptr->size();
#ifndef _nana_std_has_string_view
							std::wstring_view text_sv{ data_ptr->text().c_str() + text_range.first, text_range.second };
							if (data_ptr->text().size() != text_sv.size())
								text_extent_size = graph.text_extent_size(text_sv);
#else
							auto text_sv = data_ptr->text().substr(text_range.first, text_range.second);
							if (data_ptr->text().size() != text_sv.size())
								text_extent_size = graph.text_extent_size(text_sv);
#endif
							if ((static_cast<int>(text_extent_size.width) > range_text_area) && (rs.pos.x != px.x_base))
							{
								//Change a new line
								rs.pos.y += static_cast<int>(px.pixels);
								px = rs.pixels[++rs.index];
								rs.pos.x = px.x_base;
							}

							const int y = rs.pos.y + _m_text_top(px, fblock_ptr, data_ptr);
							graph.string({ rs.pos.x, y }, text_sv, _m_fgcolor(fblock_ptr));
#else
							const int w = static_cast<int>(rs.allowed_width) - rs.pos.x;
							nana::size text_extent_size = data_ptr->size();
							if ((static_cast<int>(text_extent_size.width) > w) && (rs.pos.x != px.x_base))
							{
								//Change a new line
								rs.pos.y += static_cast<int>(px.pixels);
								px = rs.pixels[++rs.index];
								rs.pos.x = px.x_base;
							}

							const int y = rs.pos.y + _m_text_top(px, fblock_ptr, data_ptr);

							_m_change_font(graph, fblock_ptr);

#ifdef _nana_std_has_string_view
							std::wstring_view text_sv{ data_ptr->text() };
							if (text_range.second != text_sv.size())
							{
								text_sv = text_sv.substr(text_range.first, text_range.second);
								text_extent_size = graph.text_extent_size(text_sv);
							}

							graph.string({ rs.pos.x, y }, text_sv, _m_fgcolor(fblock_ptr));
#else
							if (text_range.second == data_ptr->text().length())
							{
								graph.string({ rs.pos.x, y }, data_ptr->text(), _m_fgcolor(fblock_ptr));
							}
							else
							{
								auto str = data_ptr->text().substr(text_range.first, text_range.second);
								text_extent_size = graph.text_extent_size(str);

								graph.string({ rs.pos.x, y }, str, _m_fgcolor(fblock_ptr));
							}
#endif
#endif //#if 0

							_m_insert_if_traceable(rs.pos.x, y, text_extent_size, fblock_ptr);
							rs.pos.x += static_cast<int>(text_extent_size.width);

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
#endif //VISUAL_LINES

				static std::pair<std::size_t, std::size_t> _m_locate(dstream::linecontainer::iterator& i, std::size_t pos)
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
				::std::deque<traceable> traceable_;

				::nana::paint::font font_;
				struct def_font_tag
				{
					::std::string font_name;
					double font_size;
					bool	font_bold;
					::nana::color fgcolor;
				}def_;
			};

			//class trigger
			//@brief: Draw the label
				struct trigger::implement
				{
					class measurer;

					widget * wd{nullptr};
					paint::graphics * graph{nullptr};
					std::unique_ptr<measurer> msr_ptr{ nullptr };

					align	text_align{align::left};
					align_v	text_align_v{align_v::top};

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

				class trigger::implement::measurer
					: public dev::widget_content_measurer_interface
				{
				public:
					measurer(implement* impl)
						: impl_{ impl }
					{}

					std::optional<size> measure(graph_reference graph, unsigned limit_pixels, bool limit_width) const override
					{
						//Label now doesn't support to measure content with a specified height.
						if (graph && ((0 == limit_pixels) || limit_width))
						{
							return impl_->renderer.measure(graph, limit_pixels, impl_->text_align, impl_->text_align_v);
						}
						return{};
					}

					size extension() const override
					{
						return{ 2, 2 };
					}
				private:
					implement * const impl_;
				};

				trigger::trigger()
					:impl_(new implement)
				{
					impl_->msr_ptr.reset(new trigger::implement::measurer{impl_});
				}

				trigger::~trigger()
				{
					delete impl_;
				}

				trigger::implement * trigger::impl() const
				{
					return impl_;
				}

				void trigger::attached(widget_reference widget, graph_reference graph)
				{
					impl_->graph = &graph;
					impl_->wd = &widget;
					API::dev::set_measurer(widget, impl_->msr_ptr.get());
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
					if (!API::dev::copy_transparent_background(wd, graph))
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

		bool label::transparent() const noexcept
		{
			return API::is_transparent_background(*this);
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

		label& label::click_for(window associated_window) noexcept
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
