/*
*	Elements of GUI Gadgets
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2017 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file: nana/gui/element.cpp
*/

#include <nana/gui/element.hpp>
#include <nana/gui/detail/bedrock.hpp>
#include <nana/gui/detail/element_store.hpp>
#include <nana/paint/image.hpp>
#include <map>
#include <mutex>

namespace nana
{
	//Element definitions
	namespace element
	{
		namespace detail
		{
			void factory_abstract::destroy(element_abstract* ptr)
			{
				delete ptr;
			}
		}

		class crook
			: public crook_interface
		{
			bool draw(graph_reference graph, const nana::color& bgcolor, const nana::color& fgcolor, const nana::rectangle& r, element_state es, const data& crook_data) override
			{
				::nana::color highlighted(static_cast<color_rgb>(0x5eb6f7));
				auto bld_bgcolor = bgcolor;
				auto bld_fgcolor = fgcolor;
				switch(es)
				{
				case element_state::hovered:
				case element_state::focus_hovered:
					bld_bgcolor = bgcolor.blend(highlighted, 0.2);
					bld_fgcolor = fgcolor.blend(highlighted, 0.2);
					break;
				case element_state::pressed:
					bld_bgcolor = bgcolor.blend(highlighted, 0.6);
					bld_fgcolor = fgcolor.blend(highlighted, 0.6);
					break;
				case element_state::disabled:
					bld_bgcolor = static_cast<color_rgb>(0xE0E0E0);
					bld_fgcolor = static_cast<color_rgb>(0x999A9E);
					break;
				default:
					//Leave things as they are
					break;
				}

				const int x = r.x + 1;
				const int y = r.y + 1;

				if(crook_data.radio)
				{
					graph.round_rectangle(rectangle{ x, y, 13, 13 }, 7, 7, bld_bgcolor, true, bld_bgcolor);
					graph.round_rectangle(rectangle{ x, y, 13, 13 }, 7, 7, bld_fgcolor, false, bld_fgcolor);

					if(crook_data.check_state == state::checked)
					{
						graph.round_rectangle(rectangle{ x + 3, y + 3, 7, 7 }, 4, 4, bld_fgcolor, true, bld_fgcolor);
					}
				}
				else
				{
					graph.rectangle(rectangle{ x + 1, y + 1, 11, 11 }, true, bld_bgcolor);
					graph.rectangle(rectangle{ x, y, 13, 13 }, false, bld_fgcolor);

					switch(crook_data.check_state)
					{
					case state::checked:
						{
							int sx = x + 2;
							int sy = y + 4;

							for(int i = 0; i < 3; i++)
							{
								sx++;
								sy++;
								graph.line(point{ sx, sy }, point{ sx, sy + 3 });
							}

							for(int i = 0; i < 4; i++)
							{
								sx++;
								sy--;
								graph.line(point{ sx, sy }, point{ sx, sy + 3 });
							}
						}
						break;
					case state::partial:
						graph.rectangle(rectangle{ x + 2, y + 2, 9, 9 }, true);
						break;
					default:
						break;
					}
				}
				return true;
			}
		};	//end class crook

		class menu_crook
			: public crook_interface
		{
			bool draw(graph_reference graph, const ::nana::color&, const ::nana::color& fgcolor, const nana::rectangle& r, element_state, const data& crook_data) override
			{
				if(crook_data.check_state == state::unchecked)
					return true;

				if(crook_data.radio)
				{
					int x = r.x + (static_cast<int>(r.width) - 8) / 2;
					int y = r.y + (static_cast<int>(r.height) - 8) / 2;

					graph.round_rectangle(rectangle{ x+1, y+1, 7, 7 }, 4, 4, fgcolor, true, fgcolor);
				}
				else
				{
					int x = r.x + (static_cast<int>(r.width) - 16) / 2;
					int y = r.y + (static_cast<int>(r.height) - 16) / 2;

					// graph.palette(false, fgcolor);
					// graph.line(point{ x + 3, y + 7 }, point{ x + 6, y + 10 });
					// graph.line(point{ x + 7, y + 9 }, point{ x + 12, y + 4 });

					// graph.palette(false, fgcolor.blend(colors::white, 0.5));
					// graph.line(point{ x + 3, y + 8 }, point{ x + 6, y + 11 });
					// graph.line(point{ x + 7, y + 10 }, point{ x + 12, y + 5 });
					// graph.line(point{ x + 4, y + 7 }, point{ x + 6, y + 9 });
					// graph.line(point{ x + 7, y + 8 }, point{ x + 11, y + 4 });

					int sx = x + 2;
					int sy = y + 4;

					for(int i = 0; i < 3; i++)
					{
						sx++;
						sy++;
						graph.line(point{ sx, sy }, point{ sx, sy + 3 }, fgcolor);
					}

					for(int i = 0; i < 4; i++)
					{
						sx++;
						sy--;
						graph.line(point{ sx, sy }, point{ sx, sy + 3 }, fgcolor);
					}
				}
				return true;
			}
		};

