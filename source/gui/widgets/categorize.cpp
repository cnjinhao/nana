/*
 *	A Categorize Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2014 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/categorize.cpp
 */

#include <nana/gui/wvl.hpp>
#include <nana/gui/widgets/categorize.hpp>
#include <nana/gui/widgets/float_listbox.hpp>
#include <nana/paint/gadget.hpp>
#include <nana/gui/widgets/detail/tree_cont.hpp>
#include <stdexcept>

namespace nana
{
	namespace drawerbase
	{
		namespace categorize
		{
			struct event_agent_holder
			{
				std::function<void(nana::any&)> selected;
			};

			struct item
				: public float_listbox::item_interface
			{
				nana::paint::image	item_image;
				nana::string		item_text;
			public:
				item(const nana::string& s)
					: item_text(s)
				{}
			public:
				//Implement item_interface methods
				const nana::paint::image& image() const override
				{
					return item_image;
				}

				const nana::char_t * text() const override
				{
					return item_text.data();
				}
			};

			struct item_tag
			{
				nana::size	scale;
				unsigned	pixels;
				nana::any	value;
			};

			//class renderer
					renderer::ui_element::ui_element()
						: what(none), index(0)
					{}
				renderer::~renderer(){}
			//end class renderer

			//interior_renderer

			class interior_renderer
				: public renderer
			{
			private:
				void background(graph_reference graph, window wd, const nana::rectangle& r, const ui_element& ue)
				{
					ui_el_ = ue;
					style_.bgcolor = API::background(wd);
					style_.fgcolor = API::foreground(wd);

					if(ue.what == ue.none || (API::window_enabled(wd) == false))
					{	//the mouse is out of the widget.
						style_.bgcolor = nana::paint::graphics::mix(style_.bgcolor, 0xA0C9F5, 0.9);
					}
					graph.rectangle(r, style_.bgcolor, true);
				}

				virtual void root_arrow(graph_reference graph, const nana::rectangle& r, mouse_action state)
				{
					int x = r.x + (r.width - 16) / 2;
					int y = r.y + (r.height - 16) / 2;
					if(ui_el_.what == ui_el_.item_root)
					{
						_m_item_bground(graph, r.x + 1, r.y, r.width - 2, r.height, (state == mouse_action::pressed ? mouse_action::pressed : mouse_action::over));
						graph.rectangle(r, 0x3C7FB1, false);
						if(state == mouse_action::pressed)
						{
							++x;
							++y;
						}
					}
					else
						graph.rectangle(r, style_.bgcolor, true);

					nana::paint::gadget::arrow_16_pixels(graph, x, y,
						style_.fgcolor, 3, nana::paint::gadget::directions::to_west);
				}

				void item(graph_reference graph, const nana::rectangle& r, std::size_t index, const nana::string& name, unsigned txtheight, bool has_child, mouse_action state)
				{
					nana::point strpos(r.x + 5, r.y + static_cast<int>(r.height - txtheight) / 2);

					if((ui_el_.what == ui_el_.item_arrow || ui_el_.what == ui_el_.item_name) && (ui_el_.index == index))
					{
						mouse_action state_arrow, state_name;
						if(mouse_action::pressed != state)
						{
							state_arrow = (ui_el_.what == ui_el_.item_arrow ? mouse_action::over : mouse_action::normal);
							state_name = (ui_el_.what == ui_el_.item_name ? mouse_action::over : mouse_action::normal);
						}
						else
						{
							state_name = state_arrow = mouse_action::pressed;
							++strpos.x;
							++strpos.y;
						}

						int top = r.y + 1;
						unsigned width = r.width - 2;
						unsigned height = r.height - 2;
						if(has_child)
						{
							int left = r.x + r.width - 16;
							_m_item_bground(graph, left, top, 15, height, state_arrow);
							width -= 16;
							--left;
							graph.line(left, top, left, r.y + height, 0x3C7FB1);
						}
						_m_item_bground(graph, r.x + 1, top, width, height, state_name);
						graph.rectangle(r, 0x3C7FB1, false);
					}
					graph.string(strpos.x, strpos.y, style_.fgcolor, name);

					if(has_child)
					{
						nana::paint::gadget::arrow_16_pixels(graph, r.x + r.width - 16, r.y + (r.height - 16)/2,
							style_.fgcolor, 3, nana::paint::gadget::directions::to_east);
					}
				}

				void border(graph_reference graph)
				{
					graph.rectangle(0xF0F0F0, false);
					graph.rectangle_line(nana::rectangle(graph.size()).pare_off(1),
										0x9DABB9, 0x484E55, 0x484E55, 0x9DABB9);
				}
			private:
				void _m_item_bground(graph_reference graph, int x, int y, unsigned width, unsigned height, mouse_action state)
				{
					const unsigned half = (height - 2) / 2;
					int left = x + 1;
					int top = y + 1;
					nana::color_t upcol, downcol;
					switch(state)
					{
					case mouse_action::over:
						upcol = 0x0DFF2FC;
						downcol = 0xA9DAF5;
						break;
					case mouse_action::pressed:
						upcol = 0xA6D7F2;
						downcol = 0x92C4F6;
						++left;
						++top;
						break;
					case mouse_action::normal:
					default:
						upcol = 0xEAEAEA;
						downcol = 0xDCDCDC;
						break;
					}

					graph.rectangle(left, top, width - 2, half, upcol, true);
					graph.rectangle(left, top + static_cast<int>(half), width - 2, (height - 2) - half, downcol, true);
					if(mouse_action::pressed == state)
					{
						int bottom = y + height - 1;
						int right = x + width - 1;
						graph.line(x, y, right, y, 0x6E8D9F);
						graph.line(x, y + 1, x, bottom, 0x6E8D9F);
						++x;
						++y;
						graph.line(x, y, right, y, 0xA6C7D9);
						graph.line(x, y + 1, x, bottom, 0xA6C7D9);
					}
				}

			private:
				ui_element	ui_el_;
				struct style_tag
				{
					nana::color_t bgcolor;
					nana::color_t fgcolor;
				}style_;
			};

