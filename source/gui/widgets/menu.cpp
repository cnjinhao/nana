/*
*	A Menu implementation
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2009-2019 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file: nana/gui/widgets/menu.cpp
*	@contributors:
*		kmribti(pr#102)
*		dankan1890(pr#158)
*/

#include <nana/gui/compact.hpp>
#include <nana/gui/screen.hpp>
#include <nana/gui/widgets/menu.hpp>
#include <nana/gui/timer.hpp>

#include <nana/system/platform.hpp>
#include <nana/gui/element.hpp>
#include <nana/paint/text_renderer.hpp>
#include <cctype>	//introduces tolower
#include <vector>

namespace nana
{
	namespace drawerbase
	{
		namespace menu
		{

			struct menu_type
			{
				using item_type = menu_item_type;
				using item_container = std::vector<std::unique_ptr<item_type>>;
				using iterator = item_container::iterator;

				::nana::menu* owner;
				std::vector<menu_type*>	links;
				item_container	items;
				unsigned max_pixels;
				unsigned item_pixels;
				nana::point gaps;
			};

			//A helper function to check the style parameter
			inline bool good_checks(checks s)
			{
				return (checks::none <= s && s <= checks::highlight);
			}

			//struct menu_item_type
				//Default constructor initializes the item as a splitter
				menu_item_type::menu_item_type()
				{
					flags.enabled = true;
					flags.splitter = true;
					flags.checked = false;

					linked.own_creation = false;
					linked.menu_ptr = nullptr;
				}

				menu_item_type::menu_item_type(std::string text, const event_fn_t& fn)
					: text(std::move(text)), event_handler(fn)
				{
					flags.enabled = true;
					flags.splitter = false;
					flags.checked = false;

					linked.own_creation = false;
					linked.menu_ptr = nullptr;
				}
			//end class menu_item_type

			class internal_renderer
				: public renderer_interface
			{
			public:
				internal_renderer()
					: crook_("menu_crook")
				{
					crook_.check(facade<element::crook>::state::checked);
				}
			private:
				//Implements renderer_interface
				void background(graph_reference graph, window)
				{
					nana::size sz = graph.size();
					sz.width -= 30;
					sz.height -= 2;
					graph.rectangle(false, colors::gray_border);
					graph.rectangle({ 1, 1, 28, sz.height }, true, static_cast<color_rgb>(0xf6f6f6));
					graph.rectangle({ 29, 1, sz.width, sz.height }, true, colors::white);
				}

				void item(graph_reference graph, const nana::rectangle& r, const attr& at)
				{
					if (!at.enabled)
						return;

					if(at.item_state == state::active)
					{
						graph.rectangle(r, false, static_cast<color_rgb>(0xa8d8eb));

						graph.palette(false, static_cast<color_rgb>(0xc0ddfc));
						paint::draw(graph).corner(r, 1);

						if(at.enabled)
							graph.gradual_rectangle(nana::rectangle(r).pare_off(1), static_cast<color_rgb>(0xE8F0F4), static_cast<color_rgb>(0xDBECF4), true);
					}

					if(at.checked && (checks::none != at.check_style))
					{
						graph.rectangle(r, false, static_cast<color_rgb>(0xCDD3E6));

						::nana::color clr(0xE6, 0xEF, 0xF4);
						graph.rectangle(nana::rectangle(r).pare_off(1), true, clr);

						nana::rectangle crook_r = r;
						crook_r.width = 16;
						crook_.radio(at.check_style == checks::option);
						crook_.draw(graph, clr, colors::black, crook_r, element_state::normal);
					}
				}

				void item_image(graph_reference graph, const nana::point& pos, unsigned image_px, const paint::image& img)
				{
					if (img.size().width > image_px || img.size().height > image_px)
					{
						img.stretch(rectangle{ img.size() }, graph, rectangle{ pos, ::nana::size(image_px, image_px) });
						return;
					}
					
					//Stretches menu icon only when it doesn't fit, center it otherwise.
					//Contributed by kmribti(pr#102)
					img.paste(graph, {
						pos.x + static_cast<int>(image_px - img.size().width) / 2,
						pos.y + static_cast<int>(image_px - img.size().height) / 2
					});
				}

				void item_text(graph_reference graph, const nana::point& pos, const std::string& text, unsigned text_pixels, const attr& at)
				{
					graph.palette(true, at.enabled ? colors::black : colors::gray_border);
					nana::paint::text_renderer tr(graph);

					auto wstr = to_wstring(text);
					tr.render(pos, wstr.c_str(), wstr.length(), text_pixels, paint::text_renderer::mode::truncate_with_ellipsis);
				}

				void sub_arrow(graph_reference graph, const nana::point& pos, unsigned pixels, const attr&)
				{
					facade<element::arrow> arrow("hollow_triangle");
					arrow.direction(::nana::direction::east);
					arrow.draw(graph, {}, colors::black, { pos.x, pos.y + static_cast<int>(pixels - 16) / 2, 16, 16 }, element_state::normal);
				}

			private:
				facade<element::crook> crook_;
			};

