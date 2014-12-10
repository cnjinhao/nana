
#include <nana/config.hpp>
#include PLATFORM_SPEC_HPP
#include <nana/paint/text_renderer.hpp>
#include <nana/unicode_bidi.hpp>
#include <nana/paint/detail/native_paint_interface.hpp>

namespace nana
{
	namespace paint
	{
		namespace helper
		{
			template<typename F>
			void for_each_line(const nana::char_t * str, std::size_t len, int top, F & f)
			{
				auto head = str;
				auto end = str + len;
				for(auto i = str; i != end; ++i)
				{
					if(*i == '\n')
					{
						top += static_cast<int>(f(top, head, i - head));
						head = i + 1;
					}
				}
				if(head != end)
					f(top, head, end - head);
			}

			struct draw_string
			{
				drawable_type dw;
				const int x, endpos;
				nana::unicode_bidi bidi;
				std::vector<nana::unicode_bidi::entity> reordered;
				align text_align;

				draw_string(drawable_type dw, int x, int endpos, align ta)
					: dw(dw), x(x), endpos(endpos), text_align(ta)
				{}

				unsigned operator()(int top, const nana::char_t * buf, std::size_t bufsize)
				{
					int xpos = x;
					unsigned pixels = 0;
					bidi.linestr(buf, bufsize, reordered);
					switch(text_align)
					{
					case align::left:
						for(auto & ent : reordered)
						{
							std::size_t len = ent.end - ent.begin;
							nana::size ts = detail::text_extent_size(dw, ent.begin, len);
							if(ts.height > pixels)	pixels = ts.height;
						
							if(xpos + static_cast<int>(ts.width) > 0)
								detail::draw_string(dw, xpos, top, ent.begin, len);

							xpos += static_cast<int>(ts.width);
							if(xpos >= endpos)
								break;
						}
						break;
					case align::center:
						{
							unsigned lenpx = 0;
							std::vector<unsigned> widths;
							for(auto & ent : reordered)
							{
								auto ts = detail::text_extent_size(dw, ent.begin, ent.end - ent.begin);
								if(ts.height > pixels) pixels = ts.height;
								lenpx += ts.width;
								widths.push_back(ts.width);
							}

							xpos += (endpos - xpos - static_cast<int>(lenpx))/2;
							auto ipx = widths.begin();
							for(auto & ent : reordered)
							{
								if(xpos + static_cast<int>(*ipx) > 0)
									detail::draw_string(dw, xpos, top, ent.begin, ent.end - ent.begin);

								xpos += static_cast<int>(*ipx);
								if(xpos >= endpos)
									break;
							}
						}
						break;
					case align::right:
						{
							int xend = endpos;
							std::swap(xpos, xend);
							for(auto i = reordered.rbegin(), end = reordered.rend(); i != end; ++i)
							{
								auto & ent = *i;
								std::size_t len = ent.end - ent.begin;
								nana::size ts = detail::text_extent_size(dw, ent.begin, len);
								if(ts.height > pixels)	pixels = ts.height;

								if(xpos > xend)
								{
									xpos -= static_cast<int>(ts.width);
									detail::draw_string(dw, xpos, top, i->begin, len);
								}
							
								if(xpos <= xend || xpos <= 0)
									break;
							}
						}
						break;
					}
					return pixels;
				}
			};

			struct draw_string_omitted
			{
				graphics & graph;
				int x, endpos;
				nana::color_t color;
				unsigned omitted_pixels;
				nana::unicode_bidi bidi;
				std::vector<nana::unicode_bidi::entity> reordered;

				draw_string_omitted(graphics& graph, int x, int endpos, nana::color_t color, bool omitted)
					: graph(graph), x(x), endpos(endpos), color(color)
				{
					omitted_pixels = (omitted ? graph.text_extent_size(STR("..."), 3).width : 0);
					if(endpos - x > static_cast<int>(omitted_pixels))
						this->endpos -= omitted_pixels;
					else
						this->endpos = x;
				}

