/*
 *	Window Manager Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/detail/window_manager.cpp
 *	@contributors:	Katsuhisa Yuasa
 */

#include <nana/config.hpp>
#include <nana/gui/detail/bedrock.hpp>
#include <nana/gui/detail/events_operation.hpp>
#include <nana/gui/detail/handle_manager.hpp>
#include <nana/gui/detail/window_manager.hpp>
#include <nana/gui/detail/native_window_interface.hpp>
#include <nana/gui/detail/inner_fwd_implement.hpp>
#include <nana/gui/layout_utility.hpp>
#include <nana/gui/detail/effects_renderer.hpp>
#include <stdexcept>
#include <algorithm>

namespace nana
{

namespace detail
{
	template<typename Key, typename Value>
	class lite_map
	{
		struct key_value_rep
		{
			Key first;
			Value second;

			key_value_rep()
				: first{}, second{}
			{}

			key_value_rep(const Key& k)
				: first(k), second{}
			{
			}
		};
	public:
		using iterator = typename std::vector<key_value_rep>::iterator;

		Value& operator[](const Key& key)
		{
			for (auto& kv : table_)
			{
				if (kv.first == key)
					return kv.second;
			}

			table_.emplace_back(key);
			return table_.back().second;
		}

		iterator find(const Key& key)
		{
			for (auto i = table_.begin(); i != table_.end(); ++i)
				if (i->first == key)
					return i;

			return table_.end();
		}

		iterator erase(iterator pos)
		{
			return table_.erase(pos);
		}

		iterator begin()
		{
			return table_.begin();
		}

		iterator end()
		{
			return table_.end();
		}
	private:
		std::vector<key_value_rep> table_;
	};
	//class window_manager

			struct window_handle_deleter
			{
				void operator()(basic_window* wd) const
				{
					bedrock::instance().evt_operation().umake(reinterpret_cast<window>(wd));
					delete wd;
				}
			};
			
			//struct wdm_private_impl
			struct window_manager::wdm_private_impl
			{
				root_register	misc_register;
				handle_manager<core_window_t*, window_manager, window_handle_deleter>	wd_register;
				paint::image default_icon_big;
				paint::image default_icon_small;

				lite_map<core_window_t*, std::vector<std::function<void()>>> safe_place;
			};
		//end struct wdm_private_impl

		//class revertible_mutex
			window_manager::revertible_mutex::revertible_mutex()
			{
				thr_.tid = 0;
				thr_.refcnt = 0;
			}

			void window_manager::revertible_mutex::lock()
			{
				std::recursive_mutex::lock();
				if(0 == thr_.tid)
					thr_.tid = nana::system::this_thread_id();
				++thr_.refcnt;
			}

			bool window_manager::revertible_mutex::try_lock()
			{
				if(std::recursive_mutex::try_lock())
				{
					if(0 == thr_.tid)
						thr_.tid = nana::system::this_thread_id();
					++thr_.refcnt;
					return true;
				}
				return false;
			}

			void window_manager::revertible_mutex::unlock()
			{
				if(thr_.tid == nana::system::this_thread_id())
					if(0 == --thr_.refcnt)
						thr_.tid = 0;
				std::recursive_mutex::unlock();
			}

			void window_manager::revertible_mutex::revert()
			{
				if(thr_.refcnt && (thr_.tid == nana::system::this_thread_id()))
				{
					std::size_t cnt = thr_.refcnt;

					stack_.push_back(thr_);
					thr_.tid = 0;
					thr_.refcnt = 0;

					for(std::size_t i = 0; i < cnt; ++i)
						std::recursive_mutex::unlock();
				}
			}

			void window_manager::revertible_mutex::forward()
			{
				std::recursive_mutex::lock();
				if(stack_.size())
				{
					auto thr = stack_.back();
					if(thr.tid == nana::system::this_thread_id())
					{
						stack_.pop_back();
						for(std::size_t i = 0; i < thr.refcnt; ++i)
							std::recursive_mutex::lock();
						thr_ = thr;
					}
					else
						throw std::runtime_error("Nana.GUI: The forward is not matched.");
				}
				std::recursive_mutex::unlock();
			}
		//end class revertible_mutex

			//Utilities in this unit.
			namespace utl
			{
				template<typename T>
				bool erase(std::vector<T>& container, T value)
				{
					for (auto i = container.begin(), end = container.end(); i != end; ++i)
					{
						if ((*i) == value)
						{
							container.erase(i);
							return true;
						}
					}
					return false;
				}
			}

		window_manager::window_manager()
			: impl_(new wdm_private_impl)
		{
			attr_.capture.window = nullptr;
			attr_.capture.ignore_children = true;

			menu_.window = nullptr;
			menu_.owner = nullptr;
			menu_.has_keyboard = false;
		}

		window_manager::~window_manager()
		{
			delete impl_;
		}

		bool window_manager::is_queue(core_window_t* wd)
		{
			return (wd && (category::flags::root == wd->other.category));
		}

		std::size_t window_manager::number_of_core_window() const
		{
			return impl_->wd_register.size();
		}

		window_manager::mutex_type& window_manager::internal_lock() const
		{
			return mutex_;
		}

		void window_manager::all_handles(std::vector<core_window_t*> &v) const
		{
			impl_->wd_register.all(v);
		}

		void window_manager::event_filter(core_window_t* wd, bool is_make, event_code evtid)
		{
			switch(evtid)
			{
			case event_code::mouse_drop:
				wd->flags.dropable = (is_make || (0 != wd->together.events_ptr->mouse_dropfiles.length()));
				break;
			default:
				break;
			}
		}

		bool window_manager::available(core_window_t* wd)
		{
			return impl_->wd_register.available(wd);
		}

		bool window_manager::available(core_window_t * a, core_window_t* b)
		{
			return (impl_->wd_register.available(a) && impl_->wd_register.available(b));
		}

		bool window_manager::available(native_window_type wd)
		{
			if(wd)
			{
				std::lock_guard<decltype(mutex_)> lock(mutex_);
				return (impl_->misc_register.find(wd) != nullptr);
			}
			return false;
		}

