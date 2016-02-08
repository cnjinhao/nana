/*
 *	A Toolbar Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/toolbar.cpp
 *	@contributors:
 *		kmribti(pr#105)
 */

#include <nana/gui/widgets/toolbar.hpp>
#include <nana/gui/tooltip.hpp>

#include <vector>

namespace nana
{
	arg_toolbar::arg_toolbar(toolbar& tbar, std::size_t btn)
		: widget(tbar), button{btn}
	{}

	namespace drawerbase
	{
		namespace toolbar
		{
			struct item_type
			{
				enum kind{ button, container};

				typedef std::size_t size_type;

				std::string text;
				nana::paint::image image;
				unsigned	pixels{0};
				unsigned    position{ 0 }; // last item position.
				nana::size	textsize;
				bool		enable{true};

				kind type;

				item_type(const std::string& text, const nana::paint::image& img, kind type)
					:text(text), image(img), type(type)
				{}
			};



			class item_container
			{
			public:
				using container_type = std::vector<item_type*>;
				using size_type = container_type::size_type;

				~item_container()
				{
					for(auto ptr : cont_)
						delete ptr;
				}

				void insert(size_type pos, std::string text, const nana::paint::image& img, item_type::kind type)
				{
					item_type* m = new item_type(std::move(text), img, type);

					if(pos < cont_.size())
						cont_.insert(cont_.begin() + pos, m);
					else
						cont_.push_back(m);
				}

				void push_back(const std::string& text, const nana::paint::image& img)
				{
					insert(cont_.size(), text, img, item_type::kind::button);
				}

				void push_back(const std::string& text)
				{
					insert(cont_.size(), text, nana::paint::image(), item_type::kind::button);
				}

				//Contributed by kmribti(pr#105)
				void go_right() noexcept
				{
					right_ = cont_.size();
				}

				//Contributed by kmribti(pr#105)
				size_t right() const noexcept
				{
					return right_;
				}

				void insert(size_type pos)
				{
					if(pos < cont_.size())
						cont_.insert(cont_.begin() + pos, static_cast<item_type*>(nullptr)); //both works in C++0x and C++2003
					else
						cont_.push_back(nullptr);
				}

				void separate()
				{
					cont_.push_back(nullptr);
				}

				size_type size() const noexcept
				{
					return cont_.size();
				}

				container_type& container() noexcept
				{
					return cont_;
				}

				item_type * at(size_type pos)
				{
					return cont_.at(pos);
				}
			private:
				container_type cont_;
				size_t    right_{ npos };
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
					if (state != state_t::normal)
					{
						nana::rectangle background_r(x, y, width, height);
						graph.rectangle(background_r, false, static_cast<color_rgb>(0x3399FF));

						if (state_t::highlighted == state || state_t::selected == state)
							graph.gradual_rectangle(background_r.pare_off(1), bgcolor, static_cast<color_rgb>(state_t::selected == state ? 0x99CCFF : 0xC0DDFC), true);
					}

