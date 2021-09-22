/*
 *	A Toolbar Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2021 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/toolbar.cpp
 *	@contributors:
 *		kmribti(pr#105)
 */

#include <nana/gui/compact.hpp>
#include <nana/gui/widgets/toolbar.hpp>
#include <nana/gui/tooltip.hpp>
#include <nana/gui/element.hpp>
#include <nana/gui/widgets/float_listbox.hpp>
#include <nana/gui/layout_utility.hpp>
#include <nana/paint/text_renderer.hpp>

#include <vector>

// some of these should go into the control scheme (still missing)
const unsigned int TOOLS_HEIGHT = 16;
const unsigned int SEPARATOR_WIDTH = 8;
const unsigned int DROPDOWN_WIDTH = 13;
const unsigned int DROPDOWN_LIST_EXTRA = 60;


namespace nana
{
	arg_toolbar::arg_toolbar(toolbar& tbar, std::size_t btn)
		: widget(tbar), button{btn}
	{}

	namespace drawerbase::toolbar
	{
		struct dropdown_item
			: public float_listbox::item_interface
		{
		public:
			bool				enable_{ true };
			std::string			text_;
			nana::paint::image	image_;
			event_fn_t			event_handler_;

			dropdown_item(const std::string& txt, const nana::paint::image& img, const event_fn_t& handler)
				: text_(txt), image_(img), event_handler_(handler)
			{}

			//implement item_interface methods
			const nana::paint::image& image() const override
			{
				return image_;
			}

			const char* text() const override
			{
				return text_.data();
			}

			// own methods
			bool enable() const
			{
				return enable_;
			}
		};


		// dropdown item custom renderer
		class dropdown_item_renderer
			: public nana::float_listbox::item_renderer
		{
			unsigned image_pixels_{ TOOLS_HEIGHT };
			unsigned item_h_{ 24 };
			nana::color bg_{ colors::white };

			void image(bool /*enbable*/, unsigned px)
			{
				// always show the space for the image
				image_pixels_ = px;
			}

			void background(widget_reference, graph_reference graph)
			{
				graph.rectangle(false, colors::gray_border);
				graph.rectangle(nana::rectangle(graph.size()).pare_off(1), false, bg_);
			}

			void item(widget_reference, graph_reference graph, const nana::rectangle& r, const item_interface* item, state_t state)
			{
				graph.rectangle(r, true, bg_);

				const dropdown_item* dd_item = (dropdown_item*)item;

				if(state == StateHighlighted && dd_item->enable())
				{
					graph.rectangle(r, false, static_cast<color_rgb>(0xa8d8eb));

					graph.palette(false, static_cast<color_rgb>(0xc0ddfc));
					paint::draw(graph).corner(r, 1);

					graph.gradual_rectangle(nana::rectangle(r).pare_off(1), static_cast<color_rgb>(0xE8F0F4), static_cast<color_rgb>(0xDBECF4), true);
				}

				int x = r.x + 4;

				// image
				if(dd_item->image())
				{
					auto imgsz = dd_item->image().size();
					if(imgsz.width > image_pixels_ || imgsz.height > image_pixels_)
					{
						imgsz = nana::fit_zoom(imgsz, { image_pixels_, image_pixels_ });
					}

					nana::point to_pos(x, r.y);
					to_pos.x += (image_pixels_ - imgsz.width) / 2;
					to_pos.y += (r.height - imgsz.height) / 2;
					dd_item->image().stretch(::nana::rectangle{ dd_item->image().size() }, graph, nana::rectangle(to_pos, imgsz));
				}
				x += (image_pixels_ + 8);

				// text
				int text_top_off = static_cast<int>(r.height - graph.text_extent_size(L"jh({[").height) / 2;
				graph.string({ x, r.y + text_top_off }, dd_item->text(), dd_item->enable() ? colors::black : colors::gray_border);
			}

			unsigned item_pixels(graph_reference graph) const
			{
				unsigned ascent, descent, ileading;
				graph.text_metrics(ascent, descent, ileading);
				return std::max(item_h_ , ascent + descent + 4);
			}


		public:
			unsigned image_pixels() { return image_pixels_; }
			void image_pixels(unsigned px) { image_pixels_ = px; }

			void item_height(unsigned px) { item_h_ = px; }

			void bgcolor(const nana::color& bg) { bg_ = bg; }
		};


