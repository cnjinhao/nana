/*
 *	A Treebox Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/treebox.cpp
 */
#include <nana/gui/widgets/treebox.hpp>
#include <nana/gui/widgets/scroll.hpp>
#include <nana/gui/element.hpp>
#include <nana/gui/layout_utility.hpp>
#include <nana/system/platform.hpp>
#include <stdexcept>

namespace nana
{
	arg_treebox::arg_treebox(treebox& wdg, drawerbase::treebox::item_proxy& m, bool op)
		: widget(wdg), item(m), operated{op}
	{}

	namespace drawerbase
	{
		//Here defines some function objects
		namespace treebox
		{
			using node_type = trigger::node_type;

			bool no_sensitive_compare(const std::string& text, const char *pattern, std::size_t len)
			{
				if(len <= text.length())
				{
					auto s = text.c_str();
					for(std::size_t i = 0; i < len; ++i)
					{
						if('a' <= s[i] && s[i] <= 'z')
						{
							if(pattern[i] != s[i] - ('a' - 'A'))
								return false;
						}
						else
							if(pattern[i] != s[i]) return false;
					}
					return true;
				}
				return false;
			}

			const node_type* find_track_child_node(const node_type* node, const node_type * end, const char* pattern, std::size_t len, bool &finish)
			{
				if(node->value.second.expanded)
				{
					node = node->child;
					while(node)
					{
						if(no_sensitive_compare(node->value.second.text, pattern, len)) return node;

						if(node == end) break;

						if(node->value.second.expanded)
						{
							auto t = find_track_child_node(node, end, pattern, len, finish);
							if(t || finish)
								return t;
						}
						node = node->next;
					}
				}

				finish = (node && (node == end));
				return nullptr;
			}

			class tlwnd_drawer
				: public drawer_trigger, public compset_interface
			{
			public:
				using graph_reference = drawer_trigger::graph_reference;

				void assign(const item_attribute_t & item_attr, const pat::cloneable<renderer_interface>* renderer, const pat::cloneable<compset_placer_interface> * compset_placer)
				{
					if(renderer && compset_placer)
					{
						renderer_ = *renderer;
						placer_ = *compset_placer;

						item_attr_ = item_attr;

						_m_draw();
					}
				}
			private:
				void _m_draw()
				{
					item_r_.x = item_r_.y = 0;
					item_r_.width = placer_->item_width(*this->graph_, item_attr_);
					item_r_.height = placer_->item_height(*this->graph_);

					comp_attribute_t attr;
					if(comp_attribute(component::text, attr))
					{
						nana::paint::graphics item_graph({ item_r_.width, item_r_.height });
						item_graph.typeface(graph_->typeface());

						renderer_->set_color(widget_->bgcolor(), widget_->fgcolor());
						renderer_->bground(item_graph, this);
						renderer_->expander(item_graph, this);
						renderer_->crook(item_graph, this);
						renderer_->icon(item_graph, this);
						renderer_->text(item_graph, this);

						item_graph.paste(attr.area, *graph_, 1, 1);
						graph_->rectangle(false, colors::black);
					}
				}
			private:
				// Implementation of drawer_trigger
				void attached(widget_reference wd, graph_reference graph) override
				{
					widget_ = &wd;
					graph_ = &graph;
					graph.typeface(widget_->typeface());
				}
			private:
				// Implementation of compset_interface
				virtual const item_attribute_t& item_attribute() const override
				{
					return item_attr_;
				}

				virtual bool comp_attribute(component_t comp, comp_attribute_t& comp_attr) const override
				{
					comp_attr.area = item_r_;
					return placer_->locate(comp, item_attr_, &comp_attr.area);
				}
			private:
				::nana::paint::graphics * graph_;
				::nana::pat::cloneable<renderer_interface> renderer_;
				::nana::pat::cloneable<compset_placer_interface> placer_;
				widget	*widget_;
				item_attribute_t item_attr_;
				nana::rectangle item_r_;
			};//end class tlwnd_drawer

			class tooltip_window
				: public widget_object<category::root_tag, tlwnd_drawer>
			{
			public:
				tooltip_window(window wd, const rectangle& r)
					: widget_object<category::root_tag, tlwnd_drawer>(wd, false, rectangle(r).pare_off(-1), appear::bald<appear::floating>())
				{
					API::take_active(handle(), false, nullptr);
				}

				drawer_trigger_t & impl()
				{
					return get_drawer_trigger();
				}
			};//end class tooltip_window

			//item_locator should be defined before the definition of basic_implement
			class trigger::item_locator
			{
			public:
				typedef tree_cont_type::node_type node_type;

				item_locator(implement * impl, int item_pos, int x, int y);
				int operator()(node_type &node, int affect);
				node_type * node() const;
				component what() const;
				bool item_body() const;

				nana::rectangle text_pos() const;
			private:
				trigger::implement * impl_;
				nana::point item_pos_;
				const nana::point pos_;		//Mouse pointer position
				component	what_;
				node_type * node_;
				node_attribute node_attr_;
				nana::rectangle node_r_;
				nana::rectangle node_text_r_;
			};

			struct pred_allow_child
			{
				bool operator()(const trigger::tree_cont_type::node_type& node)
				{
					return node.value.second.expanded;
				}
			};

			//struct implement
			//@brief:	some data for treebox trigger
			template<typename Renderer>
			struct trigger::basic_implement
			{
				typedef trigger::node_type node_type;

				struct rep_tag
				{
					nana::paint::graphics * graph;
					::nana::treebox * widget_ptr;
					trigger * trigger_ptr;

					pat::cloneable<compset_placer_interface> comp_placer;
					pat::cloneable<renderer_interface> renderer;
					bool stop_drawing;
				}data;

				struct shape_tag
				{
					nana::upoint border;
					nana::scroll<true> scroll;
					std::size_t	prev_first_value;

					mutable std::map<std::string, node_image_tag> image_table;

					tree_cont_type::node_type * first;
					int indent_pixels;
					int offset_x;
				}shape;

				struct attribute_tag
				{
					bool auto_draw;
					tree_cont_type tree_cont;
				}attr;

				struct node_state_tag
				{
					tooltip_window * tooltip;
					component	comp_pointed;
					tree_cont_type::node_type * pointed;
					tree_cont_type::node_type * selected;
					tree_cont_type::node_type * event_node;
				}node_state;

				struct track_node_tag
				{
					::std::string key_buf;
					std::size_t key_time;
				}track_node;

				struct adjust_tag
				{
					int offset_x_adjust;	//It is a new value of offset_x, and offset_x will be djusted to the new value
					tree_cont_type::node_type * node;
					std::size_t scroll_timestamp;
					nana::timer timer;
				}adjust;
			public:
				basic_implement()
				{
					data.graph			= nullptr;
					data.widget_ptr		= nullptr;
					data.stop_drawing	= false;

					shape.prev_first_value = 0;
					shape.first = nullptr;
					shape.indent_pixels = 10;
					shape.offset_x = 0;

					attr.auto_draw = true;

					node_state.tooltip = nullptr;
					node_state.comp_pointed = component::end;
					node_state.pointed = nullptr;
					node_state.selected = nullptr;
					node_state.event_node = nullptr;

					track_node.key_time = 0;

					adjust.offset_x_adjust = 0;
					adjust.node = nullptr;
					adjust.scroll_timestamp = 0;
				}

