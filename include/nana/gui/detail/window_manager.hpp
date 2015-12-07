/*
 *	Window Manager Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2014 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/detail/window_manager.hpp
 *
 *	<Knowledge: 1, 2007-8-17, "Difference between destroy and destroy_handle">
 *		destroy method destroys a window handle and the handles of its children, but it doesn't delete the handle which type is a root window or a frame
 *		destroy_handle method just destroys the handle which type is a root window or a frame
 *
 */

#ifndef NANA_GUI_DETAIL_WINDOW_MANAGER_HPP
#define NANA_GUI_DETAIL_WINDOW_MANAGER_HPP

#include <vector>
#include "window_layout.hpp"
#include "event_code.hpp"
#include "inner_fwd.hpp"
#include <functional>

#if defined(STD_THREAD_NOT_SUPPORTED)
	#include <nana/std_mutex.hpp>
#else
	#include <mutex>
#endif

namespace nana
{
	class widget;	//forward declaration
	namespace paint
	{
		class image;
	}
}

namespace nana{
namespace detail
{
	class widget_notifier_interface;	//forward declaration

	struct root_misc;

	class window_manager
	{
		class revertible_mutex
			: public std::recursive_mutex
		{
			struct thr_refcnt
			{
				unsigned tid;
				std::size_t refcnt;
			};
		public:
			revertible_mutex();

			void lock();
			bool try_lock();
			void unlock();

			void revert();
			void forward();
		private:
			thr_refcnt thr_;
			std::vector<thr_refcnt> stack_;
		};
	public:
		using native_window = native_window_type;
		using mutex_type = revertible_mutex;

		using core_window_t = basic_window;
		using window_layer = window_layout;

		window_manager();
		~window_manager();

		static bool is_queue(core_window_t*);
		std::size_t number_of_core_window() const;
		mutex_type & internal_lock() const;
		void all_handles(std::vector<core_window_t*>&) const;

		void event_filter(core_window_t*, bool is_make, event_code);

		bool available(core_window_t*);
		bool available(core_window_t *, core_window_t*);
		bool available(native_window_type);

		core_window_t* create_root(core_window_t*, bool nested, rectangle, const appearance&, widget*);
		core_window_t* create_widget(core_window_t*, const rectangle&, bool is_lite, widget*);
		core_window_t* create_frame(core_window_t*, const rectangle&, widget*);

		bool insert_frame(core_window_t* frame, native_window);
		bool insert_frame(core_window_t* frame, core_window_t*);
		void close(core_window_t*);

		//destroy
		//@brief:	Delete the window handle
		void destroy(core_window_t*);

		//destroy_handle
		//@brief:	Delete window handle, the handle type must be a root and a frame.
		void destroy_handle(core_window_t*);

		void default_icon(const paint::image& _small_icon, const paint::image& big_icon);
		void icon(core_window_t*, const paint::image& small_icon, const paint::image& big_icon);

		//show
		//@brief: show or hide a window
		bool show(core_window_t* wd, bool visible);

		core_window_t* find_window(native_window_type root, int x, int y);

		//move the wnd and its all children window, x and y is a relatively coordinate for wnd's parent window
		bool move(core_window_t*, int x, int y, bool passive);
		bool move(core_window_t*, const rectangle&);

		bool size(core_window_t*, nana::size, bool passive, bool ask_update);

		core_window_t* root(native_window_type) const;

		//Copy the root buffer that wnd specified into DeviceContext
		void map(core_window_t*, bool forced, const rectangle* update_area = nullptr);

		bool update(core_window_t*, bool redraw, bool force, const rectangle* update_area = nullptr);
		void refresh_tree(core_window_t*);

		bool do_lazy_refresh(core_window_t*, bool force_copy_to_screen);

		bool get_graphics(core_window_t*, nana::paint::graphics&);
		bool get_visual_rectangle(core_window_t*, nana::rectangle&);

		std::vector<core_window_t*> get_children(core_window_t*) const;
		bool set_parent(core_window_t* wd, core_window_t* new_parent);
		core_window_t* set_focus(core_window_t*, bool root_has_been_focused);

		core_window_t* capture_redirect(core_window_t*);
		void capture_ignore_children(bool ignore);
		bool capture_window_entered(int root_x, int root_y, bool& prev);
		core_window_t * capture_window() const;
		core_window_t* capture_window(core_window_t*, bool value);

		void enable_tabstop(core_window_t*);
		core_window_t* tabstop(core_window_t*, bool forward) const;	//forward means move to next in logic.

		void remove_trash_handle(unsigned tid);

		bool enable_effects_bground(core_window_t*, bool);

		bool calc_window_point(core_window_t*, nana::point&);

		root_misc* root_runtime(native_window_type) const;

		bool register_shortkey(core_window_t*, unsigned long key);
		void unregister_shortkey(core_window_t*, bool with_children);
		std::vector<std::pair<core_window_t*, unsigned long>> shortkeys(core_window_t*, bool with_children);

		core_window_t* find_shortkey(native_window_type, unsigned long key);

		void set_safe_place(core_window_t* wd, std::function<void()>&& fn);
		void call_safe_place(unsigned thread_id);
	private:
		void _m_disengage(core_window_t*, core_window_t* for_new);
		void _m_destroy(core_window_t*);
		void _m_move_core(core_window_t*, const point& delta);
		core_window_t* _m_find(core_window_t*, const point&);
		static bool _m_effective(core_window_t*, const point& root_pos);
	private:
		mutable mutex_type mutex_;

		struct wdm_private_impl;
		wdm_private_impl * const impl_;

		struct attribute
		{
			struct captured
			{
				core_window_t	*window;
				bool		inside;
				bool		ignore_children;
				std::vector<std::pair<core_window_t*, bool> > history;
			}capture;
		}attr_;

		struct menu_tag
		{
			native_window_type window;
			native_window_type owner;
			bool has_keyboard;
		}menu_;
	};//end class window_manager
}//end namespace detail
}//end namespace nana
#endif
