/*
 *	Window Manager Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
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
#include <nana/gui/detail/window_manager.hpp>
#include <nana/gui/detail/window_layout.hpp>
#include <nana/gui/detail/native_window_interface.hpp>
#include <nana/gui/layout_utility.hpp>

#include "effects_renderer.hpp"
#include "window_register.hpp"
#include "inner_fwd_implement.hpp"

#include <stdexcept>
#include <algorithm>
#include <iterator>

#if defined(STD_THREAD_NOT_SUPPORTED)
#include <nana/std_mutex.hpp>
#else
#include <mutex>
#endif

namespace nana
{

	namespace detail
	{
		using window_layer = window_layout;

		//class shortkey_container
		struct shortkey_rep
		{
			window handle;
			std::vector<unsigned long> keys;
		};

		struct shortkey_container::implementation
		{
			std::vector<shortkey_rep> base;
		};

		shortkey_container::shortkey_container()
			:impl_(new implementation)
		{}

		shortkey_container::shortkey_container(shortkey_container&& other)
			: impl_(other.impl_)
		{
			other.impl_ = nullptr;
		}

		shortkey_container::~shortkey_container()
		{
			delete impl_;
		}

		void shortkey_container::clear()
		{
			impl_->base.clear();
		}

		bool shortkey_container::make(window wd, unsigned long key)
		{
			if (wd == nullptr) return false;
			if (key < 0x61) key += (0x61 - 0x41);

			for (auto & m : impl_->base)
			{
				if (m.handle == wd)
				{
					m.keys.emplace_back(key);
					return true;
				}
			}

#ifdef _nana_std_has_emplace_return_type
			auto & rep = impl_->base.emplace_back();
#else
			impl_->base.emplace_back();
			auto & rep = impl_->base.back();
#endif
			rep.handle = wd;
			rep.keys.emplace_back(key);

			return true;
		}

		void shortkey_container::umake(window wd)
		{
			if (wd == nullptr) return;

			for (auto i = impl_->base.begin(); i != impl_->base.end(); ++i)
			{
				if (i->handle == wd)
				{
					impl_->base.erase(i);
					break;
				}
			}
		}

		const std::vector<unsigned long>* shortkey_container::keys(window wd) const
		{
			if (wd)
			{
				for (auto & m : impl_->base)
				{
					if (m.handle == wd)
						return &m.keys;
				}
			}
			return nullptr;
		}

		window shortkey_container::find(unsigned long key) const
		{
			if (key < 0x61) key += (0x61 - 0x41);

			for (auto & m : impl_->base)
			{
				for (auto n : m.keys)
				{
					if (key == n)
						return m.handle;
				}
			}
			return nullptr;
		}
		//end class shortkey_container


		//struct root_misc
		root_misc::root_misc(root_misc&& other):
			window(other.window),
			wpassoc(other.wpassoc),
			root_graph(std::move(other.root_graph)),
			shortkeys(std::move(other.shortkeys)),
			condition(std::move(other.condition))
		{
			other.wpassoc = nullptr;	//moved-from
		}

		root_misc::root_misc(basic_window * wd, unsigned width, unsigned height)
			: window(wd),
			root_graph({ width, height })
		{
			condition.ignore_tab = false;
			condition.pressed = nullptr;
			condition.pressed_by_space = nullptr;
			condition.hovered = nullptr;
		}

		root_misc::~root_misc()
		{
			bedrock::delete_platform_assoc(wpassoc);
		}
		//end struct root_misc

		//class root_register
		struct root_register::implementation
		{
			//Cached
			native_window_type	recent_access{ nullptr };
			root_misc *			misc_ptr{ nullptr };

			std::map<native_window_type, root_misc> table;
		};

		root_register::root_register()
			: impl_(new implementation)
		{}

		root_register::~root_register()
		{
			delete impl_;
		}

		root_misc* root_register::insert(native_window_type wd, root_misc&& misc)
		{
			impl_->recent_access = wd;
			auto ret = impl_->table.emplace(wd, std::move(misc));
			impl_->misc_ptr = &(ret.first->second);
			return impl_->misc_ptr;
		}

		root_misc * root_register::find(native_window_type wd)
		{
			if (wd == impl_->recent_access)
				return impl_->misc_ptr;

			impl_->recent_access = wd;

			auto i = impl_->table.find(wd);
			if (i != impl_->table.end())
				impl_->misc_ptr = &(i->second);
			else
				impl_->misc_ptr = nullptr;

			return impl_->misc_ptr;
		}

		void root_register::erase(native_window_type wd)
		{
			impl_->table.erase(wd);
			impl_->recent_access = wd;
			impl_->misc_ptr = nullptr;
		}
		//end class root_register
	}

namespace detail
{
	template<typename Key, typename Value>
	class lite_map
	{
		struct key_value_rep
		{
			Key first;
			Value second;

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

#ifdef _nana_std_has_emplace_return_type
			return table_.emplace_back(key).second;
#else
			table_.emplace_back(key);
			return table_.back().second;
#endif
		}

		iterator find(const Key& key)
		{
			for (auto i = table_.begin(); i != table_.end(); ++i)
				if (i->first == key)
					return i;

			return table_.end();
		}

		std::vector<key_value_rep>& table()
		{
			return table_;
		}
	private:
		std::vector<key_value_rep> table_;
	};

	//class window_manager
			//struct wdm_private_impl
			struct window_manager::wdm_private_impl
			{
				root_register	misc_register;
				window_register wd_register;

				paint::image default_icon_big;
				paint::image default_icon_small;

				lite_map<basic_window*, std::vector<std::function<void()>>> safe_place;
			};
		//end struct wdm_private_impl

			//class revertible_mutex
			struct thread_refcount
			{
				thread_t tid;	//Thread ID
				std::vector<unsigned> callstack_refs;

				thread_refcount(thread_t thread_id, unsigned refs)
					: tid(thread_id)
				{
					callstack_refs.push_back(refs);
				}
			};

			struct window_manager::revertible_mutex::implementation
			{
				std::recursive_mutex mutex;

				thread_t thread_id;	//Thread ID
				unsigned refs;	//Ref count

				std::vector<thread_refcount> records;
			};

			window_manager::revertible_mutex::revertible_mutex()
				: impl_(new implementation)
			{
				impl_->thread_id = 0;
				impl_->refs = 0;
			}

			window_manager::revertible_mutex::~revertible_mutex()
			{
				delete impl_;
			}

			void window_manager::revertible_mutex::lock()
			{
				impl_->mutex.lock();

				if (0 == impl_->thread_id)
					impl_->thread_id = nana::system::this_thread_id();

				++(impl_->refs);
			}

			bool window_manager::revertible_mutex::try_lock()
			{
				if (impl_->mutex.try_lock())
				{
					if (0 == impl_->thread_id)
						impl_->thread_id = nana::system::this_thread_id();

					++(impl_->refs);
					return true;
				}
				return false;
			}

			void window_manager::revertible_mutex::unlock()
			{
				if (impl_->thread_id == nana::system::this_thread_id())
					if (0 == --(impl_->refs))
						impl_->thread_id = 0;

				impl_->mutex.unlock();
			}

			void window_manager::revertible_mutex::revert()
			{
				if (impl_->thread_id == nana::system::this_thread_id())
				{
					auto const current_refs = impl_->refs;

					//Check if there is a record
					for (auto & r : impl_->records)
					{
						if (r.tid == impl_->thread_id)
						{
							r.callstack_refs.push_back(current_refs);
							impl_->thread_id = 0;	//Indicates a record is existing
							break;
						}
					}

					if (impl_->thread_id)
					{
						//Creates a new record
						impl_->records.emplace_back(impl_->thread_id, current_refs);
						impl_->thread_id = 0;
					}

					impl_->refs = 0;

					for (std::size_t i = 0; i < current_refs; ++i)
						impl_->mutex.unlock();
				}
				else
					throw std::runtime_error("The revert is not allowed");
			}

			void window_manager::revertible_mutex::forward()
			{
				impl_->mutex.lock();

				if (impl_->records.size())
				{
					auto const this_tid = nana::system::this_thread_id();

					for (auto i = impl_->records.begin(); i != impl_->records.end(); ++i)
					{
						if (this_tid != i->tid)
							continue;

						auto const refs = i->callstack_refs.back();

						for (std::size_t u = 1; u < refs; ++u)
							impl_->mutex.lock();

						impl_->thread_id = this_tid;
						impl_->refs = refs;

						if (i->callstack_refs.size() > 1)
							i->callstack_refs.pop_back();
						else
							impl_->records.erase(i);
						return;
					}

					throw std::runtime_error("The forward is not matched. Please report this issue");
				}

				impl_->mutex.unlock();
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

		std::size_t window_manager::window_count() const
		{
			//Thread-Safe Required!
			std::lock_guard<mutex_type> lock(mutex_);

			return impl_->wd_register.size();
		}

		window_manager::mutex_type& window_manager::internal_lock() const
		{
			return mutex_;
		}

		void window_manager::all_handles(std::vector<basic_window*> &v) const
		{
			//Thread-Safe Required!
			std::lock_guard<mutex_type> lock(mutex_);
			v = impl_->wd_register.queue();
		}

		void window_manager::event_filter(basic_window* wd, bool is_make, event_code evtid)
		{
			switch(evtid)
			{
			case event_code::mouse_drop:
				wd->flags.dropable = (is_make || (0 != wd->annex.events_ptr->mouse_dropfiles.length()));
				break;
			default:
				break;
			}
		}

		bool window_manager::available(basic_window* wd)
		{
			//Thread-Safe Required!
			std::lock_guard<mutex_type> lock(mutex_);
			return impl_->wd_register.available(wd);
		}

		bool window_manager::available(basic_window * a, basic_window* b)
		{
			//Thread-Safe Required!
			std::lock_guard<mutex_type> lock(mutex_);
			return (impl_->wd_register.available(a) && impl_->wd_register.available(b));
		}

		basic_window* window_manager::create_root(basic_window* owner, bool nested, rectangle r, const appearance& app, widget* wdg)
		{
			native_window_type native = nullptr;
			if (owner)
			{
				//Thread-Safe Required!
				std::lock_guard<mutex_type> lock(mutex_);

				if (impl_->wd_register.available(owner))
				{
					if (owner->flags.destroying)
						throw std::runtime_error("the specified owner is destroyed");

					native = owner->root_widget->root;
					r.x += owner->pos_root.x;
					r.y += owner->pos_root.y;
				}
				else
					owner = nullptr;
			}

			auto result = native_interface::create_window(native, nested, r, app);
			if (result.native_handle)
			{
				auto wd = new basic_window(owner, widget_notifier_interface::get_notifier(wdg), (category::root_tag**)nullptr);
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
				std::lock_guard<mutex_type> lock(mutex_);

				//create Root graphics Buffer and manage it
				auto* value = impl_->misc_register.insert(result.native_handle, root_misc(wd, result.width, result.height));

				wd->bind_native_window(result.native_handle, result.width, result.height, result.extra_width, result.extra_height, value->root_graph);
				impl_->wd_register.insert(wd);

				bedrock::inc_window(wd->thread_id);
				this->icon(wd, impl_->default_icon_small, impl_->default_icon_big);
				return wd;
			}
			return nullptr;
		}

		basic_window* window_manager::create_widget(basic_window* parent, const rectangle& r, bool is_lite, widget* wdg)
		{
			//Thread-Safe Required!
			std::lock_guard<mutex_type> lock(mutex_);
			if (impl_->wd_register.available(parent) == false)
				throw std::invalid_argument("invalid parent/owner handle");

			if (parent->flags.destroying)
				throw std::logic_error("the specified parent is destory");

			auto wdg_notifier = widget_notifier_interface::get_notifier(wdg);

			basic_window * wd;
			if (is_lite)
				wd = new basic_window(parent, std::move(wdg_notifier), r, (category::lite_widget_tag**)nullptr);
			else
				wd = new basic_window(parent, std::move(wdg_notifier), r, (category::widget_tag**)nullptr);

			impl_->wd_register.insert(wd);
			return wd;
		}

		void window_manager::close(basic_window *wd)
		{
			//Thread-Safe Required!
			std::lock_guard<mutex_type> lock(mutex_);
			if (impl_->wd_register.available(wd) == false)	return;

			if (wd->flags.destroying)
				return;

			if(category::flags::root == wd->other.category)
			{
				auto &brock = bedrock::instance();
				arg_unload arg;
				arg.window_handle = wd;
				arg.cancel = false;
				brock.emit(event_code::unload, wd, arg, true, brock.get_thread_context());
				if (false == arg.cancel)
				{
					//Before close the window, its owner window should be activated, otherwise other window will be
					//activated due to the owner window is not enabled.
					if(wd->flags.modal || (wd->owner == nullptr) || wd->owner->flags.take_active)
						native_interface::activate_owner(wd->root);

					if (!wd->flags.destroying)
					{
						//Close should detach the drawer and send destroy signal to widget object.
						//Otherwise, when a widget object is been deleting in other thread by delete operator, the object will be destroyed
						//before the window_manager destroys the window, and then, window_manager detaches the
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
		void window_manager::destroy(basic_window* wd)
		{
			//Thread-Safe Required!
			std::lock_guard<mutex_type> lock(mutex_);
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

		void window_manager::destroy_handle(basic_window* wd)
		{
			//Thread-Safe Required!
			std::lock_guard<mutex_type> lock(mutex_);
			if (impl_->wd_register.available(wd) == false)	return;

			if (category::flags::root == wd->other.category)
			{
				impl_->misc_register.erase(wd->root);
				impl_->wd_register.remove(wd);
			}
		}

		void window_manager::icon(basic_window* wd, const paint::image& small_icon, const paint::image& big_icon)
		{
			if(!big_icon.empty() || !small_icon.empty())
			{
				if (nullptr == wd)
				{
					impl_->default_icon_big = big_icon;
					impl_->default_icon_small = small_icon;
				}
				else
				{
					std::lock_guard<mutex_type> lock(mutex_);
					if (impl_->wd_register.available(wd))
					{
						if (category::flags::root == wd->other.category)
							native_interface::window_icon(wd->root, small_icon, big_icon);
					}
				}
			}
		}

		//show
		//@brief: show or hide a window
		bool window_manager::show(basic_window* wd, bool visible)
		{
			//Thread-Safe Required!
			std::lock_guard<mutex_type> lock(mutex_);
			if (!impl_->wd_register.available(wd))
				return false;

			if(visible != wd->visible)
			{
				auto nv = (category::flags::root == wd->other.category ? wd->root : nullptr);

				if(visible && wd->effect.bground)
					window_layer::make_bground(wd);

				//Don't set the visible attr of a window if it is a root.
				//The visible attr of a root will be set in the expose event.
				if(category::flags::root != wd->other.category)
					bedrock::instance().event_expose(wd, visible);

				if (nv)
					native_interface::show_window(nv, visible, wd->flags.take_active);
			}
			return true;
		}

		basic_window* window_manager::find_window(native_window_type root, const point& pos, bool ignore_captured)
		{
			if (nullptr == root)
				return nullptr;

			//Thread-Safe Required!
			std::lock_guard<mutex_type> lock(mutex_);

			if (ignore_captured || (nullptr == attr_.capture.window))
			{
				auto rrt = root_runtime(root);
				if (rrt && _m_effective(rrt->window, pos))
					return _m_find(rrt->window, pos);

				return nullptr;
			}
		
			if (attr_.capture.ignore_children)
				return attr_.capture.window;

			auto rrt = root_runtime(root);
			if (rrt && _m_effective(rrt->window, pos))
			{
				auto target = _m_find(rrt->window, pos);

				auto p = target;
				while (p)
				{
					if (p == attr_.capture.window)
						return target;
					p = p->parent;
				}
			}

			return attr_.capture.window;
		}

		//move the wnd and its all children window, x and y is a relatively coordinate for wnd's parent window
		bool window_manager::move(basic_window* wd, int x, int y, bool passive)
		{
			//Thread-Safe Required!
			std::lock_guard<mutex_type> lock(mutex_);
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
						arg.window_handle = wd;
						arg.x = x;
						arg.y = y;

						if (wd->effect.bground)
							wd->other.upd_state = basic_window::update_state::request_refresh;

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

		bool window_manager::move(basic_window* wd, const rectangle& r)
		{
			//Thread-Safe Required!
			std::lock_guard<mutex_type> lock(mutex_);
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
					auto delta = r.position() - wd->pos_owner;
					wd->pos_owner = r.position();
					_m_move_core(wd, delta);
					moved = true;

					if ((!size_changed) && wd->effect.bground)
						wd->other.upd_state = basic_window::update_state::request_refresh;

					arg_move arg;
					arg.window_handle = wd;
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
					arg.window_handle = wd;
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
		bool window_manager::size(basic_window* wd, nana::size sz, bool passive, bool ask_update)
		{
			//Thread-Safe Required!
			std::lock_guard<mutex_type> lock(mutex_);
			if (!impl_->wd_register.available(wd))
				return false;

			auto & brock = bedrock::instance();
			if (sz != wd->dimension)
			{
				arg_resizing arg;
				arg.window_handle = wd;
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

			std::vector<basic_window*> presence;

			if (wd->dimension.width < sz.width || wd->dimension.height < sz.height)
			{
				auto wd_r = rectangle{ wd->dimension };
				for (auto child : wd->children)
				{
					auto child_r = rectangle{ child->pos_owner, child->dimension };
					if (!overlapped(wd_r, child_r))
						presence.push_back(child);
				}
			}

			//Before resizing the window, creates the new graphics
			paint::graphics graph;
			paint::graphics root_graph;
			if (category::flags::lite_widget != wd->other.category)
			{
				//If allocation fails, here throws std::bad_alloc.
				graph.make(sz);
				graph.typeface(wd->drawer.graphics.typeface());
				if (category::flags::root == wd->other.category)
					root_graph.make(sz);
			}

			auto pre_sz = wd->dimension;

			wd->dimension = sz;

			if(category::flags::lite_widget != wd->other.category)
			{
				bool graph_state = wd->drawer.graphics.empty();
				wd->drawer.graphics.swap(graph);

				//It shall make a typeface_changed() call when the graphics state is changing.
				//Because when a widget is created with zero-size, it may get some wrong results in typeface_changed() call
				//due to the invaliable graphics object.
				if(graph_state != wd->drawer.graphics.empty())
					wd->drawer.typeface_changed();

				if(category::flags::root == wd->other.category)
				{
					//wd->root_graph->make(sz);
					wd->root_graph->swap(root_graph);
					if(false == passive)
						if (!native_interface::window_size(wd->root, sz + nana::size(wd->extra_width, wd->extra_height)))
						{
							wd->dimension = pre_sz;
							wd->drawer.graphics.swap(graph);
							wd->root_graph->swap(root_graph);
							wd->drawer.typeface_changed();
							return false;
						}
				}
				else if(wd->effect.bground && wd->parent)
				{
					//update the bground buffer of glass window.
					wd->other.glass_buffer.make(sz);
					window_layer::make_bground(wd);
				}
			}

			for (auto child : presence)
			{
				refresh_tree(child);
			}

			arg_resized arg;
			arg.window_handle = wd;
			arg.width = sz.width;
			arg.height = sz.height;
			brock.emit(event_code::resized, wd, arg, ask_update, brock.get_thread_context());
			return true;
		}

		basic_window* window_manager::root(native_window_type wd) const
		{
			static std::pair<native_window_type, basic_window*> cache;
			if(cache.first == wd) return cache.second;

			//Thread-Safe Required!
			std::lock_guard<mutex_type> lock(mutex_);

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
		void window_manager::map(basic_window* wd, bool forced, const rectangle* update_area)
		{
			//Thread-Safe Required!
			std::lock_guard<mutex_type> lock(mutex_);
			if (impl_->wd_register.available(wd) && !wd->is_draw_through())
				bedrock::instance().flush_surface(wd, forced, update_area);
		}

		//update
		//@brief:	update is used for displaying the screen-off buffer.
		//			Because of a good efficiency, if it is called in an event procedure and the event procedure window is the
		//			same as update's, update would not map the screen-off buffer and just set the window for lazy refresh
		bool window_manager::update(basic_window* wd, bool redraw, bool forced, const rectangle* update_area)
		{
			//Thread-Safe Required!
			std::lock_guard<mutex_type> lock(mutex_);
			if (impl_->wd_register.available(wd) == false) return false;

			if ((wd->other.category == category::flags::root) && wd->is_draw_through())
			{
				native_interface::refresh_window(wd->root);
				return true;
			}

			if (wd->displayed())
			{
				using paint_operation = window_layer::paint_operation;

				if(forced || (false == wd->belong_to_lazy()))
				{
					if (!wd->flags.refreshing)
					{
						if (!wd->try_lazy_update(redraw))
						{
							window_layer::paint(wd, (redraw ? paint_operation::try_refresh : paint_operation::none), false);
							this->map(wd, forced, update_area);
						}
						return true;
					}
					else if (forced)
					{
						window_layer::paint(wd, paint_operation::none, false);
						this->map(wd, true, update_area);
						return true;
					}
				}
				else if (redraw)
					window_layer::paint(wd, paint_operation::try_refresh, false);

				if (wd->other.upd_state == basic_window::update_state::lazy)
					wd->other.upd_state = basic_window::update_state::refreshed;
			}
			return true;
		}

		void window_manager::update_requesters(basic_window* root_wd)
		{
			//Thread-Safe Required!
			std::lock_guard<mutex_type> lock(mutex_);

			if (this->available(root_wd) && root_wd->other.attribute.root->update_requesters.size())
			{
				for (auto wd : root_wd->other.attribute.root->update_requesters)
				{
					using paint_operation = window_layer::paint_operation;
					if (!this->available(wd))
						continue;

					//#431
					//Redraws the widget when it has beground effect.
					//Because the widget just redraw if it didn't have bground effect when it was inserted to the update_requesters queue
					window_layer::paint(wd, (wd->effect.bground ? paint_operation::try_refresh : paint_operation::have_refreshed), false);
					this->map(wd, true);
				}
			}

		}

		void window_manager::refresh_tree(basic_window* wd)
		{
			//Thread-Safe Required!
			std::lock_guard<mutex_type> lock(mutex_);

			//It's not worthy to redraw if visible is false
			if (impl_->wd_register.available(wd) && wd->displayed())
				window_layer::paint(wd, window_layer::paint_operation::try_refresh, true);
		}

		//do_lazy_refresh
		//@brief: defined a behavior of flush the screen
		void window_manager::do_lazy_refresh(basic_window* wd, bool force_copy_to_screen, bool refresh_tree)
		{
			//Thread-Safe Required!
			std::lock_guard<mutex_type> lock(mutex_);

			if (false == impl_->wd_register.available(wd))
				return;

			//It's not worthy to redraw if visible is false
			if(wd->visible && (!wd->is_draw_through()))
			{
				using paint_operation = window_layer::paint_operation;
				if (wd->visible_parents())
				{
					if ((wd->other.upd_state == basic_window::update_state::refreshed) || (wd->other.upd_state == basic_window::update_state::request_refresh) || force_copy_to_screen)
					{
						if (!wd->try_lazy_update(wd->other.upd_state == basic_window::update_state::request_refresh))
						{
							window_layer::paint(wd, (wd->other.upd_state == basic_window::update_state::request_refresh ? paint_operation::try_refresh : paint_operation::have_refreshed), refresh_tree);
							this->map(wd, force_copy_to_screen);
						}
					}
					else if (effects::edge_nimbus::none != wd->effect.edge_nimbus)
					{
						//The window is still mapped because of edge nimbus effect.
						//Avoid duplicate copy if action state is not changed and the window is not focused.
						if (wd->flags.action != wd->flags.action_before)
						{
							if (!wd->try_lazy_update(false))
								this->map(wd, true);
						}
					}
				}
				else
					window_layer::paint(wd, paint_operation::try_refresh, refresh_tree);	//only refreshing if it has an invisible parent
			}

			wd->other.upd_state = basic_window::update_state::none;
		}

		bool window_manager::set_parent(basic_window* wd, basic_window* newpa)
		{
			//Thread-Safe Required!
			std::lock_guard<mutex_type> lock(mutex_);
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
		basic_window* window_manager::set_focus(basic_window* wd, bool root_has_been_focused, arg_focus::reason reason)
		{
			//Thread-Safe Required!
			std::lock_guard<mutex_type> lock(mutex_);

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
					if(prev_focus->annex.caret_ptr)
						prev_focus->annex.caret_ptr->activate(false);

					arg.getting = false;
					arg.window_handle = prev_focus;
					arg.receiver = wd->root;
					arg.focus_reason = arg_focus::reason::general;
					brock.emit(event_code::focus, prev_focus, arg, true, brock.get_thread_context());
				}

				//Check the prev_focus again, because it may be closed in focus event
				if (!impl_->wd_register.available(prev_focus))
					prev_focus = nullptr;
			}
			else if(wd->root == native_interface::get_focus_window())
				return prev_focus; //no new focus_window


			if(wd->annex.caret_ptr)
				wd->annex.caret_ptr->activate(true);

			arg.window_handle = wd;
			arg.getting = true;
			arg.receiver = wd->root;
			arg.focus_reason = reason;
			brock.emit(event_code::focus, wd, arg, true, brock.get_thread_context());

			if (!root_has_been_focused)
				native_interface::set_focus(root_wd->root);

			//A fix by Katsuhisa Yuasa
			//The menubar token window will be redirected to the prev focus window when the new
			//focus window is a menubar.
			//The focus window will be restored to the prev focus which losts the focus because of
			//memberbar.
			if (prev_focus && (wd == wd->root_widget->other.attribute.root->menubar))
				wd = prev_focus;

			if (wd != wd->root_widget->other.attribute.root->menubar)
				brock.set_menubar_taken(wd);

			return prev_focus;
		}

		basic_window* window_manager::capture_redirect(basic_window* wd)
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

		basic_window * window_manager::capture_window() const
		{
			return attr_.capture.window;
		}

		void window_manager::capture_window(basic_window* wd, bool captured, bool ignore_children)
		{
			if (!this->available(wd))
				return;

			nana::point pos = native_interface::cursor_position();
			auto & attr_cap = attr_.capture.history;

			if (captured)
			{
				if(wd != attr_.capture.window)
				{
					//Thread-Safe Required!
					std::lock_guard<mutex_type> lock(mutex_);

					if (impl_->wd_register.available(wd))
					{
						wd->flags.captured = true;
						native_interface::capture_window(wd->root, captured);

						if (attr_.capture.window)
							attr_cap.emplace_back(attr_.capture.window, attr_.capture.ignore_children);

						attr_.capture.window = wd;
						attr_.capture.ignore_children = ignore_children;
						native_interface::calc_window_point(wd->root, pos);
						attr_.capture.inside = _m_effective(wd, pos);
					}
				}
			}
			else if(wd == attr_.capture.window)
			{
				attr_.capture.window = nullptr;
				wd->flags.captured = false;
				if(attr_cap.size())
				{
					std::pair<basic_window*, bool> last = attr_cap.back();
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
			}
		}

		//enable_tabstop
		//@brief: when users press a TAB, the focus should move to the next widget.
		//	this method insert a window which catches an user TAB into a TAB window container
		//	the TAB window container is held by a wd's root widget. Not every widget has a TAB window container,
		//	the container is created while a first Tab Window is setting
		void window_manager::enable_tabstop(basic_window* wd)
		{
			//Thread-Safe Required!
			std::lock_guard<mutex_type> lock(mutex_);
			if (impl_->wd_register.available(wd) && (detail::tab_type::none == wd->flags.tab))
			{
				wd->root_widget->other.attribute.root->tabstop.push_back(wd);
				wd->flags.tab |= detail::tab_type::tabstop;
			}
		}

		// preconditions of get_tabstop: tabstop is not empty and at least one window is visible
		basic_window* get_tabstop(basic_window* wd, bool forward)
		{
			auto & tabs = wd->root_widget->other.attribute.root->tabstop;

			auto end = tabs.end();
			if (forward)
			{
				if (detail::tab_type::none == wd->flags.tab)
					return (tabs.front());
				else if (detail::tab_type::tabstop & wd->flags.tab)
				{
					auto i = std::find(tabs.begin(), end, wd);
					if (i != end)
					{
						++i;
						basic_window* ts = (i != end ? (*i) : tabs.front());
						return (ts != wd ? ts : 0);
					}
					else
						return tabs.front();
				}
			}
			else if (tabs.size() > 1)	//at least 2 elments in tabs are required when moving backward.
			{
				auto i = std::find(tabs.begin(), end, wd);
				if (i != end)
					return (tabs.begin() == i ? tabs.back() : *(i - 1));
			}
			return nullptr;
		}

		auto window_manager::tabstop(basic_window* wd, bool forward) const -> basic_window*
		{
			//Thread-Safe Required!
			std::lock_guard<mutex_type> lock(mutex_);
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

		void window_manager::remove_trash_handle(thread_t tid)
		{
			//Thread-Safe Required!
			std::lock_guard<mutex_type> lock(mutex_);
			impl_->wd_register.delete_trash(tid);
		}

		bool window_manager::enable_effects_bground(basic_window* wd, bool enabled)
		{
			//Thread-Safe Required!
			std::lock_guard<mutex_type> lock(mutex_);
			if (impl_->wd_register.available(wd))
				return window_layer::enable_effects_bground(wd, enabled);

			return false;
		}

		bool window_manager::calc_window_point(basic_window* wd, nana::point& pos)
		{
			//Thread-Safe Required!
			std::lock_guard<mutex_type> lock(mutex_);
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

		root_misc* window_manager::root_runtime(native_window native_wd) const
		{
			return impl_->misc_register.find(native_wd);
		}

		bool window_manager::register_shortkey(basic_window* wd, unsigned long key)
		{
			//Thread-Safe Required!
			std::lock_guard<mutex_type> lock(mutex_);
			if (impl_->wd_register.available(wd))
			{
				//the root runtime must exist, because the wd is valid. Otherse, it's bug of library
				return root_runtime(wd->root)->shortkeys.make(wd, key);
			}
			return false;
		}

		void window_manager::unregister_shortkey(basic_window* wd, bool with_children)
		{
			//Thread-Safe Required!
			std::lock_guard<mutex_type> lock(mutex_);
			if (impl_->wd_register.available(wd) == false) return;

			auto root_rt = root_runtime(wd->root);
			if (root_rt)
			{
				root_rt->shortkeys.umake(wd);
				if (with_children)
				{
					for (auto child : wd->children)
						unregister_shortkey(child, true);
				}
			}
		}

		basic_window* window_manager::find_shortkey(native_window_type native_window, unsigned long key)
		{
			if(native_window)
			{
				//Thread-Safe Required!
				std::lock_guard<mutex_type> lock(mutex_);
				auto object = root_runtime(native_window);
				if(object)
					return object->shortkeys.find(key);
			}
			return nullptr;
		}

		void window_manager::set_safe_place(basic_window* wd, std::function<void()>&& fn)
		{
			if (fn)
			{
				std::lock_guard<mutex_type> lock(mutex_);
				if (!available(wd))
					return;

				impl_->safe_place[wd].emplace_back(std::move(fn));
			}
		}

		void window_manager::call_safe_place(thread_t thread_id)
		{
			std::lock_guard<mutex_type> lock(mutex_);

			auto& safe_place = impl_->safe_place.table();
			for (auto i = safe_place.begin(); i != safe_place.end();)
			{
				if (i->first->thread_id == thread_id)
				{
					for (auto & fn : i->second)
						fn();

					i = safe_place.erase(i);
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

		void window_manager::_m_disengage(basic_window* wd, basic_window* for_new)
		{
			auto * const wdpa = wd->parent;

			bool established = (for_new && (wdpa != for_new));
			decltype(for_new->root_widget->other.attribute.root) pa_root_attr = nullptr;

			if (established)
				pa_root_attr = for_new->root_widget->other.attribute.root;

			auto * root_attr = wd->root_widget->other.attribute.root;

			//Holds the shortkeys of wd and its children, and then
			//register these shortkeys for establishing.
			std::vector<std::pair<basic_window*,unsigned long>> sk_holder;

			if ((!established) || (pa_root_attr != root_attr))
			{
				if (established)
				{
					if (check_tree(wd, attr_.capture.window))
						capture_window(attr_.capture.window, false, false);	//The 3rd parameter is ignored

					if (root_attr->focus && check_tree(wd, root_attr->focus))
						root_attr->focus = nullptr;

					if (root_attr->menubar && check_tree(wd, root_attr->menubar))
						root_attr->menubar = nullptr;

					_m_shortkeys(wd, true, sk_holder);
				}
				else
				{
					if (wd == attr_.capture.window)
						capture_window(attr_.capture.window, false, false);	//The 3rd parameter is ignored.

					if (root_attr->focus == wd)
						root_attr->focus = nullptr;

					if (root_attr->menubar == wd)
						root_attr->menubar = nullptr;
				}

				if (wd->other.category == category::flags::root)
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
				edge_nimbus_renderer::instance().erase(wd);
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
					for (auto i = pa_children.begin(), end = pa_children.end(); i != end; ++i)
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
					utl::erase(wd->parent->children, wd);
					if (for_new->children.empty())
						wd->index = 0;
					else
						wd->index = for_new->children.back()->index + 1;
					for_new->children.push_back(wd);
				}
			}

			if (established)
			{
				wd->parent = for_new;
				wd->root = for_new->root;
				wd->root_graph = for_new->root_graph;
				wd->root_widget = for_new->root_widget;

				wd->pos_owner.x = wd->pos_owner.y = 0;

				auto delta_pos = wd->pos_root - for_new->pos_root;

				std::function<void(basic_window*, const nana::point&)> set_pos_root;
				set_pos_root = [&set_pos_root](basic_window* wd, const nana::point& delta_pos)
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

		void window_manager::_m_destroy(basic_window* wd)
		{
			if(wd->flags.destroying) return;

			bedrock & brock = bedrock::instance();
			brock.thread_context_destroy(wd);

			wd->flags.destroying = true;

			if(wd->annex.caret_ptr)
			{
				//The deletion of caret wants to know whether the window is destroyed under SOME platform. Such as X11
				delete wd->annex.caret_ptr;
				wd->annex.caret_ptr = nullptr;
			}

			//remove the window from edge nimbus effect when it is destroying
			edge_nimbus_renderer::instance().erase(wd);

			arg_destroy arg;
			arg.window_handle = wd;
			brock.emit(event_code::destroy, wd, arg, true, brock.get_thread_context());

			//Delete the children widgets.
			while (!wd->children.empty())
			{
				auto child = wd->children.back();
				if (category::flags::root == child->other.category)
				{
					//Only the nested_form meets the condition
					native_interface::close_window(child->root);
					continue;
				}
				_m_destroy(child);
				wd->children.pop_back();
			}

			_m_disengage(wd, nullptr);
			window_layer::enable_effects_bground(wd, false);

			wd->drawer.detached();
			wd->widget_notifier->destroy();

			if(wd->other.category != category::flags::root)	//Not a root window
				impl_->wd_register.remove(wd);

			//Release graphics immediately.
			wd->drawer.graphics.release();
		}

		void window_manager::_m_move_core(basic_window* wd, const point& delta)
		{
			if(category::flags::root != wd->other.category)	//A root widget always starts at (0, 0) and its children are not to be changed
			{
				wd->pos_root += delta;

				if (wd->annex.caret_ptr && wd->annex.caret_ptr->visible())
					wd->annex.caret_ptr->update();

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

		void window_manager::_m_shortkeys(basic_window* wd, bool with_children, std::vector<std::pair<basic_window*, unsigned long>>& keys) const
		{
			if (impl_->wd_register.available(wd))
			{
				//The root_rt must exist, because wd is valid. Otherwise, it's a bug of the library.
				auto root_rt = root_runtime(wd->root);

				auto pkeys = root_rt->shortkeys.keys(wd);
				if (pkeys)
				{
					for (auto key : *pkeys)
						keys.emplace_back(wd, key);
				}

				if (with_children)
				{
					for (auto child : wd->children)
						_m_shortkeys(child, true, keys);
				}
			}
		}

		//_m_find
		//@brief: find a window on root window through a given root coordinate.
		//		the given root coordinate must be in the rectangle of wnd.
		basic_window* window_manager::_m_find(basic_window* wd, const point& pos)
		{
			if(!wd->visible)
				return nullptr;

			if (!wd->children.empty())
			{
				auto index = wd->children.size();

				do
				{
					auto child = wd->children[--index];
					if ((child->other.category != category::flags::root) && _m_effective(child, pos))
					{
						child = _m_find(child, pos);
						if (child)
							return child;
					}
				} while (0 != index);
			}
			return wd;
		}

		//_m_effective, test if the window is a handle of window that specified by (root_x, root_y)
		bool window_manager::_m_effective(basic_window* wd, const point& root_pos)
		{
			if(wd == nullptr || false == wd->visible)	return false;
			return rectangle{ wd->pos_root, wd->dimension }.is_hit(root_pos);
		}
	//end class window_manager
}//end namespace detail
}//end namespace nana