		struct toolbar_item
		{
			// common
			std::string			text;
			nana::paint::image	image;
			bool				enable{ true };
			bool				textout{ false };
			tools				type{ tools::button };

			// tools::toggle
			bool				toggle{ false };
			std::string			toggle_group;

			// tools::dropdown
			std::vector<std::shared_ptr<dropdown_item>> dropdown_items;
			nana::float_listbox::module_type dropdown_module;
			nana::float_listbox* dropdown_lister{ nullptr };

			event_fn_t			event_handler;

			// internal use
			unsigned			pixels{ 0 };
			nana::point			position{ 0, 0 };
			nana::size			textsize;

			toolbar_item(tools type, const std::string& text, const nana::paint::image& img, const event_fn_t& fn)
				: text(text), image(img), type(type), event_handler(fn)
			{}
		};


		class item_container
		{
		public:
			using container_type = std::vector<toolbar_item*>;
			using size_type = container_type::size_type;

			~item_container()
			{
				clear();
			}

			void insert(size_type pos, tools type, std::string text, const nana::paint::image& img, const event_fn_t& fn)
			{
				toolbar_item* m = new toolbar_item(type, std::move(text), img, fn);

				if(pos < cont_.size())
					cont_.insert(cont_.begin() + pos, m);
				else
					cont_.push_back(m);
			}

			void push_back(tools type, const std::string& text, const nana::paint::image& img, const event_fn_t& fn)
			{
				insert(cont_.size(), type, text, img, fn);
			}

			void push_back_separator()
			{
				cont_.push_back(nullptr);
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

			size_type size() const noexcept
			{
				return cont_.size();
			}

			container_type& container() noexcept
			{
				return cont_;
			}

			toolbar_item* at(size_type pos)
			{
				return cont_.at(pos);
			}

			toolbar_item* back()
			{
				return cont_.back();
			}

			void clear()
			{
				for(auto ptr : cont_)
					delete ptr;

				cont_.clear();
				right_ = npos;
			}


			void update_toggle_group(toolbar_item* item, bool toggle_state, bool clicked = true)
			{
				if(!item)
					return;

				if(item->toggle_group.empty())
				{
					item->toggle = toggle_state;
					return;
				}

				// group rules:
				//		1. inside a group only one item at the time is selected
				//		2. inside a group one item must always be selected
				//		3. a group with only one item IS NOT a group

				bool is_group = false;

				// look for other items inside the group
				for(auto i : cont_)
				{
					if(i == item)
						continue;

					if(i && i->toggle_group == item->toggle_group)
					{
						if(toggle_state == false && clicked == false) // needs to avoid to break rule no. 2
							return;

						is_group = true;
						i->toggle = false;
					}
				}

				item->toggle = is_group ? true : toggle_state;
			}


		private:
			container_type cont_;
			size_t right_{ npos };
		};

		class item_renderer
		{
		public:
			enum class state_t{normal, highlighted, selected};
			const static unsigned extra_size = 6;

			item_renderer(nana::paint::graphics& graph, unsigned scale, const ::nana::color& bgcolor, const ::nana::color& fgcolor)
				:graph(graph), scale(scale), bgcolor(bgcolor), fgcolor(fgcolor)
			{}