				void assign_node_attr(node_attribute& ndattr, const node_type* node) const
				{
					ndattr.has_children = (nullptr != node->child);
					ndattr.expended = node->value.second.expanded;
					ndattr.text = node->value.second.text;
					ndattr.checked = node->value.second.checked;
					ndattr.mouse_pointed = (node_state.pointed == node);
					ndattr.selected = (node_state.selected == node);

					ndattr.icon_hover.close();
					ndattr.icon_normal.close();
					ndattr.icon_expanded.close();
					if(data.comp_placer->enabled(component::icon))
					{
						auto i = shape.image_table.find(node->value.second.img_idstr);
						if(i != shape.image_table.end())
						{
							ndattr.icon_normal = i->second.normal;
							ndattr.icon_expanded = i->second.expanded;
							ndattr.icon_hover = i->second.hovered;
						}
					}
				}

				bool draw(bool scrollbar_react)
				{
					if(data.graph && (false == data.stop_drawing))
					{
						if(scrollbar_react)
							show_scroll();

						//Draw background
						data.graph->rectangle(true, data.widget_ptr->bgcolor());

						//Draw tree
						attr.tree_cont.for_each(shape.first, Renderer(this, nana::point(static_cast<int>(attr.tree_cont.indent_size(shape.first) * shape.indent_pixels) - shape.offset_x, 1)));
						return true;
					}
					return false;
				}

				const trigger::node_type* find_track_node(wchar_t key)
				{
					std::string pattern;

					if('a' <= key && key <= 'z') key -= 'a' - 'A';

					unsigned long now = nana::system::timestamp();

					if (now - track_node.key_time > 1000)
						track_node.key_buf.clear();
					else if((!track_node.key_buf.empty()) && (track_node.key_buf.back() != key))
						pattern = track_node.key_buf;

					track_node.key_time = now;

					if (key <= 0x7f)
					{
						pattern += static_cast<char>(key);
						track_node.key_buf += static_cast<char>(key);
					}
					else
					{
						wchar_t wstr[2] = { key, 0 };
						pattern += to_utf8(wstr);
						track_node.key_buf += to_utf8(wstr);
					}

					const node_type *begin = node_state.selected ? node_state.selected : attr.tree_cont.get_root()->child;

					if(begin)
					{
						const node_type *node = begin;
						const node_type *end = nullptr;
						if(pattern.length() == 1)
						{
							if(node->value.second.expanded && node->child)
							{
								node = node->child;
							}
							else if(!node->next)
							{
								if(!node->owner->next)
								{
									end = begin;
									node = attr.tree_cont.get_root()->child;
								}
								else
									node = node->owner->next;
							}
							else
								node = node->next;
						}

						while(node)
						{
							if(no_sensitive_compare(node->value.second.text, pattern.c_str(), pattern.length())) return node;

							bool finish;
							const node_type *child = find_track_child_node(node, end, pattern.c_str(), pattern.length(), finish);
							if(child)
								return child;

							if(finish || (node == end))
								return nullptr;

							if(!node->next)
							{
								node = (node->owner ? node->owner->next : nullptr);
								if(nullptr == node)
								{
									node = attr.tree_cont.get_root()->child;
									end = begin;
								}
							}
							else
								node = node->next;
						}
					}
					return nullptr;
				}

				static bool check_kinship(const node_type* parent, const node_type * child)
				{
					if((!parent) || (!child))
						return false;

					while(child && (child != parent))
						child = child->owner;

					return (nullptr != child);
				}

				bool make_adjust(node_type * node, int reason)
				{
					if(!node) return false;

					auto & tree_container = attr.tree_cont;

					switch(reason)
					{
					case 0:
						//adjust if the node expanded and the number of its children are over the max number allowed
						if(shape.first != node)
						{
							unsigned child_size = tree_container.child_size_if(*node, pred_allow_child());
							const std::size_t max_allow = max_allowed();

							if(child_size < max_allow)
							{
								unsigned off1 = tree_container.distance_if(shape.first, pred_allow_child());
								unsigned off2 = tree_container.distance_if(node, pred_allow_child());
								const unsigned size = off2 - off1 + child_size + 1;
								if(size > max_allow)
									shape.first = tree_container.advance_if(shape.first, size - max_allow, pred_allow_child());
							}
							else
								shape.first = node;
						}
						break;
					case 1:
					case 2:
					case 3:
						//param is the begin pos of an item in absolute.
						{
							int beg = static_cast<int>(tree_container.indent_size(node) * shape.indent_pixels) - shape.offset_x;
							int end = beg + static_cast<int>(node_w_pixels(node));

							bool take_adjust = false;
							if(reason == 1)
								take_adjust = (beg < 0 || (beg > 0 && end > visible_w_pixels()));
							else if(reason == 2)
								take_adjust = (beg < 0);
							else if(reason == 3)
								return (beg > 0 && end > visible_w_pixels());

							if(take_adjust)
							{
								adjust.offset_x_adjust = shape.offset_x + beg;
								adjust.node = node;
								adjust.timer.start();
								return true;
							}
						}
						break;
					case 4:
						if(shape.first != node)
						{
							unsigned off_first = tree_container.distance_if(shape.first, pred_allow_child());
							unsigned off_node = tree_container.distance_if(node, pred_allow_child());
							if(off_node < off_first)
							{
								shape.first = node;
								return true;
							}
							else if(off_node - off_first > max_allowed())
							{
								shape.first = tree_container.advance_if(0, off_node - max_allowed() + 1, pred_allow_child());
								return true;
							}
						}
						break;
					}
					return false;
				}

				bool set_checked(node_type * node, checkstate cs)
				{
					if (node && node->value.second.checked != cs)
					{
						node->value.second.checked = cs;

						//Don't call the extevent when node is the root node.
						if (node->owner)
						{
							data.stop_drawing = true;
							item_proxy iprx(data.trigger_ptr, node);
							data.widget_ptr->events().checked.emit(::nana::arg_treebox{ *data.widget_ptr, iprx, (checkstate::unchecked != cs) });
							data.stop_drawing = false;
						}
						return true;
					}
					return false;
				}

				bool set_selected(node_type * node)
				{
					if(node_state.selected != node)
					{
						data.stop_drawing = true;
						if (node_state.selected)
						{
							item_proxy iprx(data.trigger_ptr, node_state.selected);
							data.widget_ptr->events().selected.emit(::nana::arg_treebox{ *data.widget_ptr, iprx, false });
						}

						node_state.selected = node;
						if (node)
						{
							item_proxy iprx(data.trigger_ptr, node_state.selected);
							data.widget_ptr->events().selected.emit(::nana::arg_treebox{ *data.widget_ptr, iprx, true });
						}
						data.stop_drawing = false;
						return true;
					}
					return false;
				}

				bool set_expanded(node_type* node, bool value)
				{
					if(node && node->value.second.expanded != value)
					{
						if(value == false)
						{
							//if contracting a parent of the selected node, select the contracted node.
							if(check_kinship(node, node_state.selected))
								set_selected(node);
						}

						node->value.second.expanded = value;
						if(node->child)
						{
							data.stop_drawing = true;
							//attr.ext_event.expand(data.widget_ptr->handle(), item_proxy(data.trigger_ptr, node), value);
							item_proxy iprx(data.trigger_ptr, node);
							data.widget_ptr->events().expanded.emit(::nana::arg_treebox{ *data.widget_ptr, iprx, value });
							data.stop_drawing = false;
						}
						return true;
					}
					return false;
				}

