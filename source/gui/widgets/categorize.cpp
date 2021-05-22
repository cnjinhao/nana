/**
 *	A Categorize Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2021 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file nana/gui/widgets/categorize.cpp
 */

#include <nana/gui/compact.hpp>
#include <nana/gui/widgets/categorize.hpp>
#include <nana/gui/widgets/float_listbox.hpp>
#include <nana/gui/element.hpp>
#include <nana/gui/widgets/detail/tree_cont.hpp>
#include <stdexcept>

namespace nana::drawerbase::categorize
{
	struct event_agent_holder
	{
		std::function<void(std::any&)> selected;
	};

	struct item
		: public float_listbox::item_interface
	{
		nana::paint::image	item_image;
		std::string		item_text;
	public:
		item(const std::string& s)
			: item_text(s)
		{}
	public:
		//Implement item_interface methods
		const nana::paint::image& image() const override
		{
			return item_image;
		}

		const char * text() const override
		{
			return item_text.data();
		}
	};

	struct item_type
	{
		nana::size	scale;
		unsigned	pixels;
		std::any	value;
	};

	//class renderer
	
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
			style_.bgcolor = api::bgcolor(wd);
			style_.fgcolor = api::fgcolor(wd);

			if(ue.what == elements::none || (api::window_enabled(wd) == false))
			{	//the mouse is out of the widget.
				style_.bgcolor = style_.bgcolor.blend(static_cast<color_rgb>(0xa0c9f5), 0.1);
			}
			graph.rectangle(r, true, style_.bgcolor);
		}

		virtual void root_arrow(graph_reference graph, const nana::rectangle& r, mouse_action state)
		{
			::nana::rectangle arrow_r{r.x + static_cast<int>(r.width - 16) / 2, r.y + static_cast<int>(r.height - 16) / 2, 16, 16};

			if(ui_el_.what == elements::item_root)
			{
				_m_item_bground(graph, r.x + 1, r.y, r.width - 2, r.height, (state == mouse_action::pressed ? mouse_action::pressed : mouse_action::hovered));
				graph.rectangle(r, false, static_cast<color_rgb>(0x3C7FB1));
				if(state == mouse_action::pressed)
				{
					++arrow_r.x;
					++arrow_r.y;
				}
			}
			else
				graph.rectangle(r, true, style_.bgcolor);

			facade<element::arrow> arrow("double");
			arrow.direction(::nana::direction::west);
			arrow.draw(graph, {}, style_.fgcolor, arrow_r, element_state::normal);
		}

		void item(graph_reference graph, const nana::rectangle& r, std::size_t index, const std::string& name_utf8, unsigned txtheight, bool has_child, mouse_action state)
		{
			nana::point strpos(r.x + 5, r.y + static_cast<int>(r.height - txtheight) / 2);

			if((ui_el_.what == elements::item_arrow || ui_el_.what == elements::item_name) && (ui_el_.index == index))
			{
				mouse_action state_arrow, state_name;
				if(mouse_action::pressed != state)
				{
					state_arrow = (ui_el_.what == elements::item_arrow ? mouse_action::hovered : mouse_action::normal);
					state_name = (ui_el_.what == elements::item_name ? mouse_action::hovered : mouse_action::normal);
				}
				else
				{
					state_name = state_arrow = mouse_action::pressed;
					++strpos;
				}

				int top = r.y + 1;
				unsigned width = r.width - 2;
				unsigned height = r.height - 2;

				::nana::color clr{static_cast<color_rgb>(0x3C7FB1)};
				if(has_child)
				{
					width -= 16;

					int left = r.x + r.width - 17;
					_m_item_bground(graph, left + 1, top, 15, height, state_arrow);
					graph.line({ left, top }, { left, r.y + static_cast<int>(height) }, clr);
				}

				_m_item_bground(graph, r.x + 1, top, width, height, state_name);
				graph.rectangle(r, false, clr);
			}
			graph.string(strpos, name_utf8, style_.fgcolor);

			if(has_child)
			{
				facade<element::arrow> arrow("double");
				arrow.direction(::nana::direction::east);
				arrow.draw(graph, {}, style_.fgcolor, { r.right() - 16, r.y + static_cast<int>(r.height - 16) / 2, 16, 16 }, element_state::normal);
			}
		}

		void border(graph_reference graph)
		{
			rectangle r{ graph.size() };
			graph.rectangle(r, false, static_cast<color_rgb>(0xf0f0f0));

			color lb(static_cast<color_rgb>(0x9dabb9));
			color tr(static_cast<color_rgb>(0x484e55));
			graph.frame_rectangle(r.pare_off(1), lb, tr, tr, lb);
		}
	private:
		void _m_item_bground(graph_reference graph, int x, int y, unsigned width, unsigned height, mouse_action state)
		{
			const unsigned half = (height - 2) / 2;
			int left = x + 1;
			int top = y + 1;
			nana::color clr_top(static_cast<color_rgb>(0xEAEAEA)), clr_bottom(static_cast<color_rgb>(0xDCDCDC));
			switch(state)
			{
			case mouse_action::hovered:
				clr_top.from_rgb(0xdf, 0xf2, 0xfc);
				clr_bottom.from_rgb(0xa9, 0xda, 0xf5);
				break;
			case mouse_action::pressed:
				clr_top.from_rgb(0xa6, 0xd7, 0xf2);
				clr_bottom.from_rgb(0x92, 0xc4, 0xf6);
				++left;
				++top;
				break;
			default:
				break;
			}

			graph.rectangle(rectangle{ left, top, width - 2, half }, true, clr_top);
			graph.rectangle(rectangle{ left, top + static_cast<int>(half), width - 2, (height - 2) - half }, true, clr_bottom);
			if(mouse_action::pressed == state)
			{
				int bottom = y + height - 1;
				int right = x + width - 1;

				graph.palette(false, static_cast<color_rgb>(0x6E8D9F));
				graph.line(point{ x, y }, point{ right, y });
				graph.line(point{ x, y + 1 }, point{ x, bottom });

				++x;
				++y;

				graph.palette(false, static_cast<color_rgb>(0xa6c7d9));
				graph.line(point{ x, y }, point{ right, y });
				graph.line(point{ x, y + 1 }, point{ x, bottom });
			}
		}

	private:
		ui_element	ui_el_;
		struct style_tag
		{
			color bgcolor;
			color fgcolor;
		}style_;
	};

	class tree_wrapper
	{
	public:
		using container = widgets::detail::tree_cont<item_type>;
		using node_handle = container::node_type*;

		tree_wrapper()
			: splitstr_("\\")
		{}

		/// Returns the path, range at [head, tail)
		std::vector<node_handle> node_path(std::size_t first, std::size_t last = nana::npos) const
		{
			std::vector<node_handle> nodes;
			if (first < last)
			{
				auto root = tree_.get_root();
				for (auto i = cur_; i && (i != root); i = i->owner)
					nodes.insert(nodes.begin(), i);

				if ((last != nana::npos) && (last <= nodes.size()))
					nodes.erase(nodes.begin() + last, nodes.end());

				if (first < nodes.size())
				{
					if (first)
						nodes.erase(nodes.begin(), nodes.begin() + first);
				}
				else
					nodes.clear();
			}
			return nodes;
		}

		void splitstr(const std::string& ss)
		{
			if(ss.size())
				splitstr_ = ss;
		}

		const std::string& splitstr() const noexcept
		{
			return splitstr_;
		}

		std::string path() const
		{
			auto v = node_path(0);
			std::string p;

			for(auto i : v)
			{
				if(!p.empty())
					p += splitstr_;

				p += i->value.first;
			}

			return p;
		}

		void path(const std::string& key)
		{
			cur_ = tree_.ref(key);
		}

		node_handle at(std::size_t pos) const
		{
			auto v = node_path(0);
			return (pos < v.size() ? v[pos] : nullptr);
		}

		node_handle tail(std::size_t index)
		{
			auto node = at(index);
			if(node)
				cur_ = node;
			return node;
		}

		node_handle cur() const noexcept
		{
			return cur_;
		}

		void cur(node_handle node) noexcept
		{
			cur_ = node;
		}

		void insert(const std::string& name, const std::any& value)
		{
			item_type m;
			m.pixels = 0;
			m.value = value;
			cur_ = tree_.insert(cur_, name, m);
		}

		bool childset(const std::string& name, const std::any& value)
		{
			if(cur_)
			{
				auto cur_before_insert = cur_;

				insert(name, value);
				cur_ = cur_before_insert;
				return true;
			}
			return false;
		}

		// Erases the child node which is specified by name. If name is empty, it clears all nodes.
		bool erase(const std::string& name)
		{
			if (name.empty())
			{
				//Clear all nodes
				if (tree_.get_root()->child)
				{
					tree_.clear(tree_.get_root());
					return true;
				}
			}
			else
			{
				auto node = find_child(name);
				if (node)
				{
					tree_.remove(node);
					return true;
				}
			}
			return false;
		}

		node_handle find_child(const std::string& name) const noexcept
		{
			if(cur_)
			{
				for(auto node = cur_->child; node; node = node->next)
				{
					if(node->value.first == name)
						return node;
				}
			}
			return nullptr;
		}
	private:
		container tree_;
		std::string splitstr_ ;
		node_handle cur_{ nullptr };
	};

	//class scheme
	class trigger::scheme
	{
	public:
		using container = tree_wrapper;
		using node_handle = container::node_handle;
		using ui_element = renderer::ui_element;
		using elements = renderer::elements;

		enum class mode
		{
			normal, floatlist
		};

		scheme()
		{
			renderer_ = pat::cloneable<renderer>(interior_renderer());
			style_.mode = mode::normal;
			style_.listbox = nullptr;
		}

		void set_handle(window wd)
		{
			if (wd)
			{
				window_ = wd;
				api::bgcolor(wd, colors::white);
			}
			else
				window_ = nullptr;
		}

		window window_handle() const noexcept
		{
			return window_;
		}

		container& tree() noexcept
		{
			return treebase_;
		}

		void draw(graph_reference graph)
		{
			_m_calc_scale(graph);

			nana::rectangle r = _m_make_rectangle(); //_m_make_rectangle must be called after _m_calc_scale()
			_m_calc_pixels(r);

			renderer_->background(graph, window_, r, ui_el_);
			if(head_)
				renderer_->root_arrow(graph, _m_make_root_rectangle(), style_.state);
			_m_draw_items(graph, r);
			renderer_->border(graph);
		}

		bool locate(const point pos) const
		{
			if(head_)
			{
				auto r = _m_make_root_rectangle();
				if (r.is_hit(pos))
				{
					style_.active_item_rectangle = r;
					if(ui_el_.what == renderer::elements::item_root)
						return false;
					ui_el_.what = renderer::elements::item_root;
					return true;
				}
			}

			nana::rectangle r = _m_make_rectangle();
			if(r.is_hit(pos))
			{
				const int xbase = r.x;
				const int xend = r.right();

				//Change the meaning of variable r. Now, r indicates the area of a item
				r.height = item_height_;

				auto index = head_;	//absolute index of a node
				auto node_path = treebase_.node_path(head_);
				for(auto i : node_path)
				{
					r.width = i->value.second.pixels;
					//If the item is over the right border of widget, the item would be painted at
					//the beginning of the next line.
					if(r.right() > xend)
					{
						r.x = xbase;
						r.y += r.height;
					}

					if(r.is_hit(pos))
					{
						style_.active_item_rectangle = r;

						auto const what = ((i->child && (r.right() - 16 < pos.x))
												? elements::item_arrow : elements::item_name);
						if(what == ui_el_.what && index == ui_el_.index)
							return false;

						ui_el_.what = what;
						ui_el_.index = index;
						return true;
					}
					r.x += r.width;
					++index;
				}
			}
			
			if(ui_el_.what == elements::somewhere)
				return false;

			ui_el_.what = elements::somewhere;
			return true;
		}

		bool erase_locate() noexcept
		{
			ui_el_.index = npos;
			if(ui_el_.what == elements::none)
				return false;

			ui_el_.what = elements::none;
			return true;
		}

		const ui_element& locate() const noexcept
		{
			return ui_el_;
		}

		void mouse_pressed()
		{
			style_.state = mouse_action::pressed;

			//Check the click whether to show the list
			if (elements::item_root == ui_el_.what || elements::item_arrow == ui_el_.what)
			{
				_m_show_list();
				style_.mode = mode::floatlist;
			}
		}

		void mouse_release()
		{
			if(style_.mode != mode::floatlist)
			{
				style_.state = mouse_action::normal;
				if (elements::item_name == ui_el_.what)
					_m_selected(treebase_.tail(ui_el_.index));
			}
		}

		bool is_list_shown() const noexcept
		{
			return (nullptr != style_.listbox);
		}

		event_agent_holder& evt_holder() noexcept
		{
			return evt_holder_;
		}
	private:
		void _m_selected(node_handle node)
		{
			if(node)
			{
				api::dev::window_caption(window_handle(), nana::detail::to_nstring(tree().path()));
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
			if(ui_el_.what == elements::item_arrow)
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
			else if(elements::item_root == ui_el_.what)
			{
				auto node_path = treebase_.node_path(0, head_);
				for(auto i : node_path)
					style_.module.items.emplace_back(std::make_shared<item>(i->value.first));

				r = style_.active_item_rectangle;
			}
			r.y += r.height;
			r.width = r.height = 100;

			style_.listbox = &(form_loader<nana::float_listbox>()(window_, r, true));
			style_.listbox->set_module(style_.module, 16);

			style_.listbox->events().destroy.connect_unignorable([this](const arg_destroy&)
			{
				//Close list when listbox is destroyed
				style_.listbox = nullptr;
				style_.mode = mode::normal;
				style_.state = mouse_action::normal;

				if ((style_.module.index != npos) && style_.module.have_selected)
				{
					node_handle node = nullptr;
					if (elements::item_arrow == style_.list_trigger)
					{
						treebase_.tail(style_.active);
						node = treebase_.find_child(style_.module.items[style_.module.index]->text());
						if (!node)
							return;

						treebase_.cur(node);
					}
					else if (elements::item_root == style_.list_trigger)
						node = treebase_.tail(style_.module.index);

					_m_selected(node);
				}

				api::refresh_window(window_);
				api::update_window(window_);
			});
		}

	private:
		unsigned _m_item_fix_scale() const
		{
			return (api::window_size(this->window_handle()).height - 2);
		}

		nana::rectangle _m_make_root_rectangle() const
		{
			return{ 1, 1, 16, _m_item_fix_scale() };
		}

		//_m_make_rectangle
		//@brief: This function calculate the items area. This must be called after _m_calc_scale()
		nana::rectangle _m_make_rectangle() const
		{
			auto dimension = api::window_size(this->window_handle());
			nana::rectangle r(1, 1, dimension.width - 2, _m_item_fix_scale());
			
			unsigned px = r.width;
			std::size_t lines = item_lines_;
			auto v = treebase_.node_path(0);
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
						//Too many items, the rest items are ignored
						r.x += 16;
						r.width -= 16;
						return r;
					}
				}
				px -= node->value.second.scale.width;
			}

			return r;
		}

		void _m_calc_scale(graph_reference graph)
		{
			unsigned highest = 0;
			auto v = treebase_.node_path(0);
			for(auto node : v)
			{
				node->value.second.scale = graph.text_extent_size(node->value.first);

				if(highest < node->value.second.scale.height)
					highest = node->value.second.scale.height;

				node->value.second.scale.width += (node->child ? 26 : 10);
			}

			highest += 6; //the default height of item.

			auto fixed_scale_px = _m_item_fix_scale();

			item_lines_ = fixed_scale_px / highest;
			if(item_lines_ == 0)
				item_lines_ = 1;
			item_height_ = (1 != item_lines_ ? highest : fixed_scale_px);
		}

		void _m_calc_pixels(const nana::rectangle& r)
		{
			std::size_t lines = item_lines_;

			unsigned px = 0;
			head_ = 0;
			auto path = treebase_.node_path(0);
			for(auto i = path.rbegin(); i != path.rend(); ++i)
			{
				auto & m = (*i)->value.second;
				if(r.width >= px + m.scale.width)
				{
					px += m.scale.width;
					m.pixels = m.scale.width;
					continue;
				}

				//In fact, this item must be in the font of a line.
				m.pixels = (r.width >= m.scale.width ? m.scale.width : _m_minimial_pixels());

				if (px && (0 == --lines))
				{
					head_ = std::distance(i, path.rend());
					break;
				}
				px = m.pixels;
			}
		}

		static unsigned _m_minimial_pixels() noexcept
		{
			return 46;
		}

		void _m_draw_items(graph_reference graph, const nana::rectangle& r)
		{
			nana::rectangle item_r = r;
			item_r.height = item_height_;

			std::size_t index = head_;
			auto v = treebase_.node_path(head_);
			for (auto i : v)
			{
				if (static_cast<int>(i->value.second.pixels) + item_r.x > r.right())
				{
					item_r.x = r.x;
					item_r.y += item_height_;
				}
				item_r.width = i->value.second.pixels;
				renderer_->item(graph, item_r, index++, i->value.first, i->value.second.scale.height, nullptr != i->child, style_.state);
				item_r.x += item_r.width;
			}
		}
	private:
		window	window_{nullptr};
		std::size_t	head_;
		unsigned	item_height_;
		std::size_t	item_lines_;
		container	treebase_;

		mutable ui_element	ui_el_;
		struct style_tag
		{
			elements list_trigger;
			std::size_t active;	//It indicates the item corresponding listbox.
			mutable ::nana::rectangle active_item_rectangle;
			::nana::float_listbox::module_type module;
			::nana::float_listbox * listbox;
			scheme::mode	mode;
			mouse_action	state;	//The state of mouse
		}style_;
		
		pat::cloneable<renderer> renderer_;
		event_agent_holder	evt_holder_;
	};

	//class trigger
	trigger::trigger()
		: scheme_(new scheme)
	{}

	trigger::~trigger()
	{
		delete scheme_;
	}

	void trigger::insert(const std::string& str, std::any value)
	{
		throw_not_utf8(str);
		scheme_->tree().insert(str, value);
		api::dev::window_caption(scheme_->window_handle(), nana::detail::to_nstring(scheme_->tree().path()));
		api::refresh_window(this->scheme_->window_handle());
	}

	bool trigger::childset(const std::string& str, std::any value)
	{
		if(scheme_->tree().childset(str, value))
		{
			api::refresh_window(this->scheme_->window_handle());
			return true;
		}
		return false;
	}

	bool trigger::erase(const std::string& name)
	{
		if(scheme_->tree().erase(name))
		{
			api::refresh_window(this->scheme_->window_handle());
			return true;
		}
		return false;
	}

	void trigger::splitstr(const std::string& sstr)
	{
		scheme_->tree().splitstr(sstr);
	}

	const std::string& trigger::splitstr() const noexcept
	{
		return scheme_->tree().splitstr();
	}

	void trigger::set_caption(std::string text)
	{
		scheme_->tree().path(text);
		api::dev::window_caption(scheme_->window_handle(), nana::detail::to_nstring(scheme_->tree().path()));
	}

	std::any& trigger::value() const
	{
		auto node = scheme_->tree().cur();
		if(node)
			return node->value.second.value;

		throw std::runtime_error("nana::categorize::value, current category is empty");
	}

	void trigger::_m_event_agent_ready()
	{
		auto evt_agent = event_agent_.get();
		scheme_->evt_holder().selected = [evt_agent](std::any& val){
			evt_agent->selected(val);
		};
	}

	void trigger::attached(widget_reference widget, graph_reference)
	{
		scheme_->set_handle(widget);
	}

	void trigger::detached()
	{
		scheme_->set_handle(nullptr);
	}

	void trigger::refresh(graph_reference graph)
	{
		scheme_->draw(graph);
	}

	void trigger::mouse_down(graph_reference graph, const arg_mouse&)
	{
		if(scheme_->locate().what > elements::somewhere)
		{
			if(api::window_enabled(scheme_->window_handle()))
			{
				scheme_->mouse_pressed();
				scheme_->draw(graph);
				api::dev::lazy_refresh();
			}
		}
	}

	void trigger::mouse_up(graph_reference graph, const arg_mouse&)
	{
		if(scheme_->locate().what > elements::somewhere)
		{
			if(api::window_enabled(scheme_->window_handle()))
			{
				scheme_->mouse_release();
				scheme_->draw(graph);
				api::dev::lazy_refresh();
			}
		}
	}

	void trigger::mouse_move(graph_reference graph, const arg_mouse& arg)
	{
		if(scheme_->locate(arg.pos) && api::window_enabled(scheme_->window_handle()))
		{
			scheme_->draw(graph);
			api::dev::lazy_refresh();
		}
	}

	void trigger::mouse_leave(graph_reference graph, const arg_mouse&)
	{
		if(api::window_enabled(scheme_->window_handle()) && (scheme_->is_list_shown() == false) && scheme_->erase_locate())
		{
			scheme_->draw(graph);
			api::dev::lazy_refresh();
		}
	}
	//end class trigger
}//end namespace nana::drawerbase::categorize