			class tree_wrapper
			{
			public:
				typedef widgets::detail::tree_cont<item_tag> container;
				typedef container::node_type * node_handle;

				tree_wrapper()
					:splitstr_(STR("\\")), cur_(nullptr)
				{}

				bool seq(std::size_t index, std::vector<node_handle> & seqv) const
				{
					_m_read_node_path(seqv);

					if(index < seqv.size())
					{
						if(index)
							seqv.erase(seqv.begin(), seqv.begin() + index);
						return true;
					}
					return false;
				}

				void splitstr(const nana::string& ss)
				{
					if(ss.size())
						splitstr_ = ss;
				}

				const nana::string& splitstr() const
				{
					return splitstr_;
				}

				nana::string path() const
				{
					std::vector<node_handle> v;
					_m_read_node_path(v);

					nana::string str;
					bool not_head = false;
					for(auto i : v)
					{
						if(not_head)
							str += splitstr_;
						else
							not_head = true;
						str += i->value.first;
					}
					return str;
				}

				void path(const nana::string& key)
				{
					cur_ = tree_.ref(key);
				}

				node_handle at(std::size_t index) const
				{
					std::vector<node_handle> v;
					_m_read_node_path(v);
					return (index < v.size() ? v[index] : nullptr);
				}

				node_handle tail(std::size_t index)
				{
					node_handle i = at(index);
					if(i)	cur_ = i;
					return i;
				}

				node_handle cur() const
				{
					return cur_;
				}

				void cur(node_handle i)
				{
					cur_ = i;
				}

				void insert(const nana::string& name, const nana::any& value)
				{
					item_tag m;
					m.pixels = 0;
					m.value = value;
					cur_ = tree_.insert(cur_, name, m);
				}