		class border_depressed
			: public border_interface
		{
			bool draw(graph_reference graph, const ::nana::color& bgcolor, const ::nana::color&, const ::nana::rectangle& r, element_state estate, unsigned)
			{
				graph.rectangle(r, false, static_cast<color_rgb>((element_state::focus_hovered == estate || element_state::focus_normal == estate) ? 0x0595E2 : 0x999A9E));
				graph.rectangle(::nana::rectangle(r).pare_off(1), false, bgcolor);
				return true;
			}
		};

		class arrow_solid_triangle
			: public arrow_interface
		{
			bool draw(graph_reference graph, const ::nana::color&, const ::nana::color&, const ::nana::rectangle& r, element_state, direction dir) override
			{
				::nana::point pos{ r.x + 3, r.y + 3 };
				switch (dir)
				{
				case ::nana::direction::east:
					pos.x += 3;
					pos.y += 1;
					for (int i = 0; i < 5; ++i)
						graph.line(point{ pos.x + i, pos.y + i }, point{ pos.x + i, pos.y + 8 - i });
					break;
				case ::nana::direction::south:
					pos.y += 3;
					for (int i = 0; i < 5; ++i)
						graph.line(point{ pos.x + i, pos.y + i }, point{ pos.x + 8 - i, pos.y + i });
					break;
				case ::nana::direction::west:
					pos.x += 5;
					pos.y += 1;
					for (int i = 0; i < 5; ++i)
						graph.line(point{ pos.x - i, pos.y + i }, point{ pos.x - i, pos.y + 8 - i });
					break;
				case ::nana::direction::north:
					pos.y += 7;
					for (int i = 0; i < 5; ++i)
						graph.line(point{ pos.x + i, pos.y - i }, point{ pos.x + 8 - i, pos.y - i });
					break;
				case direction::southeast:
					pos.x += 2;
					pos.y += 7;
					for (int i = 0; i < 6; ++i)
						graph.line(point{ pos.x + i, pos.y - i }, point{ pos.x + 5, pos.y - i });
					break;
				}
				return true;
			}
		};//end class arrow_solid_triangle

		class arrow_hollow_triangle
			: public arrow_interface
		{
			bool draw(graph_reference graph, const ::nana::color&, const ::nana::color&, const ::nana::rectangle& r, element_state, ::nana::direction dir) override
			{
				int x = r.x + 3;
				int y = r.y + 3;
				switch (dir)
				{
				case ::nana::direction::east:
					x += 3;
					graph.line(point{ x, y + 1 }, point{ x, y + 9 });
					graph.line(point{ x + 1, y + 2 }, point{ x + 4, y + 5 });
					graph.line(point{ x + 3, y + 6 }, point{ x + 1, y + 8 });
					break;
				case direction::southeast:
					x += 7;
					y += 6;
					graph.line(point{ x - 5 , y + 1 }, point{ x, y + 1 });
					graph.line(point{ x, y - 4 }, point{ x, y });
					graph.line(point{ x - 4, y }, point{ x - 1, y - 3 });
					break;
				case direction::south:
					y += 3;
					graph.line(point{ x, y }, point{ x + 8, y });
					graph.line(point{ x + 1, y + 1 }, point{ x + 4, y + 4 });
					graph.line(point{ x + 7, y + 1 }, point{ x + 5, y + 3 });
					break;
				case direction::west:
					x += 5;
					y += 1;
					graph.line(point{ x, y }, point{ x, y + 8 });
					graph.line(point{ x - 4, y + 4 }, point{ x - 1, y + 1 });
					graph.line(point{ x - 3, y + 5 }, point{ x - 1, y + 7 });
					break;
				case direction::north:
					y += 7;
					graph.line(point{ x, y }, point{ x + 8, y });
					graph.line(point{ x + 1, y - 1 }, point{ x + 4, y - 4 });
					graph.line(point{ x + 5, y - 3 }, point{ x + 7, y - 1 });
					break;
				}
				return true;
			}
		};//end class arrow_hollow_triangle

		class arrowhead
			: public arrow_interface
		{
			bool draw(graph_reference graph, const ::nana::color&, const ::nana::color&, const ::nana::rectangle& r, element_state, ::nana::direction dir) override
			{
				int x = r.x;
				int y = r.y + 5;
				switch (dir)
				{
				case direction::north:
				{
					x += 3;
					int pixels = 1;
					for (int l = 0; l < 4; ++l)
					{
						for (int i = 0; i < pixels; ++i)
						{
							if (l == 3 && i == 3)
								continue;
							graph.set_pixel(x + i, y);
						}

						x--;
						y++;
						pixels += 2;
					}

					graph.set_pixel(x + 1, y);
					graph.set_pixel(x + 2, y);
					graph.set_pixel(x + 6, y);
					graph.set_pixel(x + 7, y);
				}
				break;
				case direction::south:
				{
					graph.set_pixel(x, y);
					graph.set_pixel(x + 1, y);
					graph.set_pixel(x + 5, y);
					graph.set_pixel(x + 6, y);

					++y;
					int pixels = 7;
					for (int l = 0; l < 4; ++l)
					{
						for (int i = 0; i < pixels; ++i)
						{
							if (l != 0 || i != 3)
								graph.set_pixel(x + i, y);
						}

						x++;
						y++;
						pixels -= 2;
					}
				}
				default:break;
				}
				return true;
			}
		};//end class arrowhead

