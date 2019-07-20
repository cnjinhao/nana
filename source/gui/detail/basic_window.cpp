/*
*	A Basic Window Widget Definition
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file: nana/gui/detail/basic_window.cpp
*/

#include "basic_window.hpp"
#include <nana/gui/detail/native_window_interface.hpp>

namespace nana
{
	namespace detail
	{
		//class caret
			caret::caret(basic_window* owner, const size& size):
				owner_(owner),
				size_(size)
			{}

			caret::~caret()
			{
				if (owner_)
					native_interface::caret_destroy(owner_->root);
			}

			void caret::activate(bool activity)
			{
				if (owner_)
				{
					if (activity)
					{
						native_interface::caret_create(owner_->root, size_);

						visibility_ = visible_state::invisible;
						this->position(position_);
					}
					else
						native_interface::caret_destroy(owner_->root);

					owner_->root_widget->other.attribute.root->ime_enabled = activity;
				}
			}

			basic_window* caret::owner() const noexcept
			{
				return owner_;
			}

			void caret::update()
			{
				auto pos = position_;
				auto size = size_;

				auto rect = effect_range_;
				if (0 == effect_range_.width || 0 == effect_range_.height)
				{
					rect.x = rect.y = 0;
					rect.dimension(owner_->dimension);
				}
				else
				{
					pos += effect_range_.position();
				}

				if ((pos.x + static_cast<int>(size.width) <= rect.x) || (pos.x >= rect.right()) ||
					(pos.y + static_cast<int>(size.height) <= rect.y) || (pos.y >= rect.bottom())
					)
				{//Out of Range without overlap
					if (false == out_of_range_)
					{
						out_of_range_ = true;

						if (visible_state::invisible != visibility_)
							visible(false);
					}
				}
				else
				{
					if (pos.x < rect.x)
					{
						size.width -= (rect.x - pos.x);
						pos.x = rect.x;
					}
					else if (pos.x + static_cast<int>(size.width) > rect.right())
					{
						size.width -= pos.x + size.width - rect.right();
					}

					if (pos.y < rect.y)
					{
						size.width -= (rect.y - pos.y);
						pos.y = rect.y;
					}
					else if (pos.y + static_cast<int>(size.height) > rect.bottom())
						size.height -= pos.y + size.height - rect.bottom();

					if (out_of_range_)
					{
						if (visual_size_ == size)
							visible(true);

						out_of_range_ = false;
					}

					if (visual_size_ != size)
					{
						bool vs = (visible_state::invisible != visibility_);
						native_interface::caret_destroy(owner_->root);
						native_interface::caret_create(owner_->root, size);

						visibility_ = visible_state::invisible;
						if (vs)
							visible(true);


						visual_size_ = size;
					}

					native_interface::caret_pos(owner_->root, owner_->pos_root + pos);
				}
			}
		
			//Implement caret_interface functions
			void caret::disable_throw() noexcept
			{
				//This function is useless for class caret, see caret_proxy.
			}

			void caret::effective_range(const rectangle& r)
			{
				auto range = r;
				//Chech rect
				if (range.width && range.height && range.right() > 0 && range.bottom() > 0)
				{
					if (range.x < 0)
					{
						range.width += range.x;
						range.x = 0;
					}

					if (range.y < 0)
					{
						range.height += range.y;
						range.y = 0;
					}

					if (effect_range_ != range)
					{
						effect_range_ = range;
						update();
					}
				}
			}

			void caret::position(const point& pos)
			{
				position_ = pos;
				update();
			}

			point caret::position() const
			{
				return position_;
			}

			size caret::dimension() const
			{
				return size_;
			}

			void caret::dimension(const size& s)
			{
				size_ = s;
				update();

				if (visible_state::invisible != visibility_)
					visible(true);
			}

			void caret::visible(bool visibility)
			{
				auto pre_displayed = (visible_state::displayed == visibility_);

				if (visibility)
				{
					visibility_ = visible_state::visible;
					if (owner_->displayed() && (!out_of_range_))
						visibility_ = visible_state::displayed;
				}
				else
					visibility_ = visible_state::invisible;

				if (pre_displayed != (visible_state::displayed == visibility_))
					native_interface::caret_visible(owner_->root, !pre_displayed);
			}

			bool caret::visible() const
			{
				return (visible_state::invisible != visibility_);
			}

			bool caret::activated() const
			{
				return (visible_state::displayed == visibility_);
			}
		//end class caret