		window_manager::core_window_t* window_manager::create_root(core_window_t* owner, bool nested, rectangle r, const appearance& app, widget* wdg)
		{
			native_window_type native = nullptr;
			if (owner)
			{
				//Thread-Safe Required!
				std::lock_guard<decltype(mutex_)> lock(mutex_);

				if (impl_->wd_register.available(owner))
				{
					if (owner->flags.destroying)
						throw std::logic_error("the specified owner is destory");

					native = (category::flags::frame == owner->other.category ?
										owner->other.attribute.frame->container : owner->root_widget->root);
					r.x += owner->pos_root.x;
					r.y += owner->pos_root.y;
				}
				else
					owner = nullptr;
			}

			auto result = native_interface::create_window(native, nested, r, app);
			if (result.native_handle)
			{
				core_window_t* wd = new core_window_t(owner, widget_notifier_interface::get_notifier(wdg), (category::root_tag**)nullptr);
				if (nested)
				{
					wd->owner = nullptr;
					wd->parent = owner;
					wd->index = static_cast<unsigned>(owner->children.size());
					owner->children.push_back(wd);
				}

				wd->flags.take_active = !app.no_activate;
				wd->title = native_interface::window_caption(result.native_handle);

				//Thread-Safe Required!
				std::lock_guard<decltype(mutex_)> lock(mutex_);

				//create Root graphics Buffer and manage it
				root_misc misc(wd, result.width, result.height);
				auto* value = impl_->misc_register.insert(result.native_handle, misc);

				wd->bind_native_window(result.native_handle, result.width, result.height, result.extra_width, result.extra_height, value->root_graph);
				impl_->wd_register.insert(wd, wd->thread_id);

				if (owner && owner->other.category == category::frame_tag::value)
					insert_frame(owner, wd);

				bedrock::inc_window(wd->thread_id);
				this->icon(wd, impl_->default_icon_small, impl_->default_icon_big);
				return wd;
			}
			return nullptr;
		}

		window_manager::core_window_t* window_manager::create_frame(core_window_t* parent, const rectangle& r, widget* wdg)
		{
			//Thread-Safe Required!
			std::lock_guard<decltype(mutex_)> lock(mutex_);

			if (impl_->wd_register.available(parent) == false)	return nullptr;

			core_window_t * wd = new core_window_t(parent, widget_notifier_interface::get_notifier(wdg), r, (category::frame_tag**)nullptr);
			wd->frame_window(native_interface::create_child_window(parent->root, rectangle(wd->pos_root.x, wd->pos_root.y, r.width, r.height)));
			impl_->wd_register.insert(wd, wd->thread_id);

			//Insert the frame_widget into its root frames container.
			wd->root_widget->other.attribute.root->frames.push_back(wd);
			return (wd);
		}

		bool window_manager::insert_frame(core_window_t* frame, native_window wd)
		{
			if(frame)
			{
				//Thread-Safe Required!
				std::lock_guard<decltype(mutex_)> lock(mutex_);
				if(frame->other.category == category::frame_tag::value)
					frame->other.attribute.frame->attach.push_back(wd);
				return true;
			}
			return false;
		}

		bool window_manager::insert_frame(core_window_t* frame, core_window_t* wd)
		{
			if(frame)
			{
				//Thread-Safe Required!
				std::lock_guard<decltype(mutex_)> lock(mutex_);
				if(frame->other.category == category::frame_tag::value)
				{
					if (impl_->wd_register.available(wd) && wd->other.category == category::root_tag::value && wd->root != frame->root)
					{
						frame->other.attribute.frame->attach.push_back(wd->root);
						return true;
					}
				}
			}
			return false;
		}

		window_manager::core_window_t* window_manager::create_widget(core_window_t* parent, const rectangle& r, bool is_lite, widget* wdg)
		{
			//Thread-Safe Required!
			std::lock_guard<decltype(mutex_)> lock(mutex_);
			if (impl_->wd_register.available(parent) == false)
				throw std::invalid_argument("invalid parent/owner handle");

			if (parent->flags.destroying)
				throw std::logic_error("the specified parent is destory");

			auto wdg_notifier = widget_notifier_interface::get_notifier(wdg);

			core_window_t * wd;
			if (is_lite)
				wd = new core_window_t(parent, std::move(wdg_notifier), r, (category::lite_widget_tag**)nullptr);
			else
				wd = new core_window_t(parent, std::move(wdg_notifier), r, (category::widget_tag**)nullptr);
			impl_->wd_register.insert(wd, wd->thread_id);
			return wd;
		}

		void window_manager::close(core_window_t *wd)
		{
			//Thread-Safe Required!
			std::lock_guard<decltype(mutex_)> lock(mutex_);
			if (impl_->wd_register.available(wd) == false)	return;

			if (wd->flags.destroying)
				return;

			if(wd->other.category == category::root_tag::value)
			{
				auto &brock = bedrock::instance();
				arg_unload arg;
				arg.window_handle = reinterpret_cast<window>(wd);
				arg.cancel = false;
				brock.emit(event_code::unload, wd, arg, true, brock.get_thread_context());
				if (false == arg.cancel)
				{
					//Before close the window, its owner window should be actived, otherwise other window will be
					//activated due to the owner window is not enabled.
					if(wd->flags.modal || (wd->owner == nullptr) || wd->owner->flags.take_active)
						native_interface::activate_owner(wd->root);

					if (!wd->flags.destroying)
					{
						//Close should detach the drawer and send destroy signal to widget object.
						//Otherwise, when a widget object is been deleting in other thread by delete operator, the object will be destroyed
						//before the window_manager destroyes the window, and then, window_manager detaches the
						//non-existing drawer_trigger which is destroyed by destruction of widget. Crash!
						wd->drawer.detached();
						wd->widget_notifier->destroy();
					}

					native_interface::close_window(wd->root);
				}
			}
			else
				destroy(wd);
		}

