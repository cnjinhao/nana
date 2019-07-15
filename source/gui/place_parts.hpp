/**
 *	Parts of Class Place
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file nana/gui/place_parts.hpp
 */
#ifndef NANA_GUI_PLACE_PARTS_HPP
#define NANA_GUI_PLACE_PARTS_HPP

#include <nana/gui/widgets/form.hpp>
#include <nana/gui/widgets/tabbar.hpp>
#include <nana/gui/element.hpp>
#include <nana/paint/text_renderer.hpp>
#include <stdexcept>
#include <deque>

namespace nana
{
	namespace place_parts
	{
		class splitter_interface
		{
		public:
			virtual ~splitter_interface(){}
		};

		class drawer_splitter
			: public drawer_trigger
		{
		public:
			void set_renderer(const std::function<void(window, paint::graphics&, mouse_action)>& rd)
			{
				renderer_ = rd;
			}
		private:
			void attached(widget_reference wdg, graph_reference) override
			{
				window_handle_ = wdg;
			}

			void refresh(graph_reference graph) override
			{
				API::dev::copy_transparent_background(window_handle_, graph);
				if (renderer_)
					renderer_(window_handle_, graph, API::mouse_action(window_handle_));
			}

			void mouse_enter(graph_reference graph, const arg_mouse&) override
			{
				refresh(graph);
				API::dev::lazy_refresh();
			}

			void mouse_move(graph_reference graph, const arg_mouse&) override
			{
				refresh(graph);
				API::dev::lazy_refresh();
			}
			
			void mouse_leave(graph_reference graph, const arg_mouse&) override
			{
				refresh(graph);
				API::dev::lazy_refresh();
			}

			void mouse_down(graph_reference graph, const arg_mouse&) override
			{
				refresh(graph);
				API::dev::lazy_refresh();
			}

			void mouse_up(graph_reference graph, const arg_mouse&) override
			{
				refresh(graph);
				API::dev::lazy_refresh();
			}
		private:
			window window_handle_{nullptr};
			std::function<void(window, paint::graphics&, mouse_action)> renderer_;
		};
		
		class splitter
			: public widget_object<category::widget_tag, drawer_splitter>,
			public splitter_interface
		{
		public:
			void set_renderer(const std::function<void(window, paint::graphics&, mouse_action)>& rd)
			{
				get_drawer_trigger().set_renderer(rd);
			}
		private:
			void _m_complete_creation() override
			{
				this->caption("place-splitter");
				widget_object<category::widget_tag, drawer_splitter>::_m_complete_creation();

				API::effects_bground(*this, effects::bground_transparent(0), 0);
			}
		};


		class dock_notifier_interface
		{
		public:
			virtual ~dock_notifier_interface() = default;

			virtual void notify_float() = 0;
			virtual void notify_dock() = 0;
			virtual void notify_move() = 0;
			virtual void notify_move_stopped() = 0;

			//a dockarea requests to close the dockpane
			virtual void request_close() = 0;
		};

		class dockcaption_dtrigger
			: public drawer_trigger
		{
		public:
			void on_close(std::function<void()>&& fn)
			{
				close_fn_ = std::move(fn);
			}

			bool hit_close() const
			{
				return x_pointed_;
			}

		private:
			virtual void attached(widget_reference wdg, graph_reference graph) override
			{
				window_handle_ = wdg;
				text_rd_.reset(new paint::text_renderer(graph));
			}

			void refresh(graph_reference& graph) override
			{
				graph.palette(true, colors::white);
				graph.rectangle(true, static_cast<color_rgb>(0x83EB));

				//draw caption
				auto text = to_wstring(API::window_caption(window_handle_));
				text_rd_->render({ 3, 1 }, text.data(), text.size(), graph.size().width - 20, paint::text_renderer::mode::truncate_with_ellipsis);

				//draw x button
				auto r = _m_button_area();
				if (x_pointed_)
				{
					color xclr = colors::red;

					if(x_state_ == ::nana::mouse_action::pressed)
						xclr = xclr.blend(colors::white, 0.2);

					graph.rectangle(r, true, xclr);
				}

				r.x += (r.width - 16) / 2;
				r.y = (r.height - 16) / 2;

				x_icon_.draw(graph, colors::red, colors::white, r, element_state::normal);
			}

			void mouse_move(graph_reference graph, const arg_mouse& arg) override
			{
				x_pointed_ = _m_button_area().is_hit(arg.pos);

				refresh(graph);
				API::dev::lazy_refresh();
			}

			void mouse_leave(graph_reference graph, const arg_mouse&) override
			{
				x_pointed_ = false;
				refresh(graph);
				API::dev::lazy_refresh();
			}

			void mouse_down(graph_reference graph, const arg_mouse&) override
			{
				if (!x_pointed_)
					return;

				x_state_ = ::nana::mouse_action::pressed;

				refresh(graph);
				API::dev::lazy_refresh();
			}

			void mouse_up(graph_reference graph, const arg_mouse&) override
			{
				if (!x_pointed_)
					return;

				x_state_ = ::nana::mouse_action::hovered;
				refresh(graph);
				API::dev::lazy_refresh();

				close_fn_();
			}
		private:
			::nana::rectangle _m_button_area() const
			{
				auto sz = API::window_size(window_handle_);
				return{static_cast<int>(sz.width) - 20, 0, 20, sz.height};
			}
		public:
			window window_handle_;
			std::unique_ptr<paint::text_renderer> text_rd_;
			bool x_pointed_{ false };
			::nana::mouse_action x_state_{ ::nana::mouse_action::normal };
			facade<element::x_icon>	x_icon_;

			std::function<void()>	close_fn_;
		};