			class menu_builder
				: noncopyable
			{
			public:
				using item_type = menu_item_type;
				using event_fn_t = item_type::event_fn_t;
				using iterator = menu_type::item_container::iterator;

				menu_builder(::nana::menu* owner)
				{
					root_.owner = owner;
					root_.max_pixels = screen::primary_monitor_size().width * 2 / 3;
					root_.item_pixels = 24;
					renderer_ = pat::cloneable<renderer_interface>(internal_renderer());
				}

				~menu_builder()
				{
					//Disconnects the link.

					//Clears the all links which are parent of mine
					for (auto link : root_.links)
					{
						for (auto & m : link->items)
						{
							if (m->linked.menu_ptr == &root_)
							{
								m->linked.own_creation = false;
								m->linked.menu_ptr = nullptr;
							}
						}
					}

					for (auto & m : root_.items)
					{
						if (m->linked.menu_ptr)
						{
							for (auto i = m->linked.menu_ptr->links.begin(); i != m->linked.menu_ptr->links.end();)
							{
								if ((*i) == &root_)
									i = m->linked.menu_ptr->links.erase(i);
								else
									++i;
							}

							if (m->linked.own_creation)
								delete m->linked.menu_ptr->owner;
						}
					}
				}

				void check_style(std::size_t index, checks s)
				{
					if(good_checks(s))
					{
						if(root_.items.size() > index)
							root_.items[index]->style = s;
					}
				}

				void checked(std::size_t pos, bool check)
				{
					if (root_.items.size() <= pos)
						return;

					item_type & m = *(root_.items[pos]);

					if(check && (checks::option == m.style))
					{
						//find a splitter in front of pos
						if (pos > 0)
						{
							do
							{
								if (root_.items[--pos]->flags.splitter)
								{
									++pos;
									break;
								}
							}while(pos > 0);
						}

						while (pos < root_.items.size())
						{
							item_type & el = *(root_.items[pos++]);
							if(el.flags.splitter) break;

							if(checks::option == el.style)
								el.flags.checked = false;
						}
					}

					m.flags.checked = check;
				}

				menu_type& data()
				{
					return root_;
				}

				//Returns false if the linked menu is already existing
				bool set_linkage(std::size_t pos, menu_type &linked, bool own_creation)
				{
					auto mi = root_.items.at(pos).get();

					if (mi->linked.menu_ptr)
						return false;

					mi->linked.menu_ptr = &linked;
					mi->linked.own_creation = own_creation;
					linked.links.emplace_back(&root_);

					return true;
				}

				pat::cloneable<renderer_interface>& renderer()
				{
					return renderer_;
				}

				void renderer(const pat::cloneable<renderer_interface>& rd)
				{
					renderer_ = rd;
				}
			private:
				menu_type root_;
				pat::cloneable<renderer_interface> renderer_;
			};//end class menu_builder


			//class menu_item_type::item_proxy
			menu_item_type::item_proxy::item_proxy(std::size_t pos, nana::menu* m):
				pos_{pos},
				menu_{m}
			{}

			menu_item_type::item_proxy& menu_item_type::item_proxy::enabled(bool v)
			{
				menu_->enabled(pos_, v);
				return *this;
			}

			bool menu_item_type::item_proxy::enabled() const
			{
				return menu_->enabled(pos_);
			}

			menu_item_type::item_proxy&	menu_item_type::item_proxy::check_style(checks style)
			{
				menu_->check_style(pos_, style);
				return *this;
			}

			menu_item_type::item_proxy&	menu_item_type::item_proxy::checked(bool ck)
			{
				menu_->checked(pos_, ck);
				return *this;
			}

			bool menu_item_type::item_proxy::checked() const
			{
				return menu_->checked(pos_);
			}

			menu_item_type::item_proxy& menu_item_type::item_proxy::text(std::string title)
			{
				menu_->text(pos_, title);
				return *this;
			}

			std::string menu_item_type::item_proxy::text() const
			{
				return menu_->text(pos_);
			}

			std::size_t menu_item_type::item_proxy::index() const
			{
				return pos_;
			}
			//end class item_proxy