		//destroy
		//@brief:	Delete the window handle
		void window_manager::destroy(core_window_t* wd)
		{
			//Thread-Safe Required!
			std::lock_guard<decltype(mutex_)> lock(mutex_);
			if (impl_->wd_register.available(wd) == false)	return;

			rectangle update_area(wd->pos_owner, wd->dimension);

			auto parent = wd->parent;
			if (parent)
				utl::erase(parent->children, wd);

			_m_destroy(wd);

			while (parent && (parent->other.category == ::nana::category::flags::lite_widget))
			{
				update_area.x += parent->pos_owner.x;
				update_area.y += parent->pos_owner.y;
				parent = parent->parent;
			}

			update(parent, false, false, &update_area);
		}

		//destroy_handle
		//@brief:	Delete window handle, the handle type must be a root and a frame.
		void window_manager::destroy_handle(core_window_t* wd)
		{
			//Thread-Safe Required!
			std::lock_guard<decltype(mutex_)> lock(mutex_);
			if (impl_->wd_register.available(wd) == false)	return;

			if((wd->other.category == category::root_tag::value) || (wd->other.category != category::frame_tag::value))
			{
				impl_->misc_register.erase(wd->root);
				impl_->wd_register.remove(wd);
			}
		}

		void window_manager::default_icon(const nana::paint::image& _small, const nana::paint::image& big)
		{
			impl_->default_icon_big = big;
			impl_->default_icon_small = _small;
		}

		void window_manager::icon(core_window_t* wd, const paint::image& small_icon, const paint::image& big_icon)
		{
			if(!big_icon.empty() || !small_icon.empty())
			{
				std::lock_guard<decltype(mutex_)> lock(mutex_);
				if (impl_->wd_register.available(wd))
				{
					if(wd->other.category == category::root_tag::value)
						native_interface::window_icon(wd->root, small_icon, big_icon);
				}
			}
		}

		//show
		//@brief: show or hide a window
		bool window_manager::show(core_window_t* wd, bool visible)
		{
			//Thread-Safe Required!
			std::lock_guard<decltype(mutex_)> lock(mutex_);
			if (!impl_->wd_register.available(wd))
				return false;

			if(visible != wd->visible)
			{
				native_window_type nv = nullptr;
				switch(wd->other.category)
				{
				case category::root_tag::value:
					nv = wd->root; break;
				case category::frame_tag::value:
					nv = wd->other.attribute.frame->container; break;
				default:	//category::widget_tag, category::lite_widget_tag
					break;
				}

				if(visible && wd->effect.bground)
					window_layer::make_bground(wd);

				//Don't set the visible attr of a window if it is a root.
				//The visible attr of a root will be set in the expose event.
				if(category::flags::root != wd->other.category)
					bedrock::instance().event_expose(wd, visible);

				if(nv)
					native_interface::show_window(nv, visible, wd->flags.take_active);
			}
			return true;
		}

		window_manager::core_window_t* window_manager::find_window(native_window_type root, int x, int y)
		{
			if (nullptr == root)
				return nullptr;

			if((false == attr_.capture.ignore_children) || (nullptr == attr_.capture.window) || (attr_.capture.window->root != root))
			{
				//Thread-Safe Required!
				std::lock_guard<decltype(mutex_)> lock(mutex_);
				auto rrt = root_runtime(root);
				point pos{ x, y };
				if (rrt && _m_effective(rrt->window, pos))
					return _m_find(rrt->window, pos);
			}
			return attr_.capture.window;
		}

		//move the wnd and its all children window, x and y is a relatively coordinate for wnd's parent window
		bool window_manager::move(core_window_t* wd, int x, int y, bool passive)
		{
			//Thread-Safe Required!
			std::lock_guard<decltype(mutex_)> lock(mutex_);
			if (impl_->wd_register.available(wd))
			{
				if (category::flags::root != wd->other.category)
				{
					//Move child widgets
					if (x != wd->pos_owner.x || y != wd->pos_owner.y)
					{
						point delta{ x - wd->pos_owner.x, y - wd->pos_owner.y };

						wd->pos_owner.x = x;
						wd->pos_owner.y = y;
						_m_move_core(wd, delta);

						auto &brock = bedrock::instance();
						arg_move arg;
						arg.window_handle = reinterpret_cast<window>(wd);
						arg.x = x;
						arg.y = y;
						brock.emit(event_code::move, wd, arg, true, brock.get_thread_context());
						return true;
					}
				}
				else if (!passive)
				{
					//Check if this root is a nested
					if (wd->parent && (category::flags::root != wd->parent->other.category))
					{
						//The parent of the window is not a root, the position should
						//be transformed to a position based on its parent.

						x += wd->parent->pos_root.x;
						y += wd->parent->pos_root.y;
					}

					native_interface::move_window(wd->root, x, y);
				}
			}

			return false;
		}

		bool window_manager::move(core_window_t* wd, const rectangle& r)
		{
			//Thread-Safe Required!
			std::lock_guard<decltype(mutex_)> lock(mutex_);
			if (!impl_->wd_register.available(wd))
				return false;
				
			auto & brock = bedrock::instance();
			bool moved = false;
			const bool size_changed = (r.width != wd->dimension.width || r.height != wd->dimension.height);
			if(category::flags::root != wd->other.category)
			{
				//Move child widgets
				if(r.x != wd->pos_owner.x || r.y != wd->pos_owner.y)
				{
					point delta{ r.x - wd->pos_owner.x, r.y - wd->pos_owner.y };
					wd->pos_owner.x = r.x;
					wd->pos_owner.y = r.y;
					_m_move_core(wd, delta);
					moved = true;

					arg_move arg;
					arg.window_handle = reinterpret_cast<window>(wd);
					arg.x = r.x;
					arg.y = r.y;
					brock.emit(event_code::move, wd, arg, true, brock.get_thread_context());
				}

				if(size_changed)
					size(wd, nana::size{r.width, r.height}, true, false);
			}
			else
			{
				::nana::rectangle root_r = r;
				//Move event should not get called here,
				//because the window is a root, the event will get called by system event handler.

				//Check if this root is a nested
				if (wd->parent && (category::flags::root != wd->parent->other.category))
				{
					//The parent of the window is not a root, the position should
					//be transformed to a position based on its parent.

					root_r.x += wd->parent->pos_root.x;
					root_r.y += wd->parent->pos_root.y;
				}

				if(size_changed)
				{
					wd->dimension.width = root_r.width;
					wd->dimension.height = root_r.height;
					wd->drawer.graphics.make(wd->dimension);
					wd->root_graph->make(wd->dimension);
					native_interface::move_window(wd->root, root_r);

					arg_resized arg;
					arg.window_handle = reinterpret_cast<window>(wd);
					arg.width = root_r.width;
					arg.height = root_r.height;
					brock.emit(event_code::resized, wd, arg, true, brock.get_thread_context());
				}
				else
					native_interface::move_window(wd->root, root_r.x, root_r.y);
			}

			return (moved || size_changed);
		}

