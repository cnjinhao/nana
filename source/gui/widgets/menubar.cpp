/*
*	A Menubar implementation
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2009-2015 Jinhao(cnjinhao@hotmail.com)
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
			m._m_popup(wd, x, y, true);
		}
	};

	namespace drawerbase
	{
		namespace menubar
		{
			struct item_type
			{
				item_type(const native_string_type text, unsigned long shortkey)
					: text(text), shortkey(shortkey)
				{}

				native_string_type	text;
				unsigned long	shortkey;
				::nana::menu	menu_obj;
				::nana::point	pos;
				::nana::size	size;
			};

			class trigger::itembase
			{
			public:
				using container = std::vector<item_type*>;

				~itembase()
				{
					for(auto i : cont_)
						delete i;
				}

				void append(const native_string_type& text, unsigned long shortkey)
				{
					if(shortkey && shortkey < 0x61) shortkey += (0x61 - 0x41);
					cont_.push_back(new item_type(text, shortkey));
				}

				std::size_t find(unsigned long shortkey) const
				{
					if(shortkey)
					{
						if(shortkey < 0x61) shortkey += (0x61 - 0x41);

						std::size_t index = 0;
						for(auto i : cont_)
						{
							if(i->shortkey == shortkey)
								return index;
							++index;
						}
					}
					return npos;
				}

				const container& cont() const
				{
					return cont_;
				}
			private:
				container cont_;
			};

			//class item_renderer
				item_renderer::item_renderer(window wd, graph_reference graph)
					:handle_(wd), graph_(graph)
				{}

				void item_renderer::background(const nana::point& pos, const nana::size& size, state item_state)
				{
					auto bground = API::fgcolor(handle_);
					::nana::color border, body, corner;

					switch (item_state)
					{
					case state::highlighted:
						border = colors::highlight;
						body.from_rgb(0xC0, 0xDD, 0xFC);
						corner = body.blend(bground, 0.5);
						break;
					case state::selected:
						border = colors::dark_border;
						body = colors::white;
						corner = body.blend(bground, 0.5);
						break;
					default:	//Don't process other states.
						return;
					}

					nana::rectangle r(pos, size);
					graph_.rectangle(r, false, border);

					int right = pos.x + static_cast<int>(size.width) - 1;
					int bottom = pos.y + static_cast<int>(size.height) - 1;
					graph_.palette(false, corner);
					graph_.set_pixel(pos.x, pos.y);
					graph_.set_pixel(right, pos.y);
					graph_.set_pixel(pos.x, bottom);
					graph_.set_pixel(right, bottom);
					graph_.rectangle(r.pare_off(1), true, body);
				}

				void item_renderer::caption(const point& pos, const native_string_type& text)
				{
					graph_.string(pos, text, colors::black);
				}
			//end class item_renderer

			//class trigger
				trigger::trigger()
					: items_(new itembase)
				{}

				trigger::~trigger()
				{
					delete items_;
				}

				nana::menu* trigger::push_back(const std::string& text)
				{
					wchar_t shkey;
					API::transform_shortkey_text(text, shkey, nullptr);

					if(shkey)
						API::register_shortkey(widget_->handle(), shkey);

					auto pos = items_->cont().size();
					items_->append(to_nstring(text), shkey);
					refresh(*graph_);
					API::update_window(*widget_);

					return at(pos);
				}

				nana::menu* trigger::at(std::size_t pos) const
				{
					if (pos < items_->cont().size())
						return &(items_->cont()[pos]->menu_obj);

					return nullptr;
				}

				std::size_t trigger::size() const
				{
					return items_->cont().size();
				}

				void trigger::attached(widget_reference widget, graph_reference graph)
				{
					graph_ = &graph;
					widget_ = &widget;
				}

				void trigger::refresh(graph_reference graph)
				{
					auto bgcolor = API::bgcolor(*widget_);
					graph_->rectangle(true, bgcolor);

					item_renderer ird(*widget_, graph);

					nana::point item_pos(2, 2);
					nana::size item_s(0, 23);

					unsigned long index = 0;
					for (auto i : items_->cont())
					{
						//Transform the text if it contains the hotkey character
						wchar_t hotkey;
						::std::wstring::size_type hotkey_pos;
						auto text = API::transform_shortkey_text(to_utf8(i->text), hotkey, &hotkey_pos);

						nana::size text_s = graph.text_extent_size(text);

						item_s.width = text_s.width + 16;

						i->pos = item_pos;
						i->size = item_s;

						using state = item_renderer::state;
						state item_state = (index != state_.active ? state::normal : (state_.menu_active ? state::selected : state::highlighted));
						ird.background(item_pos, item_s, item_state);

						if (state::selected == item_state)
						{
							int x = item_pos.x + item_s.width;
							int y1 = item_pos.y + 2, y2 = item_pos.y + item_s.height - 1;
							graph.line({ x, y1 }, { x, y2 }, bgcolor.blend(colors::gray_border, 0.4));
							graph.line({ x + 1, y1 }, { x + 1, y2 }, bgcolor.blend(colors::button_face_shadow_end, 0.5));
						}

						//Draw text, the text is transformed from orignal for hotkey character
						int text_top_off = (item_s.height - text_s.height) / 2;
						ird.caption({ item_pos.x + 8, item_pos.y + text_top_off }, to_nstring(text));

						if (hotkey)
						{
							unsigned off_w = (hotkey_pos ? graph.text_extent_size(text.c_str(), static_cast<unsigned>(hotkey_pos)).width : 0);
							nana::size hotkey_size = graph.text_extent_size(text.c_str() + hotkey_pos, 1);

							unsigned ascent, descent, inleading;
							graph.text_metrics(ascent, descent, inleading);
							int x = item_pos.x + 8 + off_w;
							int y = item_pos.y + text_top_off + ascent + 1;
							graph.line({ x, y }, { x + static_cast<int>(hotkey_size.width) - 1, y }, ::nana::colors::black);
						}

						item_pos.x += i->size.width;
						++index;
					}
				}

				void trigger::mouse_move(graph_reference graph, const arg_mouse& arg)
				{
					if (arg.pos != state_.mouse_pos)
						state_.nullify_mouse = false;

					bool popup = false;
					if(state_.behavior == state_type::behavior_focus)
					{
						std::size_t index = _m_item_by_pos(arg.pos);
						if(index != npos && state_.active != index)
						{
							state_.active = index;
							popup = true;
						}
					}
					else
						popup = _m_track_mouse(arg.pos);

					if(popup)
					{
						_m_popup_menu();
						refresh(graph);
						API::lazy_refresh();
					}

					state_.mouse_pos = arg.pos;
				}

				void trigger::mouse_leave(graph_reference graph, const arg_mouse& arg)
				{
					state_.nullify_mouse = false;
					mouse_move(graph, arg);
				}

				void trigger::mouse_down(graph_reference graph, const arg_mouse& arg)
				{
					state_.nullify_mouse = false;
					state_.active = _m_item_by_pos(arg.pos);

					if (npos != state_.active)
					{
						if (!state_.menu_active)
							state_.menu_active = true;

						_m_popup_menu();
					}
					else
						_m_total_close();

					refresh(graph);
					API::lazy_refresh();
				}

				void trigger::mouse_up(graph_reference graph, const arg_mouse&)
				{
					state_.nullify_mouse = false;

					if(state_.behavior != state_.behavior_menu)
					{
						if(state_.menu_active)
							state_.behavior = state_.behavior_menu;
					}
					else
					{
						state_.behavior = state_.behavior_none;
						_m_total_close();
						refresh(graph);
						API::lazy_refresh();
					}
				}

				void trigger::focus(graph_reference graph, const arg_focus& arg)
				{
					if((arg.getting == false) && (state_.active != npos))
					{
						state_.behavior = state_type::behavior_none;
						state_.nullify_mouse = true;
						state_.menu_active = false;
						_m_close_menu();
						state_.active = npos;
						refresh(graph);
						API::lazy_refresh();
					}
				}

				void trigger::key_press(graph_reference graph, const arg_keyboard& arg)
				{
					state_.nullify_mouse = true;
					if(state_.menu)
					{
						switch(arg.key)
						{
						case keyboard::os_arrow_down:
						case keyboard::backspace:
						case keyboard::os_arrow_up:
							state_.menu->goto_next(keyboard::os_arrow_down == arg.key);
							break;
						case keyboard::os_arrow_right:
							if(state_.menu->goto_submen() == false)
								_m_move(false);
							break;
						case keyboard::os_arrow_left:
							if(state_.menu->exit_submenu() == false)
								_m_move(true);
							break;
						case keyboard::escape:
							if(state_.menu->exit_submenu() == false)
							{
								_m_close_menu();
								state_.behavior = state_.behavior_focus;
								state_.menu_active = false;
							}
							break;
						case keyboard::enter:
							state_.menu->pick();
							break;
						default:
							//Katsuhisa Yuasa: menubar key_press improvements
							//send_shortkey has 3 states, 0 = UNKNOWN KEY, 1 = ITEM, 2 = GOTO SUBMENU
							int sk_state = state_.menu->send_shortkey(arg.key);
							switch(sk_state)
							{
							case 0: //UNKNOWN KEY
								break;
							case 1: //ITEM
								if (state_.active != npos)
								{
									_m_total_close();
									if (arg.key == 18) //ALT
										state_.behavior = state_.behavior_focus;
								}
								break;
							case 2: //GOTO SUBMENU
								state_.menu->goto_submen();
								break;
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
							_m_move(keyboard::os_arrow_right != arg.key);
							break;
						case keyboard::os_arrow_up:
						case keyboard::os_arrow_down:
						case keyboard::enter:
							state_.menu_active = true;
							if(_m_popup_menu())
								state_.menu->goto_next(true);
							break;
						case keyboard::escape:
							if(state_.behavior == state_.behavior_focus)
							{
								state_.active= npos;
								state_.behavior = state_.behavior_none;
							}
							break;
						default:
							std::size_t index = items_->find(arg.key);
							if(index != npos)
							{
								state_.active = index;
								state_.menu_active = true;
								if(_m_popup_menu())
									state_.menu->goto_next(true);
							}
							break;
						}
					}

					refresh(graph);
					API::lazy_refresh();
				}

				void trigger::key_release(graph_reference graph, const arg_keyboard& arg)
				{
					if(arg.key == 18)
					{
						if(state_.behavior == state_type::behavior_none)
						{
							state_.behavior = state_type::behavior_focus;
							state_.active = 0;
						}
						else
						{
							state_.behavior = state_type::behavior_none;
							nana::point pos = API::cursor_position();
							API::calc_window_point(widget_->handle(), pos);
							state_.active = _m_item_by_pos(pos);
						}

						state_.menu_active = false;
						refresh(graph);
						API::lazy_refresh();
					}
				}

				void trigger::shortkey(graph_reference graph, const arg_keyboard& arg)
				{
					API::focus_window(widget_->handle());

					std::size_t index = items_->find(arg.key);
					if(index != npos && (index != state_.active || nullptr == state_.menu))
					{
						_m_close_menu();
						state_.menu_active = true;
						state_.nullify_mouse = true;
						state_.active = index;

						if(_m_popup_menu())
							state_.menu->goto_next(true);

						refresh(graph);
						API::lazy_refresh();
						state_.behavior = state_.behavior_menu;
					}
				}

				void trigger::_m_move(bool to_left)
				{
					if(items_->cont().empty()) return;

					const std::size_t last_pos = items_->cont().size() - 1;
					std::size_t index = state_.active;
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

					if(index != state_.active)
					{
						state_.active = index;
						refresh(*graph_);
						API::lazy_refresh();

						if(_m_popup_menu())
							state_.menu->goto_next(true);
					}
				}

				bool trigger::_m_popup_menu()
				{
					auto& items = items_->cont();

					auto pos = state_.active;
					if (pos >= items.size())
						return false;

					if(state_.menu_active && (state_.menu != &(items[pos]->menu_obj)))
					{
						API::dev::delay_restore(true);
						_m_close_menu();
						API::dev::delay_restore(false);
						state_.active = pos;

						auto & m = items[pos];
						state_.menu = &(m->menu_obj);
						state_.menu->destroy_answer([this]
						{
							state_.menu = nullptr;
							if (state_.passive_close)
							{
								_m_total_close();

								refresh(*graph_);
								API::update_window(widget_->handle());
							}
						});

						if (API::focus_window() != this->widget_->handle())
							API::focus_window(widget_->handle());
						menu_accessor::popup(*state_.menu, *widget_, m->pos.x, m->pos.y + static_cast<int>(m->size.height));
						return true;
					}
					return false;
				}

				void trigger::_m_total_close()
				{
					_m_close_menu();
					state_.menu_active = false;
					state_.behavior = state_.behavior_none;

					auto pos = API::cursor_position();
					API::calc_window_point(widget_->handle(), pos);
					state_.active = _m_item_by_pos(pos);
				}

				bool trigger::_m_close_menu()
				{
					if(state_.menu)
					{
						state_.passive_close = false;
						state_.menu->close();
						state_.passive_close = true;
						state_.menu = nullptr;
						return true;
					}
					return false;
				}

				std::size_t trigger::_m_item_by_pos(const ::nana::point& pos)
				{
					if((2 <= pos.x) && (2 <= pos.y) && (pos.y < 25))
					{
						int item_x = 2;
						std::size_t index = 0;
						for(auto i : items_->cont())
						{
							if(item_x <= pos.x && pos.x < item_x + static_cast<int>(i->size.width))
								return index;

							item_x += i->size.width;
							++index;
						}
					}

					return npos;
				}

				bool trigger::_m_track_mouse(const ::nana::point& pos)
				{
					if(state_.nullify_mouse == false)
					{
						std::size_t which = _m_item_by_pos(pos);
						if((which != state_.active) && (which != npos || (false == state_.menu_active)))
						{
							state_.active = which;
							return true;
						}
					}
					return false;
				}

				//struct state_type
					trigger::state_type::state_type()
						:	active(npos),
							behavior(behavior_none),
							menu_active(false),
							passive_close(true),
							nullify_mouse(false),
							menu(nullptr)
					{}
				//end struct state_type
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
			widget_object<category::widget_tag, drawerbase::menubar::trigger>
				::create(wd, rectangle(nana::size(API::window_size(wd).width, 28)));

			API::dev::set_menubar(handle(), true);
			evt_resized_ = API::events(wd).resized([this](const ::nana::arg_resized& arg)
			{
				auto sz = this->size();
				sz.width = arg.width;
				this->size(sz);
			});
		}

		menu& menubar::push_back(const std::string& text)
		{
			return *(get_drawer_trigger().push_back(text));
		}

		menu& menubar::at(std::size_t index) const
		{
			menu* p = get_drawer_trigger().at(index);
			if(nullptr == p)
				throw std::out_of_range("menubar::at, out of range");
			return *p;
		}

		std::size_t menubar::length() const
		{
			return get_drawer_trigger().size();
		}
	//end class menubar
}//end namespace nana