				void show_scroll()
				{
					if(nullptr == data.graph) return;

					std::size_t max_allow = max_allowed();
					std::size_t visual_items = visual_item_size();

					auto & scroll = shape.scroll;
					if(visual_items <= max_allow)
					{
						if(!scroll.empty())
						{
							scroll.close();
							shape.first = nullptr;
						}
					}
					else
					{
						if(scroll.empty())
						{
							shape.prev_first_value = 0;
							scroll.create(*data.widget_ptr, nana::rectangle(data.graph->width() - 16, 0, 16, data.graph->height()));

							auto fn = [this](const arg_mouse& arg){
								this->event_scrollbar(arg);
							};
							auto & events = scroll.events();
							events.mouse_down(fn);
							events.mouse_move(fn);
							events.mouse_wheel(fn);
						}

						scroll.amount(visual_items);
						scroll.range(max_allow);
					}

					scroll.value(attr.tree_cont.distance_if(shape.first, pred_allow_child()));
				}

				void event_scrollbar(const arg_mouse& arg)
				{
					if((event_code::mouse_wheel == arg.evt_code) || arg.is_left_button())
					{
						if(shape.prev_first_value != shape.scroll.value())
						{
							shape.prev_first_value = shape.scroll.value();
							adjust.scroll_timestamp = nana::system::timestamp();
							adjust.timer.start();

							shape.first = attr.tree_cont.advance_if(nullptr, shape.prev_first_value, pred_allow_child());

							if(arg.window_handle == shape.scroll.handle())
							{
								draw(false);
								API::update_window(data.widget_ptr->handle());
							}
						}
					}
				}

				std::size_t visual_item_size() const
				{
					return attr.tree_cont.child_size_if(std::string(), pred_allow_child{});
				}

				int visible_w_pixels() const
				{
					if(!data.graph)
						return 0;

					return static_cast<int>(data.graph->width() - (shape.scroll.empty() ? 0 : shape.scroll.size().width));
				}

				unsigned node_w_pixels(const node_type *node) const
				{
					node_attribute node_attr;
					assign_node_attr(node_attr, node);
					return data.comp_placer->item_width(*data.graph, node_attr);
				}

				std::size_t max_allowed() const
				{
					return (data.graph->height() / data.comp_placer->item_height(*data.graph));
				}

				nana::paint::image* image(const node_type* node)
				{
					const std::string& idstr = node->value.second.img_idstr;
					if(idstr.size())
					{
						auto i = shape.image_table.find(idstr);
						if(i == shape.image_table.end())
							return nullptr;

						unsigned long state = static_cast<unsigned long>(-1);
						if(node_state.pointed == node && (node_state.comp_pointed == component::text || node_state.comp_pointed == component::crook || node_state.comp_pointed == component::icon))
							state = (node_state.pointed != node_state.selected ? 0: 1);
						else if(node_state.selected == node)
							state = 2;

						node_image_tag & nodeimg = i->second;
						if(node->value.second.expanded || (state == 1 || state == 2))
							if(nodeimg.expanded.empty() == false)	return &nodeimg.expanded;

						if(node->value.second.expanded == false && state == 0)
							if(nodeimg.hovered.empty() == false)	return &nodeimg.hovered;

						return &nodeimg.normal;
					}
					return nullptr;
				}

				bool track_mouse(int x, int y)
				{
					int xpos = attr.tree_cont.indent_size(shape.first) * shape.indent_pixels - shape.offset_x;
					item_locator nl(this, xpos, x, y);
					attr.tree_cont.template for_each<item_locator&>(shape.first, nl);

					bool redraw = false;
					node_state.event_node = nl.node();

					if(nl.node() && (nl.what() != component::end))
					{
						if((nl.what() != node_state.comp_pointed || nl.node() != node_state.pointed))
						{
							node_state.comp_pointed = nl.what();

							if (node_state.pointed)
							{
								item_proxy iprx(data.trigger_ptr, node_state.pointed);
								data.widget_ptr->events().hovered.emit(::nana::arg_treebox{ *data.widget_ptr, iprx, false });

								if (nl.node() != node_state.pointed)
									close_tooltip_window();
							}


							node_state.pointed = nl.node();
							item_proxy iprx(data.trigger_ptr, node_state.pointed);
							data.widget_ptr->events().hovered.emit(::nana::arg_treebox{ *data.widget_ptr, iprx, true });

							redraw = (node_state.comp_pointed != component::end);

							if (component::text == node_state.comp_pointed)
							{
								make_adjust(node_state.pointed, 2);
								adjust.scroll_timestamp = 1;

								show_tooltip_window(nl.text_pos());
							}
						}
					}
					else if(node_state.pointed)
					{
						redraw = true;
						node_state.comp_pointed = component::end;
						item_proxy iprx(data.trigger_ptr, node_state.pointed);
						data.widget_ptr->events().hovered.emit(::nana::arg_treebox{ *data.widget_ptr, iprx, false });

						close_tooltip_window();
						node_state.pointed = nullptr;
					}

					return redraw;
				}

				void show_tooltip_window(const rectangle& text_r)
				{
					if(text_r.right() > visible_w_pixels())
					{
						node_state.tooltip = new tooltip_window(data.widget_ptr->handle(), text_r);

						node_attribute node_attr;
						assign_node_attr(node_attr, node_state.pointed);
						node_state.tooltip->impl().assign(node_attr, &data.renderer, &data.comp_placer);
						node_state.tooltip->show();

						auto fn = [this](const arg_mouse& arg)
						{
							switch (arg.evt_code)
							{
							case event_code::mouse_leave:
								close_tooltip_window();
								break;
							case event_code::mouse_move:
								mouse_move_tooltip_window();
								break;
							case event_code::mouse_down:
							case event_code::mouse_up:
							case event_code::dbl_click:
								click_tooltip_window(arg);
								break;
							default:	//ignore other events
								break;
							}
						};

						auto & events = node_state.tooltip->events();
						events.mouse_leave(fn);
						events.mouse_move(fn);
						events.mouse_down.connect(fn);
						events.mouse_up.connect(fn);
						events.dbl_click.connect(fn);
					}
				}

				void close_tooltip_window()
				{
					if(node_state.tooltip)
					{
						tooltip_window * x = node_state.tooltip;
						node_state.tooltip = nullptr;
						delete x;

						if (node_state.pointed)
						{
							node_state.pointed = nullptr;
							draw(false);
							API::update_window(data.widget_ptr->handle());
						}
					}
				}

				void mouse_move_tooltip_window()
				{
					nana::point pos = API::cursor_position();
					API::calc_window_point(data.widget_ptr->handle(), pos);

					if(pos.x >= visible_w_pixels())
						close_tooltip_window();
				}

				void click_tooltip_window(const arg_mouse& arg)
				{
					switch(arg.evt_code)
					{
					case event_code::dbl_click:
					case event_code::mouse_down:
						if(make_adjust(node_state.pointed, 1))
							adjust.scroll_timestamp = 1;
						return;
					case event_code::mouse_up:
						if(node_state.selected == node_state.pointed)
							return;
						set_selected(node_state.pointed);
						break;
					default:
						set_expanded(node_state.selected, !node_state.selected->value.second.expanded);
					}

					draw(false);
					API::update_window(data.widget_ptr->handle());
				}

				void check_child(node_type * node, bool checked)
				{
					set_checked(node, (checked ? checkstate::checked : checkstate::unchecked));
					node = node->child;
					while(node)
					{
						check_child(node, checked);
						node = node->next;
					}
				}
			}; //end struct trigger::implement;

			//class item_proxy
				item_proxy::item_proxy(trigger* trg, trigger::node_type* node)
					: trigger_(trg), node_(node)
				{
					//Make it an end itertor if one of them is a nullptr
					if(nullptr == trg || nullptr == node)
					{
						trigger_ = nullptr;
						node_ = nullptr;
					}
				}