		//size
		//@brief: Size a window
		//@param: passive, if it is true, the function would not change the size if wd is a root_widget.
		//			e.g, when the size of window is changed by OS/user, the function should not resize the
		//			window again, otherwise, it causes an infinite loop, because when a root_widget is resized,
		//			window_manager will call the function.
		bool window_manager::size(core_window_t* wd, nana::size sz, bool passive, bool ask_update)
		{	
			//Thread-Safe Required!
			std::lock_guard<decltype(mutex_)> lock(mutex_);
			if (!impl_->wd_register.available(wd))
				return false;
			
			auto & brock = bedrock::instance();
			if (sz != wd->dimension)
			{
				arg_resizing arg;
				arg.window_handle = reinterpret_cast<window>(wd);
				arg.border = window_border::none;
				arg.width = sz.width;
				arg.height = sz.height;
				brock.emit(event_code::resizing, wd, arg, false, brock.get_thread_context());
				sz.width = arg.width;
				sz.height = arg.height;
			}

			if(wd->max_track_size.width && wd->max_track_size.height)
			{
				if(sz.width > wd->max_track_size.width)
					sz.width = wd->max_track_size.width;
				if(sz.height > wd->max_track_size.height)
					sz.height = wd->max_track_size.height;
			}
			if(wd->min_track_size.width && wd->min_track_size.height)
			{
				if(sz.width < wd->min_track_size.width)
					sz.width = wd->min_track_size.width;
				if(sz.height < wd->min_track_size.height)
					sz.height = wd->min_track_size.height;
			}

			if (wd->dimension == sz)
				return false;

			wd->dimension = sz;

			if(category::lite_widget_tag::value != wd->other.category)
			{
				bool graph_state = wd->drawer.graphics.empty();
				wd->drawer.graphics.make(sz);

				//It shall make a typeface_changed() call when the graphics state is changing.
				//Because when a widget is created with zero-size, it may get some wrong result in typeface_changed() call
				//due to the invaliable graphics object.
				if(graph_state != wd->drawer.graphics.empty())
					wd->drawer.typeface_changed();

				if(category::root_tag::value == wd->other.category)
				{
					wd->root_graph->make(sz);
					if(false == passive)
						native_interface::window_size(wd->root, sz + nana::size(wd->extra_width, wd->extra_height));
				}
				else if(category::frame_tag::value == wd->other.category)
				{
					native_interface::window_size(wd->other.attribute.frame->container, sz);
					for(auto natwd : wd->other.attribute.frame->attach)
						native_interface::window_size(natwd, sz);
				}
				else
				{
					//update the bground buffer of glass window.
					if(wd->effect.bground && wd->parent)
					{
						wd->other.glass_buffer.make(sz);
						window_layer::make_bground(wd);
					}
				}
			}

			arg_resized arg;
			arg.window_handle = reinterpret_cast<window>(wd);
			arg.width = sz.width;
			arg.height = sz.height;
			brock.emit(event_code::resized, wd, arg, ask_update, brock.get_thread_context());
			return true;
		}

		window_manager::core_window_t* window_manager::root(native_window_type wd) const
		{
			static std::pair<native_window_type, core_window_t*> cache;
			if(cache.first == wd) return cache.second;

			//Thread-Safe Required!
			std::lock_guard<decltype(mutex_)> lock(mutex_);

			auto rrt = root_runtime(wd);
			if(rrt)
			{
				cache.first = wd;
				cache.second = rrt->window;
				return cache.second;
			}
			return nullptr;
		}

		//Copy the root buffer that wnd specified into DeviceContext
		void window_manager::map(core_window_t* wd, bool forced, const rectangle* update_area)
		{
			//Thread-Safe Required!
			std::lock_guard<decltype(mutex_)> lock(mutex_);
			if (impl_->wd_register.available(wd) && !wd->is_draw_through())
			{
				auto parent = wd->parent;
				while (parent)
				{
					if (parent->flags.refreshing)
						return;
					parent = parent->parent;
				}

				//Copy the root buffer that wd specified into DeviceContext
#if defined(NANA_LINUX) || defined(NANA_MACOS)
				wd->drawer.map(reinterpret_cast<window>(wd), forced, update_area);
#elif defined(NANA_WINDOWS)
				if(nana::system::this_thread_id() == wd->thread_id)
					wd->drawer.map(reinterpret_cast<window>(wd), forced, update_area);
				else
					bedrock::instance().map_thread_root_buffer(wd, forced, update_area);
#endif
			}
		}

		//update
		//@brief:	update is used for displaying the screen-off buffer.
		//			Because of a good efficiency, if it is called in an event procedure and the event procedure window is the
		//			same as update's, update would not map the screen-off buffer and just set the window for lazy refresh
		bool window_manager::update(core_window_t* wd, bool redraw, bool forced, const rectangle* update_area)
		{
			//Thread-Safe Required!
			std::lock_guard<decltype(mutex_)> lock(mutex_);
			if (impl_->wd_register.available(wd) == false) return false;

			if (wd->displayed())
			{
				if(forced || (false == wd->belong_to_lazy()))
				{
					if (!wd->flags.refreshing)
					{
						window_layer::paint(wd, redraw, false);
						this->map(wd, forced, update_area);
						return true;
					}
				}
				else if (redraw)
					window_layer::paint(wd, true, false);

				if (wd->other.upd_state == core_window_t::update_state::lazy)
					wd->other.upd_state = core_window_t::update_state::refresh;
			}
			return true;
		}

