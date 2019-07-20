/**
 *	Window Manager Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/detail/window_manager.hpp
 *
 *	<Knowledge: 1, 2007-8-17, "Difference between destroy and destroy_handle">
 *		destroy method destroys a window handle and the handles of its children, but it doesn't delete the handle which type is a root window
 *		destroy_handle method just destroys the handle which type is a root window
 *
 */

#ifndef NANA_GUI_DETAIL_WINDOW_MANAGER_HPP
#define NANA_GUI_DETAIL_WINDOW_MANAGER_HPP

#include <nana/push_ignore_diagnostic>

#include "event_code.hpp"
#include "inner_fwd.hpp"
#include <functional>

namespace nana
{
	class widget;	//forward declaration
	namespace paint
	{
		class image;
		class graphics;
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
		{
			revertible_mutex(const revertible_mutex&) = delete;
			revertible_mutex& operator=(const revertible_mutex&) = delete;
			revertible_mutex(revertible_mutex&&) = delete;
			revertible_mutex& operator=(revertible_mutex&&) = delete;
		public:
			revertible_mutex();
			~revertible_mutex();

			void lock();
			bool try_lock();
			void unlock();

			void revert();
			void forward();
		private:
			struct implementation;
			implementation * const impl_;
		};
	public:
		using native_window = native_window_type;
		using mutex_type = revertible_mutex;

		window_manager();
		~window_manager();

		std::size_t window_count() const;
		mutex_type & internal_lock() const;
		void all_handles(std::vector<basic_window*>&) const;

		void event_filter(basic_window*, bool is_make, event_code);

		bool available(basic_window*);
		bool available(basic_window *, basic_window*);

		basic_window* create_root(basic_window*, bool nested, rectangle, const appearance&, widget*);
		basic_window* create_widget(basic_window*, const rectangle&, bool is_lite, widget*);
		void close(basic_window*);

		//destroy
		//@brief:	Delete the window handle
		void destroy(basic_window*);

		//destroy_handle
		//@brief:	Delete window handle, the handle type must be a root and a frame.

		// Deletes a window whose category type is a root type or a frame type.
		void destroy_handle(basic_window*);

		void icon(basic_window*, const paint::image& small_icon, const paint::image& big_icon);

		bool show(basic_window* wd, bool visible);

		//find a widget window at specified position
		//@param root A root window
		//@param pos Position
		//@param ignore_captured A flag indicates whether to ignore redirecting the result to its captured window. If this paramter is true, it returns the window at the position, if the parameter is false, it returns the captured window if the captured window don't ignore children.
		basic_window* find_window(native_window_type root, const point& pos, bool ignore_captured = false);

		//move the wnd and its all children window, x and y is a relatively coordinate for wnd's parent window
		bool move(basic_window*, int x, int y, bool passive);
		bool move(basic_window*, const rectangle&);

		bool size(basic_window*, nana::size, bool passive, bool ask_update);

		basic_window* root(native_window_type) const;

		//Copy the root buffer that wnd specified into DeviceContext
		void map(basic_window*, bool forced, const rectangle* update_area = nullptr);

		bool update(basic_window*, bool redraw, bool force, const rectangle* update_area = nullptr);
		void update_requesters(basic_window* root_wd);
		void refresh_tree(basic_window*);

		void do_lazy_refresh(basic_window*, bool force_copy_to_screen, bool refresh_tree = false);

		bool set_parent(basic_window* wd, basic_window* new_parent);
		basic_window* set_focus(basic_window*, bool root_has_been_focused, arg_focus::reason);

		basic_window* capture_redirect(basic_window*);

		bool capture_window_entered(int root_x, int root_y, bool& prev);
		basic_window * capture_window() const;
		void capture_window(basic_window*, bool capture, bool ignore_children_if_captured);

		void enable_tabstop(basic_window*);
		basic_window* tabstop(basic_window*, bool forward) const;	//forward means move to next in logic.

		void remove_trash_handle(thread_t tid);

		bool enable_effects_bground(basic_window*, bool);

		bool calc_window_point(basic_window*, nana::point&);

		root_misc* root_runtime(native_window) const;

		bool register_shortkey(basic_window*, unsigned long key);
		void unregister_shortkey(basic_window*, bool with_children);

		basic_window* find_shortkey(native_window_type, unsigned long key);

		void set_safe_place(basic_window* wd, std::function<void()>&& fn);
		void call_safe_place(thread_t thread_id);
	private:
		void _m_disengage(basic_window*, basic_window* for_new);
		void _m_destroy(basic_window*);
		void _m_move_core(basic_window*, const point& delta);
		void _m_shortkeys(basic_window*, bool with_chlidren, std::vector<std::pair<basic_window*, unsigned long>>& keys) const;
		basic_window* _m_find(basic_window*, const point&);
		static bool _m_effective(basic_window*, const point& root_pos);
	private:
		mutable mutex_type mutex_;

		struct wdm_private_impl;
		wdm_private_impl * const impl_;

		struct attribute
		{
			struct captured
			{
				basic_window	*window;
				bool		inside;
				bool		ignore_children;
				std::vector<std::pair<basic_window*, bool> > history;
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

#include <nana/pop_ignore_diagnostic>

#endif