			void operator()(int x, int y, unsigned width, unsigned height, toolbar_item& item, state_t state)
			{
				//draw background
				if(state != state_t::normal)
				{
					nana::rectangle background_r(x, y, width, height);
					graph.rectangle(background_r, false, static_cast<color_rgb>(0x3399FF));

					if (state_t::highlighted == state || state_t::selected == state)
						graph.gradual_rectangle(background_r.pare_off(1), bgcolor, static_cast<color_rgb>(state_t::selected == state ? 0x99CCFF : 0xC0DDFC), true);
				}
				else if((item.type == tools::toggle && item.toggle) || (item.type == tools::dropdown && item.dropdown_lister))
				{
					nana::rectangle background_r(x, y, width, height);
					graph.rectangle(background_r, false, static_cast<color_rgb>(item.enable ? 0x3399FF : 0x999999));

					graph.gradual_rectangle(background_r.pare_off(1), bgcolor, static_cast<color_rgb>(item.enable ? 0xC0DDFC : 0x969696), true);
				}

				if(!item.image.empty())
				{
					auto imgsize = nana::fit_zoom(item.image.size(), { scale, scale });
					nana::point pos{ x + static_cast<int>(scale + extra_size - imgsize.width) / 2, y + static_cast<int>(height - imgsize.height) / 2 };

					item.image.stretch(rectangle{ item.image.size() }, graph, rectangle{ pos, imgsize });
					
					if(item.enable == false)
					{
						nana::paint::graphics gh(imgsize);
						gh.bitblt(::nana::rectangle{ imgsize }, graph, pos);
						gh.rgb_to_wb(true);
						gh.paste(graph, pos.x, pos.y);
					}
					else if(state == state_t::normal)
					{
						graph.blend(nana::rectangle(pos, imgsize), ::nana::color(0xc0, 0xdd, 0xfc).blend(bgcolor, 0.5), 0.25);
					}
				}

				if(item.textout)
				{
					int txtx = x;
					if(item.image.empty())
						txtx += item_renderer::extra_size / 2;
					else
						txtx += scale + item_renderer::extra_size;

					graph.string({ txtx, y + static_cast<int>(height - item.textsize.height) / 2 }, item.text, fgcolor);
				}

				if(item.type == tools::dropdown)
				{
					int ddx = x + width - DROPDOWN_WIDTH;
					facade<element::arrow> arrow("solid_triangle");
					arrow.direction(direction::south);
					arrow.draw(graph, bgcolor, fgcolor, { ddx-1, static_cast<int>(y + height/4), 0, 0 }, element_state::normal);

					if(state != state_t::normal && ddx != x)
						graph.line({ ddx, y + 1 }, { ddx, y + static_cast<int>(scale + extra_size) - 1 }, static_cast<color_rgb>(0x808080));
				}
			}

		protected:
			nana::paint::graphics& graph;
			unsigned scale;
			::nana::color bgcolor;
			::nana::color fgcolor;
		};

		struct drawer::drawer_impl_type
		{
			paint::graphics* graph_ptr{ nullptr };

			unsigned scale{ TOOLS_HEIGHT };
			size_type which{npos};
			item_renderer::state_t state{item_renderer::state_t::normal};

			item_container	items;
			::nana::tooltip tooltip;

			size_type dropdown_which{ npos };
			dropdown_item_renderer dropdown_renderer;
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

				auto bgcolor = api::bgcolor(widget_->handle());
				auto fgcolor = api::fgcolor(widget_->handle());
				graph.palette(true, bgcolor);
				graph.gradual_rectangle(rectangle{ graph.size() }, bgcolor.blend(colors::white, 0.1), bgcolor.blend(colors::black, 0.05), true);

				item_renderer ir(graph, impl_->scale, bgcolor, fgcolor);
				size_type index = 0;

				for (auto item : impl_->items.container())
				{
					if (item)
					{
						_m_calc_pixels(item, false);
						item->position.x = x;
						item->position.y = y;
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
								total_x += SEPARATOR_WIDTH;
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

				widget_ = static_cast<::nana::toolbar*>(&widget);
				widget.caption("nana toolbar");
			}

			void drawer::detached()
			{
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
						widget_->events().leave.emit(arg, widget_->handle());
					}

					impl_->which = which;
					if (which == npos || container.at(which)->enable)
					{
						impl_->state = item_renderer::state_t::highlighted;

						refresh(graph);
						api::dev::lazy_refresh();

						if (impl_->state == item_renderer::state_t::highlighted)
						{
							::nana::arg_toolbar arg{ *widget_, which };
							widget_->events().enter.emit(arg, widget_->handle());
						}
					}

					if(which != npos)
					{
						if(!(*(container.begin() + which))->text.empty())
							impl_->tooltip.show(widget_->handle(), nana::point(arg.pos.x, arg.pos.y + 20), (*(container.begin() + which))->text, 0);
					}
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
					api::dev::lazy_refresh();

					if (which != npos && impl_->items.at(which)->enable)
					{
						::nana::arg_toolbar arg{ *widget_, which };
						widget_->events().leave.emit(arg, widget_->handle());
					}
				}
				impl_->tooltip.close();
			}