		void window_manager::refresh_tree(core_window_t* wd)
		{
			//Thread-Safe Required!
			std::lock_guard<decltype(mutex_)> lock(mutex_);

			//It's not worthy to redraw if visible is false
			if (impl_->wd_register.available(wd) && wd->displayed())
				window_layer::paint(wd, true, true);
		}

		//do_lazy_refresh
		//@brief: defined a behavior of flush the screen
		//@return: it returns true if the wnd is available
		bool window_manager::do_lazy_refresh(core_window_t* wd, bool force_copy_to_screen)
		{
			//Thread-Safe Required!
			std::lock_guard<decltype(mutex_)> lock(mutex_);

			//It's not worthy to redraw if visible is false
			if (false == impl_->wd_register.available(wd))
				return false;

			if(wd->visible && (!wd->is_draw_through()))
			{
				if (wd->visible_parents())
				{
					if ((wd->other.upd_state == core_window_t::update_state::refresh) || force_copy_to_screen)
					{
						window_layer::paint(wd, false, false);
						this->map(wd, force_copy_to_screen);
					}
					else if (effects::edge_nimbus::none != wd->effect.edge_nimbus)
					{
						this->map(wd, true);
					}
				}
				else
					window_layer::paint(wd, true, false);	//only refreshing if it has an invisible parent
			}
			wd->other.upd_state = core_window_t::update_state::none;
			return true;
		}

		//get_graphics
		//@brief: Get a copy of the graphics object of a window.
		//	the copy of the graphics object has a same buf handle with the graphics object's, they are count-refered
		//	here returns a reference that because the framework does not guarantee the wnd's
		//	graphics object available after a get_graphics call.
		bool window_manager::get_graphics(core_window_t* wd, nana::paint::graphics& result)
		{
			//Thread-Safe Required!
			std::lock_guard<decltype(mutex_)> lock(mutex_);
			if (!impl_->wd_register.available(wd))
				return false;

			result.make(wd->drawer.graphics.size());
			result.bitblt(0, 0, wd->drawer.graphics);
			window_layer::paste_children_to_graphics(wd, result);
			return true;
		}

		bool window_manager::get_visual_rectangle(core_window_t* wd, nana::rectangle& r)
		{
			//Thread-Safe Required!
			std::lock_guard<decltype(mutex_)> lock(mutex_);
			return (impl_->wd_register.available(wd) ?
				window_layer::read_visual_rectangle(wd, r) :
				false);
		}

		std::vector<window_manager::core_window_t*> window_manager::get_children(core_window_t* wd) const
		{
			std::lock_guard<decltype(mutex_)> lock(mutex_);
			if (impl_->wd_register.available(wd))
				return wd->children;
			return{};
		}

		bool window_manager::set_parent(core_window_t* wd, core_window_t* newpa)
		{	
			//Thread-Safe Required!
			std::lock_guard<decltype(mutex_)> lock(mutex_);
			if (!impl_->wd_register.available(wd))
				return false;

			if ((category::flags::lite_widget != wd->other.category) && (category::flags::widget != wd->other.category))
				return false;

			if (impl_->wd_register.available(newpa) && (nullptr == wd->owner) && (wd->parent != newpa) && (!wd->flags.modal))
			{
				//Check the newpa's parent. If wd is ancestor of newpa, return false.
				if (wd->is_ancestor_of(newpa->parent))
					return false;

				auto wdpa = wd->parent;
				this->_m_disengage(wd, newpa);
				this->update(wdpa, true, true);
				this->update(wd, false, true);
				return true;
			}
			return false;
		}

		//set_focus
		//@brief: set a keyboard focus to a window. this may fire a focus event.
		window_manager::core_window_t* window_manager::set_focus(core_window_t* wd, bool root_has_been_focused)
		{
			//Thread-Safe Required!
			std::lock_guard<decltype(mutex_)> lock(mutex_);

			if (!impl_->wd_register.available(wd))
				return nullptr;

			auto & brock = bedrock::instance();
			auto root_wd = wd->root_widget;
			auto prev_focus = root_wd->other.attribute.root->focus;

			arg_focus arg;
			if(wd != prev_focus)
			{
				//kill the previous window focus
				root_wd->other.attribute.root->focus = wd;

				if (impl_->wd_register.available(prev_focus))
				{
					if(prev_focus->together.caret)
						prev_focus->together.caret->set_active(false);

					arg.getting = false;
					arg.window_handle = reinterpret_cast<window>(prev_focus);
					arg.receiver = wd->root;
					brock.emit(event_code::focus, prev_focus, arg, true, brock.get_thread_context());
				}

				//Check the prev_focus again, because it may be closed in focus event
				if (!impl_->wd_register.available(prev_focus))
					prev_focus = nullptr;
			}
			else if(wd->root == native_interface::get_focus_window())
				return prev_focus; //no new focus_window


			if(wd->together.caret)
				wd->together.caret->set_active(true);

			arg.window_handle = reinterpret_cast<window>(wd);
			arg.getting = true;
			arg.receiver = wd->root;
			brock.emit(event_code::focus, wd, arg, true, brock.get_thread_context());

			if (!root_has_been_focused)
				native_interface::set_focus(root_wd->root);

			//A fix by Katsuhisa Yuasa
			//The menubar token window will be redirected to the prev focus window when the new
			//focus window is a menubar.
			//The focus window will be restore to the prev focus which losts the focus becuase of
			//memberbar. 
			if (prev_focus && (wd == wd->root_widget->other.attribute.root->menubar))
				wd = prev_focus;

			if (wd != wd->root_widget->other.attribute.root->menubar)
				brock.set_menubar_taken(wd);

			return prev_focus;
		}

		window_manager::core_window_t* window_manager::capture_redirect(core_window_t* wd)
		{
			if(attr_.capture.window && (attr_.capture.ignore_children == false) && (attr_.capture.window != wd))
			{
				//Tests if the wd is a child of captured window,
				//and returns the wd if it is.
				if (attr_.capture.window->is_ancestor_of(wd))
					return wd;
			}
			return attr_.capture.window;
		}