			class menu_drawer
				: public drawer_trigger
			{
			public:
				using item_proxy = menu_item_type::item_proxy;

				menu_drawer()
				{
					state_.active = npos;
					state_.sub_window = false;
					state_.nullify_mouse = false;

					detail_.border.x = detail_.border.y = 2;
				}

				void set_run(menu_builder& mbuilder, menu_type& menu, std::function<void()> && menu_tree_destroyer)
				{
					mbuilder_ = &mbuilder;
					menu_ = &menu;
					fn_close_tree_ = std::move(menu_tree_destroyer);
				}

				menu_builder& mbuilder()
				{
					return *mbuilder_;
				}

				void attached(widget_reference widget, graph_reference graph)
				{
					graph_ = &graph;
					widget_ = &widget;
					//Get the current cursor pos to determinate the monitor
					detail_.monitor_pos = API::cursor_position();
				}

				void mouse_move(graph_reference graph, const arg_mouse& arg)
				{
					state_.nullify_mouse = false;
					if(track_mouse(arg.pos))
					{
						refresh(graph);
						API::dev::lazy_refresh();
					}
				}

				void mouse_leave(graph_reference graph, const arg_mouse& arg)
				{
					mouse_move(graph, arg);
				}

				void mouse_down(graph_reference, const arg_mouse&)
				{
					state_.nullify_mouse = false;
				}

				void refresh(graph_reference graph)
				{
					if (!(mbuilder_ && menu_))
						return;

					_m_adjust_window_size();

					auto renderer = mbuilder_->renderer().get();
					renderer->background(graph, *widget_);

					const unsigned item_h_px = _m_item_height();
					const unsigned image_px = item_h_px - 2;
					nana::rectangle item_r(2, 2, graph_->width() - 4, item_h_px);

					unsigned strpixels = item_r.width - 60;

					int text_top_off = static_cast<int>(item_h_px - graph.text_extent_size(L"jh({[").height) / 2;

					std::size_t pos = 0;
					for (auto & m : menu_->items)
					{
						auto item_ptr = m.get();
						if (item_ptr->flags.splitter)
						{
							graph_->line({ item_r.x + 40, item_r.y }, { static_cast<int>(graph.width()) - 1, item_r.y }, colors::gray_border);
							item_r.y += 2;
							++pos;
							continue;
						}

						renderer_interface::attr attr = _m_make_renderer_attr(pos == state_.active, *item_ptr);
						//Draw item background
						renderer->item(*graph_, item_r, attr);

						//Draw text, the text is transformed from orignal for hotkey character
						wchar_t hotkey;
						std::string::size_type hotkey_pos;
						auto text = API::transform_shortkey_text(item_ptr->text, hotkey, &hotkey_pos);

						if (item_ptr->image.empty() == false)
							renderer->item_image(graph, nana::point(item_r.x + 5, item_r.y + static_cast<int>(item_h_px - image_px) / 2 - 1), image_px, item_ptr->image);

						renderer->item_text(graph, nana::point(item_r.x + 40, item_r.y + text_top_off), text, strpixels, attr);

						item_ptr->hotkey = hotkey;
						if (hotkey && item_ptr->flags.enabled)
								API::dev::draw_shortkey_underline(*graph_, text, hotkey, hotkey_pos, {item_r.x + 40, item_r.y + text_top_off}, colors::black);

						if (item_ptr->linked.menu_ptr)
							renderer->sub_arrow(graph, nana::point(graph_->width() - 20, item_r.y), item_h_px, attr);

						item_r.y += item_r.height + 1;

						++pos;
					}
				}

				std::size_t active() const
				{
					return state_.active;
				}

				bool goto_next(bool forword)
				{
					state_.nullify_mouse = true;

					auto & items = menu_->items;

					if (items.empty())
						return false;

					auto pos = state_.active;
					const auto lastpos = items.size() - 1;

					bool end = false;
					while(true)
					{
						if(forword)
						{
							if(pos == lastpos)
							{
								if (end)
								{
									pos = npos;
									break;
								}

								end = true;
								pos = 0;
							}
							else
								++pos;
						}
						else
						{
							if(pos == 0 || pos == npos)
							{
								if (end)
									break;
								
								end = true;
								pos = lastpos;
							}
							else
								--pos;
						}

						auto item_ptr = items.at(pos).get();
						if (!item_ptr->flags.splitter && item_ptr->flags.enabled)
							break;
					}

					if(pos != npos && pos != state_.active)
					{
						state_.active = pos;
						state_.sub_window = false;

						refresh(*graph_);
						return true;
					}
					
					return false;
				}

				bool track_mouse(const ::nana::point& pos)
				{
					if (!state_.nullify_mouse)
					{
						auto & items = menu_->items;
						std::size_t index = _m_get_index_by_pos(pos.x, pos.y);
						if (index != state_.active)
						{
							if ((index == npos) && items.at(state_.active)->linked.menu_ptr && state_.sub_window)
								return false;

							state_.active = (index != npos && items.at(index)->flags.splitter) ? npos : index;
							state_.active_timestamp = nana::system::timestamp();
							return true;
						}
					}

					return false;
				}

				menu_type* data() const
				{
					return menu_;
				}

				void set_sub_window(bool subw)
				{
					state_.sub_window = subw;
				}

				menu_type* get_sub(nana::point& pos, unsigned long& tmstamp) const
				{
					if (npos == state_.active)
						return nullptr;

					auto & items = menu_->items;
					auto sub = items.at(state_.active)->linked.menu_ptr;
					if (sub)
					{
						pos.x = static_cast<int>(graph_->width()) - 2;
						pos.y = 2;

						auto index = state_.active;
						for (auto & m : items)
						{
							if (0 == index--)
								break;

							if (m->flags.splitter)
							{
								pos.y += 2;
								continue;
							}

							pos.y += _m_item_height() + 1;
						}

						tmstamp = state_.active_timestamp;
						return sub;
					}
					return nullptr;
				}

				//send_shortkey has 3 states, 0 = UNKNOWN KEY, 1 = ITEM, 2 = GOTO SUBMENU
				int send_shortkey(wchar_t key)
				{
					key = std::tolower(key);
					std::size_t index = 0;
					for(auto & m : menu_->items)
					{
						auto item_ptr = m.get();
						if (std::tolower(m->hotkey) != key)
						{
							++index;
							continue;
						}

						if (!item_ptr->flags.splitter)
						{
							if (item_ptr->linked.menu_ptr)
							{
								state_.active = index;
								state_.active_timestamp = nana::system::timestamp();

								API::refresh_window(*widget_);
								return 2;
							}
							else if (item_ptr->flags.enabled)
							{
								//A pointer refers to a menu object which owns the menu window.
								//After fn_close_tree_(), *this is an invalid object.
								auto owner = menu_->owner;

								fn_close_tree_();
								if (item_ptr->event_handler)
								{
									item_proxy ip{ index, owner };
									item_ptr->event_handler.operator()(ip);
								}
								return 1;
							}
						}
						break;
					}
					return 0;
				}
			private:
				static renderer_interface::attr _m_make_renderer_attr(bool active, const menu_item_type & m)
				{
					renderer_interface::attr attr;
					attr.item_state = (active ? renderer_interface::state::active : renderer_interface::state::normal);
					attr.enabled = m.flags.enabled;
					attr.checked = m.flags.checked;
					attr.check_style = m.style;
					return attr;
				}

				std::size_t _m_get_index_by_pos(int x, int y) const
				{
					if(	(x < static_cast<int>(detail_.border.x)) ||
						(x > static_cast<int>(graph_->width()) - static_cast<int>(detail_.border.x)) ||
						(y < static_cast<int>(detail_.border.y)) ||
						(y > static_cast<int>(graph_->height()) - static_cast<int>(detail_.border.y)))
						return npos;

					int pos = detail_.border.y;
					std::size_t index = 0;
					for (auto & m : menu_->items)
					{
						unsigned h = (m->flags.splitter ? 1 : _m_item_height());
						if(pos <= y && y < static_cast<int>(pos + h))
							return index;
						else if(y < pos)
							return npos;

						++index;
						pos += (h + 1);
					}
					return npos;
				}

				unsigned _m_item_height() const
				{
					return menu_->item_pixels;
				}

				nana::size _m_client_size() const
				{
					nana::size size;

					if (menu_->items.size())
					{
						for (auto & m : menu_->items)
						{
							if(false == m->flags.splitter)
							{
								nana::size item_size = graph_->text_extent_size(m->text);
								if(size.width < item_size.width)
									size.width = item_size.width;
							}
							else
								++size.height;
						}

						size.width += (35 + 40);
						size.height = static_cast<unsigned>(menu_->items.size() - size.height) * _m_item_height() + size.height + static_cast<unsigned>(menu_->items.size() - 1);
					}

					if (size.width > menu_->max_pixels)
						size.width = menu_->max_pixels;

					return size;
				}

				void _m_adjust_window_size() const
				{
					nana::size size = _m_client_size();

					size.width += detail_.border.x * 2;
					size.height += detail_.border.y * 2;

					widget_->size(size);

					nana::point pos;
					API::calc_screen_point(*widget_, pos);

					//get the screen coordinates of the widget pos.
					auto scr_area = screen().from_point(detail_.monitor_pos).workarea();

					if(pos.x + static_cast<int>(size.width) > scr_area.right())
						pos.x = scr_area.right() - static_cast<int>(size.width);
					if(pos.x < scr_area.x) pos.x = scr_area.x;

					if(pos.y + static_cast<int>(size.height) > scr_area.bottom())
						pos.y = scr_area.bottom() - static_cast<int>(size.height);
					if(pos.y < scr_area.y) pos.y = scr_area.y;

					API::calc_window_point(API::get_owner_window(*widget_), pos);
					widget_->move(pos.x, pos.y);
				}
			private:
				widget		*widget_{nullptr};
				paint::graphics	*graph_{nullptr};

				menu_builder* mbuilder_{ nullptr };
				menu_type* menu_{ nullptr };

				std::function<void()> fn_close_tree_;

				struct state
				{
					std::size_t	active;
					unsigned	active_timestamp;
					bool	sub_window: 1;
					bool	nullify_mouse: 1;
				}state_;

				struct widget_detail
				{
					nana::point	monitor_pos;	//It is used for determining the monitor.
					nana::upoint border;
				}detail_;
			};//end class menu_drawer

