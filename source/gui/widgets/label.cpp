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


namespace nana
{
	namespace drawerbase
	{
		namespace label
		{
			class renderer
			{
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

				//this is a helper variable, it just keeps the status while drawing.
				struct render_status
				{
					unsigned allowed_width;
					align text_align;
					align_v text_align_v;

					nana::point pos;
					std::vector<visual_line> vslines; //The lines description of a line of text. substitute of member pixels.
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

				bool format(bool fm) noexcept
				{
					if (fm == format_enabled_)
						return false;
					
					format_enabled_ = fm;
					return true;
				}

				void render(graph_reference graph, const ::nana::color& fgcolor, align th, align_v tv)
				{
					traceable_.clear();

					render_status rs;

					rs.allowed_width = graph.size().width;
					rs.text_align = th;
					rs.text_align_v = tv;

					::nana::size extent_size;

					//All visual lines data of whole text.
					auto content_lines = _m_measure_extent_size(graph, th, tv, true, graph.size().width, extent_size);

					if ((tv != align_v::top) && extent_size.height < graph.height())
					{
						rs.pos.y = static_cast<int>(graph.height() - extent_size.height);

						if (align_v::center == tv)
							rs.pos.y >>= 1;
					}
					else
						rs.pos.y = 0;

					auto pre_font = graph.typeface();	//used for restoring the font
					_m_set_default(pre_font, fgcolor);


					for (auto & line : content_lines)
					{
						rs.index = 0;
						rs.vslines.swap(line);
						rs.pos.x = rs.vslines.front().x_base;

						if (!_m_foreach_visual_line(graph, rs))
							break;

						//Now the y-position of rs has been modified to next line.
					}

					if (transient_.current_font != pre_font)
					{
						graph.typeface(pre_font);
						transient_.current_font.release();
						transient_.current_fblock = nullptr;
					}
				}