				bool childset(const nana::string& name, const nana::any& value)
				{
					if(cur_)
					{
						item_tag m;
						m.pixels = 0;
						m.value = value;
						tree_.insert(cur_, name, m);
						return true;
					}
					return false;
				}

				bool childset_erase(const nana::string& name)
				{
					if(cur_)
					{
						for(node_handle i = cur_->child; i; i = i->next)
						{
							if(i->value.first == name)
							{
								tree_.remove(i);
								return true;
							}
						}
					}
					return false;
				}

				node_handle find_child(const nana::string& name) const
				{
					if(cur_)
					{
						for(node_handle i = cur_->child; i; i = i->next)
						{
							if(i->value.first == name)
								return i;
						}
					}
					return nullptr;
				}

				bool clear()
				{
					if(tree_.get_root()->child)
					{
						tree_.clear();
						return true;
					}
					return false;
				}
			private:
				void _m_read_node_path(std::vector<node_handle>& v) const
				{
					node_handle root = tree_.get_root();
					for(node_handle i = cur_; i && (i != root); i = i->owner)
						v.insert(v.begin(), i);
				}
			private:
				container tree_;
				nana::string splitstr_;
				node_handle cur_;
			};

			//class scheme
			class trigger::scheme
			{
			public:
				typedef tree_wrapper container;
				typedef container::node_handle node_handle;
				typedef renderer::ui_element	ui_element;

				enum class mode
				{
					normal, floatlist
				};

				scheme()
					: graph_(nullptr)
				{
					proto_.ui_renderer = pat::cloneable<renderer>(interior_renderer());
					style_.mode = mode::normal;
					style_.listbox = nullptr;
				}

				void attach(window wd, nana::paint::graphics* graph)
				{
					window_ = wd;
					API::background(wd, 0xFFFFFF);
					graph_ = graph;
				}

				void detach()
				{
					window_ = nullptr;
					graph_ = nullptr;
				}

				window window_handle() const
				{
					return window_;
				}

				const container& tree() const
				{
					return treebase_;
				}

				container& tree()
				{
					return treebase_;
				}

				void draw()
				{
					_m_calc_scale();

					nana::rectangle r = _m_make_rectangle(); //_m_make_rectangle must be called after _m_calc_scale()
					_m_calc_pixels(r);

					proto_.ui_renderer->background(*graph_, window_, r, ui_el_);
					if(head_)
						proto_.ui_renderer->root_arrow(*graph_, _m_make_root_rectangle(), style_.state);
					_m_draw_items(r);
					proto_.ui_renderer->border(*graph_);
				}

				bool locate(int x, int y) const
				{
					if(graph_)
					{
						if(head_)
						{
							auto r = _m_make_root_rectangle();
							if (r.is_hit(x, y))
							{
								style_.active_item_rectangle = r;
								if(ui_el_.what == ui_el_.item_root)
									return false;
								ui_el_.what = ui_el_.item_root;
								return true;
							}
						}

						nana::rectangle r = _m_make_rectangle();
						std::vector<node_handle> seq;
						if(r.is_hit(x, y) && treebase_.seq(head_, seq))
						{
							const int xbase = r.x;
							const int xend = static_cast<int>(r.width) + r.x;

							//Change the meaning of variable r. Now, r indicates the area of a item
							r.height = item_height_;

							std::size_t seq_index = 0;
							for(auto i : seq)
							{
								r.width = i->value.second.pixels;
								//If the item is over the right border of widget, the item would be painted at
								//the begining of the next line.
								if(static_cast<int>(r.width) + r.x > xend)
								{
									r.x = xbase;
									r.y += r.height;
								}

								if(r.is_hit(x, y))
								{
									style_.active_item_rectangle = r;
									std::size_t index = seq_index + head_;

									ui_element::t what = ((i->child && (r.x + static_cast<int>(r.width) - 16 < x))
															? ui_el_.item_arrow : ui_el_.item_name);
									if(what == ui_el_.what && index == ui_el_.index)
										return false;

									ui_el_.what = what;
									ui_el_.index = index;
									return true;
								}
								r.x += r.width;
								++seq_index;
							}
						}
					}
					if(ui_el_.what == ui_el_.somewhere) return false;
					ui_el_.what = ui_el_.somewhere;
					return true;
				}