			class menu_window
				:	public widget_object<category::root_tag, menu_drawer>
			{
				using drawer_type = menu_drawer;
				using base_type = widget_object<category::root_tag, menu_drawer>;
			public:
				using item_type = menu_builder::item_type;

				
				menu_window(window wd, bool is_wd_parent_menu, const point& pos, menu_builder& mbuilder, menu_type& menu_data)
					//add a is_wd_parent_menu to determine whether the menu wants the focus.
					//if a submenu gets the focus, the program may cause a crash error when the submenu is being destroyed
					:	base_type(wd, false, rectangle(pos, nana::size(2, 2)), appear::bald<appear::floating>()),
						want_focus_{ (!wd) || ((!is_wd_parent_menu) && (API::root(API::focus_window()) != API::root(wd))) },
						event_focus_{ nullptr }
				{
					caption("nana menu window");
					get_drawer_trigger().set_run(mbuilder, menu_data, [this]{ this->_m_close_all(); });

					state_.owner_menubar = state_.self_submenu = false;
					state_.auto_popup_submenu = true;

					submenu_.child = submenu_.parent = nullptr;
					submenu_.object = nullptr;

					state_.mouse_pos = API::cursor_position();
					events().mouse_move.connect_unignorable([this](const arg_mouse&){
						nana::point pos = API::cursor_position();
						if (pos != state_.mouse_pos)
						{
							menu_window * root = this;
							while (root->submenu_.parent)
								root = root->submenu_.parent;
							root->state_.auto_popup_submenu = true;

							state_.mouse_pos = pos;
						}
					});
				}

				void popup(bool owner_menubar)
				{
					if (!want_focus_)
					{
						API::activate_window(this->parent());
						API::take_active(this->handle(), false, nullptr);
					}

					if(submenu_.parent == nullptr)
					{
						state_.owner_menubar = owner_menubar;
						API::dev::register_menu_window(this->handle(), !owner_menubar);
					}

					auto & events = this->events();
					events.destroy.connect_unignorable([this](const arg_destroy&){
						_m_destroy();
					});

					events.key_press.connect_unignorable([this](const arg_keyboard& arg){
						_m_key_down(arg);
					});

					auto fn = [this](const arg_mouse& arg)
					{
						if (event_code::mouse_down == arg.evt_code)
							_m_open_sub(0);	//Try to open submenu immediately
						else if ((event_code::mouse_up == arg.evt_code) && (mouse::left_button == arg.button))
							pick();
					};

					events.mouse_down.connect_unignorable(fn);
					events.mouse_up.connect_unignorable(fn);

					timer_.interval(std::chrono::milliseconds{ 100 });
					timer_.elapse([this]{
						this->_m_open_sub(500);	//Try to open submenu
					});
					timer_.start();

					show();
					
					if (want_focus_)
					{
						event_focus_ = events.focus.connect_unignorable([this](const arg_focus& arg)
						{
							//when the focus of the menu window is losing, close the menu.
							//But here is not every menu window may have focus event installed,
							//It is only installed when the owner of window is the desktop window.

							if (false == arg.getting && (arg.receiver != API::root(*this)))
							{
								for (auto child = submenu_.child; child; child = child->submenu_.child)
								{
									if (API::root(child->handle()) == arg.receiver)
										return;
								}

								_m_close_all();
							}
						});

						focus();
						activate();
					}
				}

				void goto_next(bool forward)
				{
					menu_window * object = this;

					while(object->submenu_.child)
						object = object->submenu_.child;

					state_.auto_popup_submenu = false;

					if(object->get_drawer_trigger().goto_next(forward))
						API::update_window(object->handle());
				}

				bool submenu(bool enter)
				{
					menu_window * menu_wd = this;
					while (menu_wd->submenu_.child)
						menu_wd = menu_wd->submenu_.child;

					state_.auto_popup_submenu = false;

					if (!enter)
					{
						if (menu_wd->submenu_.parent)
						{
							auto & sub = menu_wd->submenu_.parent->submenu_;
							sub.child = nullptr;
							sub.object = nullptr;

							menu_wd->close();
							return true;
						}
						return false;
					}

					return menu_wd->_m_manipulate_sub(0, true);
				}

				int send_shortkey(wchar_t key)
				{
					menu_window * object = this;
					while(object->submenu_.child)
						object = object->submenu_.child;

					return object->get_drawer_trigger().send_shortkey(key);
				}

				void pick()
				{
					menu_window * object = this;
					while (object->submenu_.child)
						object = object->submenu_.child;

					auto active = object->get_drawer_trigger().active();
					auto * menu = object->get_drawer_trigger().data();
					if ((npos == active) || !menu)
						return;
					
					menu_item_type & item = *(menu->items.at(active));

					if ((!item.flags.enabled) || item.flags.splitter || item.linked.menu_ptr)
						return;

					if (checks::highlight == item.style)
					{
						item.flags.checked = !item.flags.checked;
					}
					else if (checks::option == item.style)
					{
						get_drawer_trigger().mbuilder().checked(active, true);
					}

					this->_m_close_all();	//means deleting this;
					//The deleting operation has moved here, because item.event_handler.operator()(ip)
					//may create a window, which make a killing focus for menu window, if so the close_all
					//operation performs after item.event_handler.operator()(ip), that would be deleting this object twice!

					if (item.event_handler)
					{
						item_type::item_proxy ip{ active, menu->owner };
						item.event_handler.operator()(ip);
					}
				}
			private:
				//_m_destroy just destroys the children windows.
				//The all window including parent windows want to be closed by calling the _m_close_all() instead of close()
				void _m_destroy()
				{
					if(this->submenu_.parent)
					{
						this->submenu_.parent->submenu_.child = nullptr;
						this->submenu_.parent->submenu_.object = nullptr;
					}

					if(this->submenu_.child)
					{
						menu_window * tail = this;
						while(tail->submenu_.child) tail = tail->submenu_.child;

						while(tail != this)
						{
							menu_window * junk = tail;
							tail = tail->submenu_.parent;
							junk->close();
						}
					}
				}

				void _m_close_all()
				{
					menu_window * root = this;
					while(root->submenu_.parent)
						root = root->submenu_.parent;

					//Avoid generating a focus event when the menu is destroying and a focus event.
					if (event_focus_)
						umake_event(event_focus_);

					if(root != this)
					{
						//Disconnect the menu chain at this menu, and delete the menus before this.
						this->submenu_.parent->submenu_.child = nullptr;
						this->submenu_.parent->submenu_.object = nullptr;
						this->submenu_.parent = nullptr;
						root->close();
						//The submenu is treated its parent menu as an owner window,
						//if the root is closed, the all submenus will be closed
					}
					else
					{
						//Then, delete the menus from this.
						this->close();
					}
				}

				void _m_key_down(const arg_keyboard& arg)
				{
					switch(arg.key)
					{
					case keyboard::os_arrow_up:
					case keyboard::os_arrow_down:
						this->goto_next(keyboard::os_arrow_down == arg.key);
						break;
					case keyboard::os_arrow_left:
					case keyboard::os_arrow_right:
						this->submenu(keyboard::os_arrow_right == arg.key);
						break;
					case keyboard::enter:
						this->pick();
						break;
					case keyboard::escape:
						//Leave sub menu. But if the sub menu doesn't exist,
						//close the menu.
						if (!this->submenu(false))
							close();
						break;
					default:
						switch (send_shortkey(arg.key))
						{
						case 0:
							if (this->empty() == false)
								close();
							break;
						case 1:	//The menu has been closed
							break;
						case 2:
							this->submenu(true);
							break;
						}
					}
				}

				bool _m_manipulate_sub(unsigned long delay_ms, bool forced)
				{
					auto & drawer = get_drawer_trigger();
					::nana::point pos;
					unsigned long tmstamp;

					auto menu_ptr = drawer.get_sub(pos, tmstamp);

					if (menu_ptr == submenu_.object)
						return false;

					if (menu_ptr && (::nana::system::timestamp() - tmstamp < delay_ms))
						return false;

					if (submenu_.object && (menu_ptr != submenu_.object))
					{
						drawer.set_sub_window(false);
						submenu_.child->close();
						submenu_.child = nullptr;
						submenu_.object = nullptr;
					}

					if (menu_ptr)
					{
						menu_window * root = this;
						while (root->submenu_.parent)
							root = root->submenu_.parent;

						if ((submenu_.object == nullptr) && menu_ptr && (forced || root->state_.auto_popup_submenu))
						{
							menu_ptr->item_pixels = drawer.data()->item_pixels;
							menu_ptr->gaps = drawer.data()->gaps;
							pos += menu_ptr->gaps;

							menu_window & mwnd = form_loader<menu_window, false>()(handle(), true, pos, drawer.mbuilder(), *menu_ptr);
							mwnd.state_.self_submenu = true;
							submenu_.child = &mwnd;
							submenu_.child->submenu_.parent = this;
							submenu_.object = menu_ptr;

							API::set_window_z_order(handle(), mwnd.handle(), z_order_action::none);

							mwnd.popup(state_.owner_menubar);
							drawer.set_sub_window(true);
							if (forced)
								mwnd.goto_next(true);

							return true;
						}
					}
					return false;
				}

				void _m_open_sub(unsigned delay_ms)	//check_repeatly
				{
					if(state_.auto_popup_submenu)
					{
						auto pos = API::cursor_position();

						API::calc_window_point(handle(), pos);
						get_drawer_trigger().track_mouse(pos);

						_m_manipulate_sub(delay_ms, false);
					}
				}
			private:
				const bool want_focus_;
				event_handle event_focus_;

				timer timer_;
				struct state_type
				{
					bool self_submenu; //Indicates whether the menu window is used for a submenu
					bool owner_menubar;
					bool auto_popup_submenu;
					nana::point mouse_pos;
				}state_;

				struct submenu_type
				{
					menu_window*	parent;
					menu_window*	child;
					const menu_type *object;
				}submenu_;
			};
			//end class menu_window
		}//end namespace menu
	}//end namespace drawerbase

