/*
*	A Menu implementation
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2009-2015 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file: nana/gui/widgets/menu.cpp
*	@contributors:
*		kmribti(pr#102)
*/

#include <nana/gui/widgets/menu.hpp>
#include <nana/system/platform.hpp>
#include <nana/gui/element.hpp>
#include <nana/gui/wvl.hpp>
#include <nana/paint/text_renderer.hpp>
#include <cctype>	//introduces tolower

namespace nana
{
	namespace drawerbase
	{
		namespace menu
		{
			//A helper function to check the style parameter
			inline bool good_checks(checks s)
			{
				return (checks::none <= s && s <= checks::highlight);
			}

			//struct menu_item_type
				//class item_proxy
				//@brief: this class is used as parameter of menu event function.
					menu_item_type::item_proxy::item_proxy(std::size_t index, menu_item_type &item)
						:index_(index), item_(item)
					{}

					menu_item_type::item_proxy& menu_item_type::item_proxy::enabled(bool v)
					{
						item_.flags.enabled = v;
						return *this;
					}

					bool menu_item_type::item_proxy::enabled() const
					{
						return item_.flags.enabled;
					}

					menu_item_type::item_proxy&	menu_item_type::item_proxy::check_style(checks style)
					{
						if (good_checks(style))
							item_.style = style;
						return *this;
					}

					menu_item_type::item_proxy&	menu_item_type::item_proxy::checked(bool ck)
					{
						item_.flags.checked = ck;
						return *this;
					}

					bool menu_item_type::item_proxy::checked() const
					{
						return item_.flags.checked;
					}

					std::size_t menu_item_type::item_proxy::index() const
					{
						return index_;
					}
				//end class item_proxy

				//Default constructor initializes the item as a splitter
				menu_item_type::menu_item_type()
				{
					flags.enabled = true;
					flags.splitter = true;
					flags.checked = false;
				}