		//struct basic_window
			//struct basic_window::other_tag
				basic_window::other_tag::other_tag(category::flags categ)
					: category(categ), active_window(nullptr), upd_state(update_state::none)
				{
					if (category::flags::root == categ)
						attribute.root = new attr_root_tag;
					else
						attribute.root = nullptr;
				}

				basic_window::other_tag::~other_tag()
				{
					if (category::flags::root == category)
						delete attribute.root;
				}
			//end struct basic_window::other_tag

			//basic_window
			//@brief: constructor for the root window
			basic_window::basic_window(basic_window* owner, std::unique_ptr<widget_notifier_interface>&& wdg_notifier, category::root_tag**)
				: widget_notifier(std::move(wdg_notifier)), other(category::flags::root)
			{
				drawer.bind(this);
				_m_init_pos_and_size(nullptr, rectangle());
				this->_m_initialize(owner);
			}

			basic_window::~basic_window()
			{
				delete annex.caret_ptr;
				annex.caret_ptr = nullptr;

				delete effect.bground;
				effect.bground = nullptr;
			}

			//bind_native_window
			//@brief: bind a native window and baisc_window
			void basic_window::bind_native_window(native_window_type wd, unsigned width, unsigned height, unsigned extra_width, unsigned extra_height, nana::paint::graphics& graphics)
			{
				if(category::flags::root == this->other.category)
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
					if (basic_window::update_state::refreshed == wd->other.upd_state)
						return true;
				}
				return false;
			}

			const basic_window * basic_window::child_caret() const
			{
				for (auto child : children) {
					//Only return the child who has activated caret.
					if (child->annex.caret_ptr && child->annex.caret_ptr->activated())
						return child;

					auto caret = child->child_caret();
					if (caret)
						return caret;
				}
				return nullptr;
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

			void basic_window::set_action(mouse_action act)
			{
				flags.action_before = flags.action;
				flags.action = act;
			}


			bool basic_window::try_lazy_update(bool try_refresh)
			{
				if (drawer.graphics.empty())
					return true;

				if (!this->root_widget->other.attribute.root->lazy_update)
					return false;
				
				if (nullptr == effect.bground)
				{
					if (try_refresh)
					{
						flags.refreshing = true;
						drawer.refresh();
						flags.refreshing = false;
					}
				}

				for (auto i = this->root_widget->other.attribute.root->update_requesters.cbegin(); i != this->root_widget->other.attribute.root->update_requesters.cend();)
				{
					auto req = *i;
					//Avoid redundancy, don't insert the window if it or its ancestor window already exist in the container.
					if ((req == this) || req->is_ancestor_of(this))
						return true;

					//If there is a window which is a child or child's child of the window, remove it.
					if (this->is_ancestor_of(req))
						i = this->root_widget->other.attribute.root->update_requesters.erase(i);
					else
						++i;
				}

				this->root_widget->other.attribute.root->update_requesters.push_back(this);
				return true;
			}

			void basic_window::_m_init_pos_and_size(basic_window* parent, const rectangle& r)
			{
				pos_owner = pos_root = r.position();
				dimension = r.dimension();

				if (parent)
					pos_root += parent->pos_root;
			}

			void basic_window::_m_initialize(basic_window* agrparent)
			{
				if(category::flags::root == other.category)
				{
					if(agrparent && (nana::system::this_thread_id() != agrparent->thread_id))
						agrparent = nullptr;

					while(agrparent && (category::flags::root != agrparent->other.category))
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
					agrparent->children.emplace_back(this);
				}

				predef_cursor = cursor::arrow;
				flags.captured = false;
				flags.dbl_click = true;
				flags.enabled = true;
				flags.modal = false;
				flags.take_active = true;
				flags.draggable = false;
				flags.dropable = false;
				flags.fullscreen = false;
				flags.tab = nana::detail::tab_type::none;
				flags.action = mouse_action::normal;
				flags.action_before = mouse_action::normal;

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

				extra_width = extra_height = 0;

				//The window must keep its thread_id same as its parent if it is a child.
				//Otherwise, its root buffer would be mapped repeatedly if it is in its parent thread.
				thread_id = nana::system::this_thread_id();
				if(agrparent && (thread_id != agrparent->thread_id))
					thread_id = agrparent->thread_id;
			}

			bool basic_window::set_events(const std::shared_ptr<general_events>& p)
			{
				annex.events_ptr = p;
				return true;
			}

			general_events * basic_window::get_events() const
			{
				return annex.events_ptr.get();
			}
		//end struct basic_window
	}//end namespace detail
}//end namespace nana
