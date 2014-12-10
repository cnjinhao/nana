#include <nana/gui/detail/basic_window.hpp>
#include <nana/gui/detail/native_window_interface.hpp>

namespace nana
{
	namespace detail
	{
		//class caret_descriptor
			caret_descriptor::caret_descriptor(core_window_t* wd, unsigned width, unsigned height)
				:wd_(wd), size_(width, height), visible_(false), real_visible_state_(false), out_of_range_(false)
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
						native_interface::caret_create(wd_->root, size_.width, size_.height);
						real_visible_state_ = false;
						visible_ = false;
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
				if(	(rect.width && rect.height) &&
					(rect.x + rect.width > 0) &&
					(rect.y + rect.height > 0))
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

			void caret_descriptor::visible(bool isshow)
			{
				if(visible_ != isshow)
				{
					visible_ = isshow;
					if(visible_ == false || false == out_of_range_)
						_m_visible(isshow);
				}
			}

			bool caret_descriptor::visible() const
			{
				return visible_;
			}

			nana::size caret_descriptor::size() const
			{
				return size_;
			}

			void caret_descriptor::size(const nana::size& s)
			{
				size_ = s;
				update();

				if(visible_)	this->visible(true);
			}

			void caret_descriptor::_m_visible(bool isshow)
			{
				if(real_visible_state_ != isshow)
				{
					real_visible_state_ = isshow;
					native_interface::caret_visible(wd_->root, isshow);
				}
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

				if(	(pos.x + static_cast<int>(size.width) <= rect.x) || (pos.x >= rect.x + static_cast<int>(rect.width)) ||
					(pos.y + static_cast<int>(size.height) <= rect.y) || (pos.y >= rect.y + static_cast<int>(rect.height))
					)
				{//Out of Range without overlap
					if(false == out_of_range_)
					{
						out_of_range_ = true;

						if(visible_)
							_m_visible(false);
					}
				}
				else
				{
					if(pos.x < rect.x)
					{
						size.width -= (rect.x - pos.x);
						pos.x = rect.x;
					}
					else if(pos.x + size.width > rect.x + rect.width)
					{
						size.width -= pos.x + size.width - (rect.x + rect.width);
					}

					if(pos.y < rect.y)
					{
						size.width -= (rect.y - pos.y);
						pos.y = rect.y;
					}
					else if(pos.y + size.height > rect.y + rect.height)
						size.height -= pos.y + size.height - (rect.y + rect.height);

					if(out_of_range_)
					{
						if(paint_size_ == size)
							_m_visible(true);

						out_of_range_ = false;
					}

					if(paint_size_ != size)
					{
						native_interface::caret_destroy(wd_->root);
						native_interface::caret_create(wd_->root, size.width, size.height);
						real_visible_state_ = false;
						if(visible_)
							_m_visible(true);

						paint_size_ = size;
					}
				
					native_interface::caret_pos(wd_->root, wd_->pos_root.x + pos.x, wd_->pos_root.y + pos.y);
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
						attribute.root->context.focus_changed = false;
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
			basic_window::basic_window(basic_window* owner, widget* wdg, category::root_tag**)
				: widget_ptr(wdg), other(category::root_tag::value)
			{
				drawer.bind(this);
				_m_init_pos_and_size(0, rectangle());
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

			bool basic_window::belong_to_lazy() const
			{
				for (auto wd = this; wd; wd = wd->parent)
				{
					if (basic_window::update_state::refresh == wd->other.upd_state)
						return true;
				}
				return false;
			}

			void basic_window::_m_init_pos_and_size(basic_window* parent, const rectangle& r)
			{
				pos_owner = pos_root = r;
				dimension = r;

				if(parent)
				{
					pos_root.x += parent->pos_root.x;
					pos_root.y += parent->pos_root.y;
				}
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
				flags.capture = false;
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

				visible = false;

				color.foreground = 0x0;
				color.background = nana::color::button_face;
				color.active = 0x60C8FD;

				effect.edge_nimbus = effects::edge_nimbus::none;
				effect.bground = nullptr;
				effect.bground_fade_rate = 0;

				together.caret = nullptr;
				together.attached_events = nullptr;

				extra_width = extra_height = 0;

				//The window must keep its thread_id same as its parent if it is a child.
				//Otherwise, its root buffer would be mapped repeatly if it is in its parent thread.
				thread_id = nana::system::this_thread_id();
				if(agrparent && (thread_id != agrparent->thread_id))
					thread_id = agrparent->thread_id;
			}

			bool basic_window::set_events(const std::shared_ptr<general_events>& p)
			{
				if (together.attached_events)
					return false;
				together.events_ptr = p;
				together.attached_events = p.get();
				return true;
			}

			general_events * basic_window::get_events() const
			{
				return together.attached_events;
			}
		//end struct basic_window
	}//end namespace detail
}//end namespace nana
