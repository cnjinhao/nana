/*
 *	A Toolbar Implementation
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/toolbar.cpp
 */

#include <nana/gui/widgets/toolbar.hpp>
#include <vector>
#include <stdexcept>
#include <nana/gui/tooltip.hpp>

namespace nana
{
	arg_toolbar::arg_toolbar(toolbar& tbar, std::size_t btn)
		: widget{ tbar }, button{btn}
	{}

	namespace drawerbase
	{
		namespace toolbar
		{
			struct listitem
			{
				nana::string text;
				nana::paint::image image;
				bool enable;
			};

			struct item_type
			{
				enum kind{ button, container};

				typedef std::size_t size_type;

				nana::string text;
				nana::paint::image image;
				unsigned	pixels{0};
				nana::size	textsize;
				bool		enable{true};
				window other{nullptr};

				kind type;
				std::function<void(size_type, size_type)> answer;
				std::vector<listitem> children;

				item_type(const nana::string& text, const nana::paint::image& img, kind type)
					:text(text), image(img), type(type)
				{}
			};

			class container
			{
				container(const container&) = delete;
				container& operator=(const container&) = delete;
			public:
				typedef std::vector<item_type*>::size_type size_type;
				typedef std::vector<item_type*>::iterator iterator;
				typedef std::vector<item_type*>::const_iterator const_iterator;

				container() = default;
				~container()
				{
					for(auto ptr : cont_)
						delete ptr;
				}

				void insert(size_type pos, const nana::string& text, const nana::paint::image& img, item_type::kind type)
				{
					item_type* m = new item_type(text, img, type);

					if(pos < cont_.size())
						cont_.insert(cont_.begin() + pos, m);
					else
						cont_.push_back(m);
				}

				void push_back(const nana::string& text, const nana::paint::image& img)
				{
					insert(cont_.size(), text, img, item_type::kind::button);
				}

				void push_back(const nana::string& text)
				{
					insert(cont_.size(), text, nana::paint::image(), item_type::kind::button);
				}

				void insert(size_type pos)
				{
					if(pos < cont_.size())
						cont_.insert(cont_.begin() + pos, static_cast<item_type*>(nullptr)); //both works in C++0x and C++2003
					else
						cont_.push_back(nullptr);
				}

				void push_back()
				{
					cont_.push_back(nullptr);
				}

				size_type size() const
				{
					return cont_.size();
				}

				item_type* at(size_type n)
				{
					if(n < cont_.size())
						return cont_[n];

					throw std::out_of_range("toolbar: bad index!");
				}

				iterator begin()
				{
					return cont_.begin();
				}

				iterator end()
				{
					return cont_.end();
				}

				const_iterator begin() const
				{
					return cont_.cbegin();
				}

				const_iterator end() const
				{
					return cont_.cend();
				}
			private:
				std::vector<item_type*> cont_;
			};

			class item_renderer
			{
			public:
				enum class state_t{normal, highlighted, selected};
				const static unsigned extra_size = 6;

				item_renderer(nana::paint::graphics& graph, bool textout, unsigned scale, const ::nana::color& bgcolor)
					:graph(graph), textout(textout), scale(scale), bgcolor(bgcolor)
				{}

				void operator()(int x, int y, unsigned width, unsigned height, item_type& item, state_t state)
				{
					//draw background
					if(state != state_t::normal)
						graph.rectangle({ x, y, width, height }, false, { 0x33, 0x99, 0xFF });
					switch(state)
					{
					case state_t::highlighted:
						graph.gradual_rectangle({ x + 1, y + 1, width - 2, height - 2 }, bgcolor, { 0xC0, 0xDD, 0xFC }, true);
						break;
					case state_t::selected:
						graph.gradual_rectangle({ x + 1, y + 1, width - 2, height - 2 }, bgcolor, { 0x99, 0xCC, 0xFF }, true);
					default:	break;
					}

					if(item.image.empty() == false)
					{
						nana::size size = item.image.size();
						if(size.width > scale) size.width = scale;
						if(size.height > scale) size.height = scale;

						nana::point pos(x, y);
						pos.x += static_cast<int>(scale + extra_size - size.width) / 2;
						pos.y += static_cast<int>(height - size.height) / 2;

						item.image.paste(size, graph, pos);
						if(item.enable == false)
						{
							nana::paint::graphics gh(size);
							gh.bitblt(size, graph, pos);
							gh.rgb_to_wb();
							gh.paste(graph, pos.x, pos.y);
						}
						else if(state == state_t::normal)
							graph.blend(nana::rectangle(pos, size), ::nana::color(0xc0, 0xdd, 0xfc).blend(bgcolor, 0.5), 0.25);

						x += scale;
						width -= scale;
					}

					if(textout)
					{
						graph.string({ x + static_cast<int>(width - item.textsize.width) / 2, y + static_cast<int>(height - item.textsize.height) / 2 }, item.text);
					}
				}

