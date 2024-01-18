/*
 *	A float_listbox Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2021 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/float_listbox.cpp
 */

#include <nana/gui/widgets/float_listbox.hpp>
#include <nana/gui/widgets/scroll.hpp>

#include <nana/gui/layout_utility.hpp>
#include <nana/gui/screen.hpp>

namespace nana
{
	namespace drawerbase::float_listbox
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

			void background(widget_reference, graph_reference graph)
			{
				graph.rectangle(false, colors::black);
				graph.rectangle(nana::rectangle(graph.size()).pare_off(1), false, colors::white);
			}

			void item(widget_reference, graph_reference graph, const nana::rectangle& r, const item_interface* item, state_t state)
			{
				if (state == StateHighlighted)
				{
					graph.rectangle(r, false, static_cast<color_rgb>(0xafc7e3));

					graph.palette(false, colors::white);

					paint::draw draw{ graph };
					draw.corner(r, 1);

					graph.palette(false, static_cast<color_rgb>(0xafc7e3));

					auto inner_r = r;
					draw.corner(inner_r.pare_off(1), 1);

					graph.rectangle(inner_r, false, static_cast<color_rgb>(0xEBF4FB));
					graph.gradual_rectangle(inner_r.pare_off(1), static_cast<color_rgb>(0xDDECFD), static_cast<color_rgb>(0xC2DCFD), true);
				}
				else
					graph.rectangle(r, true, colors::white);

				int x = r.x + 2;
				if (image_enabled_)
				{
					unsigned vpix = (r.height - 4);
					if (item->image())
					{
						auto imgsz = nana::fit_zoom(item->image().size(), { image_pixels_, vpix });

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
				unsigned ascent, descent, ileading;
				graph.text_metrics(ascent, descent, ileading);
				return ascent + descent + 4;
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
				if (ignore_first_mouseup_)
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
				if (scrollbar_.empty()) return;

				const auto before_change = state_.offset_y;
				if (upwards)
				{
					if (before_change)
						--(state_.offset_y);
				}
				else
				{
					if ((before_change + module_->max_items) < module_->items.size())
						++(state_.offset_y);
				}

				if (before_change != state_.offset_y)
				{
					draw();
					scrollbar_.value(state_.offset_y);
					api::update_window(*widget_);
				}
			}

			void move_items(bool upwards, bool recycle)
			{
				if (module_ && module_->items.size())
				{
					std::size_t init_index = state_.index;
					if (state_.index != npos)
					{
						unsigned last_offset_y = 0;
						if (module_->items.size() > module_->max_items)
							last_offset_y = static_cast<unsigned>(module_->items.size() - module_->max_items);

						if (upwards)
						{
							if (state_.index)
								--(state_.index);
							else if (recycle)
							{
								state_.index = module_->items.size() - 1;
								state_.offset_y = last_offset_y;
							}

							if (state_.index < state_.offset_y)
								state_.offset_y = state_.index;
						}
						else
						{
							if (state_.index < module_->items.size() - 1)
								++(state_.index);
							else if (recycle)
							{
								state_.index = 0;
								state_.offset_y = 0;
							}

							if (state_.index >= state_.offset_y + module_->max_items)
								state_.offset_y = state_.index - module_->max_items + 1;
						}
					}
					else
						state_.index = 0;

					if (init_index != state_.index)
					{
						draw();
						scrollbar_.value(state_.offset_y);
						api::update_window(*widget_);
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

			void attach(widget& wd, nana::paint::graphics& graph)
			{
				widget_ = &wd;
				wd.events().mouse_wheel.connect_unignorable([this](const arg_wheel& arg) {
					scroll_items(arg.upwards);
					});

				graph_ = &graph;
			}

			void detach()
			{
				graph_ = nullptr;
			}

			void resize()
			{
				if (module_)
				{

					auto const items = (module_->max_items <= module_->items.size() ? module_->max_items : module_->items.size());

					rectangle list_r{
						0, 0,
						widget_->size().width,
						static_cast<unsigned>(items * state_.renderer->item_pixels(*graph_)) + 4
					};

					//Test if the listbox excesses the screen

					screen scr;
					auto& disp = scr.from_window(*widget_);

					auto disp_r = disp.area();

					point pos;
					api::calc_screen_point(*widget_, pos);
					list_r.position(pos);

					if (widget_->size().width >= disp_r.width)
					{
						pos.x = 0;
						list_r.width = disp_r.width;
					}
					else if (list_r.right() > disp_r.right())
						pos.x = disp_r.right() - static_cast<int>(list_r.width);

					if (list_r.height >= disp_r.height)
					{
						pos.y = 0;
						list_r.height = disp_r.height;
					}
					else if (list_r.bottom() > disp_r.bottom())
						pos.y = disp_r.bottom() - static_cast<int>(list_r.height);

					api::calc_window_point(api::get_owner_window(*widget_), pos);
					list_r.position(pos);

					widget_->move(list_r);
				}
			}

			void set_module(const module_def& md, unsigned pixels)
			{
				module_ = &md;
				md.have_selected = false;
				if (md.index >= md.items.size())
					md.index = npos;

				image_pixels_ = pixels;
			}

			void set_result()
			{
				if (module_)
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
				if (this->right_area(graph, x, y))
				{
					const unsigned n = (y - 2) / state_.renderer->item_pixels(graph) + static_cast<unsigned>(state_.offset_y);
					if (n != state_.index)
					{
						state_.index = n;
						return true;
					}
				}
				else if (deselect_on_mouse_leave_ && npos != state_.index)
				{
					state_.index = npos;
					return true;
				}
				return false;
			}

			void deselect_on_mouse_leave(bool value)
			{
				deselect_on_mouse_leave_ = value;
			}

			void button_size(unsigned bs)
			{
				button_size_ = bs;
			}

			std::size_t length() const
			{
				return (module_ ? module_->items.size() : 0);
			}

			std::optional<std::string> text(std::size_t pos) const
			{
				if (module_ && (pos < module_->items.size()))
					return std::string{ module_->items[pos]->text() };

				return {};
			}

			void select(std::size_t pos)
			{
				if (!(module_ && (pos < module_->items.size())))
					return;

				if (state_.index != pos)
				{
					if (pos - state_.offset_y >= module_->max_items)
						state_.offset_y = pos - (pos % module_->max_items);
					
					state_.index = pos;
					draw();
					scrollbar_.value(state_.offset_y);
					api::update_window(*widget_);
				}
			}

			void draw()
			{
				if (module_)
				{
					bool multipage = (module_->max_items < module_->items.size());
					const unsigned outter_w = (multipage ? 20 : 4);

					if (graph_->width() > outter_w && graph_->height() > 4)
					{
						//Draw items
						std::size_t items = (multipage ? module_->max_items : module_->items.size());
						items += state_.offset_y;

						const unsigned item_pixels = state_.renderer->item_pixels(*graph_);
						nana::rectangle item_r(2, 2, graph_->width() - outter_w, item_pixels);

						state_.renderer->image(_m_image_enabled(), image_pixels_);
						for (std::size_t i = state_.offset_y; i < items; ++i)
						{
							auto state = (i != state_.index ? item_renderer::StateNone : item_renderer::StateHighlighted);

							state_.renderer->item(*widget_, *graph_, item_r, module_->items[i].get(), state);
							item_r.y += item_pixels;
						}
					}
					_m_open_scrollbar(*widget_, multipage);
				}
				else
					graph_->string({ 4, 4 }, L"Empty Listbox, No Module!", static_cast<color_rgb>(0x808080));

				//Draw border
				state_.renderer->background(*widget_, *graph_);
			}
		private:
			bool _m_image_enabled() const
			{
				for (auto& i : module_->items)
				{
					if (false == i->image().empty())
						return true;
				}
				return false;
			}

			void _m_open_scrollbar(widget_reference wd, bool v)
			{
				if (!v)
				{
					scrollbar_.close();
					return;
				}

				if (scrollbar_.empty() && module_)
				{
					scrollbar_.create(wd, rectangle(static_cast<int>(wd.size().width - button_size_ - 2), 2, button_size_, wd.size().height - 4));
					scrollbar_.scheme().button_size = button_size_;
					scrollbar_.amount(module_->items.size());
					scrollbar_.range(module_->max_items);
					scrollbar_.value(state_.offset_y);

					auto& events = scrollbar_.events();
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
							api::update_window(*widget_);
						}
					};
					events.mouse_move.connect(fn);
					events.mouse_up.connect(fn);
				}
			}
		private:
			widget* widget_{ nullptr };
			nana::paint::graphics* graph_{ nullptr };
			unsigned image_pixels_{ 16 };		//Define the width pixels of the image area

			bool ignore_first_mouseup_{ true };

			unsigned button_size_{ 16 };	//Width/height of scroll and its button
			bool deselect_on_mouse_leave_{ false }; //false: is the behaviour of the combox
												   //true: is the behaviour of the menu

			struct state_type
			{
				std::size_t offset_y{ 0 };
				std::size_t index{ npos };			//The index of the selected item.

				item_renderer* const orig_renderer;
				item_renderer* renderer;

				state_type() : orig_renderer(new def_item_renderer), renderer(orig_renderer) {}
				~state_type()
				{
					delete orig_renderer;
				}
			}state_;
			nana::scroll<true> scrollbar_;

			const module_def* module_{ nullptr };
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
			drawer_->attach(widget, graph);
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
			if (drawer_->set_mouse(graph, arg.pos.x, arg.pos.y))
			{
				drawer_->draw();
				api::dev::lazy_refresh();
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
	}//end namespace drawerbase::float_listbox

	//class float_listbox
	float_listbox::float_listbox(window wd, const rectangle& r, bool is_ignore_first_mouse_up)
		:base_type(wd, false, r, appear::bald<appear::floating, appear::no_activate>())
	{
		this->set_capture(false);

		api::take_active(handle(), false, parent());
		auto& impl = get_drawer_trigger().get_drawer_impl();
		impl.clear_state();
		impl.ignore_first_mouse_up(is_ignore_first_mouse_up);
	}

	void float_listbox::set_module(const float_listbox::module_type& md, unsigned pixels)
	{
		auto& impl = get_drawer_trigger().get_drawer_impl();
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
		auto& impl = get_drawer_trigger().get_drawer_impl();
		impl.renderer(ir);
		impl.resize();
	}

	std::size_t float_listbox::index() const
	{
		return get_drawer_trigger().get_drawer_impl().index();
	}

	void float_listbox::deselect_on_mouse_leave(bool value)
	{
		get_drawer_trigger().get_drawer_impl().deselect_on_mouse_leave(value);
	}

	void float_listbox::button_size(unsigned bs)
	{
		get_drawer_trigger().get_drawer_impl().button_size(bs);
	}

	float_listbox::size_type float_listbox::length() const
	{
		return get_drawer_trigger().get_drawer_impl().length();
	}
	
	std::optional<std::string> float_listbox::text(size_type pos) const
	{
		internal_scope_guard lock;
		return get_drawer_trigger().get_drawer_impl().text(pos);
	}
	
	void float_listbox::select(size_type pos)
	{
		internal_scope_guard lock;
		get_drawer_trigger().get_drawer_impl().select(pos);
	}
	//end class float_listbox
}