				bool erase_locate()
				{
					ui_el_.index = npos;
					if(ui_el_.what != ui_el_.none)
					{
						ui_el_.what = ui_el_.none;
						return true;
					}
					return false;
				}

				ui_element locate() const
				{
					return ui_el_;
				}

				void mouse_pressed()
				{
					style_.state = mouse_action::pressed;
					switch(ui_el_.what)
					{
					case ui_element::item_root:
					case ui_element::item_arrow:
						_m_show_list();
						style_.mode = mode::floatlist;
						break;
					default:	//Don't take care about other elements
						break;
					}
				}

				void mouse_release()
				{
					if(style_.mode != mode::floatlist)
					{
						style_.state = mouse_action::normal;
						switch(ui_el_.what)
						{
						case ui_element::item_name:
							_m_selected(treebase_.tail(ui_el_.index));
							break;
						default:	break;
						}
					}
				}

				bool is_list_shown() const
				{
					return (nullptr != style_.listbox);
				}

				event_agent_holder& evt_holder() const
				{
					return evt_holder_;
				}
			private:
				void _m_selected(node_handle node)
				{
					if(node)
					{
						API::dev::window_caption(window_handle(), tree().path());
						if(evt_holder_.selected)
							evt_holder_.selected(node->value.second.value);
					}
				}

				void _m_show_list()
				{
					if(style_.listbox)
						style_.listbox->close();

					style_.module.items.clear();

					nana::rectangle r;
					style_.list_trigger = ui_el_.what;
					if(ui_el_.what == ui_el_.item_arrow)
					{
						style_.active = ui_el_.index;
						node_handle i = treebase_.at(ui_el_.index);
						if(i)
						{
							for(node_handle child = i->child; child; child = child->next)
								style_.module.items.emplace_back(std::make_shared<item>(child->value.first));
						}
						r = style_.active_item_rectangle;
					}
					else if(ui_el_.item_root == ui_el_.what)
					{
						std::vector<node_handle> v;
						if(treebase_.seq(0, v))
						{
							auto end = v.cbegin() + head_;
							for(auto i = v.cbegin(); i != end; ++i)
								style_.module.items.emplace_back(std::make_shared<item>((*i)->value.first));
						}
						r = style_.active_item_rectangle;
					}
					r.y += r.height;
					r.width = r.height = 100;
					style_.listbox = &(form_loader<nana::float_listbox>()(window_, r, true));
					style_.listbox->set_module(style_.module, 16);
					style_.listbox->events().destroy.connect([this]
					{
						_m_list_closed();
					});
				}

				void _m_list_closed()
				{
					style_.mode = mode::normal;
					style_.state = mouse_action::normal;

					bool is_draw = false;
					if((style_.module.index != npos) && style_.module.have_selected)
					{
						switch(style_.list_trigger)
						{
						case ui_element::item_arrow:
							{
								treebase_.tail(style_.active);
								nana::string name = style_.module.items[style_.module.index]->text();
								node_handle node = treebase_.find_child(name);
								if(node)
								{
									treebase_.cur(node);
									_m_selected(node);
									is_draw = true;
								}
							}
							break;
						case ui_element::item_root:
							_m_selected(treebase_.tail(style_.module.index));
							is_draw = true;
							break;
						default:	//Don't take care about other elements
							break;
						}
					}
					else
						is_draw = true;

					if(is_draw)
					{
						draw();
						API::update_window(window_);
					}
					style_.listbox = nullptr;
				}
			private:
				unsigned _m_item_fix_scale() const
				{
					return graph_->height() - 2;
				}