			protected:
				nana::paint::graphics& graph;
				bool textout;
				unsigned scale;
				::nana::color bgcolor;
			};

			struct drawer::drawer_impl_type
			{
				event_handle event_size{nullptr};
				unsigned scale{16};
				bool textout{false};
				size_type which{npos};
				item_renderer::state_t state{item_renderer::state_t::normal};

				container cont;
				::nana::tooltip tooltip;
			};

			//class drawer
				drawer::drawer()
					: impl_(new drawer_impl_type)
				{
				}

				drawer::~drawer()
				{
					delete impl_;
				}

				void drawer::append(const nana::string& text, const nana::paint::image& img)
				{
					impl_->cont.push_back(text, img);
				}

				void drawer::append()
				{
					impl_->cont.push_back();
				}

				bool drawer::enable(drawer::size_type n) const
				{
					if(impl_->cont.size() > n)
					{
						auto item = impl_->cont.at(n);
						return (item && item->enable);
					}
					return false;
				}

				bool drawer::enable(size_type n, bool eb)
				{
					if(impl_->cont.size() > n)
					{
						item_type * item = impl_->cont.at(n);
						if(item && (item->enable != eb))
						{
							item->enable = eb;
							return true;
						}
					}
					return false;
				}

				void drawer::scale(unsigned s)
				{
					impl_->scale = s;

					for(auto m : impl_->cont)
						_m_fill_pixels(m, true);
				}

				void drawer::refresh(graph_reference)
				{
					_m_draw();
				}

				void drawer::attached(widget_reference widget, graph_reference graph)
				{
					graph_ = &graph;

					widget_ = static_cast< ::nana::toolbar*>(&widget);
					widget.caption(STR("Nana Toolbar"));
					impl_->event_size = widget.events().resized.connect(std::bind(&drawer::_m_owner_sized, this, std::placeholders::_1));

				}

				void drawer::detached()
				{
					API::umake_event(impl_->event_size);
					impl_->event_size = nullptr;
				}

				void drawer::mouse_move(graph_reference graph, const arg_mouse& arg)
				{
					if(arg.left_button == false)
					{
						size_type which = _m_which(arg.pos.x, arg.pos.y, true);
						if(impl_->which != which)
						{
							if (impl_->which != npos && impl_->cont.at(impl_->which)->enable)
							{
								::nana::arg_toolbar arg{ *widget_, impl_->which };
								widget_->events().leave.emit(arg);
							}

							impl_->which = which;
							if(which == npos || impl_->cont.at(which)->enable)
							{
								impl_->state = (arg.left_button ? item_renderer::state_t::selected : item_renderer::state_t::highlighted);

								_m_draw();
								API::lazy_refresh();

								if (impl_->state == item_renderer::state_t::highlighted)
								{
									::nana::arg_toolbar arg{ *widget_, which };
									widget_->events().enter.emit(arg);
								}
							}

							if(which != npos)
								impl_->tooltip.show(widget_->handle(), nana::point(arg.pos.x, arg.pos.y + 20), (*(impl_->cont.begin() + which))->text, 0);
							else
								impl_->tooltip.close();
						}
					}
				}

				void drawer::mouse_leave(graph_reference, const arg_mouse&)
				{
					if(impl_->which != npos)
					{
						size_type which = impl_->which;

						impl_->which = npos;
						_m_draw();
						API::lazy_refresh();

						if (which != npos && impl_->cont.at(which)->enable)
						{
							::nana::arg_toolbar arg{ *widget_, which };
							widget_->events().leave.emit(arg);
						}
					}
					impl_->tooltip.close();
				}

				void drawer::mouse_down(graph_reference, const arg_mouse&)
				{
					impl_->tooltip.close();
					if(impl_->which != npos && (impl_->cont.at(impl_->which)->enable))
					{
						impl_->state = item_renderer::state_t::selected;
						_m_draw();
						API::lazy_refresh();
					}
				}