			void drawer::mouse_down(graph_reference graph, const arg_mouse&)
			{
				impl_->tooltip.close();
				if(impl_->which != npos)
				{
					auto item = impl_->items.at(impl_->which);
					if(item->enable)
					{
						if(item->type == tools::dropdown)
						{
							if((nullptr == item->dropdown_lister) && !item->dropdown_items.empty())
							{
								// calc float_listbox width
								unsigned fl_w = 0;
								for(auto& i : item->dropdown_items)
								{
									nana::size item_size = graph.text_extent_size(i->text());
									if(item_size.width > fl_w)
										fl_w = item_size.width;
								}
								fl_w += DROPDOWN_LIST_EXTRA;

								// calc float_listbox position and size
								nana::point dd_pos(item->position.x + 1, std::min(item->position.y + impl_->scale + item_renderer::extra_size - 1, widget_->size().height));
								nana::size dd_size(std::max(item->pixels, fl_w), 1);

								impl_->dropdown_which = impl_->which;

								item->dropdown_module.items.clear();
								item->dropdown_module.index = nana::npos;
								std::copy(item->dropdown_items.cbegin(), item->dropdown_items.cend(), std::back_inserter(item->dropdown_module.items));
								item->dropdown_lister = &form_loader<nana::float_listbox, false>()(widget_->handle(), nana::rectangle(dd_pos, dd_size), true);
								impl_->dropdown_renderer.bgcolor(widget_->bgcolor());
								impl_->dropdown_renderer.image_pixels(impl_->scale);
								item->dropdown_lister->renderer(&impl_->dropdown_renderer);
								item->dropdown_lister->deselect_on_mouse_leave(true);
								item->dropdown_lister->set_module(item->dropdown_module, impl_->dropdown_renderer.image_pixels());

								//The lister window closes by itself. I just take care about the destroy event.
								item->dropdown_lister->events().destroy.connect_unignorable([this](const arg_destroy&)
									{
										auto item = impl_->items.at(impl_->dropdown_which);
										item->dropdown_lister = nullptr;	//The lister closes by itself.

										if(item->dropdown_module.index != nana::npos)
										{
											auto dd_item = item->dropdown_items.at(item->dropdown_module.index);
											if(dd_item->event_handler_)
											{
												item_proxy ip{ item, &impl_->items, widget_ };
												dd_item->event_handler_.operator()(ip);
											}
										}

										api::refresh_window(*widget_);
									});
							}
						}

						impl_->state = item_renderer::state_t::selected;
						refresh(graph);
						api::dev::lazy_refresh();
					}
				}
			}

			void drawer::mouse_up(graph_reference graph, const arg_mouse& arg)
			{
				if(impl_->which != npos)
				{
					size_type which = _m_which(arg.pos, false);
					if(impl_->which == which)
					{
						auto item = impl_->items.at(impl_->which);
							
						// update toggle state
						if(item->type == tools::toggle)
							impl_->items.update_toggle_group(item, !item->toggle);

						nana::arg_toolbar arg_tb{ *widget_, which };
						widget_->events().selected.emit(arg_tb, widget_->handle());
							
						if(item->event_handler)
						{
							item_proxy ip{ item, &impl_->items, widget_ };
							item->event_handler.operator()(ip);
						}

						impl_->state = item_renderer::state_t::highlighted;
					}
					else
					{
						impl_->which = which;
						impl_->state = (which == npos ? item_renderer::state_t::normal : item_renderer::state_t::highlighted);
					}

					refresh(graph);
					api::dev::lazy_refresh();
				}
			}