		class dockarea_caption
			: public widget_object<category::widget_tag, dockcaption_dtrigger>
		{
		public:
			using widget_object<category::widget_tag, dockcaption_dtrigger>::get_drawer_trigger;
		};

		static unsigned differ(unsigned x, unsigned y) noexcept
		{
			return (x > y ? x - y : 0);
		}

		class dockarea
			: public widget_object <category::lite_widget_tag, drawer_trigger>
		{
			using base_type = widget_object<category::lite_widget_tag, drawer_trigger>;

			using factory = std::function<std::unique_ptr<widget>(window)>;

			struct panel
			{
				std::unique_ptr<widget> widget_ptr;
			};
		public:
			void set_notifier(place_parts::dock_notifier_interface* notifier)
			{
				notifier_ = notifier;
			}

			void create(window parent)
			{
				host_window_ = parent;
				base_type::create(parent, true);
				this->caption("dockarea");
				caption_.create(*this, true);
				caption_.get_drawer_trigger().on_close([this]
				{
					if (tabbar_)
					{
						tabbar_->erase(tabbar_->selected());
						if (tabbar_->length())
							return;
					}

					notifier_->request_close();
				});

				this->events().resized.connect_unignorable([this](const arg_resized& arg)
				{
					rectangle r{ 0, 0, arg.width, 20 };
					caption_.move(r);

					if (arg.height > 20)
					{
						r.y = 20;
						if (tabbar_)
						{
							tabbar_->move({ 0, int(arg.height) - 20, arg.width, 20 });
							r.height = arg.height - 40;
						}
						else
							r.height = arg.height - 20;
					}

					for (auto & pn : panels_)
					{
						if (pn.widget_ptr)
							pn.widget_ptr->move(r);
					}
				});

				auto grab_fn = [this](const arg_mouse& arg)
				{
					if (event_code::mouse_down == arg.evt_code)
					{
						if (::nana::mouse::left_button == arg.button)
						{
							moves_.started = true;
							moves_.start_pos = API::cursor_position();
							moves_.start_container_pos = (floating() ? container_->pos() : this->pos());
							caption_.set_capture(true);
						}
					}
					else if (event_code::mouse_move == arg.evt_code)
					{
						if (arg.left_button && moves_.started)
						{
							auto move_pos = API::cursor_position() - moves_.start_pos;
							if (!floating())
							{
								if (std::abs(move_pos.x) > 4 || std::abs(move_pos.y) > 4)
									float_away(move_pos);
							}
							else
							{
								move_pos += moves_.start_container_pos;
								API::move_window(container_->handle(), move_pos);

								if(!caption_.get_drawer_trigger().hit_close())
									notifier_->notify_move();
							}
						}
					}
					else if (event_code::mouse_up == arg.evt_code)
					{
						if ((::nana::mouse::left_button == arg.button) && moves_.started)
						{
							moves_.started = false;
							caption_.release_capture();
							notifier_->notify_move_stopped();
						}
					}
				};

				caption_.events().mouse_down.connect(grab_fn);
				caption_.events().mouse_move.connect(grab_fn);
				caption_.events().mouse_up.connect(grab_fn);

			}

			widget* add_pane(factory & fn)
			{
				auto fn_ptr = &fn;
                widget * w = nullptr;
				API::dev::affinity_execute(*this, [this, fn_ptr, &w]
				{
					w=_m_add_pane(*fn_ptr);
				});
                return w;
			}

			void float_away(const ::nana::point& move_pos)
			{
				if (container_)
					return;

				caption_.release_capture();

				rectangle r{ pos() + move_pos, size() };
				container_.reset(new form(host_window_, r.pare_off(-1), form::appear::bald<form::appear::sizable>()));
				drawing dw(container_->handle());
				dw.draw([](paint::graphics& graph)
				{
					graph.rectangle(false, colors::coral);
				});

				API::set_parent_window(handle(), container_->handle());
				this->move({ 1, 1 });

				container_->events().resized.connect_unignorable([this](const arg_resized& arg)
				{
					this->size({arg.width - 2, arg.height - 2});
				});

				container_->show();
				caption_.set_capture(true);

				notifier_->notify_float();
			}

			void dock()
			{
				caption_.release_capture();

				API::set_parent_window(handle(), host_window_);
				container_.reset();
				notifier_->notify_dock();
			}

			bool floating() const
			{
				return (nullptr != container_);
			}
		private:
			widget* _m_add_pane(factory & fn)
			{
				rectangle r{ this->size() };

				//get a rectangle excluding caption
				r.y = 20;
				r.height = differ(r.height, 20);

				if (!tabbar_)
				{
					if (panels_.size() > 0)
					{
						tabbar_.reset(new tabbar_lite(*this));

						tabbar_->events().selected.clear();
						tabbar_->events().selected.connect([this](const event_arg&)
						{
							auto handle = tabbar_->attach(tabbar_->selected());
							//Set caption through a caption of window specified by handle
							//Empty if handle is null
							caption_.caption(API::window_caption(handle));
						});

						r.height -= 20;
						tabbar_->move({ 0, r.bottom(), r.width, 20 });

						std::size_t pos = 0;
						for (auto & pn : panels_)
						{
							tabbar_->push_back(pn.widget_ptr->caption());
							tabbar_->attach(pos++, *pn.widget_ptr);
						}
					}
				}
				else
					r.height -= 20;

				auto wdg = fn(*this);
				if (wdg)
				{
					if (tabbar_)
					{
						tabbar_->push_back(::nana::charset(wdg->caption()));
						tabbar_->attach(panels_.size(), wdg->handle());
					}

					if (panels_.empty())
					{
						caption_.caption(wdg->caption());
					}

					auto wdg_ptr = wdg.get();
#ifdef _nana_std_has_emplace_return_type
					panels_.emplace_back().widget_ptr = std::move(wdg);
#else
					panels_.emplace_back();
					panels_.back().widget_ptr.swap(wdg);
#endif
					for (auto & pn : panels_)
					{
						if (pn.widget_ptr)
							pn.widget_ptr->move(r);
					}
					return wdg_ptr;
				}
				return nullptr;
			}
		private:
			window host_window_{nullptr};
			place_parts::dock_notifier_interface* notifier_{ nullptr };
			std::unique_ptr<form>	container_;
			dockarea_caption	caption_;
			std::deque<panel>	panels_;
			std::unique_ptr<tabbar_lite> tabbar_;

			struct moves
			{
				bool started{ false };
				::nana::point start_pos;
				::nana::point start_container_pos;
			}moves_;
		};//class dockarea