		class arrow_double
			: public arrow_interface
		{
			bool draw(graph_reference graph, const ::nana::color&, const ::nana::color&, const ::nana::rectangle& r, element_state, ::nana::direction dir) override
			{
				int x = r.x;
				int y = r.y;
				switch (dir)
				{
				case direction::east:
					_m_line(graph, x + 4, y + 6, true);
					_m_line(graph, x + 5, y + 7, true);
					_m_line(graph, x + 6, y + 8, true);
					_m_line(graph, x + 5, y + 9, true);
					_m_line(graph, x + 4, y + 10, true);
					break;
				case direction::west:
					_m_line(graph, x + 5, y + 6, true);
					_m_line(graph, x + 4, y + 7, true);
					_m_line(graph, x + 3, y + 8, true);
					_m_line(graph, x + 4, y + 9, true);
					_m_line(graph, x + 5, y + 10, true);
					break;
				case direction::south:
					_m_line(graph, x + 5, y + 4, false);
					_m_line(graph, x + 6, y + 5, false);
					_m_line(graph, x + 7, y + 6, false);
					_m_line(graph, x + 8, y + 5, false);
					_m_line(graph, x + 9, y + 4, false);
					break;
				case direction::north:
					_m_line(graph, x + 5, y + 6, false);
					_m_line(graph, x + 6, y + 5, false);
					_m_line(graph, x + 7, y + 4, false);
					_m_line(graph, x + 8, y + 5, false);
					_m_line(graph, x + 9, y + 6, false);
					break;
				default:
					break;
				}
				return true;
			}

			static void _m_line(nana::paint::graphics & graph, int x, int y, bool horizontal)
			{
				graph.set_pixel(x, y);
				if (horizontal)
				{
					graph.set_pixel(x + 1, y);
					graph.set_pixel(x + 4, y);
					graph.set_pixel(x + 5, y);
				}
				else
				{
					graph.set_pixel(x, y + 1);
					graph.set_pixel(x, y + 4);
					graph.set_pixel(x, y + 5);
				}
			}
		};//end class arrow_double

		class annex_button
			: public element_interface
		{
			bool draw(graph_reference graph, const ::nana::color& arg_bgcolor, const ::nana::color&, const rectangle& r, element_state estate) override
			{
				auto bgcolor = arg_bgcolor;

				switch (estate)
				{
				case element_state::hovered:
				case element_state::focus_hovered:
					bgcolor = arg_bgcolor.blend(colors::white, 0.2);
					break;
				case element_state::pressed:
					bgcolor = arg_bgcolor.blend(colors::black, 0.2);
					break;
				case element_state::disabled:
					bgcolor = colors::dark_gray;
				default:
					break;
				}

				auto part_px = (r.height - 3) * 5 / 13;
				graph.rectangle(r, false, bgcolor.blend(colors::black, 0.4));
				
				::nana::point left_top{ r.x + 1, r.y + 1 }, right_top{r.right() - 2, r.y + 1};
				::nana::point left_mid{ r.x + 1, r.y + 1 + static_cast<int>(part_px) }, right_mid{ right_top.x, left_mid.y };
				::nana::point left_bottom{ r.x + 1, r.bottom() - 2 }, right_bottom{ r.right() - 2, r.bottom() - 2 };

				graph.palette(false, bgcolor.blend(colors::white, 0.1));
				graph.line(left_top, left_mid);
				graph.line(right_top, right_mid);

				graph.palette(false, bgcolor.blend(colors::white, 0.5));
				graph.line(left_top, right_top);

				left_mid.y++;
				right_mid.y++;
				graph.palette(false, bgcolor.blend(colors::black, 0.2));
				graph.line(left_mid, left_bottom);
				graph.line(right_mid, right_bottom);

				::nana::rectangle part_r{ r.x + 2, r.y + 2, r.width - 4, part_px };
				graph.rectangle(part_r, true, bgcolor.blend(colors::white, 0.2));

				part_r.y += static_cast<int>(part_r.height);
				part_r.height = (r.height - 3 - part_r.height);
				graph.rectangle(part_r, true, bgcolor);
				return true;
			}
		};//end class annex_button

