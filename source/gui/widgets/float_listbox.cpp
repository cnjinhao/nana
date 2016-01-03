/*
 *	A float_listbox Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/float_listbox.cpp
 */

#include <nana/gui/widgets/float_listbox.hpp>
#include <nana/gui/widgets/scroll.hpp>

namespace nana
{
	namespace drawerbase{
		namespace float_listbox
		{
			class def_item_renderer
				: public item_renderer
			{
				bool image_enabled_;
				unsigned image_pixels_;

				void image(bool enb, unsigned px)
				{
					image_enabled_ = enb;
					image_pixels_ = px;
				}

				void render(widget_reference, graph_reference graph, const nana::rectangle& r, const item_interface* item, state_t state)
				{
					if (state == StateHighlighted)
					{
						::nana::color clr(static_cast<color_rgb>(0xafc7e3));
						graph.rectangle(r, false, clr);

						auto right = r.right() - 1;
						auto bottom = r.bottom() - 1;
						graph.palette(false, colors::white);
						graph.set_pixel(r.x, r.y);
						graph.set_pixel(right, r.y);
						graph.set_pixel(r.x, bottom);
						graph.set_pixel(right, bottom);

						--right;
						--bottom;
						graph.palette(false, clr);
						graph.set_pixel(r.x + 1, r.y + 1);
						graph.set_pixel(right, r.y + 1);
						graph.set_pixel(r.x + 1, bottom);
						graph.set_pixel(right, bottom);

						nana::rectangle po_r(r);
						graph.rectangle(po_r.pare_off(1), false, static_cast<color_rgb>(0xEBF4FB));
						graph.gradual_rectangle(po_r.pare_off(1), static_cast<color_rgb>(0xDDECFD), static_cast<color_rgb>(0xC2DCFD), true);
					}
					else
						graph.rectangle(r, true, colors::white);
					
					int x = r.x + 2;
					if(image_enabled_)
					{
						unsigned vpix = (r.height - 4);
						if(item->image())
						{
							nana::size imgsz = item->image().size();
							if(imgsz.width > image_pixels_)
							{
								unsigned new_h = image_pixels_ * imgsz.height / imgsz.width;
								if(new_h > vpix)
								{
									imgsz.width = vpix * imgsz.width / imgsz.height;
									imgsz.height = vpix;
								}
								else
								{
									imgsz.width = image_pixels_;
									imgsz.height = new_h;
								}
							}
							else if(imgsz.height > vpix)
							{
								unsigned new_w = vpix * imgsz.width / imgsz.height;
								if(new_w > image_pixels_)
								{
									imgsz.height = image_pixels_ * imgsz.height / imgsz.width;
									imgsz.width = image_pixels_;
								}
								else
								{
									imgsz.height = vpix;
									imgsz.width = new_w;
								}
							}

							nana::point to_pos(x, r.y + 2);
							to_pos.x += (image_pixels_ - imgsz.width) / 2;
							to_pos.y += (vpix - imgsz.height) / 2;
							item->image().stretch(::nana::rectangle{ item->image().size() }, graph, nana::rectangle(to_pos, imgsz));
						}
						x += (image_pixels_ + 2);
					}

					graph.string({ x, r.y + 2 }, item->text(), colors::black);
				}

				unsigned item_pixels(graph_reference graph) const
				{
					return graph.text_extent_size(L"jHWn/?\\{[(0569").height + 4;
				}
			};//end class item_renderer

			//class drawer_impl
			class drawer_impl
			{
			public:
				using widget_reference = widget&;
				using graph_reference = paint::graphics&;

				void clear_state()
				{
					state_.offset_y = 0;
					state_.index = npos;
				}

				void ignore_first_mouse_up(bool value)
				{
					ignore_first_mouseup_ = value;
				}

				bool ignore_emitting_mouseup()
				{
					if(ignore_first_mouseup_)
					{
						ignore_first_mouseup_ = false;
						return true;
					}
					return false;
				}

				void renderer(item_renderer* ir)
				{
					state_.renderer = (ir ? ir : state_.orig_renderer);
				}

				void scroll_items(bool upwards)
				{
					if(scrollbar_.empty()) return;

					const auto before_change = state_.offset_y;
					if(upwards)
					{
						if (before_change)
							--(state_.offset_y);
					}
					else 
					{
						if ((before_change + module_->max_items) < module_->items.size())
							++(state_.offset_y);
					}

					if(before_change != state_.offset_y)
					{
						draw();
						scrollbar_.value(state_.offset_y);
						API::update_window(*widget_);
					}
				}