		void window_manager::capture_ignore_children(bool ignore)
		{
			attr_.capture.ignore_children = ignore;
		}

		bool window_manager::capture_window_entered(int root_x, int root_y, bool& prev)
		{
			if(attr_.capture.window)
			{
				bool inside = _m_effective(attr_.capture.window, point{ root_x, root_y });
				if(inside != attr_.capture.inside)
				{
					prev = attr_.capture.inside;
					attr_.capture.inside = inside;
					return true;
				}
			}
			return false;
		}

		window_manager::core_window_t * window_manager::capture_window() const
		{
			return attr_.capture.window;
		}

		//capture_window
		//@brief:	set a window that always captures the mouse event if it is not in the range of window
		//@return:	this function dose return the previous captured window. If the wnd set captured twice,
		//			the return value is NULL
		window_manager::core_window_t* window_manager::capture_window(core_window_t* wd, bool value)
		{
			if (!this->available(wd))
				return nullptr;

			nana::point pos = native_interface::cursor_position();
			auto & attr_cap = attr_.capture.history;

			if(value)
			{
				if(wd != attr_.capture.window)
				{
					//Thread-Safe Required!
					std::lock_guard<decltype(mutex_)> lock(mutex_);

					if (impl_->wd_register.available(wd))
					{
						wd->flags.captured = true;
						native_interface::capture_window(wd->root, value);
						auto prev = attr_.capture.window;
						if(prev && (prev != wd))
							attr_cap.emplace_back(prev, attr_.capture.ignore_children);

						attr_.capture.window = wd;
						attr_.capture.ignore_children = true;
						native_interface::calc_window_point(wd->root, pos);
						attr_.capture.inside = _m_effective(wd, pos);
						return prev;
					}
				}
				return attr_.capture.window;
			}
			else if(wd == attr_.capture.window)
			{
				attr_.capture.window = nullptr;
				wd->flags.captured = false;
				if(attr_cap.size())
				{
					std::pair<core_window_t*, bool> last = attr_cap.back();
					attr_cap.pop_back();

					if (impl_->wd_register.available(last.first))
					{
						attr_.capture.window = last.first;
						attr_.capture.ignore_children = last.second;
						native_interface::capture_window(last.first->root, true);
						native_interface::calc_window_point(last.first->root, pos);
						last.first->flags.captured = true;
						attr_.capture.inside = _m_effective(last.first, pos);
					}
				}

				if(wd && (nullptr == attr_.capture.window))
					native_interface::capture_window(wd->root, false);
			}
			else
			{
				for (auto i = attr_cap.begin(), end = attr_cap.end(); i != end; ++i)
				{
					if (i->first == wd)
					{
						attr_cap.erase(i);
						break;
					}
				}

				return attr_.capture.window;
			}
			return wd;
		}

		//enable_tabstop
		//@brief: when users press a TAB, the focus should move to the next widget.
		//	this method insert a window which catchs an user TAB into a TAB window container
		//	the TAB window container is held by a wd's root widget. Not every widget has a TAB window container,
		//	the container is created while a first Tab Window is setting
		void window_manager::enable_tabstop(core_window_t* wd)
		{
			//Thread-Safe Required!
			std::lock_guard<decltype(mutex_)> lock(mutex_);
			if (impl_->wd_register.available(wd) && (detail::tab_type::none == wd->flags.tab))
			{
				wd->root_widget->other.attribute.root->tabstop.push_back(wd);
				wd->flags.tab |= detail::tab_type::tabstop;
			}
		}


		// preconditions of get_tabstop: tabstop is not empty and at least one window is visible
		window_manager::core_window_t* get_tabstop(window_manager::core_window_t* wd, bool forward)
		{
			auto & tabs = wd->root_widget->other.attribute.root->tabstop;

			if (forward)
			{
				if (detail::tab_type::none == wd->flags.tab)
					return (tabs.front());
				else if (detail::tab_type::tabstop & wd->flags.tab)
				{
					auto end = tabs.cend();
					auto i = std::find(tabs.cbegin(), end, wd);
					if (i != end)
					{
						++i;
						window_manager::core_window_t* ts = (i != end ? (*i) : tabs.front());
						return (ts != wd ? ts : 0);
					}
					else
						return tabs.front();
				}
			}
			else if (tabs.size() > 1)	//at least 2 elments in tabs are required when moving backward. 
			{
				auto i = std::find(tabs.cbegin(), tabs.cend(), wd);
				if (i != tabs.cend())
					return (tabs.cbegin() == i ? tabs.back() : *(i - 1));
			}
			return nullptr;
		}

		auto window_manager::tabstop(core_window_t* wd, bool forward) const -> core_window_t*
		{
			//Thread-Safe Required!
			std::lock_guard<decltype(mutex_)> lock(mutex_);
			if (!impl_->wd_register.available(wd))
				return nullptr;

			auto & tabs = wd->root_widget->other.attribute.root->tabstop;
			if (tabs.empty())
				return nullptr;

			bool precondition = false;
			for (auto & tab_wd : tabs)
			{
				if (tab_wd->displayed())
				{
					precondition = true;
					break;
				}
			}

			if (precondition)
			{
				auto new_stop = get_tabstop(wd, forward);

				while (new_stop && (wd != new_stop))
				{
					if (new_stop->flags.enabled && new_stop->displayed())
						return new_stop;

					new_stop = get_tabstop(new_stop, forward);
				}
			}

			return nullptr;
		}

		void window_manager::remove_trash_handle(unsigned tid)
		{
			impl_->wd_register.delete_trash(tid);
		}

		bool window_manager::enable_effects_bground(core_window_t* wd, bool enabled)
		{
			//Thread-Safe Required!
			std::lock_guard<decltype(mutex_)> lock(mutex_);
			if (impl_->wd_register.available(wd))
				return window_layer::enable_effects_bground(wd, enabled);

			return false;
		}

