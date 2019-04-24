/*
 *	A Treebox Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
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
#include <map>

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

			class exclusive_scroll_operation
				: public scroll_operation_interface
			{
			public:
				exclusive_scroll_operation(std::shared_ptr<nana::scroll<true>>& scroll_wdg)
					:scroll_(scroll_wdg)
				{}

				bool visible(bool vert) const override
				{
					if (vert)
						return !scroll_->empty();

					return false;
				}
			private:
				std::shared_ptr<nana::scroll<true>> scroll_;
			};

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

						renderer_->begin_paint(*widget_);
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

			//item_locator should be defined before the definition of implementation
			class trigger::item_locator
			{
			public:
				using node_type = tree_cont_type::node_type;

				item_locator(implementation * impl, int item_pos, int x, int y);
				int operator()(node_type &node, int affect);
				node_type * node() const;
				component what() const;
				bool item_body() const;

				nana::rectangle text_pos() const;
			private:
				implementation * const impl_;
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

			//struct implementation
			//@brief:	some data for treebox trigger
			class trigger::implementation
			{
				class item_rendering_director
					: public compset_interface
				{
				public:
					using node_type = tree_cont_type::node_type;

					item_rendering_director(implementation * impl, const nana::point& pos):
						impl_(impl),
						pos_(pos)
					{
					}

					//affect
					//0 = Sibling, the last is a sibling of node
					//1 = Owner, the last is the owner of node
					//>=2 = Children, the last is a child of a node that before this node.
					int operator()(const node_type& node, int affect)
					{
						iterated_node_ = &node;
						switch (affect)
						{
						case 1:
							pos_.x += impl_->data.scheme_ptr->indent_displacement;
							break;
						default:
							if (affect >= 2)
								pos_.x -= impl_->data.scheme_ptr->indent_displacement * (affect - 1);
						}

						auto & comp_placer = impl_->data.comp_placer;

						impl_->assign_node_attr(node_attr_, iterated_node_);
						node_r_.x = node_r_.y = 0;
						node_r_.width = comp_placer->item_width(*impl_->data.graph, node_attr_);
						node_r_.height = comp_placer->item_height(*impl_->data.graph);

						auto renderer = impl_->data.renderer;
						renderer->begin_paint(*impl_->data.widget_ptr);
						renderer->bground(*impl_->data.graph, this);
						renderer->expander(*impl_->data.graph, this);
						renderer->crook(*impl_->data.graph, this);
						renderer->icon(*impl_->data.graph, this);
						renderer->text(*impl_->data.graph, this);

						pos_.y += node_r_.height;

						if (pos_.y > static_cast<int>(impl_->data.graph->height()))
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
						if (impl_->data.comp_placer->locate(comp, node_attr_, &attr.area))
						{
							attr.mouse_pointed = node_attr_.mouse_pointed;
							attr.area.x += pos_.x;
							attr.area.y += pos_.y;
							return true;
						}
						return false;
					}
				private:
					implementation * const impl_;
					::nana::point pos_;
					const node_type * iterated_node_;
					item_attribute_t node_attr_;
					::nana::rectangle node_r_;
				};

			public:
				using node_type = trigger::node_type;

				struct rep_tag
				{
					nana::paint::graphics * graph;
					::nana::treebox * widget_ptr;
					::nana::treebox::scheme_type* scheme_ptr;
					trigger * trigger_ptr;

					pat::cloneable<compset_placer_interface> comp_placer;
					pat::cloneable<renderer_interface> renderer;
					bool stop_drawing;
				}data;

				struct shape_tag
				{
					std::shared_ptr<nana::scroll<true>> scroll;

					mutable std::map<std::string, node_image_tag> image_table;

					tree_cont_type::node_type * first; //The node at the top of screen
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
					node_type * pointed;
					node_type * selected;
					node_type * pressed_node;
				}node_state;

				struct track_node_tag
				{
					::std::string key_buf;
					std::size_t key_time;
				}track_node;

				struct adjust_tag
				{
					int offset_x_adjust;	//It is a new value of offset_x, and offset_x will be adjusted to the new value
					tree_cont_type::node_type * node;
					std::size_t scroll_timestamp;
					nana::timer timer;
				}adjust;
			public:
				implementation()
				{
					data.graph			= nullptr;
					data.widget_ptr		= nullptr;
					data.stop_drawing	= false;

					shape.first = nullptr;
					shape.offset_x = 0;
					shape.scroll = std::make_shared<nana::scroll<true>>();

					attr.auto_draw = true;

					node_state.tooltip = nullptr;
					node_state.comp_pointed = component::end;
					node_state.pointed = nullptr;
					node_state.selected = nullptr;
					node_state.pressed_node = nullptr;

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

				bool unlink(node_type* node, bool perf_clear)
				{
					if (!attr.tree_cont.verify(node))
						return false;

					if (node->is_ancestor_of(shape.first))
					{
						shape.first = node->front();
						if (shape.first)
							shape.first = node->owner;
					}

					if (node->is_ancestor_of(node_state.pointed))
						node_state.pointed = nullptr;

					if (node->is_ancestor_of(node_state.selected))
						node_state.selected = nullptr;

					if (perf_clear)
					{
						if (node->child)
						{
							attr.tree_cont.clear(node);
							return true;
						}
						return false;
					}

					attr.tree_cont.remove(node);
					return true;
				}

				static constexpr unsigned margin_top_bottom()
				{
					return 1;
				}

				bool draw(bool reset_scroll, bool ignore_update = false, bool ignore_auto_draw = false)
				{
					if(data.graph && (false == data.stop_drawing))
					{
						if (reset_scroll)
							show_scroll();

						if (attr.auto_draw || ignore_auto_draw)
						{
							//Draw background
							rectangle bground_r{ data.graph->size() };
							if (!API::dev::copy_transparent_background(data.widget_ptr->handle(), bground_r, *data.graph, {}))
								data.graph->rectangle(true, data.widget_ptr->bgcolor());

							//Draw tree
							attr.tree_cont.for_each(shape.first, item_rendering_director(this, nana::point(static_cast<int>(attr.tree_cont.indent_size(shape.first) * data.scheme_ptr->indent_displacement) - shape.offset_x, margin_top_bottom())));

							if (!ignore_update)
								API::update_window(data.widget_ptr->handle());

							return true;
						}
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

				node_type* last(bool ignore_folded_children) const
				{
					auto p = attr.tree_cont.get_root();

					while (true)
					{
						while (p->next)
							p = p->next;

						if (p->child)
						{
							if (p->value.second.expanded || !ignore_folded_children)
							{
								p = p->child;
								continue;
							}
						}

						break;
					}

					return p;
				}

				std::size_t screen_capacity(bool completed) const
				{
					auto const item_px = data.comp_placer->item_height(*data.graph);
					auto screen_px = data.graph->size().height - (margin_top_bottom() << 1);
					
					if (completed || ((screen_px % item_px) == 0))
						return screen_px / item_px;

					return screen_px / item_px + 1;
				}

				bool scroll_into_view(node_type* node, bool use_bearing, align_v bearing)
				{
					auto & tree = attr.tree_cont;

					auto parent = node->owner;

					std::vector<node_type*> parent_path;
					while (parent)
					{
						parent_path.push_back(parent);
						parent = parent->owner;
					}

					bool has_expanded = false;

					//Expands the shrinked nodes which are ancestors of node
					for (auto i = parent_path.rbegin(); i != parent_path.rend(); ++i)
					{
						if (!(*i)->value.second.expanded)
						{
							has_expanded = true;
							(*i)->value.second.expanded = true;
							item_proxy iprx(data.trigger_ptr, *i);
							data.widget_ptr->events().expanded.emit(::nana::arg_treebox{ *data.widget_ptr, iprx, true }, data.widget_ptr->handle());
						}
					}

					auto pos = tree.distance_if(node, pred_allow_child{});
					auto last_pos = tree.distance_if(last(true), pred_allow_child{});

					auto const capacity = screen_capacity(true);

					//If use_bearing is false, it calculates a bearing depending on the current
					//position of the requested item.
					if (!use_bearing)
					{
						auto first_pos = tree.distance_if(shape.first, pred_allow_child{});

						if (pos < first_pos)
							bearing = align_v::top;
						else if (pos >= first_pos + capacity)
							bearing = align_v::bottom;
						else
						{
							//The item is already in the view.
							//Returns true if a draw operation is needed
							return has_expanded;
						}
					}

					if (align_v::top == bearing)
					{
						if (last_pos - pos + 1 < capacity)
						{
							if (last_pos + 1 >= capacity)
								pos = last_pos + 1 - capacity;
							else
								pos = 0;
						}
					}
					else if (align_v::center == bearing)
					{
						auto const short_side = (std::min)(pos, last_pos - pos);
						if (short_side >= capacity / 2)
							pos -= capacity / 2;
						else if (short_side == pos || (last_pos + 1 < capacity))
							pos = 0;
						else
							pos = last_pos + 1 - capacity;
					}
					else if (align_v::bottom == bearing)
					{
						if (pos + 1 >= capacity)
							pos = pos + 1 - capacity;
						else
							pos = 0;
					}

					auto prv_first = shape.first;
					shape.first = attr.tree_cont.advance_if(nullptr, pos, drawerbase::treebox::pred_allow_child{});

					//Update the position of scroll
					show_scroll();

					return has_expanded || (prv_first != shape.first);
				}

				bool make_adjust(node_type * node, int reason)
				{
					if(!node) return false;

					auto & tree = attr.tree_cont;

					auto const first_pos = tree.distance_if(shape.first, pred_allow_child{});
					auto const node_pos = tree.distance_if(node, pred_allow_child{});
					auto const max_allow = max_allowed();
					switch(reason)
					{
					case 0:
						if (node->value.second.expanded)
						{
							//adjust if the number of its children are over the max number allowed
							if (shape.first != node)
							{
								auto child_size = tree.child_size_if(*node, pred_allow_child());
								if (child_size < max_allow)
								{
									auto const size = node_pos - first_pos + child_size + 1;
									if (size > max_allow)
										shape.first = tree.advance_if(shape.first, size - max_allow, pred_allow_child{});
								}
								else
									shape.first = node;
							}
						}
						else
						{
							//The node is shrank
							auto visual_size = visual_item_size();
							if (visual_size > max_allow)
							{
								if (first_pos + max_allow > visual_size)
									shape.first = tree.advance_if(nullptr, visual_size - max_allow, pred_allow_child{});
							}
							else
								shape.first = nullptr;
						}
						break;
					case 1:
					case 2:
					case 3:
						//param is the begin pos of an item in absolute.
						{
							int beg = static_cast<int>(tree.indent_size(node) * data.scheme_ptr->indent_displacement) - shape.offset_x;
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
							if (node_pos < first_pos)
							{
								shape.first = node;
								return true;
							}
							else if (node_pos - first_pos > max_allow)
							{
								shape.first = tree.advance_if(nullptr, node_pos - max_allow + 1, pred_allow_child{});
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
							data.widget_ptr->events().checked.emit(::nana::arg_treebox{ *data.widget_ptr, iprx, (checkstate::unchecked != cs) }, data.widget_ptr->handle());
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
							data.widget_ptr->events().selected.emit(::nana::arg_treebox{ *data.widget_ptr, iprx, false }, data.widget_ptr->handle());
						}

						node_state.selected = node;
						if (node)
						{
							item_proxy iprx(data.trigger_ptr, node_state.selected);
							data.widget_ptr->events().selected.emit(::nana::arg_treebox{ *data.widget_ptr, iprx, true }, data.widget_ptr->handle());
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
							if (node->is_ancestor_of(node_state.selected))
								set_selected(node);
						}

						node->value.second.expanded = value;
						if(node->child)
						{
							data.stop_drawing = true;
							item_proxy iprx(data.trigger_ptr, node);
							data.widget_ptr->events().expanded.emit(::nana::arg_treebox{ *data.widget_ptr, iprx, value }, data.widget_ptr->handle());
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

					auto & scroll = *shape.scroll;
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
							scroll.create(*data.widget_ptr, nana::rectangle(data.graph->width() - 16, 0, 16, data.graph->height()));

							scroll.events().value_changed.connect_unignorable([this](const arg_scroll&)
							{
								adjust.scroll_timestamp = nana::system::timestamp();
								adjust.timer.start();

								shape.first = attr.tree_cont.advance_if(nullptr, shape.scroll->value(), pred_allow_child{});
								draw(false, false, true);
							});
						}

						scroll.amount(visual_items);
						scroll.range(max_allow);
					}

					auto pos = attr.tree_cont.distance_if(shape.first, pred_allow_child{});
					scroll.value(pos);
				}

				std::size_t visual_item_size() const
				{
					return attr.tree_cont.child_size_if(std::string(), pred_allow_child{});
				}

				int visible_w_pixels() const
				{
					if(!data.graph)
						return 0;

					return static_cast<int>(data.graph->width() - (shape.scroll->empty() ? 0 : shape.scroll->size().width));
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
					int xpos = attr.tree_cont.indent_size(shape.first) * data.scheme_ptr->indent_displacement - shape.offset_x;
					item_locator nl(this, xpos, x, y);
					attr.tree_cont.template for_each<item_locator&>(shape.first, nl);

					bool redraw = false;
					auto const node = nl.node();
					if (node && (nl.what() != component::end))
					{
						if ((nl.what() != node_state.comp_pointed) || (node != node_state.pointed))
						{
							node_state.comp_pointed = nl.what();

							if (node_state.pointed)
							{
								item_proxy iprx(data.trigger_ptr, node_state.pointed);
								data.widget_ptr->events().hovered.emit(::nana::arg_treebox{ *data.widget_ptr, iprx, false }, data.widget_ptr->handle());

								if (node != node_state.pointed)
									close_tooltip_window();
							}

							node_state.pointed = node;
							item_proxy iprx(data.trigger_ptr, node_state.pointed);
							data.widget_ptr->events().hovered.emit(::nana::arg_treebox{ *data.widget_ptr, iprx, true }, data.widget_ptr->handle());

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
						data.widget_ptr->events().hovered.emit(::nana::arg_treebox{ *data.widget_ptr, iprx, false }, data.widget_ptr->handle());

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
						
						//PR#406 Error Flynn's contribution
						//fix: tooltip window doesn't have tree scheme & typeface 
						API::dev::set_scheme(node_state.tooltip->handle(), API::dev::get_scheme(data.widget_ptr->handle()));
						node_state.tooltip->typeface(data.widget_ptr->typeface());

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
						events.mouse_leave.connect_unignorable(fn);
						events.mouse_move.connect_unignorable(fn);
						events.mouse_down.connect_unignorable(fn);
						events.mouse_up.connect_unignorable(fn);
						events.dbl_click.connect_unignorable(fn);
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
					//Make it an end iterator if one of them is a nullptr
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
					trigger_->impl()->draw(false);
					return *this;
				}

				item_proxy& item_proxy::clear()
				{
					if (node_)
					{
						auto impl = trigger_->impl();
						if(impl->unlink(node_, true))
							impl->draw(true);
					}
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
						impl->draw(true);

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
						impl->draw(true);

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

					//Fixed by ErrorFlynn
					//this method incorrectly returned the number of levels beneath the nodes using child = child->child
					for(auto child = node_->child; child; child = child->next)
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

				//Undocumented methods for internal use.
				trigger::node_type * item_proxy::_m_node() const
				{
					return node_;
				}
			//end class item_proxy

			class internal_placer
				: public compset_placer_interface
			{
			public:
				internal_placer(const scheme& schm):
					scheme_(schm)
				{}
			private:
				//Implement the compset_locator_interface

				virtual void enable(component_t comp, bool enabled) override
				{
					switch(comp)
					{
					case component_t::crook:
						enable_crook_ = enabled;
						break;
					case component_t::icon:
						enable_icon_ = enabled;
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
						return enable_crook_;
					case component_t::icon:
						return enable_icon_;
					default:
						break;
					}
					return true;
				}

				virtual unsigned item_height(graph_reference graph) const override
				{
					auto m = std::max((enable_crook_ ? scheme_.crook_size : 0), (enable_icon_ ? scheme_.icon_size : 0));

					unsigned as = 0, ds = 0, il;
					graph.text_metrics(as, ds, il);
					return std::max(as + ds + 8, m);
				}

				virtual unsigned item_width(graph_reference graph, const item_attribute_t& attr) const override
				{
					return graph.text_extent_size(attr.text).width + (enable_crook_ ? scheme_.crook_size : 0) + (enable_icon_ ? scheme_.icon_size : 0) + (scheme_.text_offset << 1) + scheme_.item_offset;
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
					case component_t::expander:
						if(attr.has_children)
						{
							r->width = scheme_.item_offset;
							return true;
						}
						return false;
					case component_t::bground:
						return true;
					case component_t::crook:
						if(enable_crook_)
						{
							r->x += scheme_.item_offset;
							r->width = scheme_.crook_size;
							return true;
						}
						return false;
					case component_t::icon:
						if(enable_icon_)
						{
							r->x += scheme_.item_offset + (enable_crook_ ? scheme_.crook_size : 0);
							r->y = 2;
							r->width = scheme_.icon_size;
							r->height -= 2;
							return true;
						}
						return false;
					case component_t::text:
						{
							auto text_pos = scheme_.item_offset + (enable_crook_ ? scheme_.crook_size : 0) + (enable_icon_ ? scheme_.icon_size : 0) + scheme_.text_offset;
							r->x += text_pos;
							r->width -= (text_pos + scheme_.text_offset);
						};
						return true;
					default:
						break;
					}
					return false;
				}
			private:
				const scheme& scheme_;
				bool enable_crook_{ false };
				bool enable_icon_{ false };
			};

			class internal_renderer
				: public renderer_interface
			{
				window window_handle_;

				void begin_paint(::nana::widget& wdg) override
				{
					window_handle_ = wdg.handle();
				}

				void bground(graph_reference graph, const compset_interface * compset) const override
				{
					comp_attribute_t attr;

					if(compset->comp_attribute(component::bground, attr))
					{
						auto scheme_ptr = static_cast<::nana::treebox::scheme_type*>(API::dev::get_scheme(window_handle_));

						const ::nana::color_proxy *bg_ptr = nullptr, *fg_ptr = nullptr;
						if(compset->item_attribute().mouse_pointed)
						{
							if(compset->item_attribute().selected)
							{
								bg_ptr = &scheme_ptr->item_bg_selected_and_highlighted;
								fg_ptr = &scheme_ptr->item_fg_selected_and_highlighted;
							}
							else
							{
								bg_ptr = &scheme_ptr->item_bg_highlighted;
								fg_ptr = &scheme_ptr->item_fg_highlighted;
							}
						}
						else if(compset->item_attribute().selected)
						{
							bg_ptr = &scheme_ptr->item_bg_selected;
							fg_ptr = &scheme_ptr->item_fg_selected;
						}

						if(bg_ptr)
						{
							if (API::is_transparent_background(window_handle_))
							{
								paint::graphics item_graph{ attr.area.dimension() };
								item_graph.rectangle(false, *fg_ptr);
								item_graph.rectangle(rectangle{ attr.area.dimension() }.pare_off(1), true, *bg_ptr);

								graph.blend(attr.area, item_graph, attr.area.position(), 0.5);
							}
							else
							{
								graph.rectangle(attr.area, false, *fg_ptr);
								graph.rectangle(attr.area.pare_off(1), true, *bg_ptr);
							}
						}
					}
				}

				void expander(graph_reference graph, const compset_interface * compset) const override
				{
					comp_attribute_t attr;
					if(compset->comp_attribute(component::expander, attr))
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
						arrow.draw(graph, API::bgcolor(window_handle_), (attr.mouse_pointed ? colors::deep_sky_blue : colors::black), r, element_state::normal);
					}
				}

				void crook(graph_reference graph, const compset_interface * compset) const override
				{
					comp_attribute_t attr;
					if(compset->comp_attribute(component::crook, attr))
					{
						attr.area.y += (attr.area.height - 16) / 2;
						crook_.check(compset->item_attribute().checked);
						crook_.draw(graph, API::bgcolor(window_handle_), API::fgcolor(window_handle_), attr.area, attr.mouse_pointed ? element_state::hovered : element_state::normal);
					}
				}

				virtual void icon(graph_reference graph, const compset_interface * compset) const override
				{
					comp_attribute_t attr;
					if(compset->comp_attribute(component::icon, attr))
					{
						const nana::paint::image * img = nullptr;
						auto & item_attr = compset->item_attribute();

						if (item_attr.expended)
							img = &(item_attr.icon_expanded);
						else if (item_attr.mouse_pointed)
							img = &(item_attr.icon_hover);
			
						if((nullptr == img) || img->empty())
							img = &(item_attr.icon_normal);

						if(!img->empty())
						{
							auto size = img->size();
							if(size.width > attr.area.width || size.height > attr.area.height)
							{
								nana::size fit_size;
								nana::fit_zoom(size, attr.area.dimension(), fit_size);

								attr.area.x += (attr.area.width - fit_size.width) / 2;
								attr.area.y += (attr.area.height - fit_size.height) / 2;
								attr.area.dimension(fit_size);
								img->stretch(rectangle{ size }, graph, attr.area);
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
						graph.string(point{ attr.area.x, attr.area.y + 3 }, compset->item_attribute().text, API::fgcolor(window_handle_));
				}
			private:
				mutable facade<element::crook> crook_;
			};


			//class trigger::item_locator
				trigger::item_locator::item_locator(implementation * impl, int item_pos, int x, int y)
					:	impl_(impl),
						item_pos_(item_pos, 1),
						pos_(x, y),
						what_(component::end),
						node_(nullptr)
				{}

				int trigger::item_locator::operator()(node_type &node, int affect)
				{
					switch(affect)
					{
					case 0: break;
					case 1: item_pos_.x += static_cast<int>(impl_->data.scheme_ptr->indent_displacement); break;
					default:
						if(affect >= 2)
							item_pos_.x -= static_cast<int>(impl_->data.scheme_ptr->indent_displacement) * (affect - 1);
					}

					impl_->assign_node_attr(node_attr_, &node);
					nana::rectangle node_r;
					auto & comp_placer = impl_->data.comp_placer;

					node_r.width = comp_placer->item_width(*impl_->data.graph, node_attr_);
					node_r.height = comp_placer->item_height(*impl_->data.graph);

					if ((pos_.y < item_pos_.y + static_cast<int>(node_r.height)) && (pos_.y >= item_pos_.y))
					{
						auto const logic_pos = pos_ - item_pos_;

						for (int comp = static_cast<int>(component::begin); comp != static_cast<int>(component::end); ++comp)
						{
							nana::rectangle r = node_r;
							if (!comp_placer->locate(static_cast<component>(comp), node_attr_, &r))
								continue;
							
							if (r.is_hit(logic_pos))
							{
								node_ = &node;
								what_ = static_cast<component>(comp);
								if (component::expander == what_ && (false == node_attr_.has_children))
									what_ = component::end;

								if (component::text == what_)
									node_text_r_ = r;

								break;
							}
						}

						return 0; //Stop iterating
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
					:	impl_(new implementation)
				{
					impl_->data.trigger_ptr = this;
					impl_->data.renderer = nana::pat::cloneable<renderer_interface>(internal_renderer());

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

					impl_->adjust.timer.interval(std::chrono::milliseconds{ 16 });
					impl_->adjust.timer.start();
				}

				trigger::~trigger()
				{
					delete impl_;
				}

				trigger::implementation * trigger::impl() const
				{
					return impl_;
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

				::nana::pat::cloneable<renderer_interface>& trigger::renderer() const
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

				trigger::node_type* trigger::insert(node_type* node, const std::string& key, std::string&& title)
				{
					node_type * p = impl_->attr.tree_cont.node(node, key);
					if(p)
						p->value.second.text.swap(title);
					else
						p = impl_->attr.tree_cont.insert(node, key, treebox_node_type(std::move(title)));

					if (p)
						impl_->draw(true);

					return p;
				}

				trigger::node_type* trigger::insert(const std::string& path, std::string&& title)
				{
					auto x = impl_->attr.tree_cont.insert(path, treebox_node_type(std::move(title)));
					if (x)
						impl_->draw(true);
					return x;
				}

				node_image_tag& trigger::icon(const std::string& id)
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
					if(impl_->attr.tree_cont.verify(node))
					{
						node->value.second.img_idstr = id;
						auto i = impl_->shape.image_table.find(id);
						if (i != impl_->shape.image_table.end())
							impl_->draw(true);
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
					if((key || name ) && impl_->attr.tree_cont.verify(node))
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
					impl_->data.widget_ptr = static_cast<::nana::treebox*>(&widget);
					impl_->data.scheme_ptr = static_cast<::nana::treebox::scheme_type*>(API::dev::get_scheme(widget));
					impl_->data.comp_placer = nana::pat::cloneable<compset_placer_interface>(internal_placer{ *impl_->data.scheme_ptr });

					widget.caption("nana treebox");
				}

				void trigger::detached()
				{
					//Reset the comp_placer, because after detaching, the scheme referred by comp_placer will be released
					impl_->data.comp_placer.reset();
					impl_->data.graph = nullptr;
				}

				void trigger::refresh(graph_reference)
				{
					//Don't reset the scroll and update the window
					impl_->draw(false, true);
				}

				void trigger::dbl_click(graph_reference, const arg_mouse& arg)
				{
					auto & shape = impl_->shape;

					int xpos = impl_->attr.tree_cont.indent_size(shape.first) * impl_->data.scheme_ptr->indent_displacement - shape.offset_x;
					item_locator nl(impl_, xpos, arg.pos.x, arg.pos.y);
					impl_->attr.tree_cont.for_each<item_locator&>(shape.first, nl);

					auto const node = nl.node();
					if (!node)
						return;

					switch (nl.what())
					{
					case component::icon:
					case component::text:
						impl_->set_expanded(node, !node->value.second.expanded);
						impl_->draw(true, true, false);
						API::dev::lazy_refresh();
						break;
					default:
						break;
					}
				}

				void trigger::mouse_down(graph_reference, const arg_mouse& arg)
				{
					auto & shape = impl_->shape;

					int xpos = impl_->attr.tree_cont.indent_size(shape.first) * impl_->data.scheme_ptr->indent_displacement - shape.offset_x;
					item_locator nl(impl_, xpos, arg.pos.x, arg.pos.y);
					impl_->attr.tree_cont.for_each<item_locator&>(shape.first, nl);

					auto & node_state = impl_->node_state;
					node_state.pressed_node = nl.node();

					if (node_state.pressed_node && (component::expander == nl.what()))
					{
						if(impl_->set_expanded(node_state.pressed_node, !node_state.pressed_node->value.second.expanded))
							impl_->make_adjust(node_state.pressed_node, 0);
					}
					else if (node_state.selected != node_state.pressed_node)
					{
						//impl_->set_selected(node_state.pressed_node);  // todo: emit selected after checked
					}
					else
						return;

					impl_->draw(true);
					API::dev::lazy_refresh();
				}

				void trigger::mouse_up(graph_reference, const arg_mouse& arg)
				{
					auto & shape = impl_->shape;

					int xpos = impl_->attr.tree_cont.indent_size(shape.first) * impl_->data.scheme_ptr->indent_displacement - shape.offset_x;
					item_locator nl(impl_, xpos, arg.pos.x, arg.pos.y);
					impl_->attr.tree_cont.for_each<item_locator&>(shape.first, nl);

					auto const pressed_node = impl_->node_state.pressed_node;
					impl_->node_state.pressed_node = nullptr;

					if(!nl.node())
						return;

					if (pressed_node != nl.node())
						return;	//Do not refresh

					if (nl.what() == component::crook)
					{
						checkstate cs = checkstate::unchecked;
						if (checkstate::unchecked == nl.node()->value.second.checked)
							cs = checkstate::checked;

						check(nl.node(), cs);
					}
					if ((impl_->node_state.selected != nl.node()) && (nl.item_body() || nl.what() == component::crook))
					{
						impl_->set_selected(nl.node());
						if (impl_->make_adjust(impl_->node_state.selected, 1))
							impl_->adjust.scroll_timestamp = 1;
					}

					impl_->draw(true);
					API::dev::lazy_refresh();
				}

				void trigger::mouse_move(graph_reference, const arg_mouse& arg)
				{
					if(impl_->track_mouse(arg.pos.x, arg.pos.y))
					{
						impl_->draw(false);
						API::dev::lazy_refresh();
					}
				}

				void trigger::mouse_wheel(graph_reference, const arg_wheel& arg)
				{
					auto & scroll = *impl_->shape.scroll;
					if (scroll.empty())
						return;

					auto const value_before = scroll.value();

					scroll.make_step(!arg.upwards);

					if (value_before != scroll.value())
					{
						impl_->track_mouse(arg.pos.x, arg.pos.y);

						impl_->draw(false, true, true);
						API::dev::lazy_refresh();
					}
				}

				void trigger::mouse_leave(graph_reference, const arg_mouse&)
				{
					if (impl_->node_state.pointed && (!impl_->node_state.tooltip))
					{
						item_proxy iprx(impl_->data.trigger_ptr, impl_->node_state.pointed);
						impl_->data.widget_ptr->events().hovered.emit(::nana::arg_treebox{ *impl_->data.widget_ptr, iprx, false }, impl_->data.widget_ptr->handle());
						impl_->node_state.pointed = nullptr;
						impl_->draw(false);
						API::dev::lazy_refresh();
					}
				}

				void trigger::resized(graph_reference, const arg_resized&)
				{
					impl_->draw(false);
					API::dev::lazy_refresh();
					impl_->show_scroll();
					if(!impl_->shape.scroll->empty())
					{
						nana::size s = impl_->data.graph->size();
						impl_->shape.scroll->move(rectangle{ static_cast<int>(s.width) - 16, 0, 16, s.height });
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
						API::dev::lazy_refresh();
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
						API::dev::lazy_refresh();
					}
				}
			//end class trigger
		}//end namespace treebox
	}//end namespace drawerbase

	//class treebox
		using component = drawerbase::treebox::component;

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
			auto impl = get_drawer_trigger().impl();
			if (impl->attr.auto_draw != ad)
			{
				impl->attr.auto_draw = ad;
				if (ad)
					API::refresh_window(this->handle());
			}
		}

		treebox & treebox::checkable(bool enable)
		{
			auto impl = get_drawer_trigger().impl();
			auto & comp_placer = impl->data.comp_placer;
			if (comp_placer->enabled(component::crook) != enable)
			{
				comp_placer->enable(component::crook, enable);
				impl->draw(false);
			}
			return *this;
		}

		bool treebox::checkable() const
		{
			return get_drawer_trigger().impl()->data.comp_placer->enabled(component::crook);
		}

		void treebox::clear()
		{
			auto impl = get_drawer_trigger().impl();
			if (impl->unlink(impl->attr.tree_cont.get_root(), true))
				impl->draw(true);
		}

		treebox::node_image_type& treebox::icon(const std::string& id)
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
			return item_proxy(trg, trg->impl()->attr.tree_cont.find(keypath));
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
			if (get_drawer_trigger().impl()->unlink(i._m_node(), false))
				get_drawer_trigger().impl()->draw(true);
			return next;
		}

		void treebox::erase(const std::string& keypath)
		{
			auto i = find(keypath);
			if (!i.empty())
				this->erase(i);
		}

		std::string treebox::make_key_path(item_proxy i, const std::string& splitter) const
		{
			auto & tree = get_drawer_trigger().impl()->attr.tree_cont;
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
				return path;
			}
			return{};
		}

		treebox::item_proxy treebox::selected() const
		{
			auto dw = &get_drawer_trigger();
			return item_proxy(const_cast<drawer_trigger_t*>(dw), dw->impl()->node_state.selected);
		}

		void treebox::scroll_into_view(item_proxy item, align_v bearing)
		{
			internal_scope_guard lock;
			if(get_drawer_trigger().impl()->scroll_into_view(item._m_node(), true, bearing))
				API::refresh_window(*this);
		}

		void treebox::scroll_into_view(item_proxy item)
		{
			internal_scope_guard lock;
			//The third argument for scroll_into_view is ignored if the second argument is false.
			if(get_drawer_trigger().impl()->scroll_into_view(item._m_node(), false, align_v::center))
				API::refresh_window(*this);
		}

		treebox::item_proxy treebox::hovered(bool exclude_expander) const
		{
			internal_scope_guard lock;
			auto dw = &get_drawer_trigger();
			if (dw->impl()->node_state.pointed)
			{
				//Returns empty item_proxy if the mouse is on expander and exclude_expander is required.
				if (exclude_expander && (dw->impl()->node_state.comp_pointed == drawerbase::treebox::component::expander))
					return item_proxy{};
			}
			return item_proxy(const_cast<drawer_trigger_t*>(dw), dw->impl()->node_state.pointed);
		}

		std::shared_ptr<scroll_operation_interface> treebox::_m_scroll_operation()
		{
			internal_scope_guard lock;
			return std::make_shared<drawerbase::treebox::exclusive_scroll_operation>(get_drawer_trigger().impl()->shape.scroll);
		}
	//end class treebox
}//end namespace nana