				void move_items(bool upwards, bool recycle)
				{
					if(module_ && module_->items.size())
					{
						std::size_t init_index = state_.index;
						if(state_.index != npos)
						{
							unsigned last_offset_y = 0;
							if(module_->items.size() > module_->max_items)
								last_offset_y = static_cast<unsigned>(module_->items.size() - module_->max_items);

							if(upwards)
							{
								if(state_.index)
									--(state_.index);
								else if(recycle)
								{
									state_.index = static_cast<unsigned>(module_->items.size() - 1);
									state_.offset_y = last_offset_y;
								}

								if(state_.index < state_.offset_y)
									state_.offset_y = state_.index;
							}
							else
							{
								if(state_.index < module_->items.size() - 1)
									++(state_.index);
								else if(recycle)
								{
									state_.index = 0;
									state_.offset_y = 0;
								}

								if(state_.index >= state_.offset_y + module_->max_items)
									state_.offset_y = static_cast<unsigned>(state_.index - module_->max_items + 1);
							}
						}
						else
							state_.index = 0;

						if(init_index != state_.index)
						{
							draw();
							scrollbar_.value(state_.offset_y);
							API::update_window(*widget_);
						}
					}
				}

				std::size_t index() const
				{
					return state_.index;
				}

				widget* widget_ptr()
				{
					return widget_;
				}

				void attach(widget* wd, nana::paint::graphics* graph)
				{
					if(wd)
					{
						widget_ = wd;
						wd->events().mouse_wheel.connect_unignorable([this](const arg_wheel& arg){
							scroll_items(arg.upwards);
						});
					}
					if(graph) graph_ = graph;
				}

				void detach()
				{
					graph_ = nullptr;
				}

				void resize()
				{
					if(module_)
					{
						std::size_t items = (module_->max_items <= module_->items.size() ? module_->max_items : module_->items.size());
						std::size_t h = items * state_.renderer->item_pixels(*graph_);
						widget_->size(size{ widget_->size().width, static_cast<unsigned>(h + 4) });
					}
				}

				void set_module(const module_def& md, unsigned pixels)
				{
					module_ = &md;
					md.have_selected = false;
					if(md.index >= md.items.size())
						md.index = npos;

					image_pixels_ = pixels;
				}

				void set_result()
				{
					if(module_)
					{
						module_->index = state_.index;
						module_->have_selected = true;
					}
				}

				static bool right_area(graph_reference graph, int x, int y)
				{
					return ((1 < x && 1 < y) &&
						x < static_cast<int>(graph.width()) - 2 &&
						y < static_cast<int>(graph.height()) - 2);
				}

				bool set_mouse(graph_reference graph, int x, int y)
				{
					if(this->right_area(graph, x, y))
					{
						const unsigned n = (y - 2) / state_.renderer->item_pixels(graph) + static_cast<unsigned>(state_.offset_y);
						if(n != state_.index)
						{
							state_.index = n;
							return true;
						}
					}
					return false;
				}

				void draw()
				{
					if(module_)
					{
						bool pages = (module_->max_items < module_->items.size());
						const unsigned outter_w = (pages ? 20 : 4);

						if(graph_->width() > outter_w && graph_->height() > 4 )
						{
							//Draw items
							std::size_t items = (pages ? module_->max_items : module_->items.size());							
							items += state_.offset_y;

							const unsigned item_pixels = state_.renderer->item_pixels(*graph_);
							nana::rectangle item_r(2, 2, graph_->width() - outter_w, item_pixels);

							state_.renderer->image(_m_image_enabled(), image_pixels_);
							for(std::size_t i = state_.offset_y; i < items; ++i)
							{
								auto state = (i != state_.index ? item_renderer::StateNone : item_renderer::StateHighlighted);

								state_.renderer->render(*widget_, *graph_, item_r, module_->items[i].get(), state);
								item_r.y += item_pixels;
							}
						}	
						_m_open_scrollbar(*widget_, pages);
					}
					else
						graph_->string({ 4, 4 }, L"Empty Listbox, No Module!", static_cast<color_rgb>(0x808080));

					//Draw border
					graph_->rectangle(false, colors::black);
					graph_->rectangle(nana::rectangle(graph_->size()).pare_off(1), false, colors::white);
				}
			private:
				bool _m_image_enabled() const
				{
					for(auto & i : module_->items)
					{
						if(false == i->image().empty())
							return true;
					}
					return false;
				}