	//class menu
		struct menu::implement
		{
			struct info
			{
				menu * handle;
				bool kill;
			};

			drawerbase::menu::menu_builder	mbuilder;
			drawerbase::menu::menu_window *	window_ptr;
			std::function<void()> destroy_answer;

			implement(menu* self):
				mbuilder{self}
			{
			}
		};

		menu::menu()
			:impl_(new implement(this))
		{
			impl_->window_ptr = nullptr;
		}

		menu::~menu()
		{
			this->close();
			delete impl_;
		}

		using item_type = drawerbase::menu::menu_item_type;

		auto menu::append(std::string text_utf8, const menu::event_fn_t& handler) -> item_proxy
		{
			std::unique_ptr<item_type> item{ new item_type{ std::move(text_utf8), handler } };
			impl_->mbuilder.data().items.emplace_back(std::move(item));
			return item_proxy{size() - 1, this};
		}

		void menu::append_splitter()
		{
			impl_->mbuilder.data().items.emplace_back(new item_type);
		}

		auto menu::insert(std::size_t pos, std::string text_utf8, const event_fn_t& handler) -> item_proxy
		{
			auto & items = impl_->mbuilder.data().items;
			if (pos > items.size())
				throw std::out_of_range("menu: a new item inserted to an invalid position");

			std::unique_ptr<item_type> item{ new item_type{ std::move(text_utf8), handler } };

			items.emplace(
#ifdef _MSC_VER
				items.cbegin() + pos,
#else
				items.begin() + pos,
#endif
				std::move(item));

			return item_proxy{ pos, this};
		}