				nana::rectangle _m_make_root_rectangle() const
				{
					return nana::rectangle(1, 1, 16, _m_item_fix_scale());
				}

				//_m_make_rectangle
				//@brief: This function calculate the items area. This must be called after _m_calc_scale()
				nana::rectangle _m_make_rectangle() const
				{
					nana::rectangle r(1, 1, graph_->width() - 2, _m_item_fix_scale());
					
					unsigned px = r.width;
					std::size_t lines = item_lines_;
					std::vector<node_handle> v;
					treebase_.seq(0, v);
					for(auto node : v)
					{
						if(node->value.second.scale.width > px)
						{
							if(lines > 1)
							{
								--lines;
								px = r.width;
								if(px < node->value.second.scale.width)
								{
									--lines;
									continue;
								}
							}
							else
							{
								//Too many items, so some of items cann't be displayed
								r.x += 16;
								r.width -= 16;
								return r;
							}
						}
						px -= node->value.second.scale.width;
					}

					return r;
				}

				void _m_calc_scale()
				{
					nana::size tsz;
					unsigned highest = 0;
					std::vector<node_handle> v;
					treebase_.seq(0, v);
					for(auto node : v)
					{
						node->value.second.scale = graph_->text_extent_size(node->value.first);

						if(highest < node->value.second.scale.height)
							highest = node->value.second.scale.height;

						node->value.second.scale.width += (node->child ? 26 : 10);
					}

					highest += 6; //the default height of item.

					item_lines_ = (graph_->height() - 2) / highest;
					if(item_lines_ == 0)
						item_lines_ = 1;
					item_height_ = (1 != item_lines_ ? highest : _m_item_fix_scale());
				}

				void _m_calc_pixels(const nana::rectangle& r)
				{
					std::size_t lines = item_lines_;

					unsigned px = 0;
					head_ = 0;
					std::vector<node_handle> v;
					treebase_.seq(0, v);
					for(auto vi = v.rbegin(); vi != v.rend(); ++vi)
					{
						item_tag & m = (*vi)->value.second;
						if(r.width >= px + m.scale.width)
						{
							px += m.scale.width;
							m.pixels = m.scale.width;
							continue;
						}

						//In fact, this item must be in the font of a line.
						m.pixels = (r.width >= m.scale.width ? m.scale.width : _m_minimial_pixels());
						if(0 == px)	//This line is empty, NOT a newline
						{
							px = m.pixels;
							continue;
						}

						//Newline, and check here whether is more lines.
						if(0 == --lines)
						{
							head_ = std::distance(vi, v.rend());
							break;
						}
						px = m.pixels;
					}
				}

				unsigned _m_minimial_pixels()
				{
					return 46;
				}

				void _m_draw_items(const nana::rectangle& r)
				{
					nana::rectangle item_r = r;
					item_r.height = item_height_;
					std::size_t index = head_;
					const int xend = static_cast<int>(r.width) + r.x;
					std::vector<node_handle> v;
					treebase_.seq(0, v);
					for(auto vi = v.begin() + head_; vi != v.end(); ++vi)
					{
						node_handle i = (*vi);
						if(static_cast<int>(i->value.second.pixels) + item_r.x > xend)
						{
							item_r.x = r.x;
							item_r.y += item_height_;
						}
						item_r.width = i->value.second.pixels;
						proto_.ui_renderer->item(*graph_, item_r, index++, i->value.first, i->value.second.scale.height, i->child != 0, style_.state);
						item_r.x += item_r.width;
					}
				}
			private:
				window	window_;
				nana::paint::graphics * graph_;
				nana::string splitstr_;
				std::size_t	head_;
				unsigned	item_height_;
				std::size_t	item_lines_;
				container	treebase_;