				void drawer::mouse_up(graph_reference, const arg_mouse& arg)
				{
					if(impl_->which != npos)
					{
						size_type which = _m_which(arg.pos.x, arg.pos.y, false);
						if(impl_->which == which)
						{
							::nana::arg_toolbar arg{ *widget_, which };
							widget_->events().selected.emit(arg);

							impl_->state = item_renderer::state_t::highlighted;
						}
						else
						{
							impl_->which = which;
							impl_->state = (which == npos ? item_renderer::state_t::normal : item_renderer::state_t::highlighted);
						}

						_m_draw();
						API::lazy_refresh();
					}
				}

				drawer::size_type drawer::_m_which(int x, int y, bool want_if_disabled) const
				{
					if(x < 2 || y < 2 || y >= static_cast<int>(impl_->scale + item_renderer::extra_size + 2)) return npos;

					x -= 2;

					size_type pos = 0;
					for(auto m: impl_->cont)
					{
						bool compart = (nullptr == m);

						if(x < static_cast<int>(compart ? 3 : m->pixels))
							return ((compart || (m->enable == false && want_if_disabled == false)) ? npos : pos);

						x -= (compart ? 3 : m->pixels);

						++pos;
					}
					return npos;
				}

				void drawer::_m_draw_background(const ::nana::color& clr)
				{
					graph_->gradual_rectangle(graph_->size(), clr.blend(colors::white, 0.9), clr.blend(colors::black, 0.95), true);
				}

				void drawer::_m_draw()
				{
					int x = 2, y = 2;

					auto bgcolor = API::bgcolor(widget_->handle());
					graph_->set_text_color(bgcolor);
					_m_draw_background(bgcolor);

					item_renderer ir(*graph_, impl_->textout, impl_->scale, bgcolor);
					size_type index = 0;

					for(auto item : impl_->cont)
					{
						if(item)
						{
							_m_fill_pixels(item, false);
							ir(x, y, item->pixels, impl_->scale + ir.extra_size, *item, (index == impl_->which ? impl_->state : item_renderer::state_t::normal));
							x += item->pixels;
						}
						else
						{
							graph_->line({ x + 2, y + 2 }, { x + 2, y + static_cast<int>(impl_->scale + ir.extra_size) - 4 }, { 0x80, 0x80, 0x80 });
							x += 6;
						}
						++index;
					}
				}

				void drawer::_m_owner_sized(const arg_resized& arg)
				{
					auto wd = widget_->handle();
					API::window_size(wd, nana::size(arg.width, widget_->size().height));
					_m_draw();
					API::update_window(wd);
				}

				void drawer::_m_fill_pixels(item_type* item, bool force)
				{
					if(item && (force || (0 == item->pixels)))
					{
						if(item->text.size())
							item->textsize = graph_->text_extent_size(item->text);

						if(item->image.empty() == false)
							item->pixels = impl_->scale + item_renderer::extra_size;

						if(item->textsize.width && impl_->textout)
							item->pixels += item->textsize.width + 8;
					}
				}
			//};//class drawer

		}//end namespace toolbar
	}//end namespace drawerbase

	//class toolbar
		toolbar::toolbar(window wd, bool visible)
		{
			create(wd, rectangle(), visible);
		}

		toolbar::toolbar(window wd, const rectangle& r, bool visible)
		{
			create(wd, r, visible);
		}

		void toolbar::append()
		{
			get_drawer_trigger().append();
			API::refresh_window(handle());
		}

		void toolbar::append(const nana::string& text, const nana::paint::image& img)
		{
			get_drawer_trigger().append(text, img);
			API::refresh_window(handle());
		}

		void toolbar::append(const nana::string& text)
		{
			get_drawer_trigger().append(text, nana::paint::image());
			API::refresh_window(this->handle());
		}

		bool toolbar::enable(size_type n) const
		{
			return get_drawer_trigger().enable(n);
		}

		void toolbar::enable(size_type n, bool eb)
		{
			if(get_drawer_trigger().enable(n, eb))
				API::refresh_window(this->handle());
		}

		void toolbar::scale(unsigned s)
		{
			get_drawer_trigger().scale(s);
			API::refresh_window(handle());
		}
	//}; class toolbar
}//end namespace nana
