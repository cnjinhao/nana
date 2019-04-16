/*
*	A Menubar implementation
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2009-2018 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file: nana/gui/widgets/menubar.cpp
*/

#include <nana/gui/widgets/menubar.hpp>
#include <stdexcept>

namespace nana
{
	class menu_accessor
	{
	public:
		static void popup(menu& m, window wd, int x, int y)
		{
			m._m_popup(wd, { x, y }, true);
		}
	};

	namespace drawerbase
	{
		namespace menubar
		{
			struct item_type
			{
				item_type(std::string&& text, wchar_t shortkey, std::size_t shortkey_pos):
					text(std::move(text)),
					shortkey(shortkey),
					shortkey_pos(shortkey_pos)
				{}

				std::string		text;	///< Transformed text, the shortkey character has been processed.
				wchar_t			shortkey;
				std::size_t		shortkey_pos;
				::nana::menu	menu_obj;
				::nana::point	pos;
				::nana::size	size;
			};

			struct trigger::essence
			{
				enum class behavior
				{
					none, focus, menu,
				};


				widget *widget_ptr{ nullptr };
				std::vector<item_type*> items;

				struct state_type
				{
					std::size_t active{ nana::npos };
					behavior behave{ behavior::none };

					bool menu_active{ false };
					bool passive_close{ true };

					bool nullify_mouse{ false };

					nana::menu *menu{ nullptr };
					nana::point mouse_pos;
				}state;

				//functions

				~essence()
				{
					for (auto p : items)
						delete p;
				}

				nana::menu& push_back(const std::string& text)
				{
					wchar_t shortkey;
					std::size_t shortkey_pos;
					auto transformed_text = API::transform_shortkey_text(text, shortkey, &shortkey_pos);

					if (shortkey)
						API::register_shortkey(*widget_ptr, shortkey);


					if (shortkey && shortkey < 0x61)
						shortkey += (0x61 - 0x41);

#ifdef _nana_std_has_emplace_return_type
					auto & last = items.emplace_back(new item_type{std::move(transformed_text), shortkey, shortkey_pos});
					API::refresh_window(*widget_ptr);
					return last->menu_obj;
#else
					items.emplace_back(new item_type{ std::move(transformed_text), shortkey, shortkey_pos });

					API::refresh_window(*widget_ptr);

					return this->items.back()->menu_obj;
#endif
				}
				
				bool cancel()
				{
					if (nana::npos == state.active)
						return false;

					this->close_menu();
					state.behave = behavior::none;

					auto pos = API::cursor_position();
					API::calc_window_point(widget_ptr->handle(), pos);
					state.active = find(pos);

					return true;
				}


				bool open_menu(bool activate_menu = false)
				{
					if (activate_menu)
						state.menu_active = true;

					auto pos = state.active;

					if (pos >= items.size())
						return false;

					auto item_ptr = items[pos];

					if (state.menu_active && (state.menu != &(item_ptr->menu_obj)))
					{
						API::dev::delay_restore(true);
						this->close_menu();
						API::dev::delay_restore(false);
						state.active = pos;

						//Resets this flag, because close_menu() deactivates the menu
						state.menu_active = true;

						state.menu = &(item_ptr->menu_obj);
						state.menu->destroy_answer([this]
						{
							state.menu = nullptr;
							if (state.passive_close)
							{
								cancel();
								API::refresh_window(*widget_ptr);
							}
						});

						if (API::focus_window() != this->widget_ptr->handle())
							API::focus_window(widget_ptr->handle());
						menu_accessor::popup(*state.menu, *widget_ptr, item_ptr->pos.x, item_ptr->pos.y + static_cast<int>(item_ptr->size.height));
						return true;
					}
					return false;
				}

				bool close_menu()
				{
					state.menu_active = false;

					if (state.menu)
					{
						state.passive_close = false;
						state.menu->close();
						state.passive_close = true;
						state.menu = nullptr;
						return true;
					}
					return false;
				}


				std::size_t find(wchar_t shortkey) const
				{
					if (shortkey)
					{
						if (shortkey < 0x61) shortkey += (0x61 - 0x41);

						std::size_t index = 0;
						for (auto item_ptr : items)
						{
							if (item_ptr->shortkey == shortkey)
								return index;
							++index;
						}
					}
					return npos;
				}