				menu_item_type::menu_item_type(std::string text, const event_fn_t& f)
					: text(std::move(text)), functor(f)
				{
					flags.enabled = true;
					flags.splitter = false;
					flags.checked = false;
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
				//Impelement renderer_interface
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
					if(at.item_state == state::active)
					{
						graph.rectangle(r, false, static_cast<color_rgb>(0xa8d8eb));
						nana::point points[4] = {
							nana::point(r.x, r.y),
							nana::point(r.x + r.width - 1, r.y),
							nana::point(r.x, r.y + r.height - 1),
							nana::point(r.x + r.width - 1, r.y + r.height - 1)
						};

						graph.palette(false, static_cast<color_rgb>(0xc0ddfc));
						for(int i = 0; i < 4; ++i)
							graph.set_pixel(points[i].x, points[i].y);

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
					
					//Stretchs menu icon only when it doesn't fit, center it otherwise.
					//Contributed by kmribti(pr#102)
					nana::point ipos = pos;
					ipos.x += (image_px - img.size().width ) / 2;
					ipos.y += (image_px - img.size().height) / 2;
					img.paste(graph, ipos);
				}

				void item_text(graph_reference graph, const nana::point& pos, const std::string& text, unsigned text_pixels, const attr& at)
				{
					graph.palette(true, at.enabled ? colors::black : colors::gray_border);
					nana::paint::text_renderer tr(graph);

					auto wstr = to_wstring(text);
					tr.render(pos, wstr.c_str(), wstr.length(), text_pixels, true);
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
				typedef menu_item_type item_type;

				typedef menu_type::item_container::value_type::event_fn_t event_fn_t;
				typedef menu_type::item_container::iterator iterator;
				typedef menu_type::item_container::const_iterator const_iterator;

				menu_builder()
				{
					root_.max_pixels = screen::primary_monitor_size().width * 2 / 3;
					root_.item_pixels = 24;
					renderer_ = pat::cloneable<renderer_interface>(internal_renderer());
				}

				~menu_builder()
				{
					this->destroy();
				}

				void check_style(std::size_t index, checks s)
				{
					if(good_checks(s))
					{
						if(root_.items.size() > index)
							root_.items[index].style = s;
					}
				}

				void checked(std::size_t index, bool check)
				{
					if (root_.items.size() <= index)
						return;

					item_type & m = root_.items[index];
					if(check && (checks::option == m.style))
					{
						if(index)
						{
							std::size_t i = index;
							do
							{
								item_type& el = root_.items[--i];
								if(el.flags.splitter) break;

								if(checks::option == el.style)
									el.flags.checked = false;
							}while(i);
						}

						for(std::size_t i = index + 1; i < root_.items.size(); ++i)
						{
							item_type & el = root_.items[i];
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

				const menu_type& data() const
				{
					return root_;
				}

				void insert(std::size_t pos, std::string&& text, const event_fn_t& fn)
				{
					if(pos < root_.items.size())
						root_.items.emplace(root_.items.begin() + pos, std::move(text), std::ref(fn));
					else
						root_.items.emplace_back(std::move(text), std::ref(fn));
				}

				bool set_sub_menu(std::size_t pos, menu_type &sub)
				{
					if(root_.items.size() > pos)
					{
						menu_item_type & item = root_.items[pos];
						if(item.sub_menu == nullptr)
						{
							item.sub_menu = &sub;
							sub.owner.push_back(&root_);
							return true;
						}
					}
					return false;
				}

				void destroy()
				{
					for(auto i : root_.owner)
						for(auto & m : i->items)
						{
							if(m.sub_menu == &root_)
								m.sub_menu = nullptr;
						}

					for(auto & m : root_.items)
					{
						if(m.sub_menu)
							for(auto i = m.sub_menu->owner.begin(); i != m.sub_menu->owner.end();)
							{
								if((*i) == &root_)
									i = m.sub_menu->owner.erase(i);
								else
									++i;
							}
					}
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

			class menu_drawer
				: public drawer_trigger
			{
			public:
				using item_proxy = menu_item_type::item_proxy;

				renderer_interface * renderer;

				menu_drawer()
				{
					state_.active = npos;
					state_.sub_window = false;
					state_.nullify_mouse = false;

					detail_.border.x = detail_.border.y = 2;
				}

				void close_menu_tree(std::function<void()> && fn)
				{
					fn_close_tree_ = std::move(fn);
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
						API::lazy_refresh();
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
					if (nullptr == menu_) return;

					_m_adjust_window_size();

					renderer->background(graph, *widget_);

					const unsigned item_h_px = _m_item_height();
					const unsigned image_px = item_h_px - 2;
					nana::rectangle item_r(2, 2, graph_->width() - 4, item_h_px);

					unsigned strpixels = item_r.width - 60;

					int text_top_off = (item_h_px - graph.text_extent_size(L"jh({[").height) / 2;

					std::size_t pos = 0;
					for (auto & m : menu_->items)
					{
						if (m.flags.splitter)
						{
							graph_->line({ item_r.x + 40, item_r.y }, { static_cast<int>(graph.width()) - 1, item_r.y }, colors::gray_border);
							item_r.y += 2;
							++pos;
							continue;
						}

						renderer_interface::attr attr = _m_make_renderer_attr(pos == state_.active, m);
						//Draw item background
						renderer->item(*graph_, item_r, attr);

						//Draw text, the text is transformed from orignal for hotkey character
						wchar_t hotkey;
						std::string::size_type hotkey_pos;
						auto text = to_wstring(API::transform_shortkey_text(m.text, hotkey, &hotkey_pos));

						if (m.image.empty() == false)
							renderer->item_image(graph, nana::point(item_r.x + 5, item_r.y + static_cast<int>(item_h_px - image_px) / 2 - 1), image_px, m.image);

						renderer->item_text(graph, nana::point(item_r.x + 40, item_r.y + text_top_off), to_utf8(text), strpixels, attr);

						if (hotkey)
						{
							m.hotkey = hotkey;
							if (m.flags.enabled)
							{
								unsigned off_w = (hotkey_pos ? graph.text_extent_size(text, static_cast<unsigned>(hotkey_pos)).width : 0);
								nana::size hotkey_size = graph.text_extent_size(text.c_str() + hotkey_pos, 1);
								int x = item_r.x + 40 + off_w;
								int y = item_r.y + text_top_off + hotkey_size.height;

								graph_->line({ x, y }, { x + static_cast<int>(hotkey_size.width) - 1, y }, colors::black);
							}
						}

						if (m.sub_menu)
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
					if (menu_->items.empty())
						return false;

					auto pos = state_.active;
					const auto lastpos = menu_->items.size() - 1;

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

						if(! menu_->items.at(pos).flags.splitter)
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
						std::size_t index = _m_get_index_by_pos(pos.x, pos.y);
						if (index != state_.active)
						{
							if ((index == npos) && menu_->items.at(state_.active).sub_menu && state_.sub_window)
								return false;

							state_.active = (index != npos && menu_->items.at(index).flags.splitter) ? npos : index;
							state_.active_timestamp = nana::system::timestamp();
							return true;
						}
					}

					return false;
				}

				void data(menu_type & menu)
				{
					menu_ = & menu;
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

					auto sub = menu_->items.at(state_.active).sub_menu;
					if (sub)
					{
						pos.x = static_cast<int>(graph_->width()) - 2;
						pos.y = 2;

						auto index = state_.active;
						for (auto & m : menu_->items)
						{
							if (0 == index--)
								break;

							if (m.flags.splitter)
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
						if (std::tolower(m.hotkey) != key)
						{
							++index;
							continue;
						}

						if(!m.flags.splitter)
						{
							if(m.sub_menu)
							{
								state_.active = index;
								state_.active_timestamp = nana::system::timestamp();

								API::refresh_window(*widget_);
								return 2;
							}
							else if(m.flags.enabled)
							{
								std::move(fn_close_tree_)();
								item_proxy ip(index, m);
								m.functor.operator()(ip);
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
					for(auto & m : menu_->items)
					{
						unsigned h = (m.flags.splitter ? 1 : _m_item_height());
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

					if(menu_->items.size())
					{
						for(auto & m : menu_->items)
						{
							if(false == m.flags.splitter)
							{
								nana::size item_size = graph_->text_extent_size(m.text);
								if(size.width < item_size.width)
									size.width = item_size.width;
							}
							else
								++size.height;
						}

						size.width += (35 + 40);
						size.height = static_cast<unsigned>(menu_->items.size() - size.height) * _m_item_height() + size.height + static_cast<unsigned>(menu_->items.size() - 1);
					}

					if(size.width > menu_->max_pixels)
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

					auto owner = API::get_owner_window(*widget_);
					API::calc_window_point(owner, pos);
					widget_->move(pos.x, pos.y);
				}
			private:
				widget		*widget_{nullptr};
				paint::graphics	*graph_{nullptr};
				menu_type	*menu_{nullptr};

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
					nana::point	monitor_pos;	//It is used for determinating the monitor.
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

				
				menu_window(window wd, bool is_wd_parent_menu, const point& pos, renderer_interface * rdptr)
					//add a is_wd_parent_menu to determine whether the menu wants the focus.
					//if a submenu gets the focus, the program may cause a crash error when the submenu is being destroyed
					:	base_type(wd, false, rectangle(pos, nana::size(2, 2)), appear::bald<appear::floating>()),
						want_focus_{ (!wd) || ((!is_wd_parent_menu) && (API::focus_window() != wd)) },
						event_focus_{ nullptr }
				{
					caption("nana menu window");
					get_drawer_trigger().close_menu_tree([this]{ this->_m_close_all(); });
					get_drawer_trigger().renderer = rdptr;
					state_.owner_menubar = state_.self_submenu = false;
					state_.auto_popup_submenu = true;

					submenu_.child = submenu_.parent = nullptr;
					submenu_.object = nullptr;

					state_.mouse_pos = API::cursor_position();
					events().mouse_move.connect_unignorable([this]{
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

				void popup(menu_type& menu, bool owner_menubar)
				{
					get_drawer_trigger().data(menu);

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
					events.destroy.connect_unignorable([this]{
						_m_destroy();
					});

					events.key_press.connect_unignorable([this](const arg_keyboard& arg){
						_m_key_down(arg);
					});

					auto fn = [this](const arg_mouse& arg)
					{
						if (event_code::mouse_down == arg.evt_code)
							_m_open_sub(0);	//Try to open submenu immediately
						else if (event_code::mouse_up == arg.evt_code)
							if (arg.button == ::nana::mouse::left_button)
								pick();
					};

					events.mouse_down.connect_unignorable(fn);
					events.mouse_up.connect_unignorable(fn);

					timer_.interval(100);
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
					
					menu_item_type & item = menu->items.at(active);
					if (item.flags.splitter == false && item.sub_menu == nullptr)
					{
						//There is a situation that menu will not call functor if the item style is check_option
						//and it is checked before clicking.
						bool call_functor = true;

						if (checks::highlight == item.style)
						{
							item.flags.checked = !item.flags.checked;
						}
						else if (checks::option == item.style)
						{
							//Forward Looks for a splitter
							auto pos = active;
							while (pos)
							{
								if (menu->items.at(--pos).flags.splitter)
									break;
							}

							for (; pos < menu->items.size(); ++pos)
							{
								menu_item_type & im = menu->items.at(pos);
								if (im.flags.splitter) break;

								if ((checks::option == im.style) && im.flags.checked)
									im.flags.checked = false;
							}

							item.flags.checked = true;
						}

						this->_m_close_all();	//means deleting this;
						//The deleting operation has moved here, because item.functor.operator()(ip)
						//may create a window, which make a killing focus for menu window, if so the close_all
						//operation preformences after item.functor.operator()(ip), that would be deleting this object twice!

						if (call_functor && item.flags.enabled && item.functor)
						{
							item_type::item_proxy ip(active, item);
							item.functor.operator()(ip);
						}
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
					default:
						if (2 != send_shortkey(arg.key))
						{
							if (API::empty_window(*this) == false)
								close();
						}
						else
							this->submenu(true);
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

							menu_window & mwnd = form_loader<menu_window, false>()(handle(), true, pos, drawer.renderer);
							mwnd.state_.self_submenu = true;
							submenu_.child = &mwnd;
							submenu_.child->submenu_.parent = this;
							submenu_.object = menu_ptr;

							API::set_window_z_order(handle(), mwnd.handle(), z_order_action::none);

							mwnd.popup(*menu_ptr, state_.owner_menubar);
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
			drawerbase::menu::menu_window *	uiobj;
			std::function<void()> destroy_answer;
			std::map<std::size_t, info> sub_container;
		};

		menu::menu()
			:impl_(new implement)
		{
			impl_->uiobj = nullptr;
		}

		menu::~menu()
		{
			for(auto i = impl_->sub_container.rbegin(); i != impl_->sub_container.rend(); ++i)
			{
				if(i->second.kill)
					delete i->second.handle;
			}
			delete impl_;
		}

		auto menu::append(const std::string& text, const menu::event_fn_t& f) -> item_proxy
		{
			impl_->mbuilder.data().items.emplace_back(text, f);
			return item_proxy(size() - 1, impl_->mbuilder.data().items.back());
		}

		void menu::append_splitter()
		{
			impl_->mbuilder.data().items.emplace_back();
		}

		void menu::clear()
		{
			impl_->mbuilder.data().items.clear();
		}

		void menu::enabled(std::size_t index, bool enable)
		{
			impl_->mbuilder.data().items.at(index).flags.enabled = enable;
		}

		bool menu::enabled(std::size_t index) const
		{
			return impl_->mbuilder.data().items.at(index).flags.enabled;
		}

		void menu::erase(std::size_t index)
		{
			auto & items = impl_->mbuilder.data().items;
			if(index < items.size())
				items.erase(items.begin() + index);
		}

		void menu::image(std::size_t index, const paint::image& img)
		{
			impl_->mbuilder.data().items.at(index).image = img;
		}

		bool menu::link(std::size_t index, menu& menu_obj)
		{
			if(impl_->mbuilder.set_sub_menu(index, menu_obj.impl_->mbuilder.data()))
			{
				auto& minfo = impl_->sub_container[index];
				minfo.handle = &menu_obj;
				minfo.kill = false;
				return true;
			}
			return false;
		}

		menu* menu::link(std::size_t index)
		{
			auto i = impl_->sub_container.find(index);
			if(i == impl_->sub_container.end())
				return nullptr;
			return i->second.handle;
		}

		menu *menu::create_sub_menu(std::size_t index)
		{
			menu * sub = new menu;

			if (this->link(index, *sub))
			{
				auto& minfo = impl_->sub_container[index];
				minfo.handle = sub;
				minfo.kill = true;
				return sub;
			}

			delete sub;
			return nullptr;
		}

		void menu::popup(window wd, int x, int y)
		{
			_m_popup(wd, x, y, false);
		}

		void menu::popup_await(window wd, int x, int y)
		{
			_m_popup(wd, x, y, false);
			if (impl_->uiobj)
				API::wait_for(impl_->uiobj->handle());
		}

		void menu::close()
		{
			if(impl_->uiobj)
			{
				impl_->uiobj->close();
				impl_->uiobj = nullptr;
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
			return impl_->mbuilder.data().items.at(index).flags.checked;
		}

		void menu::answerer(std::size_t index, const menu::event_fn_t& fn)
		{
			impl_->mbuilder.data().items.at(index).functor = fn;
		}

		void menu::destroy_answer(const std::function<void()>& f)
		{
			impl_->destroy_answer = f;
		}

		void menu::gaps(const nana::point& pos)
		{
			impl_->mbuilder.data().gaps = pos;
		}

		void menu::goto_next(bool forward)
		{
			if(impl_->uiobj)
				impl_->uiobj->goto_next(forward);
		}

		bool menu::goto_submen()
		{
			return (impl_->uiobj ? impl_->uiobj->submenu(true) : false);
		}

		bool menu::exit_submenu()
		{
			return (impl_->uiobj ? impl_->uiobj->submenu(false) : false);
		}

		std::size_t menu::size() const
		{
			return impl_->mbuilder.data().items.size();
		}

		int menu::send_shortkey(wchar_t key)
		{
			return (impl_->uiobj ? impl_->uiobj->send_shortkey(key) : 0);
		}

		void menu::pick()
		{
			if (impl_->uiobj)
				impl_->uiobj->pick();
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

		void menu::_m_popup(window wd, int x, int y, bool called_by_menubar)
		{
			if (impl_->mbuilder.data().items.size())
			{
				close();

				impl_->uiobj = &(form_loader<drawerbase::menu::menu_window, false>()(wd, false, point(x, y), &(*impl_->mbuilder.renderer())));
				impl_->uiobj->events().destroy.connect_unignorable([this]{
					impl_->uiobj = nullptr;
					if (impl_->destroy_answer)
						impl_->destroy_answer();
				});
				impl_->uiobj->popup(impl_->mbuilder.data(), called_by_menubar);
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
			bool popup = false;
			switch(mouse_)
			{
			case mouse::left_button:
				popup = arg.left_button;
				break;
			case mouse::middle_button:
				popup = arg.mid_button;
				break;
			case mouse::right_button:
				popup = arg.right_button;
				break;
			case mouse::any_button:
				popup = true;
			default:
				break;
			}
			if(popup)
				mobj_.popup(owner_, pos_.x, pos_.y);
		}
	//end class
	}//end namespace detail
}//end namespace nana