		void menu::clear()
		{
			internal_scope_guard lock;
			impl_->mbuilder.data().items.clear();
		}

		void menu::enabled(std::size_t index, bool enable)
		{
			impl_->mbuilder.data().items.at(index)->flags.enabled = enable;
		}

		bool menu::enabled(std::size_t index) const
		{
			return impl_->mbuilder.data().items.at(index)->flags.enabled;
		}

		void menu::erase(std::size_t index)
		{
			internal_scope_guard lock;
			auto & items = impl_->mbuilder.data().items;
			if(index < items.size())
				items.erase(items.begin() + index);
		}

		void menu::image(std::size_t index, const paint::image& img)
		{
			impl_->mbuilder.data().items.at(index)->image = img;
		}

		void menu::text(std::size_t index, std::string text_utf8)
		{
			impl_->mbuilder.data().items.at(index)->text.swap(text_utf8);
		}

		std::string menu::text(std::size_t index) const
		{
			return impl_->mbuilder.data().items.at(index)->text;
		}

		bool menu::link(std::size_t index, menu& menu_obj)
		{
			return impl_->mbuilder.set_linkage(index, menu_obj.impl_->mbuilder.data(), false);
		}

		menu* menu::link(std::size_t index) const
		{
			auto mi = impl_->mbuilder.data().items.at(index).get();

			if (mi && mi->linked.menu_ptr)
				return mi->linked.menu_ptr->owner;

			return nullptr;
		}