				void _m_open_scrollbar(widget_reference wd, bool v)
				{
					if(v)
					{
						if(scrollbar_.empty() && module_)
						{
							scrollbar_.create(wd, rectangle(static_cast<int>(wd.size().width - 18), 2, 16, wd.size().height - 4));
							scrollbar_.amount(module_->items.size());
							scrollbar_.range(module_->max_items);
							scrollbar_.value(state_.offset_y);

							auto & events = scrollbar_.events();
							events.mouse_wheel.connect([this](const arg_wheel& arg)
							{
								scroll_items(arg.upwards);
							});

							auto fn = [this](const arg_mouse& arg)
							{
								if (arg.is_left_button() && (scrollbar_.value() != state_.offset_y))
								{
									state_.offset_y = static_cast<unsigned>(scrollbar_.value());
									draw();
									API::update_window(*widget_);
								}
							};
							events.mouse_move.connect(fn);
							events.mouse_up.connect(fn);
						}
					}
					else
						scrollbar_.close();
				}
			private:
				widget * widget_{nullptr};
				nana::paint::graphics * graph_{nullptr};
				unsigned image_pixels_{16};		//Define the width pixels of the image area

				bool ignore_first_mouseup_{true};
				struct state_type
				{
					std::size_t offset_y{0};
					std::size_t index{npos};			//The index of the selected item.

					item_renderer * const orig_renderer;
					item_renderer * renderer;

					state_type(): orig_renderer(new def_item_renderer), renderer(orig_renderer){}
					~state_type()
					{
						delete orig_renderer;
					}
				}state_;
				nana::scroll<true> scrollbar_;

				const module_def* module_{nullptr};
			};

			//class drawer_impl;

			
			//class trigger
				trigger::trigger()
					:drawer_(new drawer_impl)
				{}

				trigger::~trigger()
				{
					delete drawer_;
				}

				drawer_impl& trigger::get_drawer_impl()
				{
					return *drawer_;
				}

				const drawer_impl& trigger::get_drawer_impl() const
				{
					return *drawer_;
				}

				void trigger::attached(widget_reference widget, graph_reference graph)
				{
					drawer_->attach(&widget, &graph);
				}

				void trigger::detached()
				{
					drawer_->detach();
				}

				void trigger::refresh(graph_reference)
				{
					drawer_->draw();
				}

				void trigger::mouse_move(graph_reference graph, const arg_mouse& arg)
				{
					if(drawer_->set_mouse(graph, arg.pos.x, arg.pos.y))
					{
						drawer_->draw();
						API::lazy_refresh();
					}
				}

				void trigger::mouse_up(graph_reference graph, const arg_mouse& arg)
				{
					bool close_wdg = false;
					if (drawer_->right_area(graph, arg.pos.x, arg.pos.y))
					{
						drawer_->set_result();
						close_wdg = true;
					}
					else
						close_wdg = (false == drawer_->ignore_emitting_mouseup());

					if (close_wdg)
						drawer_->widget_ptr()->close();
				}
			//end class trigger
		}
	}//end namespace drawerbase

	//class float_listbox
		float_listbox::float_listbox(window wd, const rectangle & r, bool is_ignore_first_mouse_up)
			:base_type(wd, false, r, appear::bald<appear::floating, appear::no_activate>())
		{
			API::capture_window(handle(), true);
			API::capture_ignore_children(false);
			API::take_active(handle(), false, parent());
			auto & impl = get_drawer_trigger().get_drawer_impl();
			impl.clear_state();
			impl.ignore_first_mouse_up(is_ignore_first_mouse_up);
		}

		void float_listbox::set_module(const float_listbox::module_type& md, unsigned pixels)
		{
			auto & impl = get_drawer_trigger().get_drawer_impl();
			impl.set_module(md, pixels);
			impl.resize();
			show();
		}

		void float_listbox::scroll_items(bool upwards)
		{
			get_drawer_trigger().get_drawer_impl().scroll_items(upwards);
		}

		void float_listbox::move_items(bool upwards, bool circle)
		{
			get_drawer_trigger().get_drawer_impl().move_items(upwards, circle);
		}

		void float_listbox::renderer(item_renderer* ir)
		{
			auto & impl = get_drawer_trigger().get_drawer_impl();
			impl.renderer(ir);
			impl.resize();
		}

		std::size_t float_listbox::index() const
		{
			return get_drawer_trigger().get_drawer_impl().index();
		}
	//end class float_listbox
}