			drawer::size_type drawer::_m_which(point pos, bool want_if_disabled) const
			{
				if (pos.x < 2 || pos.y < 2 || pos.y >= static_cast<int>(impl_->scale + item_renderer::extra_size + 2)) return npos;

				pos.x -= 2;

				std::size_t index = 0;
				for(auto m: impl_->items.container())
				{
					if (m && (pos.x >= m->position.x) && (pos.x <= m->position.x + int(m->pixels)))
						return (((!m) || (!m->enable && !want_if_disabled)) ? npos : index);

					++index;
				}
				return npos;
			}

			void drawer::_m_calc_pixels(toolbar_item* item, bool force)
			{
				if(item && (force || (0 == item->pixels)))
				{
					item->pixels = 0;

					if(!item->image.empty())
						item->pixels += impl_->scale + item_renderer::extra_size;

					if(item->textout)
					{
						item->textsize = { 0, 0 };
						if(item->text.size())
							item->textsize = impl_->graph_ptr->text_extent_size(item->text);

						if(item->textsize.width)
						{
							item->pixels += item->textsize.width;
							if(item->image.empty())
								item->pixels += item_renderer::extra_size;
							else
								item->pixels += item_renderer::extra_size / 2;
						}
					}

					if(item->type == tools::dropdown)
						item->pixels += DROPDOWN_WIDTH;

					if(item->pixels == 0)
						item->pixels = impl_->scale + item_renderer::extra_size;
				}
			}
		//class drawer

		// Item Proxy
#if 1 //deprecated
			item_proxy& item_proxy::tooltype(tool_type type)
			{
				return this->type(static_cast<tools>(type));
			}

			bool item_proxy::istoggle() const
			{
				return item_->type == tools::toggle;
			}
#endif

			item_proxy::item_proxy(toolbar_item* item, item_container* cont, nana::toolbar* tb)
				: item_{ item }, cont_{ cont }, tb_{ tb }
			{}

			bool item_proxy::empty() const
			{
				return item_ ? false : true;
			}

			bool item_proxy::enable() const
			{
				return (item_ && item_->enable);
			}

			item_proxy& item_proxy::enable(bool enable_state)
			{
				if(item_ && item_->enable != enable_state)
				{
					item_->enable = enable_state;
					api::refresh_window(tb_->handle());
				}
				return *this;
			}

			tools item_proxy::type() const
			{
				if(item_)
					return item_->type;
				return tools::separator;
			}

			item_proxy& item_proxy::type(tools type)
			{
				if(item_ && item_->type != type)
				{
					item_->type = type;
					api::refresh_window(tb_->handle());
				}
				return *this;
			}

			item_proxy& item_proxy::textout(bool show)
			{
				if(item_ && item_->textout != show)
				{
					item_->textout = show;
					item_->pixels = 0; //force width calculation
					api::refresh_window(tb_->handle());
				}
				return *this;
			}

			item_proxy& item_proxy::answerer(const event_fn_t& handler)
			{
				if(item_)
					item_->event_handler = handler;
				return *this;
			}

			bool item_proxy::toggle() const
			{
				return (item_ && item_->toggle);
			}

			item_proxy& item_proxy::toggle(bool toggle_state)
			{
				if(item_ && item_->toggle != toggle_state)
				{
					item_->toggle = toggle_state;
					cont_->update_toggle_group(item_, toggle_state, false);
					api::refresh_window(tb_->handle());
				}
				return *this;
			}

			std::string item_proxy::toggle_group() const
			{
				if(item_)
					return item_->toggle_group;
				return {};
			}

			item_proxy& item_proxy::toggle_group(const ::std::string& group)
			{
				if(item_ && item_->toggle_group != group)
				{
					item_->toggle_group = group;
					cont_->update_toggle_group(item_, item_->toggle, false);
					api::refresh_window(tb_->handle());
				}
				return *this;
			}
			item_proxy& item_proxy::dropdown_append(const std::string& text, const nana::paint::image& img, const event_fn_t& handler)
			{
				item_->dropdown_items.emplace_back(std::make_shared<dropdown_item>(text, img, handler));
				return *this;
			}