					if(!item.image.empty())
					{
						auto imgsize = item.image.size();

						if (imgsize.width > scale) imgsize.width = scale;
						if (imgsize.height > scale) imgsize.height = scale;

						nana::point pos(
							x + static_cast<int>(scale + extra_size - imgsize.width) / 2, 
							y + static_cast<int>(height - imgsize.height) / 2);

						item.image.paste(::nana::rectangle{ imgsize }, graph, pos);
						if(item.enable == false)
						{
							nana::paint::graphics gh(imgsize);
							gh.bitblt(::nana::rectangle{ imgsize }, graph, pos);
							gh.rgb_to_wb();
							gh.paste(graph, pos.x, pos.y);
						}
						else if(state == state_t::normal)
							graph.blend(nana::rectangle(pos, imgsize), ::nana::color(0xc0, 0xdd, 0xfc).blend(bgcolor, 0.5), 0.25);

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
				event_handle event_size{ nullptr };
				paint::graphics* graph_ptr{ nullptr };

				unsigned scale{16};
				bool textout{false};
				size_type which{npos};
				item_renderer::state_t state{item_renderer::state_t::normal};

				item_container	items;
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

				item_container& drawer::items() const
				{
					return impl_->items;
				}

				void drawer::scale(unsigned s)
				{
					impl_->scale = s;

					for(auto m : impl_->items.container())
						_m_calc_pixels(m, true);
				}

				void drawer::refresh(graph_reference graph)
				{
					int x = 2, y = 2;

					auto bgcolor = API::bgcolor(widget_->handle());
					graph.palette(true, bgcolor);
					graph.gradual_rectangle(rectangle{ graph.size() }, bgcolor.blend(colors::white, 0.9), bgcolor.blend(colors::black, 0.95), true);

					item_renderer ir(graph, impl_->textout, impl_->scale, bgcolor);
					size_type index = 0;

					for (auto item : impl_->items.container())
					{
						if (item)
						{
							_m_calc_pixels(item, false);
							item->position = x;
							ir(x, y, item->pixels, impl_->scale + ir.extra_size, *item, (index == impl_->which ? impl_->state : item_renderer::state_t::normal));
							x += item->pixels;
						}
						else
						{
							x += 2;
							graph.line({ x, y + 2 }, { x, y + static_cast<int>(impl_->scale + ir.extra_size) - 4 }, static_cast<color_rgb>(0x808080));
							x += 4;
						}
						++index;

						//Reset the x position of items which are right aligned
						//Contributed by kmribti(pr#105)
						if (index == impl_->items.right() && index < impl_->items.size())
						{
							unsigned total_x = 0;
							for (size_t i = index; i < impl_->items.size(); i++) {
								if (impl_->items.at(i) == nullptr) {
									total_x += 8; // we assume that separator has width = 8.
								}
								else {
									_m_calc_pixels(impl_->items.at(i), false);
									total_x += impl_->items.at(i)->pixels;
								}
							}

							x = graph.size().width - total_x - 4;
						}
					}
				}

				void drawer::attached(widget_reference widget, graph_reference graph)
				{
					impl_->graph_ptr = &graph;

					widget_ = static_cast< ::nana::toolbar*>(&widget);
					widget.caption("nana toolbar");

					if (widget_->detached()) return;

					impl_->event_size = API::events(widget.parent()).resized.connect_unignorable([this](const arg_resized& arg)
					{
						auto wd = widget_->handle();
						API::window_size(wd, nana::size(arg.width, widget_->size().height));
						API::update_window(wd);
					});
				}

				void drawer::detached()
				{
					API::umake_event(impl_->event_size);
					impl_->event_size = nullptr;
					impl_->graph_ptr = nullptr;
				}

				void drawer::mouse_move(graph_reference graph, const arg_mouse& arg)
				{
					if (arg.left_button)
						return;

					size_type which = _m_which(arg.pos, true);
					if(impl_->which != which)
					{
						auto & container = impl_->items.container();
						if (impl_->which != npos && container.at(impl_->which)->enable)
						{
							::nana::arg_toolbar arg{ *widget_, impl_->which };
							widget_->events().leave.emit(arg);
						}

						impl_->which = which;
						if (which == npos || container.at(which)->enable)
						{
							impl_->state = item_renderer::state_t::highlighted;

							refresh(graph);
							API::lazy_refresh();

							if (impl_->state == item_renderer::state_t::highlighted)
							{
								::nana::arg_toolbar arg{ *widget_, which };
								widget_->events().enter.emit(arg);
							}
						}

						if(which != npos)
							impl_->tooltip.show(widget_->handle(), nana::point(arg.pos.x, arg.pos.y + 20), (*(container.begin() + which))->text, 0);
						else
							impl_->tooltip.close();
					}
				}

				void drawer::mouse_leave(graph_reference graph, const arg_mouse&)
				{
					if(impl_->which != npos)
					{
						size_type which = impl_->which;

						impl_->which = npos;
						refresh(graph);
						API::lazy_refresh();

						if (which != npos && impl_->items.at(which)->enable)
						{
							::nana::arg_toolbar arg{ *widget_, which };
							widget_->events().leave.emit(arg);
						}
					}
					impl_->tooltip.close();
				}

				void drawer::mouse_down(graph_reference graph, const arg_mouse&)
				{
					impl_->tooltip.close();
					if(impl_->which != npos && (impl_->items.at(impl_->which)->enable))
					{
						impl_->state = item_renderer::state_t::selected;
						refresh(graph);
						API::lazy_refresh();
					}
				}

				void drawer::mouse_up(graph_reference graph, const arg_mouse& arg)
				{
					if(impl_->which != npos)
					{
						size_type which = _m_which(arg.pos, false);
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

						refresh(graph);
						API::lazy_refresh();
					}
				}

				drawer::size_type drawer::_m_which(point pos, bool want_if_disabled) const
				{
					if (pos.x < 2 || pos.y < 2 || pos.y >= static_cast<int>(impl_->scale + item_renderer::extra_size + 2)) return npos;

					pos.x -= 2;

					std::size_t index = 0;
					for(auto m: impl_->items.container())
					{
						unsigned x = static_cast<unsigned>(pos.x);
						if (m && x >= m->position && x <= (m->position+m->pixels))
							return (((!m) || (!m->enable && !want_if_disabled)) ? npos : index);

						++index;
					}
					return npos;
				}

				void drawer::_m_calc_pixels(item_type* item, bool force)
				{
					if (item && (force || (0 == item->pixels)))
					{
						if (item->text.size())
							item->textsize = impl_->graph_ptr->text_extent_size(item->text);

						if (item->image.empty() == false)
							item->pixels = impl_->scale + item_renderer::extra_size;

						if (item->textsize.width && impl_->textout)
							item->pixels += item->textsize.width + 8;
					}
				}
			//class drawer
		}//end namespace toolbar
	}//end namespace drawerbase

	//class toolbar
		toolbar::toolbar(window wd, bool visible, bool detached) :
			detached_(detached)
		{
			create(wd, rectangle(), visible);
		}

		toolbar::toolbar(window wd, const rectangle& r, bool visible, bool detached) :
			detached_(detached)
		{
			create(wd, r, visible);
		}

		//Contributed by kmribti(pr#105)
		void toolbar::go_right()
		{
			get_drawer_trigger().items().go_right();
		}

		void toolbar::separate()
		{
			get_drawer_trigger().items().separate();
			API::refresh_window(handle());
		}

		void toolbar::append(const std::string& text, const nana::paint::image& img)
		{
			get_drawer_trigger().items().push_back(text, img);
			API::refresh_window(handle());
		}

		void toolbar::append(const std::string& text)
		{
			get_drawer_trigger().items().push_back(text, {});
			API::refresh_window(this->handle());
		}

		bool toolbar::enable(size_type pos) const
		{
			auto & items = get_drawer_trigger().items();

			if (items.size() <= pos)
				return false;

			auto m = items.at(pos);
			return (m && m->enable);
		}

		void toolbar::enable(size_type pos, bool eb)
		{
			auto & items = get_drawer_trigger().items();

			if (items.size() > pos)
			{
				auto m = items.at(pos);
				if (m && (m->enable != eb))
				{
					m->enable = eb;
					API::refresh_window(this->handle());
				}
			}
		}

		void toolbar::scale(unsigned s)
		{
			get_drawer_trigger().scale(s);
			API::refresh_window(handle());
		}
	//end class toolbar
}//end namespace nana