				item_proxy item_proxy::append(const std::string& key, std::string name)
				{
					if(nullptr == trigger_ || nullptr == node_)
						return item_proxy();
					return item_proxy(trigger_, trigger_->insert(node_, key, std::move(name)));
				}

				bool item_proxy::empty() const
				{
					return !(trigger_ && node_);
				}

				std::size_t item_proxy::level() const
				{
					std::size_t n = 0;
					if (trigger_ && node_)
					{
						auto owner = node_->owner;
						while (owner)
						{
							++n;
							owner = owner->owner;
						}
					}
					return n;
				}

				bool item_proxy::checked() const
				{
					return (node_ && (checkstate::checked == node_->value.second.checked));
				}

				item_proxy& item_proxy::check(bool ck)
				{
					trigger_->check(node_, ck ? checkstate::checked : checkstate::unchecked);
					if(trigger_->draw())
						API::update_window(trigger_->impl()->data.widget_ptr->handle());
					return *this;
				}

				bool item_proxy::expanded() const
				{
					return (node_ && node_->value.second.expanded);
				}

				item_proxy& item_proxy::expand(bool exp)
				{
					auto * impl = trigger_->impl();
					if(impl->set_expanded(node_, exp))
					{
						impl->draw(true);
						API::update_window(impl->data.widget_ptr->handle());
					}
					return *this;
				}

				bool item_proxy::selected() const
				{
					return (trigger_->impl()->node_state.selected == node_);
				}

				item_proxy& item_proxy::select(bool s)
				{
					auto * impl = trigger_->impl();
					if(impl->set_selected(s ? node_ : nullptr))
					{
						impl->draw(true);
						API::update_window(*impl->data.widget_ptr);
					}
					return *this;
				}

				const std::string& item_proxy::icon() const
				{
					return node_->value.second.img_idstr;
				}

				item_proxy& item_proxy::icon(const std::string& id)
				{
					node_->value.second.img_idstr = id;
					return *this;
				}

				item_proxy& item_proxy::key(const std::string& kstr)
				{
					trigger_->rename(node_, kstr.data(), nullptr);
					return *this;
				}

				const std::string& item_proxy::key() const
				{
					return node_->value.first;
				}

				const std::string& item_proxy::text() const
				{
					return node_->value.second.text;
				}

				item_proxy& item_proxy::text(const std::string& id)
				{
					trigger_->rename(node_, nullptr, id.data());
					return *this;
				}

				std::size_t item_proxy::size() const
				{
					std::size_t n = 0;
					for(auto child = node_->child; child; child = child->child)
						++n;

					return n;
				}

				item_proxy item_proxy::child() const
				{
					return{ trigger_, node_->child};
				}

				item_proxy item_proxy::owner() const
				{
					return{ trigger_, node_->owner };
				}

				item_proxy item_proxy::sibling() const
				{
					return{ trigger_, node_->next };
				}

				item_proxy item_proxy::begin() const
				{
					return{ trigger_, node_->child };
				}

				item_proxy item_proxy::end() const
				{
					return{};
				}

				item_proxy item_proxy::visit_recursively(std::function<bool(item_proxy)> action)
				{
					if (!action(*this))
						return *this;

					for (auto i : *this)
					{
						auto stop = i.visit_recursively(action);
						if (stop != i.end())
							return stop;
					}
					return end();
				}

				bool item_proxy::operator==(const std::string& s) const
				{
					return (node_ && (node_->value.second.text == s));
				}

				bool item_proxy::operator==(const char* s) const
				{
					return (node_ && (node_->value.second.text == s));
				}

				bool item_proxy::operator==(const wchar_t* s) const
				{
					return (node_ && s && (node_->value.second.text == to_utf8(s)));
				}

				// Behavior of Iterator
				item_proxy& item_proxy::operator=(const item_proxy& r)
				{
					if(this != &r)
					{
						trigger_ = r.trigger_;
						node_ = r.node_;
					}
					return *this;
				}

				item_proxy & item_proxy::operator++()
				{
					if(trigger_ && node_)
						node_ = node_->next;

					return *this;
				}

				item_proxy	item_proxy::operator++(int)
				{
					return sibling();
				}

				item_proxy& item_proxy::operator*()
				{
					return *this;
				}

				const item_proxy& item_proxy::operator*() const
				{
					return *this;
				}

				item_proxy* item_proxy::operator->()
				{
					return this;
				}

				const item_proxy* item_proxy::operator->() const
				{
					return this;
				}

				bool item_proxy::operator==(const item_proxy& rhs) const
				{
					if(empty() != rhs.empty())
						return false;

					//Not empty
					if(node_ && trigger_)
						return ((node_ == rhs.node_) && (trigger_ == rhs.trigger_));

					//Both are empty
					return true;
				}

				bool item_proxy::operator!=(const item_proxy& rhs) const
				{
					return !(this->operator==(rhs));
				}

				nana::any& item_proxy::_m_value()
				{
					return node_->value.second.value;
				}

				const nana::any& item_proxy::_m_value() const
				{
					return node_->value.second.value;
				}

				//Undocumentated methods for internal use.
				trigger::node_type * item_proxy::_m_node() const
				{
					return node_;
				}
			//end class item_proxy

			class internal_placer
				: public compset_placer_interface
			{
				static const unsigned item_offset = 16;
				static const unsigned text_offset = 4;
			private:
				//Implement the compset_locator_interface

				virtual void enable(component_t comp, bool enabled) override
				{
					switch(comp)
					{
					case component_t::crook:
						pixels_crook_ = (enabled ? 16 : 0);
						break;
					case component_t::icon:
						pixels_icon_ = (enabled ? 16 : 0);
						break;
					default:
						break;
					}
				}

				virtual bool enabled(component_t comp) const override
				{
					switch(comp)
					{
					case component_t::crook:
						return (0 != pixels_crook_);
					case component_t::icon:
						return (0 != pixels_icon_);
					default:
						break;
					}
					return true;
				}

				virtual unsigned item_height(graph_reference graph) const override
				{
					return graph.text_extent_size(L"jH{", 3).height + 8;
				}

				virtual unsigned item_width(graph_reference graph, const item_attribute_t& attr) const override
				{
					return graph.text_extent_size(attr.text).width + pixels_crook_ + pixels_icon_ + (text_offset << 1) + item_offset;
				}

				// Locate a component through the specified coordinate.
				// @param comp the component of the item.
				// @param attr the attribute of the item.
				// @param r the pointer refers to a rectangle for receiving the position and size of the component.
				// @returns the true when the component is located by the locator.
				virtual bool locate(component_t comp, const item_attribute_t& attr, rectangle * r) const override
				{
					switch(comp)
					{
					case component_t::expender:
						if(attr.has_children)
						{
							r->width = item_offset;
							return true;
						}
						return false;
					case component_t::bground:
						return true;
					case component_t::crook:
						if(pixels_crook_)
						{
							r->x += item_offset;
							r->width = pixels_crook_;
							return true;
						}
						return false;
					case component_t::icon:
						if(pixels_icon_)
						{
							r->x += item_offset + pixels_crook_;
							r->y = 2;
							r->width = pixels_icon_;
							r->height -= 2;
							return true;
						}
						return false;
					case component_t::text:
						{
							auto text_pos = item_offset + pixels_crook_ + pixels_icon_ + text_offset;
							r->x += text_pos;
							r->width -= (text_pos + text_offset);
						};
						return true;
					default:
						break;
					}
					return false;
				}
			private:
				unsigned pixels_crook_{0};
				unsigned pixels_icon_{0};
			};