		bool window_manager::calc_window_point(core_window_t* wd, nana::point& pos)
		{
			//Thread-Safe Required!
			std::lock_guard<decltype(mutex_)> lock(mutex_);
			if (impl_->wd_register.available(wd))
			{
				if(native_interface::calc_window_point(wd->root, pos))
				{
					pos -= wd->pos_root;
					return true;
				}
			}
			return false;
		}

		root_misc* window_manager::root_runtime(native_window_type native_wd) const
		{
			return impl_->misc_register.find(native_wd);
		}

		bool window_manager::register_shortkey(core_window_t* wd, unsigned long key)
		{
			//Thread-Safe Required!
			std::lock_guard<decltype(mutex_)> lock(mutex_);
			if (impl_->wd_register.available(wd))
			{
				auto object = root_runtime(wd->root);
				if(object)
					return object->shortkeys.make(reinterpret_cast<window>(wd), key);
			}
			return false;
		}

		void window_manager::unregister_shortkey(core_window_t* wd, bool with_children)
		{
			//Thread-Safe Required!
			std::lock_guard<decltype(mutex_)> lock(mutex_);
			if (impl_->wd_register.available(wd) == false) return;

			auto root_rt = root_runtime(wd->root);
			if (root_rt)
			{
				root_rt->shortkeys.umake(reinterpret_cast<window>(wd));
				if (with_children)
				{
					for (auto child : wd->children)
						unregister_shortkey(child, true);
				}
			}
		}

		auto window_manager::shortkeys(core_window_t* wd, bool with_children) -> std::vector<std::pair<core_window_t*, unsigned long>>
		{
			std::vector<std::pair<core_window_t*, unsigned long>> result;

			//Thread-Safe Required!
			std::lock_guard<decltype(mutex_)> lock(mutex_);
			if (impl_->wd_register.available(wd))
			{
				auto root_rt = root_runtime(wd->root);
				if (root_rt)
				{
					auto keys = root_rt->shortkeys.keys(reinterpret_cast<window>(wd));
					for (auto key : keys)
						result.emplace_back(wd, key);

					if (with_children)
					{
						for (auto child : wd->children)
						{
							auto child_keys = shortkeys(child, true);
							std::copy(child_keys.cbegin(), child_keys.cend(), std::back_inserter(result));
						}
					}
				}
			}

			return result;
		}

		window_manager::core_window_t* window_manager::find_shortkey(native_window_type native_window, unsigned long key)
		{
			if(native_window)
			{
				//Thread-Safe Required!
				std::lock_guard<decltype(mutex_)> lock(mutex_);
				auto object = root_runtime(native_window);
				if(object)
					return reinterpret_cast<core_window_t*>(object->shortkeys.find(key));
			}
			return nullptr;
		}

		void window_manager::set_safe_place(core_window_t* wd, std::function<void()>&& fn)
		{
			if (fn)
			{
				std::lock_guard<decltype(mutex_)> lock(mutex_);
				if (!available(wd))
					return;

				impl_->safe_place[wd].emplace_back(std::move(fn));
			}
		}

		void window_manager::call_safe_place(unsigned thread_id)
		{
			std::lock_guard<decltype(mutex_)> lock(mutex_);

			for (auto i = impl_->safe_place.begin(); i != impl_->safe_place.end();)
			{
				if (i->first->thread_id == thread_id)
				{
					for (auto & fn : i->second)
						fn();

					i = impl_->safe_place.erase(i);
				}
				else
					++i;
			}

		}

		bool check_tree(basic_window* wd, basic_window* const cond)
		{
			if (wd == cond)	return true;
			for (auto child : wd->children)
			{
				if (check_tree(child, cond))
					return true;
			}
			return false;
		}

		void window_manager::_m_disengage(core_window_t* wd, core_window_t* for_new)
		{
			auto * const wdpa = wd->parent;


			bool established = (for_new && (wdpa != for_new));
			decltype(for_new->root_widget->other.attribute.root) pa_root_attr = nullptr;
			
			if (established)
				pa_root_attr = for_new->root_widget->other.attribute.root;

			auto * root_attr = wd->root_widget->other.attribute.root;

			//Holds the shortkeys of wd and its children, and then
			//register these shortkeys for establishing.
			std::vector<std::pair<core_window_t*,unsigned long>> sk_holder;

			if ((!established) || (pa_root_attr != root_attr))
			{
				if (established)
				{
					if (check_tree(wd, attr_.capture.window))
						capture_window(attr_.capture.window, false);

					if (root_attr->focus && check_tree(wd, root_attr->focus))
						root_attr->focus = nullptr;

					if (root_attr->menubar && check_tree(wd, root_attr->menubar))
						root_attr->menubar = nullptr;

					sk_holder = shortkeys(wd, true);
				}
				else
				{
					if (wd == attr_.capture.window)
						capture_window(attr_.capture.window, false);

					if (root_attr->focus == wd)
						root_attr->focus = nullptr;

					if (root_attr->menubar == wd)
						root_attr->menubar = nullptr;
				}

				if (wd->other.category == category::root_tag::value)
				{
					root_runtime(wd->root)->shortkeys.clear();
					wd->other.attribute.root->focus = nullptr;
				}
				else
				{
					//Unregister all the children's shortkey, if it is disengaged for reset of parent.
					unregister_shortkey(wd, !established);
				}

				//test if wd is a TABSTOP window
				if (wd->flags.tab & detail::tab_type::tabstop)
				{
					auto & tabstop = root_attr->tabstop;
					//remove wd from root_attr, and then add it to pa_root_attr if established.
					auto wd_removed = utl::erase(tabstop, wd);
					if (established)
					{
						if (wd_removed)
							pa_root_attr->tabstop.push_back(wd);

						for (auto child : wd->children)
						{
							if(utl::erase(tabstop, child))
								pa_root_attr->tabstop.push_back(child);
						}
					}
				}
			}

			if (!established)
			{
				//remove the window from edge nimbus effect when it is destroying
				using edge_nimbus = detail::edge_nimbus_renderer<core_window_t>;
				edge_nimbus::instance().erase(wd);
			}
			else if (pa_root_attr != root_attr)
			{
				auto & cont = root_attr->effects_edge_nimbus;
				for (auto i = cont.begin(); i != cont.end();)
				{
					if ((i->window == wd) || wd->is_ancestor_of(i->window))
					{
						pa_root_attr->effects_edge_nimbus.push_back(*i);
						i = cont.erase(i);
						continue;
					}
					++i;
				}
			}

			if (wd->parent)
			{
				auto & pa_children = wd->parent->children;

				if (pa_children.size() > 1)
				{
					for (auto i = pa_children.cbegin(), end = pa_children.cend(); i != end; ++i)
					{
						if (((*i)->index) > (wd->index))
						{
							for (; i != end; ++i)
								--((*i)->index);
							break;
						}
					}
				}

				if (established)
				{
					utl::erase(pa_children, wd);
					if (for_new->children.empty())
						wd->index = 0;
					else
						wd->index = for_new->children.back()->index + 1;
					for_new->children.push_back(wd);
				}
			}

			if (wd->other.category == category::frame_tag::value)
			{
				//remove the frame handle from the WM frames manager.
				utl::erase(root_attr->frames, wd);

				if (established)
					pa_root_attr->frames.push_back(wd);
			}

			if (established)
			{
				wd->parent = for_new;
				wd->root = for_new->root;
				wd->root_graph = for_new->root_graph;
				wd->root_widget = for_new->root_widget;
				
				wd->pos_owner.x = wd->pos_owner.y = 0;

				auto delta_pos = wd->pos_root - for_new->pos_root;

				std::function<void(core_window_t*, const nana::point&)> set_pos_root;
				set_pos_root = [&set_pos_root](core_window_t* wd, const nana::point& delta_pos)
				{
					for (auto child : wd->children)
					{
						if (category::flags::root == child->other.category)
						{
							auto pos = native_interface::window_position(child->root);
							native_interface::parent_window(child->root, wd->root, false);

							pos -= delta_pos;
							native_interface::move_window(child->root, pos.x, pos.y);
						}
						else
						{
							child->root = wd->root;
							child->root_graph = wd->root_graph;
							child->root_widget = wd->root_widget;
							set_pos_root(child, delta_pos);
						}
					}

					wd->pos_root -= delta_pos;
				};

				set_pos_root(wd, delta_pos);

				for (auto & keys : sk_holder)
					register_shortkey(keys.first, keys.second);
			}
		}