		class x_icon
			: public element_interface
		{
			bool draw(graph_reference graph, const ::nana::color&, const ::nana::color& fgcolor, const rectangle& r, element_state estate) override
			{
				auto clr = fgcolor;

				switch (estate)
				{
				case element_state::hovered:
				case element_state::pressed:
					clr = clr.blend(colors::black, 0.2);
					break;
				case element_state::disabled:
					clr = colors::dark_gray;
				default:
					break;
				}

				graph.palette(false, clr);

				const int x = r.x + 4;
				const int y = r.y + 4;

				point p1{ x, y }, p2{ x + 7, y + 7 };

				graph.line(p1, p2);

				++p1.x;
				--p2.y;
				graph.line(p1, p2);

				p1.x = x;
				++p1.y;
				p2.x = x + 6;
				p2.y = y + 7;
				graph.line(p1, p2);

				p1.x += 7;
				p1.y = y;
				p2.x = x;
				graph.line(p1, p2);

				p1.x = x + 6;
				p2.y = y + 6;
				graph.line(p1, p2);

				++p1.x;
				++p1.y;
				++p2.x;
				++p2.y;
				graph.line(p1, p2);

				return true;
			}
		};
	}//end namespace element

	template<typename ElementInterface>
	class element_object
		: nana::noncopyable, nana::nonmovable
	{
		using element_type		= ElementInterface;
		using factory_interface = pat::cloneable<element::detail::factory_abstract>;
	public:
		~element_object()
		{
			if(factory_)
				factory_->destroy(element_ptr_);
		}

		void push(const factory_interface& rhs)
		{
			auto keep_f = factory_;
			auto keep_e = element_ptr_;

			factory_ = rhs;
			element_ptr_ = static_cast<element_type*>(static_cast<element::provider::factory_interface<element_type>&>(*factory_).create());

			if(nullptr == factory_ || nullptr == element_ptr_)
			{
				if(element_ptr_)
					factory_->destroy(element_ptr_);

				factory_.reset();

				factory_ = keep_f;
				element_ptr_ = keep_e;
			}
			else
				spare_.emplace_back(keep_e, keep_f);
		}

		element_type * const * cite() const
		{
			return &element_ptr_;
		}
	private:
		factory_interface factory_;	//Keep the factory for destroying the element
		element_type * element_ptr_{nullptr};
		std::vector<std::pair<element_type*, factory_interface>> spare_;
	};

	class element_manager
		: nana::noncopyable, nana::nonmovable
	{
		template<typename ElementInterface>
		struct item
		{
			element_object<ElementInterface> * employee;
			std::map<std::string, std::shared_ptr<element_object<ElementInterface>>> table;
		};

		element_manager()
		{
			crook_.employee = nullptr;
			border_.employee = nullptr;
		}

	public:
		static element_manager& instance()
		{
			static bool initial = true;
			static element_manager obj;
			if(initial)
			{
				initial = false;

				element::add_crook<element::crook>("");
				element::add_crook<element::menu_crook>("menu_crook");

				element::add_border<element::border_depressed>("");
				
				element::add_arrow<element::arrowhead>("");				//"arrowhead" in default
				element::add_arrow<element::arrow_double>("double");
				element::add_arrow<element::arrow_solid_triangle>("solid_triangle");
				element::add_arrow<element::arrow_hollow_triangle>("hollow_triangle");

				element::add_button<element::annex_button>("");	//"annex" in default

				element::add_x_icon<element::x_icon>("");
			}
			return obj;
		}

		void crook(const std::string& name, const pat::cloneable<element::provider::factory_interface<element::crook_interface>>& factory)
		{
			_m_add(name, crook_, factory);
		}

		element::crook_interface * const * crook(const std::string& name) const
		{
			return _m_get(name, crook_).cite();
		}

		void cross(const std::string& name, const pat::cloneable<element::provider::factory_interface<element::element_interface>>& factory)
		{
			_m_add(name, cross_, factory);
		}

		element::element_interface* const * cross(const std::string& name) const
		{
			return _m_get(name, cross_).cite();
		}

		void border(const std::string& name, const pat::cloneable<element::provider::factory_interface<element::border_interface>>& factory)
		{
			_m_add(name, border_, factory);
		}

		element::border_interface * const * border(const std::string& name) const
		{
			return _m_get(name, border_).cite();
		}

		void arrow(const std::string& name, const pat::cloneable<element::provider::factory_interface<element::arrow_interface>>& factory)
		{
			_m_add((name.empty() ? "arrowhead" : name), arrow_, factory);
		}

		element::arrow_interface * const * arrow(const std::string& name) const
		{
			return _m_get((name.empty() ? "arrowhead" : name), arrow_).cite();
		}

		void button(const std::string& name, const pat::cloneable<element::provider::factory_interface<element::element_interface>>& factory)
		{
			_m_add((name.empty() ? "annex" : name), button_, factory);
		}

		element::element_interface * const * button(const std::string& name) const
		{
			return _m_get((name.empty() ? "annex" : name), button_).cite();
		}

		void x_icon(const std::string& name, const pat::cloneable<element::provider::factory_interface<element::element_interface>>& factory)
		{
			_m_add(name, x_icon_, factory);
		}

		element::element_interface * const * x_icon(const std::string& name) const
		{
			return _m_get(name, x_icon_).cite();
		}
	private:
		using lock_guard = std::lock_guard<std::recursive_mutex>;

		template<typename ElementInterface>
		void _m_add(const std::string& name, item<ElementInterface>& m, const pat::cloneable<element::provider::factory_interface<ElementInterface>>& factory)
		{
			typedef element_object<ElementInterface> element_object_t;
			lock_guard lock(mutex_);

			auto & eop = m.table[name];
			if(nullptr == eop)
				eop = std::make_shared<element_object_t>();

			eop->push(factory);
			if(nullptr == m.employee)
				m.employee = eop.get();
		}

		template<typename ElementInterface>
		const element_object<ElementInterface>& _m_get(const std::string& name, const item<ElementInterface>& m) const
		{
			lock_guard lock(mutex_);

			auto i = m.table.find(name);
			if(i != m.table.end())
				return *(i->second);

			return *m.employee;
		}

	private:
		mutable std::recursive_mutex mutex_;
		item<element::crook_interface>	crook_;
		item<element::element_interface> cross_;
		item<element::border_interface>	border_;
		item<element::arrow_interface>	arrow_;
		item<element::element_interface>	button_;
		item<element::element_interface>	x_icon_;
	};

