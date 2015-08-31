/*
 *	Parts of Class Place
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/place_parts.hpp
 */
#ifndef NANA_GUI_PLACE_PARTS_HPP
#define NANA_GUI_PLACE_PARTS_HPP

#include <nana/gui/widgets/form.hpp>
#include <nana/gui/widgets/tabbar.hpp>
#include <nana/gui/element.hpp>
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

		class splitter_dtrigger
			: public drawer_trigger
		{
		};

		template<bool IsLite>
		class splitter
			: public widget_object <typename std::conditional<IsLite, category::lite_widget_tag, category::widget_tag>::type, splitter_dtrigger>,
			public splitter_interface
		{
		private:
			void _m_complete_creation() override
			{
				this->caption("place-splitter");
				widget_object <typename std::conditional<IsLite, category::lite_widget_tag, category::widget_tag>::type, splitter_dtrigger>::_m_complete_creation();
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
		};

		class dockcaption_dtrigger
			: public drawer_trigger
		{
		public:
		private:
			virtual void attached(widget_reference, graph_reference graph) override
			{
				graph.set_color(colors::slate_blue);
				graph.set_text_color(colors::white);
			}

			void refresh(graph_reference& graph) override
			{
				graph.rectangle(true, colors::red);
				
				::nana::rectangle r{ graph.size() };

				r.x = r.right() - 20;
				r.y = (r.height - 16) / 2;
				r.width = r.height = 16;

				button_.draw(graph, colors::red, colors::white, r, element_state::normal);
				x_icon_.draw(graph, colors::red, colors::white, r, element_state::normal);
			}
		public:
			::nana::string caption;
			facade<element::button> button_;
			facade<element::x_icon>	x_icon_;
		};

		class dockarea_caption
			: public widget_object < category::widget_tag, dockcaption_dtrigger >
		{};

		class dock_page
			: public form
		{
		public:
			dock_page(window host, const rectangle& r, const rectangle & tab_r)
				: form(host, r, form::appear::bald<>())
			{
				tab_form_.reset(new form(handle(), tab_r, form::appear::bald<>()));

			}
		private:
			std::unique_ptr<form> tab_form_;
		};

		class dockarea
			: public widget_object <category::lite_widget_tag, drawer_trigger>
		{
			using base_type = widget_object<category::lite_widget_tag, drawer_trigger>;

			using factory = std::function<std::unique_ptr<widget>(window)>;

			struct panel
			{
				factory factory_fn;
				std::unique_ptr<widget> widget_ptr;

				panel(factory && fn)
					: factory_fn(std::move(fn))
				{}
			};
		public:
			void create(window parent, place_parts::dock_notifier_interface* notifier)
			{
				notifier_ = notifier;
				host_window_ = parent;
				base_type::create(parent, true);
				this->caption("dockarea");
				caption_.create(*this, true);
				caption_.caption("dockarea-caption");

				this->events().resized([this](const arg_resized& arg)
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

				caption_.events().mouse_down([this](const arg_mouse& arg)
				{
					if (::nana::mouse::left_button == arg.button)
					{
						moves_.started = true;
						moves_.start_pos = API::cursor_position();
						moves_.start_container_pos = (floating() ? container_->pos() : this->pos());
						API::capture_window(caption_, true);
					}
				});

				caption_.events().mouse_move([this](const arg_mouse& arg)
				{
					if (arg.left_button && moves_.started)
					{
						auto move_pos = API::cursor_position() - moves_.start_pos;
						if (!floating())
						{
							if (std::abs(move_pos.x) > 4 || abs(move_pos.y) > 4)
								float_away(move_pos);
						}
						else
						{
							move_pos += moves_.start_container_pos;
							API::move_window(container_->handle(), move_pos);
							notifier_->notify_move();
						}
					}
				});

				caption_.events().mouse_up([this](const arg_mouse& arg)
				{
					if ((::nana::mouse::left_button == arg.button) && moves_.started)
					{
						moves_.started = false;
						API::capture_window(caption_, false);
						notifier_->notify_move_stopped();
					}
				});

				if (panels_.size() > 1)
					tabbar_.reset(new tabbar_lite(*this));

				std::size_t pos = 0;
				for (auto & pn : panels_)
				{
					if (!pn.widget_ptr)
					{
						pn.widget_ptr = pn.factory_fn(*this);
						if (tabbar_)
						{
							tabbar_->push_back(::nana::charset(pn.widget_ptr->caption()));
							tabbar_->attach(pos++, pn.widget_ptr->handle());
							//tabbar_->push_back(pn.widget_ptr->caption());
							//tabbar_->relate(pos++, pn.widget_ptr->handle());
						}
					}
				}
			}

			void add_factory(factory && fn)
			{
				panels_.emplace_back(std::move(fn));
			}

			void float_away(const ::nana::point& move_pos)
			{
				if (container_)
					return;

				API::capture_window(caption_, false);

				rectangle r{ pos() + move_pos, size() };
				container_.reset(new form(host_window_, r.pare_off(-1), form::appear::bald<>()));
				drawing dw(container_->handle());
				dw.draw([](paint::graphics& graph)
				{
					graph.rectangle(false, colors::coral);
				});

				API::set_parent_window(handle(), container_->handle());
				this->move({ 1, 1 });

				container_->show();
				API::capture_window(caption_, true);

				notifier_->notify_float();
			}

			void dock()
			{
				API::capture_window(caption_, false);
				API::set_parent_window(handle(), host_window_);
				container_.reset();
				notifier_->notify_dock();
			}

			bool floating() const
			{
				return (nullptr != container_);
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

			void reset()
			{
				kind_ = kind::none;
				value_.integer = 0;
			}

			bool is_negative() const
			{
				switch (kind_)
				{
				case kind::integer:
					return (value_.integer < 0);
				case kind::real:
				case kind::percent:
					return (value_.real < 0);
				default:
					break;
				}
				return false;
			}

			bool is_none() const
			{
				return (kind::none == kind_);
			}

			bool is_not_none() const
			{
				return (kind::none != kind_);
			}

			kind kind_of() const
			{
				return kind_;
			}

			double get_value(int ref_percent) const
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

			int integer() const
			{
				if (kind::integer == kind_)
					return value_.integer;
				return static_cast<int>(value_.real);
			}

			double real() const
			{
				if (kind::integer == kind_)
					return value_.integer;
				return value_.real;
			}

			void assign(int i)
			{
				kind_ = kind::integer;
				value_.integer = i;
			}

			void assign(double d)
			{
				kind_ = kind::real;
				value_.real = d;
			}

			void assign_percent(double d)
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

			void push(const number_t& v)
			{
				margins_.emplace_back(v);
			}

			void set_value(const number_t& v)
			{
				clear();
				margins_.emplace_back(v);
			}

			void set_array(const std::vector<number_t>& v)
			{
				all_edges_ = false;
				margins_ = v;
			}

			nana::rectangle area(const ::nana::rectangle& field_area) const
			{
				if (margins_.empty())
					return field_area;

				auto r = field_area;
				if (all_edges_)
				{
					auto px = static_cast<int>(margins_.back().get_value(static_cast<int>(r.width)));
					const auto dbl_px = static_cast<unsigned>(px << 1);
					r.x += px;
					r.width = (r.width < dbl_px ? 0 : r.width - dbl_px);

					r.y += px;
					r.height = (r.height < dbl_px ? 0 : r.height - dbl_px);
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
					default:
						il = 3;	//left
					case 3:	//top, right, bottom
						it = 0;
						ir = 1;
						ib = 2;
					}

					typedef decltype(r.height) px_type;
					auto calc = [](px_type a, px_type b)
					{
						return (a > b ? a - b : 0);
					};

					if (0 == it)	//top
					{
						auto px = static_cast<int>(margins_[it].get_value(static_cast<int>(field_area.height)));
						r.y += px;
						r.height = calc(r.height, static_cast<px_type>(px));
					}

					if (-1 != ib)	//bottom
					{
						auto px = static_cast<int>(margins_[ib].get_value(static_cast<int>(field_area.height)));
						r.height = calc(r.height, static_cast<px_type>(px));
					}

					if (-1 != il)	//left
					{
						auto px = static_cast<px_type>(margins_[il].get_value(static_cast<int>(field_area.width)));
						r.x += px;
						r.width = calc(r.width, static_cast<px_type>(px));
					}

					if (-1 != ir)	//right
					{
						auto px = static_cast<int>(margins_[ir].get_value(static_cast<int>(field_area.width)));
						r.width = calc(r.width, static_cast<px_type>(px));
					}
				}
				return r;
			}
		private:
			bool all_edges_ = true;
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
				if (values_.empty())
					return{};

				if (repeated_)
					pos %= values_.size();
				else if (pos >= values_.size())
					return{};

				return values_[pos];
			}
		private:
			bool repeated_ = false;
			std::vector<number_t> values_;
		};
	}//end namespace place_parts
}//end namespace nana

#endif //NANA_GUI_PLACE_PARTS_HPP