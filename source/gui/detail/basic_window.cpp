/*
*	A Basic Window Widget Definition
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file: nana/gui/detail/basic_window.cpp
*/

#include <nana/gui/detail/basic_window.hpp>
#include <nana/gui/detail/native_window_interface.hpp>

namespace nana
{
	namespace detail
	{
		//class caret_descriptor
			caret_descriptor::caret_descriptor(core_window_t* wd, unsigned width, unsigned height)
				:wd_(wd), size_(width, height), visible_state_(visible_state::invisible), out_of_range_(false)
			{}

			caret_descriptor::~caret_descriptor()
			{
				if(wd_)	native_interface::caret_destroy(wd_->root);
			}

			void caret_descriptor::set_active(bool active)
			{
				if(wd_)
				{
					if(active)
					{
						native_interface::caret_create(wd_->root, size_);

						visible_state_ = visible_state::invisible;
						this->position(point_.x, point_.y);
					}
					else
						native_interface::caret_destroy(wd_->root);

					wd_->root_widget->other.attribute.root->ime_enabled = active;
				}
			}

			auto caret_descriptor::window() const ->core_window_t*
			{
				return wd_;
			}

			void caret_descriptor::position(int x, int y)
			{
				point_.x = x;
				point_.y = y;

				update();
			}

			void caret_descriptor::effective_range(nana::rectangle rect)
			{
				//Chech rect
				if (rect.width && rect.height && rect.right() > 0 && rect.bottom() > 0)
				{
					if(rect.x < 0)
					{
						rect.width += rect.x;
						rect.x = 0;
					}

					if(rect.y < 0)
					{
						rect.height += rect.y;
						rect.y = 0;
					}

					if(effective_range_ != rect)
					{
						effective_range_ = rect;
						update();
					}
				}
			}

			nana::point caret_descriptor::position() const
			{
				return point_;
			}

			void caret_descriptor::visible(bool is_show)
			{
				auto pre_displayed = (visible_state::displayed == visible_state_);

				if (is_show)
				{
					visible_state_ = visible_state::visible;
					if (wd_->displayed() && (! out_of_range_))
						visible_state_ = visible_state::displayed;
				}
				else
					visible_state_ = visible_state::invisible;

				if (pre_displayed != (visible_state::displayed == visible_state_))
					native_interface::caret_visible(wd_->root, !pre_displayed);
			}

			bool caret_descriptor::visible() const
			{
				return (visible_state::invisible != visible_state_);
			}

			nana::size caret_descriptor::size() const
			{
				return size_;
			}

			void caret_descriptor::size(const nana::size& s)
			{
				size_ = s;
				update();

				if (visible_state::invisible != visible_state_)
					visible(true);
			}

			void caret_descriptor::update()
			{
				nana::point pos = point_;
				nana::size	size = size_;

				nana::rectangle rect = effective_range_;
				if(0 == effective_range_.width || 0 == effective_range_.height)
				{
					rect.x = rect.y = 0;
					rect = wd_->dimension;
				}
				else
				{
					pos.x += effective_range_.x;
					pos.y += effective_range_.y;
				}

				if(	(pos.x + static_cast<int>(size.width) <= rect.x) || (pos.x >= rect.right()) ||
					(pos.y + static_cast<int>(size.height) <= rect.y) || (pos.y >= rect.bottom())
					)
				{//Out of Range without overlap
					if(false == out_of_range_)
					{
						out_of_range_ = true;

						if (visible_state::invisible != visible_state_)
							visible(false);
					}
				}
				else
				{
					if(pos.x < rect.x)
					{
						size.width -= (rect.x - pos.x);
						pos.x = rect.x;
					}
					else if(pos.x + static_cast<int>(size.width) > rect.right())
					{
						size.width -= pos.x + size.width - rect.right();
					}

					if(pos.y < rect.y)
					{
						size.width -= (rect.y - pos.y);
						pos.y = rect.y;
					}
					else if(pos.y + static_cast<int>(size.height) > rect.bottom())
						size.height -= pos.y + size.height - rect.bottom();

					if(out_of_range_)
					{
						if (paint_size_ == size)
							visible(true);

						out_of_range_ = false;
					}

					if(paint_size_ != size)
					{
						bool vs = (visible_state::invisible != visible_state_);
						native_interface::caret_destroy(wd_->root);
						native_interface::caret_create(wd_->root, size);

						visible_state_ = visible_state::invisible;
						if (vs)
							visible(true);


						paint_size_ = size;
					}
				
					native_interface::caret_pos(wd_->root, wd_->pos_root + pos);
				}
			}
		//end class caret_descriptor

		//struct basic_window
			//struct basic_window::other_tag
				basic_window::other_tag::other_tag(category::flags categ)
					: category(categ), active_window(nullptr), upd_state(update_state::none)
				{
					switch(categ)
					{
					case category::root_tag::value:
						attribute.root = new attr_root_tag;
						break;
					case category::frame_tag::value:
						attribute.frame = new attr_frame_tag;
						break;
					default:
						attribute.root = nullptr;
					}
				}

				basic_window::other_tag::~other_tag()
				{
					switch(category)
					{
					case category::root_tag::value:
						delete attribute.root;
						break;
					case category::frame_tag::value:
						delete attribute.frame;
						break;
					default: break;
					}
				}
			//end struct basic_window::other_tag