	namespace element
	{
		//class provider
		void provider::add_crook(const std::string& name, const pat::cloneable<factory_interface<crook_interface>>& factory)
		{
			element_manager::instance().crook(name, factory);
		}

		crook_interface* const * provider::cite_crook(const std::string& name)
		{
			return element_manager::instance().crook(name);
		}

		void provider::add_cross(const std::string& name, const pat::cloneable<factory_interface<element_interface>>& factory)
		{
			element_manager::instance().cross(name, factory);
		}

		element_interface* const* provider::cite_cross(const std::string& name)
		{
			return element_manager::instance().cross(name);
		}

		void provider::add_border(const std::string& name, const pat::cloneable<factory_interface<border_interface>>& factory)
		{
			element_manager::instance().border(name, factory);
		}

		border_interface* const * provider::cite_border(const std::string& name)
		{
			return element_manager::instance().border(name);
		}

		void provider::add_arrow(const std::string& name, const pat::cloneable<factory_interface<arrow_interface>>& factory)
		{
			element_manager::instance().arrow(name, factory);
		}

		arrow_interface* const * provider::cite_arrow(const std::string& name)
		{
			return element_manager::instance().arrow(name);
		}

		void provider::add_button(const std::string& name, const pat::cloneable<factory_interface<element_interface>>& factory)
		{
			element_manager::instance().button(name, factory);
		}

		element_interface* const* provider::cite_button(const std::string& name)
		{
			return element_manager::instance().button(name);
		}

		void provider::add_x_icon(const std::string& name, const pat::cloneable<factory_interface<element_interface>>& factory)
		{
			element_manager::instance().x_icon(name, factory);
		}

		element_interface* const* provider::cite_x_icon(const std::string& name)
		{
			return element_manager::instance().x_icon(name);
		}
	}//end namespace element

	//facades
	//template<> class facade<element::crook>
		facade<element::crook>::facade(const char* name)
			:	cite_(element::provider().cite_crook(name ? name : ""))
		{
			data_.check_state = state::unchecked;
			data_.radio = false;
		}

		facade<element::crook> & facade<element::crook>::reverse()
		{
			data_.check_state = (data_.check_state == facade<element::crook>::state::unchecked ? facade<element::crook>::state::checked : facade<element::crook>::state::unchecked);
			return *this;
		}

		facade<element::crook> & facade<element::crook>::check(state s)
		{
			data_.check_state = s;
			return *this;
		}

		facade<element::crook>::state facade<element::crook>::checked() const
		{
			return data_.check_state;
		}

		facade<element::crook> & facade<element::crook>::radio(bool r)
		{
			data_.radio = r;
			return *this;
		}

		bool facade<element::crook>::radio() const
		{
			return data_.radio;
		}

		void facade<element::crook>::switch_to(const char* name)
		{
			cite_ = element::provider().cite_crook(name ? name : "");
		}

		bool facade<element::crook>::draw(graph_reference graph, const ::nana::color& bgcol, const ::nana::color& fgcol, const nana::rectangle& r, element_state es)
		{
			return (*cite_)->draw(graph, bgcol, fgcol, r, es, data_);
		}
	//end class facade<element::crook>

	//class facade<element::cross>
		facade<element::cross>::facade(const char* name)
			: cite_(element::provider().cite_cross(name ? name : ""))
		{
		
		}

		void facade<element::cross>::switch_to(const char* name)
		{
			cite_ = element::provider().cite_cross(name ? name : "");
		}

		void facade<element::cross>::thickness(unsigned thk)
		{
			thickness_ = thk;
		}

		void facade<element::cross>::size(unsigned size_px)
		{
			size_ = size_px;
		}