				std::size_t find(const ::nana::point& pos)
				{
					if ((2 <= pos.x) && (2 <= pos.y) && (pos.y < 25))
					{
						int item_x = 2;
						std::size_t index = 0;

						for (auto item_ptr : items)
						{
							if ((item_x <= pos.x) && (pos.x < item_x + static_cast<int>(item_ptr->size.width)))
								return index;

							item_x += static_cast<int>(item_ptr->size.width);
							++index;
						}
					}

					return npos;
				}
			};

			//class item_renderer
			class item_renderer
			{
			public:
				enum class state
				{
					normal, highlighted, selected
				};

				using graph_reference = paint::graphics&;
				using scheme = ::nana::drawerbase::menubar::scheme;

				item_renderer(window, graph_reference);
				virtual void background(const point&, const ::nana::size&, state);
				virtual void caption(const point&, const native_string_type&);
				scheme *scheme_ptr() const { return scheme_ptr_; };
			private:
				graph_reference graph_;
				scheme *scheme_ptr_;
			};

			item_renderer::item_renderer(window wd, graph_reference graph)
				:graph_(graph), scheme_ptr_(static_cast<scheme*>(API::dev::get_scheme(wd)))
			{}

			void item_renderer::background(const nana::point& pos, const nana::size& size, state item_state)
			{
					auto bground = scheme_ptr_->text_fgcolor;
					::nana::color border, body;

					switch (item_state)
					{
					case state::highlighted:
						border = scheme_ptr_->border_highlight;
						body = scheme_ptr_->body_highlight;
						break;
					case state::selected:
						border = scheme_ptr_->border_selected;
						body = scheme_ptr_->body_selected;
						break;
					default:	//Don't process other states.
						return;
					}

					nana::rectangle r(pos, size);
					graph_.rectangle(r, false, border);

					graph_.palette(false, body.blend(bground, 0.5));

					paint::draw{ graph_ }.corner(r, 1);
					graph_.rectangle(r.pare_off(1), true, body);
			}

			void item_renderer::caption(const point& pos, const native_string_type& text)
			{
				graph_.string(pos, text, scheme_ptr_->text_fgcolor);
			}
			//end class item_renderer

			//class trigger
				trigger::trigger()
					: ess_(new essence)
				{}

				trigger::~trigger()
				{
					delete ess_;
				}

				auto trigger::ess() const -> essence&
				{
					return *ess_;
				}

				void trigger::attached(widget_reference widget, graph_reference)
				{
					ess_->widget_ptr = &widget;
				}

				void trigger::refresh(graph_reference graph)
				{
					auto bgcolor = API::bgcolor(*ess_->widget_ptr);

					graph.rectangle(true, bgcolor);

					item_renderer ird{ *ess_->widget_ptr, graph };

					nana::point item_pos(2, 2);
					nana::size item_s(0, 23);

					unsigned long index = 0;

					for (auto pm : ess_->items)
					{
						auto text_s = graph.text_extent_size(pm->text);
						item_s.width = text_s.width + 16;

						pm->pos = item_pos;
						pm->size = item_s;

						using state = item_renderer::state;
						state item_state = (index != ess_->state.active ? state::normal : (ess_->state.menu_active ? state::selected : state::highlighted));
						ird.background(item_pos, item_s, item_state);

						if (state::selected == item_state)
						{
							int x = item_pos.x + item_s.width;
							int y1 = item_pos.y + 2, y2 = item_pos.y + item_s.height - 1;
							graph.line({ x, y1 }, { x, y2 }, bgcolor.blend(colors::gray_border, 0.6));
							graph.line({ x + 1, y1 }, { x + 1, y2 }, bgcolor.blend(colors::button_face_shadow_end, 0.5));
						}

						//Draw text, the text is transformed from orignal for hotkey character
						int text_top_off = (item_s.height - text_s.height) / 2;
						ird.caption({ item_pos.x + 8, item_pos.y + text_top_off }, to_nstring(pm->text));

						API::dev::draw_shortkey_underline(graph, pm->text, pm->shortkey, pm->shortkey_pos, { item_pos.x + 8, item_pos.y + text_top_off }, ird.scheme_ptr()->text_fgcolor);

						item_pos.x += pm->size.width;
						++index;
					}
				}