			//basic_window
			//@brief: constructor for the root window
			basic_window::basic_window(basic_window* owner, std::unique_ptr<widget_notifier_interface>&& wdg_notifier, category::root_tag**)
				: widget_notifier(std::move(wdg_notifier)), other(category::root_tag::value)
			{
				drawer.bind(this);
				_m_init_pos_and_size(nullptr, rectangle());
				this->_m_initialize(owner);
			}

			basic_window::~basic_window()
			{
				delete together.caret;
				together.caret = nullptr;

				delete effect.bground;
				effect.bground = nullptr;
			}

			//bind_native_window
			//@brief: bind a native window and baisc_window
			void basic_window::bind_native_window(native_window_type wd, unsigned width, unsigned height, unsigned extra_width, unsigned extra_height, nana::paint::graphics& graphics)
			{
				if(category::root_tag::value == this->other.category)
				{
					this->root = wd;
					dimension.width = width;
					dimension.height = height;
					this->extra_width = extra_width;
					this->extra_height = extra_height;
					this->root_widget = this;
					this->root_graph = &graphics;
				}
			}

			void basic_window::frame_window(native_window_type wd)
			{
				if(category::frame_tag::value == this->other.category)
					other.attribute.frame->container = wd;
			}

			bool basic_window::is_ancestor_of(const basic_window* wd) const
			{
				while (wd)
				{
					if (this == wd->parent)
						return true;
					wd = wd->parent;
				}
				return false;
			}

			bool basic_window::visible_parents() const
			{
				for (auto pnt = parent; pnt; pnt = pnt->parent)
				{
					if (!pnt->visible)
						return false;
				}
				return true;
			}

			bool basic_window::displayed() const
			{
				return (visible && visible_parents());
			}

			bool basic_window::belong_to_lazy() const
			{
				for (auto wd = this; wd; wd = wd->parent)
				{
					if (basic_window::update_state::refresh == wd->other.upd_state)
						return true;
				}
				return false;
			}

			const basic_window* get_child_caret(const basic_window* wd, bool this_is_a_child)
			{
				if (this_is_a_child && wd->together.caret)
					return wd;

				for (auto child : wd->children)
				{
					auto caret_wd = get_child_caret(child, true);
					if (caret_wd)
						return caret_wd;
				}

				return nullptr;
			}

			const basic_window * basic_window::child_caret() const
			{
				return get_child_caret(this, false);
			}

			bool basic_window::is_draw_through() const
			{
				if (::nana::category::flags::root == this->other.category)
					return static_cast<bool>(other.attribute.root->draw_through);
				return false;
			}

			basic_window * basic_window::seek_non_lite_widget_ancestor() const
			{
				auto anc = this->parent;
				while (anc && (category::flags::lite_widget == anc->other.category))
					anc = anc->parent;
				
				return anc;
			}

			void basic_window::_m_init_pos_and_size(basic_window* parent, const rectangle& r)
			{
				pos_owner = pos_root = r;
				dimension = r;

				if (parent)
					pos_root += parent->pos_root;
			}

			void basic_window::_m_initialize(basic_window* agrparent)
			{
				if(other.category == category::root_tag::value)
				{
					if(agrparent && (nana::system::this_thread_id() != agrparent->thread_id))
						agrparent = nullptr;

					while(agrparent && (agrparent->other.category != category::root_tag::value))
						agrparent = agrparent->parent;
				
					owner = agrparent;
					parent = nullptr;
					index = 0;
				}
				else
				{
					parent = agrparent;
					owner = nullptr;
					root_widget = agrparent->root_widget;
					root = agrparent->root;
					root_graph = agrparent->root_graph;
					index = static_cast<unsigned>(agrparent->children.size());
					agrparent->children.push_back(this);
				}

				predef_cursor = cursor::arrow;
				flags.captured = false;
				flags.dbl_click = true;
				flags.enabled = true;
				flags.modal = false;
				flags.take_active = true;
				flags.dropable = false;
				flags.fullscreen = false;
				flags.tab = nana::detail::tab_type::none;
				flags.action = mouse_action::normal;
				flags.refreshing = false;
				flags.destroying = false;
				flags.borderless = false;
				flags.make_bground_declared	= false;
				flags.ignore_menubar_focus	= false;
				flags.ignore_mouse_focus	= false;
				flags.space_click_enabled = false;

				visible = false;

				effect.edge_nimbus = effects::edge_nimbus::none;
				effect.bground = nullptr;
				effect.bground_fade_rate = 0;

				together.caret = nullptr;

				extra_width = extra_height = 0;

				//The window must keep its thread_id same as its parent if it is a child.
				//Otherwise, its root buffer would be mapped repeatly if it is in its parent thread.
				thread_id = nana::system::this_thread_id();
				if(agrparent && (thread_id != agrparent->thread_id))
					thread_id = agrparent->thread_id;
			}

			bool basic_window::set_events(const std::shared_ptr<general_events>& p)
			{
				if (together.events_ptr)
					return false;
				together.events_ptr = p;
				return true;
			}

			general_events * basic_window::get_events() const
			{
				return together.events_ptr.get();
			}
		//end struct basic_window
	}//end namespace detail
}//end namespace nana