				unsigned operator()(int top, const nana::char_t * buf, std::size_t bufsize)
				{
					drawable_type dw = graph.handle();
					int xpos = x;
					unsigned pixels = 0;
					bidi.linestr(buf, bufsize, reordered);
					for(auto & i : reordered)
					{
						std::size_t len = i.end - i.begin;
						nana::size ts = detail::text_extent_size(dw, i.begin, len);
						if(ts.height > pixels)	pixels = ts.height;
						
						if(xpos + static_cast<int>(ts.width) <= endpos)
						{
							detail::draw_string(dw, xpos, top, i.begin, len);
							xpos += static_cast<int>(ts.width);
						}
						else
						{
							nana::rectangle r;
							r.width = endpos - xpos;
							r.height = ts.height;

							nana::paint::graphics dum_graph(r.width, r.height);

							dum_graph.bitblt(r, graph, nana::point(xpos, top));
							dum_graph.string(0, 0, color, i.begin, len);

							r.x = xpos;
							r.y = top;
							graph.bitblt(r, dum_graph);
							if(omitted_pixels)
								detail::draw_string(dw, endpos, top, STR("..."), 3);
							break;
						}
					}
					return pixels;	
				}
			};


			struct draw_string_auto_changing_lines
			{
				graphics & graph;
				int x, endpos;
				nana::unicode_bidi bidi;
				std::vector<nana::unicode_bidi::entity> reordered;
				std::vector<nana::size> ts_keeper;
				align text_align;

				draw_string_auto_changing_lines(graphics& graph, int x, int endpos, align ta)
					: graph(graph), x(x), endpos(endpos), text_align(ta)
				{}

				unsigned operator()(int top, const nana::char_t * buf, std::size_t bufsize)
				{
					unsigned pixels = 0;

					drawable_type dw = graph.handle();
					unsigned str_w = 0;
					ts_keeper.clear();
					bidi.linestr(buf, bufsize, reordered);
					for(auto & i : reordered)
					{
						nana::size ts = detail::text_extent_size(dw, i.begin, i.end - i.begin);
						if(ts.height > pixels) pixels = ts.height;
						ts_keeper.push_back(ts);
						str_w += ts.width;
					}

					//Test whether the text needs the new line.
					if(x + static_cast<int>(str_w) > endpos)
					{
						pixels = 0;
						unsigned line_pixels = 0;
						int xpos = x;
						int orig_top = top;
						auto i_ts_keeper = ts_keeper.cbegin();
						for(auto & i : reordered)
						{
							if(line_pixels < i_ts_keeper->height)
								line_pixels = i_ts_keeper->height;

							bool beyond_edge = (xpos + static_cast<int>(i_ts_keeper->width) > endpos);
							if(beyond_edge)
							{
								std::size_t len = i.end - i.begin;
								if(len > 1)
								{
									unsigned * pxbuf = new unsigned[len];
									//Find the char that should be splitted
									graph.glyph_pixels(i.begin, len, pxbuf);
									std::size_t idx_head = 0, idx_splitted;

									do
									{
										idx_splitted = find_splitted(idx_head, len, xpos, endpos, pxbuf);
										if(idx_splitted == len)
										{
											detail::draw_string(dw, xpos, top, i.begin + idx_head, idx_splitted - idx_head);
											for(std::size_t i = idx_head; i < len; ++i)
												xpos += static_cast<int>(pxbuf[i]);
											break;
										}
										//Check the word whether it is splittable.
										if(splittable(i.begin, idx_splitted))
										{
											detail::draw_string(dw, xpos, top, i.begin + idx_head, idx_splitted - idx_head);
											idx_head = idx_splitted;
											xpos = x;
											top += line_pixels;
											line_pixels = i_ts_keeper->height;
										}
										else
										{
											//Search the splittable character from idx_head to idx_splitted
											const nana::char_t * u = i.begin + idx_splitted;
											const nana::char_t * head = i.begin + idx_head;

											for(; head < u; --u)
											{
												if(splittable(head, u - head))
													break;
											}

											if(u != head)
											{
												detail::draw_string(dw, xpos, top, head, u - head);
												idx_head += u - head;
												xpos = x;
												top += line_pixels;
												line_pixels = i_ts_keeper->height;
											}
											else
											{
												u = i.begin + idx_splitted;
												const nana::char_t * end = i.begin + len;
												for(; u < end; ++u)
												{
													if(splittable(head, u - head))
														break;
												}
												std::size_t splen = u - head;
												top += line_pixels;
												xpos = x;
												detail::draw_string(dw, x, top, head, splen);
												line_pixels = i_ts_keeper->height;

												for(std::size_t k = idx_head; k < idx_head + splen; ++k)
													xpos += static_cast<int>(pxbuf[k]);
												if(xpos >= endpos)
												{
													xpos = x;
													top += line_pixels;
													line_pixels = i_ts_keeper->height;
												}
												idx_head += splen;
											}
										}
									}while(idx_head < len);

									delete [] pxbuf;
								}
								else
								{
									detail::draw_string(dw, x, top += static_cast<int>(line_pixels), i.begin, 1);
									xpos = x + static_cast<int>(i_ts_keeper->width);
								}
								line_pixels = 0;
							}
							else
							{
								detail::draw_string(dw, xpos, top, i.begin, i.end - i.begin);
								xpos += static_cast<int>(i_ts_keeper->width);
							}

							++i_ts_keeper;
						}

						pixels = (top - orig_top) + line_pixels;
					}
					else
					{
						//The text could be drawn in a line.
						if((align::left == text_align) || (align::center == text_align))
						{
							int xpos = x;
							if(align::center == text_align)
								xpos += (endpos - x - static_cast<int>(str_w)) / 2;
							auto i_ts_keeper = ts_keeper.cbegin();
							for(auto & ent : reordered)
							{
								const nana::size & ts = *i_ts_keeper;

								if(xpos + static_cast<int>(ts.width) > 0)
									detail::draw_string(dw, xpos, top, ent.begin, ent.end - ent.begin);

								xpos += static_cast<int>(ts.width);
								++i_ts_keeper;
							}					
						}
						else if(align::right == text_align)
						{
							int xpos = endpos;
							auto i_ts_keeper = ts_keeper.crbegin();
							for(auto i = reordered.crbegin(), end = reordered.crend(); i != end; ++i)
							{
								auto & ent = *i;
								std::size_t len = ent.end - ent.begin;
								const nana::size & ts = *i_ts_keeper;

								xpos -= ts.width;
								if(xpos >= 0)
									detail::draw_string(dw, xpos, top, ent.begin, len);
								++i_ts_keeper;
							}							
						}
					}
					return pixels;
				}