				void trigger::mouse_move(graph_reference graph, const arg_mouse& arg)
				{
					
					if (arg.pos != ess_->state.mouse_pos)
						ess_->state.nullify_mouse = false;

					bool popup = false;
					if(essence::behavior::focus == ess_->state.behave)
					{
						auto index = ess_->find(arg.pos);
						if(index != npos && ess_->state.active != index)
						{
							ess_->state.active = index;
							popup = true;
						}
					}
					else if (!ess_->state.nullify_mouse)
					{
						auto which = ess_->find(arg.pos);
						if ((which != ess_->state.active) && (which != npos || (false == ess_->state.menu_active)))
						{
							ess_->state.active = which;
							popup = true;
						}
					}

					if(popup)
					{
						ess_->open_menu();
						refresh(graph);
						API::dev::lazy_refresh();
					}

					ess_->state.mouse_pos = arg.pos;
				}

				void trigger::mouse_leave(graph_reference graph, const arg_mouse& arg)
				{
					ess_->state.nullify_mouse = false;
					mouse_move(graph, arg);
				}

				void trigger::mouse_down(graph_reference graph, const arg_mouse& arg)
				{
					ess_->state.nullify_mouse = false;
					ess_->state.active = ess_->find(arg.pos);

					if (npos != ess_->state.active)
						ess_->open_menu(true);
					else
						ess_->cancel();

					refresh(graph);
					API::dev::lazy_refresh();
				}

				void trigger::mouse_up(graph_reference graph, const arg_mouse&)
				{
					ess_->state.nullify_mouse = false;

					if(ess_->state.behave != essence::behavior::menu)
					{
						if(ess_->state.menu_active)
							ess_->state.behave = essence::behavior::menu;
					}
					else
					{
						ess_->state.behave = essence::behavior::none;
						ess_->cancel();
						refresh(graph);
						API::dev::lazy_refresh();
					}
				}

				void trigger::focus(graph_reference graph, const arg_focus& arg)
				{
					if((arg.getting == false) && (ess_->state.active != npos))
					{
						ess_->state.behave = essence::behavior::none;
						ess_->state.nullify_mouse = true;
						ess_->close_menu();
						ess_->state.active = npos;
						refresh(graph);
						API::dev::lazy_refresh();
					}
				}

				void trigger::key_press(graph_reference graph, const arg_keyboard& arg)
				{
					ess_->state.nullify_mouse = true;

					auto & menu_ptr = ess_->state.menu;

					//menu_wd will be assigned with the handle of a menu window,
					//It is used for checking whether the menu is closed. A menu handler
					//may close the form, checking with the data member of this trigger
					//is invalid, because the form is closed, the object of menubar may not exist.
					window menu_wd = nullptr;
					if(ess_->state.menu)
					{
						menu_wd = menu_ptr->handle();
						switch(arg.key)
						{
						case keyboard::os_arrow_down:
						case keyboard::backspace:
						case keyboard::os_arrow_up:
							menu_ptr->goto_next(keyboard::os_arrow_down == arg.key);
							break;
						case keyboard::os_arrow_right:
							if(menu_ptr->goto_submen() == false)
								_m_move(graph, false);
							break;
						case keyboard::os_arrow_left:
							if(menu_ptr->exit_submenu() == false)
								_m_move(graph, true);
							break;
						case keyboard::escape:
							if(menu_ptr->exit_submenu() == false)
							{
								ess_->close_menu();
								ess_->state.behave = essence::behavior::focus;
							}
							break;
						case keyboard::enter:
							menu_ptr->pick();
							break;
						default:
							//Katsuhisa Yuasa: menubar key_press improvements
							//send_shortkey has 3 states, 0 = UNKNOWN KEY, 1 = ITEM, 2 = GOTO SUBMENU
							int sk_state = menu_ptr->send_shortkey(arg.key);
							switch(sk_state)
							{
							case 0: //UNKNOWN KEY
								break;
							case 1: //ITEM
								if (ess_->state.active != npos)
								{
									ess_->cancel();
									if (arg.key == 18) //ALT
										ess_->state.behave = essence::behavior::focus;
								}
								break;
							case 2: //GOTO SUBMENU
								menu_ptr->goto_submen();
								break;
							default: break;
							}
							break;
						}
					}
					else
					{
						switch(arg.key)
						{
						case keyboard::os_arrow_right:
						case keyboard::backspace:
						case keyboard::os_arrow_left:
							_m_move(graph, keyboard::os_arrow_right != arg.key);
							break;
						case keyboard::os_arrow_up:
						case keyboard::os_arrow_down:
						case keyboard::enter:
							if (ess_->open_menu(true))
							{
								menu_wd = menu_ptr->handle();
								menu_ptr->goto_next(true);
							}
							break;
						case keyboard::escape:
							if(essence::behavior::focus == ess_->state.behave)
							{
								ess_->state.active= npos;
								ess_->state.behave = essence::behavior::none;
							}
							break;
						default:
							auto index = ess_->find(arg.key);
							if(index != npos)
							{
								ess_->state.active = index;
								if (ess_->open_menu(true))
								{
									menu_wd = menu_ptr->handle();
									menu_ptr->goto_next(true);
								}
							}
							break;
						}
					}

					if (API::is_window(menu_wd))
					{
						refresh(graph);
						API::dev::lazy_refresh();
					}
				}