		//Implement element_interface
		bool facade<element::cross>::draw(graph_reference graph, const ::nana::color&, const ::nana::color& fgcolor, const ::nana::rectangle& r, element_state)
		{
			if (thickness_ + 2 <= size_)
			{
				int gap = (static_cast<int>(size_) - static_cast<int>(thickness_)) / 2;

				nana::point ps[12];
				ps[0].x = r.x + gap;
				ps[1].x = ps[0].x + static_cast<int>(thickness_) - 1;
				ps[1].y = ps[0].y = r.y;

				ps[2].x = ps[1].x;
				ps[2].y = r.y + gap;

				ps[3].x = ps[2].x + gap;
				ps[3].y = ps[2].y;

				ps[4].x = ps[3].x;
				ps[4].y = ps[3].y + static_cast<int>(thickness_)-1;

				ps[5].x = ps[1].x;
				ps[5].y = ps[4].y;

				ps[6].x = ps[5].x;
				ps[6].y = ps[5].y + gap;

				ps[7].x = r.x + gap;
				ps[7].y = ps[6].y;

				ps[8].x = ps[7].x;
				ps[8].y = ps[4].y;

				ps[9].x = r.x;
				ps[9].y = ps[4].y;

				ps[10].x = r.x;
				ps[10].y = r.y + gap;

				ps[11].x = r.x + gap;
				ps[11].y = r.y + gap;

				graph.palette(false, fgcolor.blend(colors::black, 1.0 - fgcolor.a()));

				for (int i = 0; i < 11; ++i)
					graph.line(ps[i], ps[i + 1]);
				graph.line(ps[11], ps[0]);

				graph.palette(false, fgcolor);

				unsigned thk_minus_2 = thickness_ - 2;
				graph.rectangle(rectangle{ ps[10].x + 1, ps[10].y + 1, (gap << 1) + thk_minus_2, thk_minus_2 }, true);
				graph.rectangle(rectangle{ ps[0].x + 1, ps[0].y + 1, thk_minus_2, (gap << 1) + thk_minus_2 }, true);
			}
			return true;
		}
	//end class facade<element::cross>

	//class facade<element::border>
		facade<element::border>::facade(const char* name)
			: cite_(element::provider().cite_border(name ? name : ""))
		{}

		void facade<element::border>::switch_to(const char* name)
		{
			cite_ = element::provider().cite_border(name ? name : "");
		}

		bool facade<element::border>::draw(graph_reference graph, const nana::color& bgcolor, const nana::color& fgcolor, const nana::rectangle& r, element_state es)
		{
			return (*cite_)->draw(graph, bgcolor, fgcolor, r, es, 2);
		}
	//end class facade<element::border>

	//class facade<element::arrow>
		facade<element::arrow>::facade(const char* name)
			: cite_(element::provider().cite_arrow(name ? name : ""))
		{
		}

		void facade<element::arrow>::switch_to(const char* name)
		{
			cite_ = element::provider().cite_arrow(name ? name : "");
		}

		void facade<element::arrow>::direction(::nana::direction dir)
		{
			dir_ = dir;
		}

		//Implement element_interface
		bool facade<element::arrow>::draw(graph_reference graph, const nana::color& bgcolor, const nana::color& fgcolor, const nana::rectangle& r, element_state estate)
		{
			graph.palette(false, fgcolor);
			return (*cite_)->draw(graph, bgcolor, fgcolor, r, estate, dir_);
		}
	//end class facade<element::arrow>

	//class facade<element::button>::
		facade<element::button>::facade(const char* name)
			: cite_(element::provider().cite_button(name ? name : ""))
		{}

		void facade<element::button>::switch_to(const char* name)
		{
			cite_ = element::provider().cite_button(name ? name : "");
		}

		//Implement element_interface
		bool facade<element::button>::draw(graph_reference graph, const ::nana::color& bgcolor, const ::nana::color& fgcolor, const ::nana::rectangle& r, element_state estate)
		{
			return (*cite_)->draw(graph, bgcolor, fgcolor, r, estate);
		}
	//end class facade<element::button>


	//class facade<element::x_icon>
		facade<element::x_icon>::facade(const char* name)
			: cite_(element::provider().cite_x_icon(name ? name : ""))
		{}

		void facade<element::x_icon>::switch_to(const char* name)
		{
			cite_ = element::provider().cite_x_icon(name ? name : "");
		}

		//Implement element_interface
		bool facade<element::x_icon>::draw(graph_reference graph, const ::nana::color& bgcolor, const ::nana::color& fgcolor, const ::nana::rectangle& r, element_state estate)
		{
			return (*cite_)->draw(graph, bgcolor, fgcolor, r, estate);
		}
	//end class facade<element::x_icon>

	namespace element
	{
		using brock = ::nana::detail::bedrock;

		void set_bground(const char* name, const pat::cloneable<element_interface>& obj)
		{
			brock::instance().get_element_store().bground(name, obj);
		}

		void set_bground(const char* name, pat::cloneable<element_interface> && obj)
		{
			brock::instance().get_element_store().bground(name, std::move(obj));
		}