			class internal_renderer
				: public renderer_interface
			{
				nana::color bgcolor_;
				nana::color fgcolor_;

				void set_color(const nana::color & bgcolor, const nana::color& fgcolor) override
				{
					bgcolor_ = bgcolor;
					fgcolor_ = fgcolor;
				}

				void bground(graph_reference graph, const compset_interface * compset) const override
				{
					comp_attribute_t attr;

					if(compset->comp_attribute(component::bground, attr))
					{
						const ::nana::color color_table[][2] = { { { 0xE8, 0xF5, 0xFD }, { 0xD8, 0xF0, 0xFA } }, //highlighted
						{ { 0xC4, 0xE8, 0xFA }, { 0xB6, 0xE6, 0xFB } }, //Selected and highlighted
						{ { 0xD5, 0xEF, 0xFC }, {0x99, 0xDE, 0xFD } }  //Selected but not highlighted
														};

						const ::nana::color *clrptr = nullptr;
						if(compset->item_attribute().mouse_pointed)
						{
							if(compset->item_attribute().selected)
								clrptr = color_table[1];
							else
								clrptr = color_table[0];
						}
						else if(compset->item_attribute().selected)
							clrptr = color_table[2];

						if (clrptr)
						{
							graph.rectangle(attr.area, false, clrptr[1]);
							graph.rectangle(attr.area.pare_off(1), true, *clrptr);
						}
					}
				}

				void expander(graph_reference graph, const compset_interface * compset) const override
				{
					comp_attribute_t attr;
					if(compset->comp_attribute(component::expender, attr))
					{
						facade<element::arrow> arrow("solid_triangle");
						arrow.direction(direction::southeast);
						if (!compset->item_attribute().expended)
						{
							arrow.switch_to("hollow_triangle");
							arrow.direction(direction::east);
						}
						auto r = attr.area;
						r.y += (attr.area.height - 16) / 2;
						r.width = r.height = 16;
						arrow.draw(graph, bgcolor_, (attr.mouse_pointed ? colors::deep_sky_blue : colors::black), r, element_state::normal);
					}
				}

				void crook(graph_reference graph, const compset_interface * compset) const override
				{
					comp_attribute_t attr;
					if(compset->comp_attribute(component::crook, attr))
					{
						attr.area.y += (attr.area.height - 16) / 2;
						crook_.check(compset->item_attribute().checked);
						crook_.draw(graph, bgcolor_, fgcolor_, attr.area, attr.mouse_pointed ? element_state::hovered : element_state::normal);
					}
				}

				virtual void icon(graph_reference graph, const compset_interface * compset) const override
				{
					comp_attribute_t attr;
					if(compset->comp_attribute(component::icon, attr))
					{
						const nana::paint::image * img = nullptr;
						auto & item_attr = compset->item_attribute();
						if (item_attr.mouse_pointed)
							img = &(item_attr.icon_hover);
						else if (item_attr.expended)
							img = &(item_attr.icon_expanded);

						if((nullptr == img) || img->empty())
							img = &(item_attr.icon_normal);

						if(! img->empty())
						{
							auto size = img->size();
							if(size.width > attr.area.width || size.height > attr.area.height)
							{
								nana::size fit_size;
								nana::fit_zoom(size, attr.area, fit_size);

								attr.area.x += (attr.area.width - fit_size.width) / 2;
								attr.area.y += (attr.area.height - fit_size.height) / 2;
								attr.area = fit_size;
								img->stretch(::nana::rectangle{ size }, graph, attr.area);
							}
							else
								img->paste(graph, point{ attr.area.x + static_cast<int>(attr.area.width - size.width) / 2, attr.area.y + static_cast<int>(attr.area.height - size.height) / 2 });
						}
					}
				}

				virtual void text(graph_reference graph, const compset_interface * compset) const override
				{
					comp_attribute_t attr;
					if (compset->comp_attribute(component::text, attr))
						graph.string(point{ attr.area.x, attr.area.y + 3 }, compset->item_attribute().text, fgcolor_);
				}
			private:
				mutable facade<element::crook> crook_;
			};


			//class trigger::item_locator
				trigger::item_locator::item_locator(implement * impl, int item_pos, int x, int y)
					:	impl_(impl),
						item_pos_(item_pos, 1),
						pos_(x, y),
						what_(component::end),
						node_(nullptr)
				{}

				int trigger::item_locator::operator()(node_type &node, int affect)
				{
					auto & node_desc = impl_->shape;

					switch(affect)
					{
					case 0: break;
					case 1: item_pos_.x += static_cast<int>(node_desc.indent_pixels); break;
					default:
						if(affect >= 2)
							item_pos_.x -= static_cast<int>(node_desc.indent_pixels) * (affect - 1);
					}

					impl_->assign_node_attr(node_attr_, &node);
					nana::rectangle node_r;
					auto & comp_placer = impl_->data.comp_placer;

					node_r.width = comp_placer->item_width(*impl_->data.graph, node_attr_);
					node_r.height = comp_placer->item_height(*impl_->data.graph);

					if(pos_.y < item_pos_.y + static_cast<int>(node_r.height))
					{
						auto logic_pos = pos_ - item_pos_;
						node_ = &node;

						for(int comp = static_cast<int>(component::begin); comp != static_cast<int>(component::end); ++comp)
						{
							nana::rectangle r = node_r;
							if(comp_placer->locate(static_cast<component>(comp), node_attr_, &r))
							{
								if(r.is_hit(logic_pos))
								{
									what_ = static_cast<component>(comp);
									if(component::expender == what_ && (false == node_attr_.has_children))
										what_ = component::end;

									if(component::text == what_)
										node_text_r_ = r;

									return 0;
								}
							}
						}
					}

					item_pos_.y += node_r.height;

					if(node.value.second.expanded && node.child)
						return 1;

					return 2;
				}

				trigger::item_locator::node_type * trigger::item_locator::node() const
				{
					return node_;
				}

				component trigger::item_locator::what() const
				{
					return what_;
				}

				bool trigger::item_locator::item_body() const
				{
					return (component::text == what_ || component::icon == what_ || component::bground == what_);
				}

				nana::rectangle trigger::item_locator::text_pos() const
				{
					return{node_text_r_.x + item_pos_.x, node_text_r_.y + item_pos_.y, node_text_r_.width, node_text_r_.height};
				}
			//end class item_locator

			class trigger::item_renderer
				: public compset_interface
			{
			public:
				typedef tree_cont_type::node_type node_type;

				item_renderer(implement * impl, const nana::point& pos)
					:	impl_(impl),
						bgcolor_(impl->data.widget_ptr->bgcolor()),
						fgcolor_(impl->data.widget_ptr->fgcolor()),
						pos_(pos)
				{
				}

				//affect
				//0 = Sibling, the last is a sibling of node
				//1 = Owner, the last is the owner of node
				//>=2 = Children, the last is a child of a node that before this node.
				int operator()(const node_type& node, int affect)
				{
					implement * draw_impl = impl_;

					iterated_node_ = &node;
					switch(affect)
					{
					case 1:
						pos_.x += draw_impl->shape.indent_pixels;
						break;
					default:
						if(affect >= 2)
							pos_.x -= draw_impl->shape.indent_pixels * (affect - 1);
					}

					auto & comp_placer = impl_->data.comp_placer;

					impl_->assign_node_attr(node_attr_, iterated_node_);
					node_r_.x = node_r_.y = 0;
					node_r_.width = comp_placer->item_width(*impl_->data.graph, node_attr_);
					node_r_.height = comp_placer->item_height(*impl_->data.graph);

					auto renderer = draw_impl->data.renderer;
					renderer->set_color(bgcolor_, fgcolor_);
					renderer->bground(*draw_impl->data.graph, this);
					renderer->expander(*draw_impl->data.graph, this);
					renderer->crook(*draw_impl->data.graph, this);
					renderer->icon(*draw_impl->data.graph, this);
					renderer->text(*draw_impl->data.graph, this);

					pos_.y += node_r_.height;

					if(pos_.y > static_cast<int>(draw_impl->data.graph->height()))
						return 0;

					return (node.child && node.value.second.expanded ? 1 : 2);
				}
			private:
				//Overrides compset_interface
				virtual const item_attribute_t& item_attribute() const override
				{
					return node_attr_;
				}

				virtual bool comp_attribute(component_t comp, comp_attribute_t& attr) const override
				{
					attr.area = node_r_;
					if(impl_->data.comp_placer->locate(comp, node_attr_, &attr.area))
					{
						attr.area.x += pos_.x;
						attr.area.y += pos_.y;
						return true;
					}
					return false;
				}
			private:
				trigger::implement * impl_;
				::nana::color bgcolor_;
				::nana::color fgcolor_;
				::nana::point pos_;
				const node_type * iterated_node_;
				item_attribute_t node_attr_;
				::nana::rectangle node_r_;
			};
		}