		//number_t is used for storing a number type variable
		//such as integer, real and percent. Essentially, percent is a typo of real.
		class number_t
		{
		public:
			enum class kind{ none, integer, real, percent };

			number_t()
				: kind_(kind::none)
			{
				value_.integer = 0;
			}

			void reset() noexcept
			{
				kind_ = kind::none;
				value_.integer = 0;
			}

			bool is_negative() const
			{
				return (((kind::integer == kind_) && (value_.integer < 0)) ||
					((kind::real == kind_ || kind::percent == kind_) && (value_.real < 0)));
			}

			bool empty() const noexcept
			{
				return (kind::none == kind_);
			}

			kind kind_of() const noexcept
			{
				return kind_;
			}

			double get_value(int ref_percent) const noexcept
			{
				switch (kind_)
				{
				case kind::integer:
					return value_.integer;
				case kind::real:
					return value_.real;
				case kind::percent:
					return value_.real * ref_percent;
				default:
					break;
				}
				return 0;
			}

			int integer() const noexcept
			{
				if (kind::integer == kind_)
					return value_.integer;
				return static_cast<int>(value_.real);
			}

			double real() const noexcept
			{
				if (kind::integer == kind_)
					return value_.integer;
				return value_.real;
			}

			void assign(int i) noexcept
			{
				kind_ = kind::integer;
				value_.integer = i;
			}

			void assign(double d) noexcept
			{
				kind_ = kind::real;
				value_.real = d;
			}

			void assign_percent(double d) noexcept
			{
				kind_ = kind::percent;
				value_.real = d / 100;
			}
		private:
			kind kind_;
			union valueset
			{
				int integer;
				double real;
			}value_;
		};//end class number_t

		/// Margin attribute
		/**
		 * Definition at https://github.com/cnjinhao/nana/wiki/Div-Text#margin
		 */
		class margin
		{
		public:
			margin& operator=(margin&& rhs)
			{
				if (this != &rhs)
				{
					all_edges_ = rhs.all_edges_;
					margins_ = std::move(rhs.margins_);
				}
				return *this;
			}