		//class cite
		cite_bground::cite_bground(const char* name)
			: ref_ptr_(brock::instance().get_element_store().bground(name))
		{
		}

		void cite_bground::set(const cloneable_element& rhs)
		{
			holder_ = rhs;
			place_ptr_ = holder_.get();
			ref_ptr_ = &place_ptr_;
		}

		void cite_bground::set(const char* name)
		{
			holder_.reset();
			ref_ptr_ = brock::instance().get_element_store().bground(name);
		}

		bool cite_bground::draw(graph_reference dst, const ::nana::color& bgcolor, const ::nana::color& fgcolor, const nana::rectangle& r, element_state state)
		{
			if (ref_ptr_ && *ref_ptr_)
				return (*ref_ptr_)->draw(dst, bgcolor, fgcolor, r, state);

			return false;
		}
		//end class cite

		//class bground
		struct bground::draw_method
		{
			virtual ~draw_method(){}

			virtual draw_method * clone() const = 0;

			virtual void paste(const nana::rectangle& from_r, graph_reference, const nana::point& dst_pos) = 0;
			virtual void stretch(const nana::rectangle& from_r, graph_reference dst, const nana::rectangle & to_r) = 0;
		};

		struct bground::draw_image
			: public draw_method
		{
			nana::paint::image image;

			draw_image(const nana::paint::image& img)
				: image(img)
			{}

			draw_method * clone() const override
			{
				return new draw_image(image);
			}

			void paste(const nana::rectangle& from_r, graph_reference dst, const nana::point& dst_pos) override
			{
				image.paste(from_r, dst, dst_pos);
			}

			void stretch(const nana::rectangle& from_r, graph_reference dst, const nana::rectangle& to_r) override
			{
				image.stretch(from_r, dst, to_r);
			}
		};

		struct bground::draw_graph
			: public draw_method
		{
			nana::paint::graphics graph;

			draw_graph()
			{}

			draw_graph(const nana::paint::graphics& g)
				: graph(g)
			{}

			draw_method * clone() const override
			{
				auto p = new draw_graph;
				p->graph.make(graph.size());
				graph.paste(p->graph, 0, 0);
				return p;
			}

			void paste(const nana::rectangle& from_r, graph_reference dst, const nana::point& dst_pos) override
			{
				graph.paste(from_r, dst, dst_pos.x, dst_pos.y);
			}

			void stretch(const nana::rectangle& from_r, graph_reference dst, const nana::rectangle& to_r) override
			{
				graph.stretch(from_r, dst, to_r);
			}
		};


		struct bground::implementation
		{
			draw_method * method{ nullptr };

			bool		vert{ false };
			rectangle	valid_area;
			std::vector<element_state> states;
			std::map<element_state, element_state> join;

			bool		stretch_all{ true };
			unsigned	left{ 0 }, top{ 0 }, right{ 0 }, bottom{ 0 };
		};


		bground::bground()
			: impl_{ new implementation }
		{
			reset_states();
		}

		bground::bground(const bground& rhs)
			: impl_{ new implementation(*rhs.impl_) }
		{
			if (impl_->method)
				impl_->method = impl_->method->clone();
		}

		bground::~bground()
		{
			delete impl_->method;
		}

		bground& bground::operator=(const bground& rhs)
		{
			if (this != &rhs)
			{
				delete impl_->method;

				impl_.reset(new implementation(*rhs.impl_));
				if (impl_->method)
					impl_->method = impl_->method->clone();
			}
			return *this;
		}

		//Set a picture for the background
		bground& bground::image(const paint::image& img, bool vertical, const nana::rectangle& valid_area)
		{
			delete impl_->method;
			impl_->method = new draw_image(img);
			impl_->vert = vertical;

			if (valid_area.width && valid_area.height)
				impl_->valid_area = valid_area;
			else
				impl_->valid_area = nana::rectangle(img.size());
			return *this;
		}

		bground& bground::image(const paint::graphics& graph, bool vertical, const nana::rectangle& valid_area)
		{
			delete impl_->method;
			impl_->method = new draw_graph(graph);
			impl_->vert = vertical;

			if (valid_area.width && valid_area.height)
				impl_->valid_area = valid_area;
			else
				impl_->valid_area = nana::rectangle(graph.size());
			return *this;
		}

		//Set the state sequence of the background picture.
		void bground::states(const std::vector<element_state> & s)
		{
			impl_->states = s;
		}

		void bground::states(std::vector<element_state> && s)
		{
			impl_->states = std::move(s);
		}

		void bground::reset_states()
		{
			auto & st = impl_->states;

			st.clear();
			st.push_back(element_state::normal);
			st.push_back(element_state::hovered);
			st.push_back(element_state::focus_normal);
			st.push_back(element_state::focus_hovered);
			st.push_back(element_state::pressed);
			st.push_back(element_state::disabled);
			impl_->join.clear();
		}