			item_proxy& item_proxy::dropdown_append(const std::string& text, const event_fn_t& handler)
			{
				return dropdown_append(text, {}, handler);
			}

			bool item_proxy::dropdown_enable(std::size_t index) const
			{
				if(index >= item_->dropdown_items.size())
					return false;

				return item_->dropdown_items.at(index)->enable_;
			}

			item_proxy& item_proxy::dropdown_enable(std::size_t index, bool enable_state)
			{
				if(index < item_->dropdown_items.size())
					item_->dropdown_items.at(index)->enable_ = enable_state;
				return *this;
			}

			item_proxy& item_proxy::dropdown_answerer(std::size_t index, const event_fn_t& handler)
			{
				if(index < item_->dropdown_items.size())
					item_->dropdown_items.at(index)->event_handler_ = handler;
				return *this;
			}

	}//end namespace drawerbase

	//class toolbar
#if 1 //deprecated
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

		void toolbar::separate()
		{
			append_separator();
		}

		drawerbase::toolbar::item_proxy toolbar::append(const std::string& text, const nana::paint::image& img)
		{
			return append(tools::button, text, img);
		}

		drawerbase::toolbar::item_proxy toolbar::append(const std::string& text)
		{
			return append(tools::button, text);
		}
#endif

		toolbar::item_proxy toolbar::append(tools t, const std::string& text, const nana::paint::image& img, const toolbar::event_fn_t& handler)
		{
			get_drawer_trigger().items().push_back(t, text, img, handler);
			api::refresh_window(handle());
			return { get_drawer_trigger().items().back(), &get_drawer_trigger().items(), this };
		}

		toolbar::item_proxy toolbar::append(tools t, const std::string& text, const toolbar::event_fn_t& handler)
		{
			return append(t, text, {}, handler);
		}

		void toolbar::append_separator()
		{
			get_drawer_trigger().items().push_back_separator();
			api::refresh_window(handle());
		}

		toolbar::size_type toolbar::count() const noexcept
		{
			return get_drawer_trigger().items().size();
		}

		toolbar::item_proxy toolbar::at(size_type pos)
		{
			if(count() < pos)
				return {};
			return { get_drawer_trigger().items().at(pos), &get_drawer_trigger().items(), this };
		}

		void toolbar::clear()
		{
			get_drawer_trigger().items().clear();
			api::refresh_window(this->handle());
		}

		bool toolbar::enable(size_type pos) const
		{
			auto & items = get_drawer_trigger().items();
			if(items.size() <= pos)
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
					api::refresh_window(this->handle());
				}
			}
		}

#if 1 //deprecated
		void toolbar::tooltype(size_type index, tool_type type)
		{
			at(index).type(static_cast<tools>(type));
		}

		bool toolbar::istoggle(size_type index) const
		{
			auto & items = get_drawer_trigger().items();
			if(items.size() <= index)
				return false;

			auto m = items.at(index);
			return (m && m->type == tools::toggle);
		}

		bool toolbar::toggle(size_type index) const
		{
			auto & items = get_drawer_trigger().items();
			if(items.size() <= index)
				return false;

			auto m = items.at(index);
			return (m && m->toggle);
		}

		void toolbar::toggle(size_type index, bool toggle_state)
		{
			at(index).toggle(toggle_state);
		}

		std::string toolbar::toggle_group(size_type index) const
		{
			auto & items = get_drawer_trigger().items();
			if(items.size() <= index)
				return "";

			auto m = items.at(index);
			return m ? m->toggle_group : "";
		}

		void toolbar::toggle_group(size_type index, const ::std::string& group)
		{
			at(index).toggle_group(group);
		}

		void toolbar::textout(size_type index, bool show)
		{
			at(index).textout(show);
		}

		void toolbar::scale(unsigned s)
		{
			tools_height(s);
		}
#endif
		void toolbar::tools_height(unsigned h)
		{
			get_drawer_trigger().scale(h);
			api::refresh_window(this->handle());
		}

		//Contributed by kmribti(pr#105)
		void toolbar::go_right()
		{
			get_drawer_trigger().items().go_right();
		}
	//end class toolbar
}//end namespace nana