				static std::size_t find_splitted(std::size_t begin, std::size_t end, int x, int endpos, unsigned * pxbuf)
				{
					unsigned acc_width = 0;
					for(std::size_t i = begin; i < end; ++i)
					{
						if(x + static_cast<int>(acc_width + pxbuf[i]) > endpos)
						{
							if(i == begin)
								++i;
							return i;
						}
						acc_width += pxbuf[i];
					}
					return end;
				}

				static bool splittable(const nana::char_t * str, std::size_t index)
				{
					nana::char_t ch = str[index];
					if(('0' <= ch && ch <= '9') || ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z'))
					{
						nana::char_t prch;
						if(index)
						{
							prch = str[index - 1];
							if('0' <= ch && ch <= '9')
								return !(('0' <= prch && prch <= '9') || (str[index - 1] == '-'));

							return (('z' < prch || prch < 'a') && ('Z' < prch || prch < 'A'));
						}
						else
							return false;
					}
					return true;
				}
			};

			struct extent_auto_changing_lines
			{
				graphics & graph;
				int x, endpos;
				nana::unicode_bidi bidi;
				std::vector<nana::unicode_bidi::entity> reordered;
				std::vector<nana::size> ts_keeper;
				unsigned extents;

				extent_auto_changing_lines(graphics& graph, int x, int endpos)
					: graph(graph), x(x), endpos(endpos), extents(0)
				{}