				bool find(const point& mouse_pos, std::wstring& target, std::wstring& url) const
				{
					for (auto & t : traceable_)
					{
						if(t.r.is_hit(mouse_pos))
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
					::nana::size extent_size;
					_m_measure_extent_size(graph, th, tv, false, limited, extent_size);

					return extent_size;
				}
			private:
				//Manage the fblock for a specified rectangle if it is a traceable fblock.
				void _m_insert_if_traceable(int x, int y, const nana::size& sz, widgets::skeletons::fblock* fbp)
				{
					if(fbp->target.size() || fbp->url.size())
					{
#ifdef _nana_std_has_emplace_return_type
						auto & tr = traceable_.emplace_back();
#else
						traceable_.emplace_back();
						auto & tr = traceable_.back();
#endif
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

					transient_.current_font = ft;
					transient_.current_fblock = nullptr;
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
					if (fp != transient_.current_fblock)
					{
						auto& name = _m_fontname(fp);
						auto fontsize = _m_font_size(fp);
						bool bold = _m_bold(fp);

						if((fontsize != transient_.current_font.size()) || bold != transient_.current_font.bold() || name != transient_.current_font.name())
						{
							paint::font::font_style fs;
							fs.weight = (bold ? 800 : 400);
							transient_.current_font = paint::font{ name, fontsize, fs };
							graph.typeface(transient_.current_font);
						}
						transient_.current_fblock = fp;
					}
				}

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

				std::deque<std::vector<visual_line>> _m_measure_extent_size(graph_reference graph, nana::align text_align, nana::align_v text_align_v, bool only_screen, unsigned allowed_width_px, nana::size & extent_size)
				{
					auto pre_font = graph.typeface();	//used for restoring the font

					unsigned text_ascent, text_descent, text_ileading;
					graph.text_metrics(text_ascent, text_descent, text_ileading);

					auto const def_line_pixels = text_ascent + text_descent;

					_m_set_default(pre_font, colors::black);

					render_status rs;

					rs.allowed_width = allowed_width_px;
					rs.text_align = text_align;
					rs.text_align_v = text_align_v;

					//All visual lines data of whole text.
					std::deque<std::vector<visual_line>> content_lines;

					extent_size.width = extent_size.height = 0;

					for (auto & line : dstream_)
					{
						auto width_px = _m_prepare_visual_lines(graph, line, def_line_pixels, rs);

						if (width_px > extent_size.width)
							extent_size.width = width_px;

						for (auto & vsline : rs.vslines)
							extent_size.height += static_cast<size::value_type>(vsline.extent_height_px);

						content_lines.emplace_back(std::move(rs.vslines));

						if (only_screen && (extent_size.height >= graph.height()))
							break;
					}

					//The width is not restricted if the allowed_width_px is zero.
					if (allowed_width_px && (allowed_width_px < extent_size.width))
						extent_size.width = allowed_width_px;

					if (transient_.current_font != pre_font)
					{
						graph.typeface(pre_font);
						transient_.current_font.release();
						transient_.current_fblock = nullptr;
					}

					return content_lines;
				}

				/**
				 * prepare data for rendering a line of text.
				 */
				unsigned _m_prepare_visual_lines(graph_reference graph, dstream::linecontainer& line, unsigned def_line_px, render_status& rs)
				{
					if (line.empty())
					{
						//Insert an empty visual line for empty content.
#ifdef _nana_std_has_emplace_return_type
						auto & vsline = rs.vslines.emplace_back();
#else
						rs.vslines.emplace_back();
						auto & vsline = rs.vslines.back();
#endif
						vsline.baseline = 0;
						vsline.extent_height_px = def_line_px;
						vsline.x_base = 0;

						return 0;
					}

					unsigned abs_text_px = 0;
					unsigned max_ascent = 0;
					unsigned max_descent = 0;
					unsigned max_content_height = 0;

					int text_pos = 0;

					std::vector<visual_line::element> vsline_elements;

					for (auto i = line.cbegin(); i != line.cend(); ++i)
					{
						auto const data = i->data_ptr;
						auto const fblock = i->fblock_ptr;

						_m_change_font(graph, fblock);
						data->measure(graph);

						abs_text_px += data->size().width;

						unsigned ascent = 0;
						unsigned descent = 0;


						auto extent_size = data->size();

						//Check if the content is displayed in current line.
						if ((0 == rs.allowed_width) || (text_pos + extent_size.width <= rs.allowed_width))
						{
							text_pos += static_cast<int>(extent_size.width);

							//Adjust height of extent_size for special text alignment.
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
#ifdef _nana_std_has_emplace_return_type
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
						//Adjust height of extent_size for special text alignment.
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
							//Split a text into multiple lines
							std::size_t text_begin = 0;
							while (text_begin < data->text().size())
							{
								unsigned sub_text_px = 0;
								auto sub_text_len = _m_fit_text(graph, data->text().substr(text_begin), rs.allowed_width, sub_text_px);

								//At least one character must be displayed no matter whether the width is enough or not.
								if (0 == sub_text_len)
									sub_text_len = 1;

								if (text_begin + sub_text_len < data->text().size())
								{
									//make a new visual line
#ifdef _nana_std_has_emplace_return_type
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
#ifdef _nana_std_has_emplace_return_type
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
					for (unsigned i = 0; i < text.size(); ++i)
					{
						if (text_px + pxbuf[i] > limited_width_px)
							return i;

						text_px += pxbuf[i];
					}
					return static_cast<unsigned>(text.size());
				}

				bool _m_foreach_visual_line(graph_reference graph, render_status& rs)
				{
					auto const bottom = static_cast<int>(graph.height()) - 1;

					for (auto & vsline : rs.vslines)
					{
						rs.pos.x = vsline.x_base;
						for (auto& content_elm : vsline.elements)
						{
							_m_draw_vsline_element(graph, content_elm, rs);
						}

						++rs.index;	//next line index
						rs.pos.y += static_cast<int>(vsline.extent_height_px);

						if (rs.pos.y > bottom)
							return false;
					}

					return (rs.pos.y <= bottom);
				}

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

				void _m_draw_vsline_element(graph_reference graph, const visual_line::element& vsline_elm, render_status& rs)
				{
					auto data = vsline_elm.content_element->data_ptr;
					auto fblock = vsline_elm.content_element->fblock_ptr;

					if (data->is_text())
					{
						auto const reordered = unicode_reorder(data->text().c_str() + vsline_elm.range.first, vsline_elm.range.second);

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
			private:
				dstream dstream_;
				bool format_enabled_ = false;

				::std::deque<traceable> traceable_;

				struct transient
				{
					widgets::skeletons::fblock * current_fblock{ nullptr };
					paint::font current_font;
				}transient_;

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

					if(impl_->renderer.find(arg.pos, target, url))
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
				substitute.make({ 10, 10 });
				substitute.typeface(this->typeface());
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