		//Treebox Implementation
		namespace treebox
		{
			//class trigger
				//struct treebox_node_type
					trigger::treebox_node_type::treebox_node_type()
						:expanded(false), checked(checkstate::unchecked)
					{}

					trigger::treebox_node_type::treebox_node_type(std::string text)
						:text(std::move(text)), expanded(false), checked(checkstate::unchecked)
					{}

					trigger::treebox_node_type& trigger::treebox_node_type::operator=(const treebox_node_type& rhs)
					{
						if(this != &rhs)
						{
							text = rhs.text;
							value = rhs.value;
							checked = rhs.checked;
							img_idstr = rhs.img_idstr;
						}
						return *this;
					}
				//end struct treebox_node_type

				trigger::trigger()
					:	impl_(new implement)
				{
					impl_->data.trigger_ptr = this;
					impl_->data.renderer = nana::pat::cloneable<renderer_interface>(internal_renderer());
					impl_->data.comp_placer = nana::pat::cloneable<compset_placer_interface>(internal_placer());

					impl_->adjust.timer.elapse([this]
					{
						auto & adjust = impl_->adjust;
						if (adjust.scroll_timestamp && (nana::system::timestamp() - adjust.scroll_timestamp >= 500))
						{
							if (0 == adjust.offset_x_adjust)
							{
								if (!impl_->make_adjust(adjust.node ? adjust.node : impl_->shape.first, 1))
								{
									adjust.offset_x_adjust = 0;
									adjust.node = nullptr;
									adjust.scroll_timestamp = 0;
									adjust.timer.stop();
									return;
								}
							}

							auto & shape = impl_->shape;
							const int delta = 5;
							int old = shape.offset_x;

							if (shape.offset_x < adjust.offset_x_adjust)
							{
								shape.offset_x += delta;
								if (shape.offset_x > adjust.offset_x_adjust)
									shape.offset_x = adjust.offset_x_adjust;
							}
							else if (shape.offset_x > adjust.offset_x_adjust)
							{
								shape.offset_x -= delta;
								if (shape.offset_x < adjust.offset_x_adjust)
									shape.offset_x = adjust.offset_x_adjust;
							}

							impl_->draw(false);
							API::update_window(impl_->data.widget_ptr->handle());

							if (impl_->node_state.tooltip)
							{
								nana::point pos = impl_->node_state.tooltip->pos();
								impl_->node_state.tooltip->move(pos.x - shape.offset_x + old, pos.y);
							}

							if (shape.offset_x == adjust.offset_x_adjust)
							{
								adjust.offset_x_adjust = 0;
								adjust.node = nullptr;
								adjust.scroll_timestamp = 0;
								adjust.timer.stop();
							}
						}
					});

					impl_->adjust.timer.interval(16);
					impl_->adjust.timer.start();
				}

				trigger::~trigger()
				{
					delete impl_;
				}

				trigger::implement * trigger::impl() const
				{
					return impl_;
				}

				void trigger::auto_draw(bool ad)
				{
					if(impl_->attr.auto_draw != ad)
					{
						impl_->attr.auto_draw = ad;
						if(ad)
							API::update_window(impl_->data.widget_ptr->handle());
					}
				}

				void trigger::checkable(bool enable)
				{
					auto & comp_placer = impl_->data.comp_placer;
					if(comp_placer->enabled(component::crook) != enable)
					{
						comp_placer->enable(component::crook, enable);
						if(impl_->attr.auto_draw)
						{
							impl_->draw(false);
							API::update_window(impl_->data.widget_ptr->handle());
						}
					}
				}

				bool trigger::checkable() const
				{
					return impl_->data.comp_placer->enabled(component::crook);
				}

				void trigger::check(node_type* node, checkstate cs)
				{
					if (!node->owner) return;
					///SUPER NODE, have no value. Keep independent "user-Roots" added with insert
					///The ROOT node is not operational and leave the user-node independent

					if(cs != checkstate::unchecked)
						cs = checkstate::checked;

					//Return if thay are same.
					if(node->value.second.checked == cs)
						return;

					//First, check the children of node, it prevents the use of
					//unactualized child nodes during "on_checked".
					node_type * child = node->child;
					while(child)
					{
						impl_->check_child(child, cs != checkstate::unchecked);
						child = child->next;
					}

					//After that, check self.
					impl_->set_checked(node, cs);

					//Then, change the parent node check state
					node_type * owner = node->owner;

					/// SUPER NODE, have no value. Keep independent "user-Roots" added with insert
					/// Make sure that the owner is not the ROOT node.
					while(owner->owner)
					{
						std::size_t len_checked = 0;
						std::size_t size = 0;
						checkstate cs = checkstate::unchecked;
						child = owner->child;
						while(child)
						{
							++size;
							if(checkstate::checked == child->value.second.checked)
							{
								++len_checked;
								if(size != len_checked)
								{
									cs = checkstate::partial;
									break;
								}
							}
							else if((checkstate::partial == child->value.second.checked) || (len_checked && (len_checked < size)))
							{
								cs = checkstate::partial;
								break;
							}
							child = child->next;
						}

						if(size && (size == len_checked))
							cs = checkstate::checked;

						if(cs == owner->value.second.checked)
							break;

						impl_->set_checked(owner, cs);
						owner = owner->owner;
					}
				}

				bool trigger::draw()
				{
					if (!impl_->attr.auto_draw)
						return false;

					impl_->draw(false);
					return true;
				}

				auto trigger::tree() -> tree_cont_type &
				{
					return impl_->attr.tree_cont;
				}

				auto trigger::tree() const -> tree_cont_type const &
				{
					return impl_->attr.tree_cont;
				}

				void trigger::renderer(::nana::pat::cloneable<renderer_interface>&& r)
				{
					impl_->data.renderer = std::move(r);
				}

				const ::nana::pat::cloneable<renderer_interface>& trigger::renderer() const
				{
					return impl_->data.renderer;
				}

				void trigger::placer(::nana::pat::cloneable<compset_placer_interface>&& r)
				{
					impl_->data.comp_placer = std::move(r);
				}

				const ::nana::pat::cloneable<compset_placer_interface>& trigger::placer() const
				{
					return impl_->data.comp_placer;
				}

				nana::any & trigger::value(node_type* node) const
				{
					if(impl_->attr.tree_cont.verify(node) == false)
						throw std::invalid_argument("Nana.GUI.treebox.value() invalid node");

					return node->value.second.value;
				}