		menu *menu::create_sub_menu(std::size_t index)
		{
			std::unique_ptr<menu> guard{new menu};
			if (impl_->mbuilder.set_linkage(index, guard->impl_->mbuilder.data(), true))
				return guard.release();
			
			return nullptr;
		}

		void menu::popup(window wd, int x, int y)
		{
			_m_popup(wd, { x, y }, false);
		}

		void menu::popup_await(window wd, int x, int y)
		{
			_m_popup(wd, { x, y }, false);
			if (impl_->window_ptr)
				API::wait_for(impl_->window_ptr->handle());
		}

		void menu::close()
		{
			if (impl_->window_ptr)
			{
				impl_->window_ptr->close();
				impl_->window_ptr = nullptr;
			}
		}

		void menu::check_style(std::size_t index, checks style)
		{
			impl_->mbuilder.check_style(index, style);
		}

		void menu::checked(std::size_t index, bool check)
		{
			impl_->mbuilder.checked(index, check);
		}

		bool menu::checked(std::size_t index) const
		{
			return impl_->mbuilder.data().items.at(index)->flags.checked;
		}

		void menu::answerer(std::size_t index, const event_fn_t& fn)
		{
			impl_->mbuilder.data().items.at(index)->event_handler = fn;
		}

		void menu::destroy_answer(std::function<void()> fn)
		{
			impl_->destroy_answer = std::move(fn);
		}