				void trigger::key_release(graph_reference graph, const arg_keyboard& arg)
				{
					if(arg.key == 18)
					{
						if(essence::behavior::none == ess_->state.behave)
						{
							ess_->state.behave = essence::behavior::focus;
							ess_->state.active = 0;
						}
						else
						{
							ess_->state.behave = essence::behavior::none;
							auto pos = API::cursor_position();
							API::calc_window_point(ess_->widget_ptr->handle(), pos);
							ess_->state.active = ess_->find(pos);
						}

						ess_->state.menu_active = false;
						refresh(graph);
						API::dev::lazy_refresh();
					}
				}

				void trigger::shortkey(graph_reference graph, const arg_keyboard& arg)
				{
					API::focus_window(ess_->widget_ptr->handle());

					auto index = ess_->find(arg.key);
					if(index != npos && (index != ess_->state.active || nullptr == ess_->state.menu))
					{
						ess_->close_menu();
						ess_->state.nullify_mouse = true;
						ess_->state.active = index;

						if(ess_->open_menu(true))
							ess_->state.menu->goto_next(true);

						refresh(graph);
						API::dev::lazy_refresh();
						ess_->state.behave = essence::behavior::menu;
					}
				}

				void trigger::_m_move(graph_reference graph, bool to_left)
				{
					if (ess_->items.empty()) return;

					const std::size_t last_pos = ess_->items.size() - 1;

					auto index = ess_->state.active;
					if(to_left)
					{
						--index;
						if (index > last_pos)
							index = last_pos;
					}
					else
					{
						++index;
						if(index > last_pos)
							index = 0;
					}

					if(index != ess_->state.active)
					{
						ess_->state.active = index;
						refresh(graph);
						API::dev::lazy_refresh();

						if(ess_->open_menu())
							ess_->state.menu->goto_next(true);
					}
				}
			//end class trigger
		}//end namespace menubar
	}//end namespace drawerbase


	//class menubar
		menubar::menubar(window wd)
		{
			create(wd);
		}

		menubar::~menubar()
		{
			API::umake_event(evt_resized_);
		}

		void menubar::create(window wd)
		{
			widget_object<category::widget_tag, drawerbase::menubar::trigger, ::nana::general_events, drawerbase::menubar::scheme>
				::create(wd, rectangle(nana::size(API::window_size(wd).width, 28)));

			API::dev::set_menubar(handle(), true);
			evt_resized_ = API::events(wd).resized.connect_unignorable([this](const ::nana::arg_resized& arg)
			{
				auto sz = this->size();
				sz.width = arg.width;
				this->size(sz);
			});
		}

		menu& menubar::push_back(const std::string& text)
		{
			return get_drawer_trigger().ess().push_back(text);
		}

		menu& menubar::at(std::size_t pos) const
		{
			return get_drawer_trigger().ess().items.at(pos)->menu_obj;
		}

		std::size_t menubar::length() const
		{
			return get_drawer_trigger().ess().items.size();
		}
		
		void menubar::clear()
		{
			internal_scope_guard lock;
			get_drawer_trigger().ess().items.clear();
			API::refresh_window(handle());
		}

		bool menubar::cancel()
		{
			return get_drawer_trigger().ess().cancel();
		}

		bool menubar::hovered() const
		{
			auto const native_handle = API::root(this->handle());
			if (native_handle)
			{
				auto wd = API::find_window(API::cursor_position());
				if (wd == this->handle())
					return true;

				auto & items = get_drawer_trigger().ess().items;
				while (wd)
				{
					auto owner = API::get_owner_window(wd);
					if (API::root(owner) == native_handle)
					{
						for (auto p : items)
						{
							if (p->menu_obj.handle() == wd)
								return true;
						}
					}
					wd = owner;
				}
			}
			return false;
		}
	//end class menubar
}//end namespace nana