		void bground::join(element_state target, element_state joiner)
		{
			impl_->join[joiner] = target;
		}

		void bground::stretch_parts(unsigned left, unsigned top, unsigned right, unsigned bottom)
		{
			impl_->left = left;
			impl_->right = right;
			impl_->top = top;
			impl_->bottom = bottom;

			impl_->stretch_all = !(left || right || top || bottom);
		}

		//Implement the methods of bground_interface.
		bool bground::draw(graph_reference dst, const ::nana::color&, const ::nana::color&, const nana::rectangle& to_r, element_state state)
		{
			const auto method = impl_->method;

			if (nullptr == method)
				return false;

			auto mi = impl_->join.find(state);
			if (mi != impl_->join.end())
				state = mi->second;

			std::size_t pos = 0;
			for (; pos < impl_->states.size(); ++pos)
			{
				if (impl_->states[pos] == state)
					break;
			}

			if (pos == impl_->states.size())
				return false;

			nana::rectangle from_r = impl_->valid_area;
			if (impl_->vert)
			{
				from_r.height /= static_cast<unsigned>(impl_->states.size());
				from_r.y += static_cast<int>(from_r.height * pos);
			}
			else
			{
				from_r.width /= static_cast<unsigned>(impl_->states.size());
				from_r.x += static_cast<int>(from_r.width * pos);
			}

			if (impl_->stretch_all)
			{
				if (from_r.width == to_r.width && from_r.height == to_r.height)
					method->paste(from_r, dst, to_r.position());
				else
					method->stretch(from_r, dst, to_r);

				return true;
			}


			auto perf_from_r = from_r;
			auto perf_to_r = to_r;

			const auto left = impl_->left;
			const auto right = impl_->right;
			const auto top = impl_->top;
			const auto bottom = impl_->bottom;

			if (left + right < to_r.width)
			{
				nana::rectangle src_r = from_r;
				src_r.y += static_cast<int>(top);
				src_r.height -= top + bottom;

				nana::rectangle dst_r = to_r;
				dst_r.y += static_cast<int>(top);
				dst_r.height -= top + bottom;

				if (left)
				{
					src_r.width = left;
					dst_r.width = left;

					method->stretch(src_r, dst, dst_r);

					perf_from_r.x += static_cast<int>(left);
					perf_from_r.width -= left;
					perf_to_r.x += static_cast<int>(left);
					perf_to_r.width -= left;
				}

				if (right)
				{
					src_r.x += (static_cast<int>(from_r.width) - static_cast<int>(right));
					src_r.width = right;

					dst_r.x += (static_cast<int>(to_r.width) - static_cast<int>(right));
					dst_r.width = right;

					method->stretch(src_r, dst, dst_r);

					perf_from_r.width -= right;
					perf_to_r.width -= right;
				}
			}

			if (top + bottom < to_r.height)
			{
				nana::rectangle src_r = from_r;
				src_r.x += static_cast<int>(left);
				src_r.width -= left + right;

				nana::rectangle dst_r = to_r;
				dst_r.x += static_cast<int>(left);
				dst_r.width -= left + right;

				if (top)
				{
					src_r.height = top;
					dst_r.height = top;

					method->stretch(src_r, dst, dst_r);

					perf_from_r.y += static_cast<int>(top);
					perf_to_r.y += static_cast<int>(top);
				}

				if (bottom)
				{
					src_r.y += static_cast<int>(from_r.height - bottom);
					src_r.height = bottom;

					dst_r.y += static_cast<int>(to_r.height - bottom);
					dst_r.height = bottom;

					method->stretch(src_r, dst, dst_r);
				}

				perf_from_r.height -= (top + bottom);
				perf_to_r.height -= (top + bottom);
			}

			if (left)
			{
				nana::rectangle src_r = from_r;
				src_r.width = left;
				if (top)
				{
					src_r.height = top;
					method->paste(src_r, dst, to_r.position());
				}
				if (bottom)
				{
					src_r.y += static_cast<int>(from_r.height) - static_cast<int>(bottom);
					src_r.height = bottom;
					method->paste(src_r, dst, nana::point(to_r.x, to_r.y + static_cast<int>(to_r.height - bottom)));
				}
			}

			if (right)
			{
				const int to_x = to_r.x + int(to_r.width - right);

				nana::rectangle src_r = from_r;
				src_r.x += static_cast<int>(src_r.width) - static_cast<int>(right);
				src_r.width = right;
				if (top)
				{
					src_r.height = top;
					method->paste(src_r, dst, nana::point(to_x, to_r.y));
				}
				if (bottom)
				{
					src_r.y += (static_cast<int>(from_r.height) - static_cast<int>(bottom));
					src_r.height = bottom;
					method->paste(src_r, dst, nana::point(to_x, to_r.y + int(to_r.height - bottom)));
				}
			}

			method->stretch(perf_from_r, dst, perf_to_r);
			return true;
		}
		//end class bground
	}//end namespace element
}//end namespace nana