		void menu::gaps(const nana::point& pos)
		{
			impl_->mbuilder.data().gaps = pos;
		}

		void menu::goto_next(bool forward)
		{
			if (impl_->window_ptr)
				impl_->window_ptr->goto_next(forward);
		}

		bool menu::goto_submen()
		{
			return (impl_->window_ptr ? impl_->window_ptr->submenu(true) : false);
		}

		bool menu::exit_submenu()
		{
			return (impl_->window_ptr ? impl_->window_ptr->submenu(false) : false);
		}

		std::size_t menu::size() const
		{
			return impl_->mbuilder.data().items.size();
		}

		int menu::send_shortkey(wchar_t key)
		{
			return (impl_->window_ptr ? impl_->window_ptr->send_shortkey(key) : 0);
		}

		void menu::pick()
		{
			if (impl_->window_ptr)
				impl_->window_ptr->pick();
		}

		menu& menu::max_pixels(unsigned px)
		{
			impl_->mbuilder.data().max_pixels = (px > 100 ? px : 100);
			return *this;
		}

		unsigned menu::max_pixels() const
		{
			return impl_->mbuilder.data().max_pixels;
		}

		menu& menu::item_pixels(unsigned px)
		{
			impl_->mbuilder.data().item_pixels = px;
			return *this;
		}

		unsigned menu::item_pixels() const
		{
			return impl_->mbuilder.data().item_pixels;
		}

		const pat::cloneable<menu::renderer_interface>& menu::renderer() const
		{
			return impl_->mbuilder.renderer();
		}

		void menu::renderer(const pat::cloneable<renderer_interface>& rd)
		{
			impl_->mbuilder.renderer(rd);
		}

		window menu::handle() const
		{
			return (impl_->window_ptr ? impl_->window_ptr->handle() : nullptr);
		}

		void menu::_m_popup(window wd, const point& pos, bool called_by_menubar)
		{
			if (impl_->mbuilder.data().items.size())
			{
				close();

				impl_->window_ptr = &(form_loader<drawerbase::menu::menu_window, false>()(wd, false, pos, impl_->mbuilder, impl_->mbuilder.data()));
				impl_->window_ptr->events().destroy.connect_unignorable([this](const arg_destroy&){
					impl_->window_ptr = nullptr;
					if (impl_->destroy_answer)
						impl_->destroy_answer();
				});
				impl_->window_ptr->popup(called_by_menubar);
			}
		}
	//end class menu

	detail::popuper menu_popuper(menu& mobj, mouse ms)
	{
		return detail::popuper(mobj, ms);
	}

	detail::popuper menu_popuper(menu& mobj, window owner, const point& pos, mouse ms)
	{
		return detail::popuper(mobj, owner, pos, ms);
	}

	namespace detail
	{
	//class popuper
		popuper::popuper(menu& mobj, mouse ms)
			: mobj_(mobj), owner_(nullptr), take_mouse_pos_(true), mouse_(ms)
		{}

		popuper::popuper(menu& mobj, window owner, const point& pos, mouse ms)
			: mobj_(mobj), owner_(owner), take_mouse_pos_(false), pos_(pos), mouse_(ms)
		{}

		void popuper::operator()(const arg_mouse& arg)
		{
			if(take_mouse_pos_)
			{
				switch(arg.evt_code)
				{
				case event_code::click:
				case event_code::mouse_down:
				case event_code::mouse_up:
					owner_ = arg.window_handle;
					pos_ = arg.pos;
					break;
				default:
					return;
				}
			}

			if((mouse::any_button == mouse_) || (mouse_ == arg.button))
				mobj_.popup(owner_, pos_.x, pos_.y);
		}
	//end class
	}//end namespace detail
}//end namespace nana