			void clear()
			{
				all_edges_ = true;
				margins_.clear();
			}
			
			void push(const number_t& v, bool reset = false)
			{
				if (reset)
					clear();

				margins_.emplace_back(v);
			}

			void set_array(const std::vector<number_t>& v)
			{
				all_edges_ = false;
				margins_ = v;
			}

			number_t get_edge(std::size_t edge) const
			{
				int il{ -1 }, ir{ -1 }, it{ -1 }, ib{ -1 };	//index of four corners in margin
				switch (margins_.size())
				{
				case 0:	break;
				case 1:	//top
					it = 0;
					break;
				case 2://top,bottom and left,right
					it = ib = 0;
					il = ir = 1;
					break;
				case 3:	//top, right, bottom
					it = 0;
					ir = 1;
					ib = 2;
					break;
				default: //left, top, right, bottom, left
					it = 0;
					ir = 1;
					ib = 2;
					il = 3;
				}

				int pos = 0;
				switch (edge)
				{
				case 0: //top
					pos = it; break;
				case 1: //right
					pos = ir; break;
				case 2: //bottom
					pos = ib; break;
				case 3: //left
					pos = il; break;
				default:
					return {};
				}

				return (-1 == pos ? number_t{} : margins_[pos]);
			}

			nana::rectangle area(const ::nana::rectangle& field_area) const
			{
				if (margins_.empty())
					return field_area;

				auto r = field_area;
				if (all_edges_)
				{
					auto px = static_cast<int>(margins_.back().get_value(static_cast<int>(r.width)));
					r.x += px;
					r.width = differ(r.width, (static_cast<unsigned>(px) << 1));

					r.y += px;
					r.height = differ(r.height, (static_cast<unsigned>(px) << 1));
				}
				else
				{
					int il{ -1 }, ir{ -1 }, it{ -1 }, ib{ -1 };	//index of four corners in margin
					switch (margins_.size())
					{
					case 0:	break;
					case 1:	//top
						it = 0;
						break;
					case 2://top,bottom and left,right
						it = ib = 0;
						il = ir = 1;
						break;
					case 3:	//top, right, bottom
						it = 0;
						ir = 1;
						ib = 2;
						break;
					default: //left, top, right, bottom, left
						it = 0;
						ir = 1;
						ib = 2;
						il = 3;
					}

					using px_type = decltype(r.height);

					if (0 == it)	//top
					{
						auto px = static_cast<int>(margins_[it].get_value(static_cast<int>(field_area.height)));
						r.y += px;
						r.height = differ(r.height, static_cast<px_type>(px));
					}

					if (-1 != ib)	//bottom
					{
						auto px = static_cast<int>(margins_[ib].get_value(static_cast<int>(field_area.height)));
						r.height = differ(r.height, static_cast<px_type>(px));
					}

					if (-1 != il)	//left
					{
						auto px = static_cast<px_type>(margins_[il].get_value(static_cast<int>(field_area.width)));
						r.x += px;
						r.width = differ(r.width, static_cast<px_type>(px));
					}

					if (-1 != ir)	//right
					{
						auto px = static_cast<int>(margins_[ir].get_value(static_cast<int>(field_area.width)));
						r.width = differ(r.width, static_cast<px_type>(px));
					}
				}
				return r;
			}
		private:
			bool all_edges_{ true };
			std::vector<number_t> margins_;
		};//end class margin

		class repeated_array
		{
		public:
			//A workaround for VC2013, becuase it does not generated an implicit declared move-constructor as defaulted.
			repeated_array() = default;

			repeated_array(repeated_array && other)
				: repeated_{ other.repeated_ },
				values_(std::move(other.values_))
			{
			}

			repeated_array& operator=(repeated_array&& other)
			{
				if (this != &other)
				{
					repeated_ = other.repeated_;
					other.repeated_ = false;
					values_ = std::move(other.values_);
				}
				return *this;
			}

			void assign(std::vector<number_t>&& c)
			{
				values_ = std::move(c);
			}

			bool empty() const
			{
				return values_.empty();
			}

			void reset()
			{
				repeated_ = false;
				values_.clear();
			}

			void repeated()
			{
				repeated_ = true;
			}

			void push(const number_t& n)
			{
				values_.emplace_back(n);
			}

			number_t at(std::size_t pos) const
			{
				if (values_.size() && (repeated_ || pos < values_.size()))
					return values_[pos % values_.size()];

				return{};
			}
		private:
			bool repeated_ = false;
			std::vector<number_t> values_;
		};
	}//end namespace place_parts
}//end namespace nana

#endif //NANA_GUI_PLACE_PARTS_HPP