				mutable ui_element	ui_el_;
				struct style_tag
				{
					ui_element::t list_trigger;
					std::size_t active;	//It indicates the item corresponding listbox.
					mutable ::nana::rectangle active_item_rectangle;
					::nana::float_listbox::module_type module;
					::nana::float_listbox * listbox;
					scheme::mode	mode;
					mouse_action	state;	//The state of mouse
				}style_;

				struct proto_tag
				{
					pat::cloneable<renderer> ui_renderer;
				}proto_;

				mutable event_agent_holder	evt_holder_;
			};

			//class trigger
				trigger::trigger()
					: scheme_(new scheme)
				{}

				trigger::~trigger()
				{
					delete scheme_;
				}

				void trigger::insert(const nana::string& str, nana::any value)
				{
					scheme_->tree().insert(str, value);
					API::dev::window_caption(scheme_->window_handle(), scheme_->tree().path());
					scheme_->draw();
				}

				bool trigger::childset(const nana::string& str, nana::any value)
				{
					if(scheme_->tree().childset(str, value))
					{
						scheme_->draw();
						return true;
					}
					return false;
				}

				bool trigger::childset_erase(const nana::string& str)
				{
					if(scheme_->tree().childset_erase(str))
					{
						scheme_->draw();
						return true;
					}
					return false;
				}

				bool trigger::clear()
				{
					if(scheme_->tree().clear())
					{
						scheme_->draw();
						return true;
					}
					return false;
				}

				void trigger::splitstr(const nana::string& sstr)
				{
					scheme_->tree().splitstr(sstr);
				}

				const nana::string& trigger::splitstr() const
				{
					return scheme_->tree().splitstr();
				}

				void trigger::path(const nana::string& str)
				{
					scheme_->tree().path(str);
				}

				nana::string trigger::path() const
				{
					return scheme_->tree().path();
				}

				nana::any& trigger::value() const
				{
					auto node = scheme_->tree().cur();
					if(node)
						return node->value.second.value;

					throw std::runtime_error("Nana.GUI.categorize::value(), current category is empty");
				}

				void trigger::_m_event_agent_ready() const
				{
					auto & evt = scheme_->evt_holder();
					auto evt_agent = event_agent_.get();
					evt.selected = [evt_agent](::nana::any& val){
						evt_agent->selected(val);
					};
				}

				void trigger::attached(widget_reference widget, graph_reference graph)
				{
					scheme_->attach(widget, &graph);
				}

				void trigger::detached()
				{
					scheme_->detach();
				}

				void trigger::refresh(graph_reference)
				{
					scheme_->draw();
				}

				void trigger::mouse_down(graph_reference, const arg_mouse&)
				{
					if(scheme_->locate().what > ui_element::somewhere)
					{
						if(API::window_enabled(scheme_->window_handle()))
						{
							scheme_->mouse_pressed();
							scheme_->draw();
							API::lazy_refresh();
						}
					}
				}

				void trigger::mouse_up(graph_reference, const arg_mouse&)
				{
					if(scheme_->locate().what > ui_element::somewhere)
					{
						if(API::window_enabled(scheme_->window_handle()))
						{
							scheme_->mouse_release();
							scheme_->draw();
							API::lazy_refresh();
						}
					}
				}

				void trigger::mouse_move(graph_reference, const arg_mouse& arg)
				{
					if(scheme_->locate(arg.pos.x, arg.pos.y) && API::window_enabled(scheme_->window_handle()))
					{
						scheme_->draw();
						API::lazy_refresh();
					}
				}

				void trigger::mouse_leave(graph_reference, const arg_mouse&)
				{
					if(API::window_enabled(scheme_->window_handle()) && (scheme_->is_list_shown() == false) && scheme_->erase_locate())
					{
						scheme_->draw();
						API::lazy_refresh();
					}
				}
			//end class trigger
		}//end namespace categorize
	}//end namespace draerbase
}//end namespace nana