				trigger::node_type* trigger::insert(node_type* node, const std::string& key, std::string&& title)
				{
					node_type * p = impl_->attr.tree_cont.node(node, key);
					if(p)
						p->value.second.text.swap(title);
					else
						p = impl_->attr.tree_cont.insert(node, key, treebox_node_type(std::move(title)));

					if(p && impl_->attr.auto_draw && impl_->draw(true))
						API::update_window(impl_->data.widget_ptr->handle());
					return p;
				}

				trigger::node_type* trigger::insert(const std::string& path, std::string&& title)
				{
					auto x = impl_->attr.tree_cont.insert(path, treebox_node_type(std::move(title)));
					if(x && impl_->attr.auto_draw && impl_->draw(true))
						API::update_window(impl_->data.widget_ptr->handle());
					return x;
				}

				bool trigger::verify(const void* node) const
				{
					return impl_->attr.tree_cont.verify(reinterpret_cast<const node_type*>(node));
				}

				bool trigger::verify_kinship(node_type* parent, node_type* child) const
				{
					if(false == (parent && child)) return false;

					while(child && (child != parent))
						child = child->owner;

					return (nullptr != child);
				}

				void trigger::remove(node_type* node)
				{
					if(!verify(node))
						return;

					auto & shape = impl_->shape;
					auto & node_state = impl_->node_state;

					if(verify_kinship(node, node_state.event_node))
						node_state.event_node = nullptr;

					if(verify_kinship(node, shape.first))
						shape.first = nullptr;

					if(verify_kinship(node, node_state.selected))
						node_state.selected = nullptr;

					impl_->attr.tree_cont.remove(node);
				}

				trigger::node_type* trigger::selected() const
				{
					return impl_->node_state.selected;
				}

				void trigger::selected(node_type* node)
				{
					if(impl_->attr.tree_cont.verify(node) && impl_->set_selected(node))
					{
						impl_->draw(true);
						API::update_window(impl_->data.widget_ptr->handle());
					}
				}

				void trigger::set_expand(node_type* node, bool exp)
				{
					if((impl_->data.widget_ptr) && impl_->set_expanded(node, exp))
					{
						impl_->draw(true);
						API::update_window(impl_->data.widget_ptr->handle());
					}
				}

				void trigger::set_expand(const std::string& path, bool exp)
				{
					if(impl_->set_expanded(impl_->attr.tree_cont.find(path), exp))
					{
						impl_->draw(true);
						API::update_window(impl_->data.widget_ptr->handle());
					}
				}

				node_image_tag& trigger::icon(const std::string& id) const
				{
					auto i = impl_->shape.image_table.find(id);
					if(i != impl_->shape.image_table.end())
						return i->second;

					impl_->data.comp_placer->enable(component::icon, true);

					return impl_->shape.image_table[id];
				}

				void trigger::icon_erase(const std::string& id)
				{
					impl_->shape.image_table.erase(id);
					if(0 == impl_->shape.image_table.size())
						impl_->data.comp_placer->enable(component::icon, false);
				}

				void trigger::node_icon(node_type* node, const std::string& id)
				{
					if(tree().verify(node))
					{
						node->value.second.img_idstr = id;
						auto i = impl_->shape.image_table.find(id);
						if((i != impl_->shape.image_table.end()) && impl_->draw(true))
							API::update_window(impl_->data.widget_ptr->handle());
					}
				}

				unsigned trigger::node_width(const node_type *node) const
				{
					node_attribute node_attr;
					impl_->assign_node_attr(node_attr, node);
					return impl_->data.comp_placer->item_width(*impl_->data.graph, node_attr);
				}

				bool trigger::rename(node_type *node, const char* key, const char* name)
				{
					if((key || name ) && tree().verify(node))
					{
						if(key && (key != node->value.first))
						{
							node_type * element = node->owner->child;
							for(; element; element = element->next)
							{
								if((element->value.first == key) && (node != element))
									return false;
							}
							node->value.first = key;
						}

						if(name)
							node->value.second.text = name;

						return (key || name);
					}
					return false;

				}

				void trigger::attached(widget_reference widget, graph_reference graph)
				{
					impl_->data.graph = &graph;

					widget.bgcolor(colors::white);
					impl_->data.widget_ptr = static_cast< ::nana::treebox*>(&widget);
					widget.caption("nana treebox");
				}

				void trigger::refresh(graph_reference)
				{
					impl_->draw(false);
				}

				void trigger::dbl_click(graph_reference, const arg_mouse& arg)
				{
					auto & shape = impl_->shape;

					int xpos = impl_->attr.tree_cont.indent_size(shape.first) * shape.indent_pixels - shape.offset_x;
					item_locator nl(impl_, xpos, arg.pos.x, arg.pos.y);
					impl_->attr.tree_cont.for_each<item_locator&>(shape.first, nl);

					if(nl.node() && (nl.what() == component::text || nl.what() == component::icon))
					{
						impl_->node_state.event_node = nl.node();
						impl_->set_expanded(impl_->node_state.event_node, !impl_->node_state.event_node->value.second.expanded);
						impl_->draw(true);
						API::lazy_refresh();
					}
				}

				void trigger::mouse_down(graph_reference, const arg_mouse& arg)
				{
					auto & shape = impl_->shape;

					int xpos = impl_->attr.tree_cont.indent_size(shape.first) * shape.indent_pixels - shape.offset_x;
					item_locator nl(impl_, xpos, arg.pos.x, arg.pos.y);
					impl_->attr.tree_cont.for_each<item_locator&>(shape.first, nl);

					bool has_redraw = false;

					auto & node_state = impl_->node_state;
					node_state.event_node = nullptr;

					if(nl.node())
					{
						node_state.event_node = nl.node();
						if(nl.what() != component::end)
						{
							if(nl.what() ==  component::expender)
							{
								if(impl_->set_expanded(node_state.event_node, !node_state.event_node->value.second.expanded))
									impl_->make_adjust(node_state.event_node, 0);

								has_redraw = true;
							}
							else if(nl.item_body())
							{
								if(node_state.selected != node_state.event_node)
								{
									impl_->set_selected(node_state.event_node);
									has_redraw = true;
								}
							}
						}
						else if(node_state.selected != node_state.event_node)
						{
							impl_->set_selected(node_state.event_node);
							has_redraw = true;
						}
					}

					if(has_redraw)
					{
						impl_->draw(true);
						API::lazy_refresh();
					}
				}

				void trigger::mouse_up(graph_reference, const arg_mouse& arg)
				{
					auto & shape = impl_->shape;

					int xpos = impl_->attr.tree_cont.indent_size(shape.first) * shape.indent_pixels - shape.offset_x;
					item_locator nl(impl_, xpos, arg.pos.x, arg.pos.y);
					impl_->attr.tree_cont.for_each<item_locator&>(shape.first, nl);

					if(!nl.node())
						return;

					if((impl_->node_state.selected != nl.node()) && nl.item_body())
					{
						impl_->set_selected(nl.node());
						if(impl_->make_adjust(impl_->node_state.selected, 1))
							impl_->adjust.scroll_timestamp = 1;
					}
					else if (nl.what() == component::crook)
					{
						checkstate cs = checkstate::unchecked;
						if (checkstate::unchecked == nl.node()->value.second.checked)
							cs = checkstate::checked;

						check(nl.node(), cs);
					}
					else
						return;	//Do not refresh

					impl_->draw(true);
					API::lazy_refresh();
				}

				void trigger::mouse_move(graph_reference, const arg_mouse& arg)
				{
					if(impl_->track_mouse(arg.pos.x, arg.pos.y))
					{
						impl_->draw(false);
						API::lazy_refresh();
					}
				}