				unsigned operator()(int top, const nana::char_t * buf, std::size_t bufsize)
				{
					unsigned pixels = 0;

					drawable_type dw = graph.handle();
					unsigned str_w = 0;
					ts_keeper.clear();
					bidi.linestr(buf, bufsize, reordered);
					for(auto & i : reordered)
					{
						nana::size ts = detail::text_extent_size(dw, i.begin, i.end - i.begin);
						ts_keeper.push_back(ts);
						str_w += ts.width;
					}

					auto i_ts_keeper = ts_keeper.cbegin();
					//Test whether the text needs the new line.
					if(x + static_cast<int>(str_w) > endpos)
					{
						unsigned line_pixels = 0;
						int xpos = x;
						int orig_top = top;

						for(auto & i : reordered)
						{
							if(line_pixels < i_ts_keeper->height)
								line_pixels = i_ts_keeper->height;

							bool beyond_edge = (xpos + static_cast<int>(i_ts_keeper->width) > endpos);
							if(beyond_edge)
							{
								std::size_t len = i.end - i.begin;
								if(len > 1)
								{
									unsigned * pxbuf = new unsigned[len];
									//Find the char that should be splitted
									graph.glyph_pixels(i.begin, len, pxbuf);
									std::size_t idx_head = 0, idx_splitted;

									do
									{
										idx_splitted = draw_string_auto_changing_lines::find_splitted(idx_head, len, xpos, endpos, pxbuf);

										if(idx_splitted == len)
										{
											for(std::size_t i = idx_head; i < len; ++i)
												xpos += static_cast<int>(pxbuf[i]);
											break;
										}
										//Check the word whether it is splittable.
										if(draw_string_auto_changing_lines::splittable(i.begin, idx_splitted))
										{
											idx_head = idx_splitted;
											xpos = x;
											top += line_pixels;
											line_pixels = i_ts_keeper->height;
										}
										else
										{
											//Search the splittable character from idx_head to idx_splitted
											const nana::char_t * u = i.begin + idx_splitted;
											const nana::char_t * head = i.begin + idx_head;

											for(; head < u; --u)
											{
												if(draw_string_auto_changing_lines::splittable(head, u - head))
													break;
											}

											if(u != head)
											{
												idx_head += u - head;
												xpos = x;
												top += line_pixels;
												line_pixels = i_ts_keeper->height;
											}
											else
											{
												u = i.begin + idx_splitted;
												const nana::char_t * end = i.begin + len;
												for(; u < end; ++u)
												{
													if(draw_string_auto_changing_lines::splittable(head, u - head))
														break;
												}
												std::size_t splen = u - head;
												top += line_pixels;
												xpos = x;
												line_pixels = i_ts_keeper->height;

												for(std::size_t k = idx_head; k < idx_head + splen; ++k)
													xpos += static_cast<int>(pxbuf[k]);
												if(xpos >= endpos)
												{
													xpos = x;
													top += line_pixels;
													line_pixels = i_ts_keeper->height;
												}
												idx_head += splen;
											}
										}
									}while(idx_head < len);

									delete [] pxbuf;
								}
								else
									xpos = x + static_cast<int>(i_ts_keeper->width);

								line_pixels = 0;
							}
							else
								xpos += static_cast<int>(i_ts_keeper->width);

							++i_ts_keeper;
						}

						pixels = (top - orig_top) + line_pixels;
					}
					else
					{
						while(i_ts_keeper != ts_keeper.cend())
						{
							const nana::size & ts = *(i_ts_keeper++);
							if(ts.height > pixels) pixels = ts.height;
						}
					}
					extents += pixels;
					return pixels;
				}
			};
		}//end namespace helper

		//class text_renderer
		text_renderer::text_renderer(graph_reference graph, align ta)
			: graph_(graph), text_align_(ta)
		{}

		void text_renderer::render(int x, int y, nana::color_t col, const nana::char_t * str, std::size_t len)
		{
			if(graph_)
			{
				helper::draw_string ds(graph_.handle(), x, static_cast<int>(graph_.width()), text_align_);
				ds.dw->fgcolor(col);
				helper::for_each_line(str, len, y, ds);
			}
		}

		void text_renderer::render(int x, int y, nana::color_t col, const nana::char_t * str, std::size_t len, unsigned restricted_pixels, bool omitted)
		{
			if(graph_)
			{
				helper::draw_string_omitted dso(graph_, x, x + static_cast<int>(restricted_pixels), col, omitted);
				graph_.handle()->fgcolor(col);
				helper::for_each_line(str, len, y, dso);
			}
		}

		void text_renderer::render(int x, int y, nana::color_t col, const nana::char_t * str, std::size_t len, unsigned restricted_pixels)
		{
			if(graph_)
			{
				helper::draw_string_auto_changing_lines dsacl(graph_, x, x + static_cast<int>(restricted_pixels), text_align_);
				graph_.handle()->fgcolor(col);
				helper::for_each_line(str, len, y, dsacl);
			}
		}

		nana::size text_renderer::extent_size(int x, int y, const nana::char_t* str, std::size_t len, unsigned restricted_pixels) const
		{
			nana::size extents;
			if(graph_)
			{
				helper::extent_auto_changing_lines eacl(graph_, x, x + static_cast<int>(restricted_pixels));
				helper::for_each_line(str, len, y, eacl);
				extents.width = restricted_pixels;
				extents.height = eacl.extents;
			}
			return extents;
		}
		//end class text_renderer


	}
}