		void window_manager::_m_destroy(core_window_t* wd)
		{
			if(wd->flags.destroying) return;

			bedrock & brock = bedrock::instance();
			brock.thread_context_destroy(wd);

			wd->flags.destroying = true;

			if(wd->together.caret)
			{
				//The deletion of caret wants to know whether the window is destroyed under SOME platform. Such as X11
				delete wd->together.caret;
				wd->together.caret = nullptr;
			}

			arg_destroy arg;
			arg.window_handle = reinterpret_cast<window>(wd);
			brock.emit(event_code::destroy, wd, arg, true, brock.get_thread_context());

			//Delete the children widgets.
			for (auto i = wd->children.rbegin(), end = wd->children.rend(); i != end;)
			{
				auto child = *i;

				if (category::flags::root == child->other.category)
				{
					//closing a child root window erases itself from wd->children,
					//to make sure the iterator is valid, it must be reloaded.

					auto offset = std::distance(wd->children.rbegin(), i);

					//!!!
					//a potential issue is that if the calling thread is not same with child's thread,
					//the child root window may not be erased from wd->children now.
					native_interface::close_window(child->root);

					i = wd->children.rbegin();
					std::advance(i, offset);
					end = wd->children.rend();
					continue;
				}
				_m_destroy(child);
				++i;
			}
			wd->children.clear();


			_m_disengage(wd, nullptr);
			window_layer::enable_effects_bground(wd, false);

			wd->drawer.detached();
			wd->widget_notifier->destroy();

			if(wd->other.category == category::frame_tag::value)
			{
				//The frame widget does not have an owner, and close their element windows without activating owner.
				//close the frame container window, it's a native window.
				for(auto i : wd->other.attribute.frame->attach)
					native_interface::close_window(i);

				native_interface::close_window(wd->other.attribute.frame->container);
			}

			if(wd->other.category != category::flags::root)	//Not a root window
				impl_->wd_register.remove(wd);
		}

		void window_manager::_m_move_core(core_window_t* wd, const point& delta)
		{
			if(category::flags::root != wd->other.category)	//A root widget always starts at (0, 0) and its childs are not to be changed
			{
				wd->pos_root += delta;
				if (category::flags::frame != wd->other.category)
				{
					if (wd->together.caret && wd->together.caret->visible())
						wd->together.caret->update();
				}
				else
					native_interface::move_window(wd->other.attribute.frame->container, wd->pos_root.x, wd->pos_root.y);

				if (wd->displayed() && wd->effect.bground)
					window_layer::make_bground(wd);

				for (auto child : wd->children)
					_m_move_core(child, delta);
			}
			else
			{
				auto pos = native_interface::window_position(wd->root) + delta;
				native_interface::move_window(wd->root, pos.x, pos.y);
			}
		}

		//_m_find
		//@brief: find a window on root window through a given root coordinate.
		//		the given root coordinate must be in the rectangle of wnd.
		window_manager::core_window_t* window_manager::_m_find(core_window_t* wd, const point& pos)
		{
			if(!wd->visible)
				return nullptr;

			for(auto i = wd->children.rbegin(); i != wd->children.rend(); ++i)
			{
				core_window_t* child = *i;
				if((child->other.category != category::root_tag::value) && _m_effective(child, pos))
				{
					child = _m_find(child, pos);
					if(child)
						return child;
				}
			}
			return wd;
		}

		//_m_effective, test if the window is a handle of window that specified by (root_x, root_y)
		bool window_manager::_m_effective(core_window_t* wd, const point& root_pos)
		{
			if(wd == nullptr || false == wd->visible)	return false;
			return rectangle{ wd->pos_root, wd->dimension }.is_hit(root_pos);
		}
	//end class window_manager
}//end namespace detail
}//end namespace nana