				void trigger::mouse_wheel(graph_reference, const arg_wheel& arg)
				{
					auto & shape = impl_->shape;
					std::size_t prev = shape.prev_first_value;

					shape.scroll.make_step(!arg.upwards);

					impl_->event_scrollbar(arg);

					if(prev != shape.prev_first_value)
					{
						impl_->track_mouse(arg.pos.x, arg.pos.y);

						impl_->draw(false);
						API::lazy_refresh();
					}
				}

				void trigger::mouse_leave(graph_reference, const arg_mouse&)
				{
					if (impl_->node_state.pointed && (!impl_->node_state.tooltip))
					{
						item_proxy iprx(impl_->data.trigger_ptr, impl_->node_state.pointed);
						impl_->data.widget_ptr->events().hovered.emit(::nana::arg_treebox{ *impl_->data.widget_ptr, iprx, false });
						impl_->node_state.pointed = nullptr;
						impl_->draw(false);
						API::lazy_refresh();
					}
				}

				void trigger::resized(graph_reference, const arg_resized&)
				{
					impl_->draw(false);
					API::lazy_refresh();
					impl_->show_scroll();
					if(!impl_->shape.scroll.empty())
					{
						nana::size s = impl_->data.graph->size();
						impl_->shape.scroll.move(rectangle{ static_cast<int>(s.width) - 16, 0, 16, s.height });
					}
				}

				void trigger::key_press(graph_reference, const arg_keyboard& arg)
				{
					bool redraw = false;
					bool scroll = false; //Adjust the scrollbar

					auto & node_state = impl_->node_state;

					switch(arg.key)
					{
					case keyboard::os_arrow_up:
						if(node_state.selected && node_state.selected != impl_->attr.tree_cont.get_root()->child)
						{
							node_type * prev = node_state.selected->owner;
							if(prev->child != node_state.selected)
							{
								prev = prev->child;
								while(prev->next != node_state.selected)
									prev = prev->next;

								while(prev->child && prev->value.second.expanded)
								{
									prev = prev->child;
									while(prev->next)
										prev = prev->next;
								}
							}

							impl_->set_selected(prev);

							if(impl_->make_adjust(prev, 4))
								scroll = true;

							redraw = true;
						}
						break;
					case keyboard::os_arrow_down:
						if(node_state.selected)
						{
							node_type * node = node_state.selected;
							if(node->value.second.expanded)
							{
								node = node->child;
							}
							else if(node->next)
							{
								node = node->next;
							}
							else
							{
								node = node->owner;
								while(node && (nullptr == node->next))
									node = node->owner;

								if(node)
									node = node->next;
							}

							if(node)
							{
								impl_->set_selected(node);
								redraw = true;
								scroll = impl_->make_adjust(node_state.selected, 4);
							}
						}
						break;
					case keyboard::os_arrow_left:
						if(node_state.selected)
						{
							if(node_state.selected->value.second.expanded == false)
							{
								if(node_state.selected->owner != impl_->attr.tree_cont.get_root())
								{
									impl_->set_selected(node_state.selected->owner);
									impl_->make_adjust(node_state.selected, 4);
								}
							}
							else
								impl_->set_expanded(node_state.selected, false);

							redraw = true;
							scroll = true;
						}
						break;
					case keyboard::os_arrow_right:
						if(node_state.selected)
						{
							if(node_state.selected->value.second.expanded == false)
							{
								impl_->set_expanded(node_state.selected, true);
								redraw = true;
								scroll = true;
							}
							else if(node_state.selected->child)
							{
								impl_->set_selected(node_state.selected->child);
								impl_->make_adjust(node_state.selected, 4);
								redraw = true;
								scroll = true;
							}
						}
						break;
					}

					if(redraw)
					{
						impl_->draw(scroll);
						API::lazy_refresh();
					}
				}

				void trigger::key_char(graph_reference, const arg_keyboard& arg)
				{
					int do_refresh = 0;
					if ('*' == arg.key)
					{
						item_proxy{this, impl_->node_state.selected}
							.visit_recursively([this](item_proxy && i)
						{
							/// Same semantics as i.expand(true), but more efficient.
							impl_->set_expanded(i._m_node(), true);
							return true;
						});
						do_refresh = 1;	//reacts scrollbar
					}

					auto node = const_cast<node_type*>(impl_->find_track_node(arg.key));
					if(node && (node != impl_->node_state.selected))
					{
						impl_->set_selected(node);
						impl_->make_adjust(node, 4);
						do_refresh |= 2; //No need to reacts scrollbar
					}

					if (do_refresh)
					{
						impl_->draw(do_refresh & 1);
						API::lazy_refresh();
					}
				}
			//end class trigger
		}//end namespace treebox
	}//end namespace drawerbase

	//class treebox
		treebox::treebox(){}

		treebox::treebox(window wd, bool visible)
		{
			create(wd, rectangle(), visible);
		}

		treebox::treebox(window wd, const rectangle& r, bool visible)
		{
			create(wd, r, visible);
		}

		const nana::pat::cloneable<treebox::renderer_interface> & treebox::renderer() const
		{
			return get_drawer_trigger().impl()->data.renderer;
		}

		const nana::pat::cloneable<treebox::compset_placer_interface> & treebox::placer() const
		{
			return get_drawer_trigger().impl()->data.comp_placer;
		}

		void treebox::auto_draw(bool ad)
		{
			get_drawer_trigger().auto_draw(ad);
		}

		treebox & treebox::checkable(bool enable)
		{
			get_drawer_trigger().checkable(enable);
			return *this;
		}

		bool treebox::checkable() const
		{
			return get_drawer_trigger().checkable();
		}

		treebox::node_image_type& treebox::icon(const std::string& id) const
		{
			return get_drawer_trigger().icon(id);
		}

		void treebox::icon_erase(const std::string& id)
		{
			get_drawer_trigger().icon_erase(id);
		}

		auto treebox::find(const std::string& keypath) -> item_proxy
		{
			auto * trg = &get_drawer_trigger();
			return item_proxy(trg, trg->tree().find(keypath));
		}

		treebox::item_proxy treebox::insert(const std::string& path_key, std::string title)
		{
			return item_proxy(&get_drawer_trigger(), get_drawer_trigger().insert(path_key, std::move(title)));
		}

		treebox::item_proxy treebox::insert(item_proxy i, const std::string& key, std::string title)
		{
			return item_proxy(&get_drawer_trigger(), get_drawer_trigger().insert(i._m_node(), key, std::move(title)));
		}

		treebox::item_proxy treebox::erase(item_proxy i)
		{
			auto next = i.sibling();
			get_drawer_trigger().remove(i._m_node());
			return next;
		}

		void treebox::erase(const std::string& keypath)
		{
			auto i = find(keypath);
			if(!i.empty())
				get_drawer_trigger().remove(i._m_node());
		}

		std::string treebox::make_key_path(item_proxy i, const std::string& splitter) const
		{
			auto & tree = get_drawer_trigger().tree();
			auto pnode = i._m_node();
			if(tree.verify(pnode))
			{
				auto root = tree.get_root();
				std::string path;
				std::string temp;
				while(pnode->owner != root)
				{
					temp = splitter;
					temp += pnode->value.first;
					path.insert(0, temp);
					pnode = pnode->owner;
				}

				path.insert(0, pnode->value.first);
				return std::move(path);
			}
			return{};
		}

		treebox::item_proxy treebox::selected() const
		{
			return item_proxy(const_cast<drawer_trigger_t*>(&get_drawer_trigger()), get_drawer_trigger().selected());
		}
	//end class treebox
}//end namespace nana
