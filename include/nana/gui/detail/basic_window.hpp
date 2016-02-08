/*
 *	A Basic Window Widget Definition
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/detail/basic_window.hpp
 */

#ifndef NANA_GUI_DETAIL_BASIC_WINDOW_HPP
#define NANA_GUI_DETAIL_BASIC_WINDOW_HPP
#include "drawer.hpp"
#include "events_holder.hpp"
#include "widget_colors.hpp"
#include "widget_notifier_interface.hpp"
#include <nana/basic_types.hpp>
#include <nana/system/platform.hpp>
#include <nana/gui/effects.hpp>

namespace nana{
namespace detail
{
	struct basic_window;

	enum class visible_state
	{
		invisible, visible, displayed
	};

	class caret_descriptor
	{
	public:
		typedef basic_window core_window_t;

		caret_descriptor(core_window_t*, unsigned width, unsigned height);
		~caret_descriptor();
		void set_active(bool);
		core_window_t* window() const;
		void position(int x, int y);
		void effective_range(::nana::rectangle);
		::nana::point position() const;
		void visible(bool isshow);
		bool visible() const;
		::nana::size size() const;
		void size(const ::nana::size&);
		void update();
	private:
		core_window_t*	wd_;
		::nana::point point_;
		::nana::size	size_;
		::nana::size	paint_size_;
		visible_state	visible_state_;
		bool		out_of_range_;
		::nana::rectangle effective_range_;
	};//end class caret_descriptor

	//tab_type
	//@brief: Define some constant about tab category, these flags can be combine with operator |
	struct tab_type
	{
		enum t
		{
			none,		//process by nana
			tabstop,	//move to the next tabstop window
			eating,		//process by current window
		};
	};

	//struct basic_window
	//@brief: a window data structure descriptor 
	struct basic_window
		: public events_holder
	{
		using container = std::vector<basic_window*>;

		enum class update_state
		{
			none, lazy, refresh
		};

		struct edge_nimbus_action
		{
			basic_window * window;
			bool rendered;
		};

		//basic_window
		//@brief: constructor for the root window
		basic_window(basic_window* owner, std::unique_ptr<widget_notifier_interface>&&, category::root_tag**);

		template<typename Category>
		basic_window(basic_window* parent, std::unique_ptr<widget_notifier_interface>&& wdg_notifier, const rectangle& r, Category**)
			: widget_notifier(std::move(wdg_notifier)), other(Category::value)
		{
			drawer.bind(this);
			if(parent)
			{
				_m_init_pos_and_size(parent, r);
				_m_initialize(parent);
			}
		}

		~basic_window();

		//bind_native_window
		//@brief: bind a native window and baisc_window
		void bind_native_window(native_window_type, unsigned width, unsigned height, unsigned extra_width, unsigned extra_height, paint::graphics&);

		void frame_window(native_window_type);

		bool is_ancestor_of(const basic_window* wd) const;
		bool visible_parents() const;
		bool displayed() const;
		bool belong_to_lazy() const;
		const basic_window * child_caret() const; //Returns a child which owns a caret

		bool is_draw_through() const;	///< Determines whether it is a draw-through window.

		basic_window * seek_non_lite_widget_ancestor() const;
	public:
		//Override event_holder
		bool set_events(const std::shared_ptr<general_events>&) override;
		general_events * get_events() const override;
	private:
		void _m_init_pos_and_size(basic_window* parent, const rectangle&);
		void _m_initialize(basic_window* parent);
	public:
#if defined(NANA_LINUX) || defined(NANA_MACOS)
		point	pos_native;
#endif
		point	pos_root;	//coordinate for root window
		point	pos_owner;
		size	dimension;
		::nana::size	min_track_size;
		::nana::size	max_track_size;

		bool	visible;

		unsigned extra_width;
		unsigned extra_height;

		basic_window	*parent;
		basic_window	*owner;

		native_string_type		title;
		::nana::detail::drawer	drawer;	//Self Drawer with owen graphics
		basic_window*		root_widget;	//A pointer refers to the root basic window, if the window is a root, the pointer refers to itself.
		paint::graphics*	root_graph;		//Refer to the root buffer graphics
		cursor	predef_cursor;
		std::unique_ptr<widget_notifier_interface> widget_notifier;

		struct flags_type
		{
			bool enabled	:1;
			bool dbl_click	:1;
			bool captured	:1;	//if mouse button is down, it always receive mouse move even the mouse is out of its rectangle
			bool modal		:1;
			bool take_active:1;	//If take_active is false, other.active_window still keeps the focus.
			bool refreshing	:1;
			bool destroying	:1;
			bool dropable	:1; //Whether the window has make mouse_drop event.
			bool fullscreen	:1;	//When the window is maximizing whether it fit for fullscreen.
			bool borderless :1;
			bool make_bground_declared	: 1;	//explicitly make bground for bground effects
			bool ignore_menubar_focus	: 1;	//A flag indicates whether the menubar sets the focus.
			bool ignore_mouse_focus		: 1;	//A flag indicates whether the widget accepts focus when clicking on it
			bool space_click_enabled : 1;		//A flag indicates whether enable mouse_down/click/mouse_up when pressing and releasing whitespace key.
			unsigned Reserved	:18;
			unsigned char tab;		//indicate a window that can receive the keyboard TAB
			mouse_action	action;
		}flags;

		struct
		{
			caret_descriptor* caret;
			std::shared_ptr<general_events> events_ptr;
		}together;
		
		widget_colors* scheme{ nullptr };

		struct
		{
			effects::edge_nimbus	edge_nimbus;
			effects::bground_interface * bground;
			double	bground_fade_rate;
		}effect;
		
		struct other_tag
		{
			struct	attr_frame_tag
			{
				native_window_type container{nullptr};
				std::vector<native_window_type> attach;
			};

			struct	attr_root_tag
			{
				container	frames;	//initialization is null, it will be created while creating a frame widget. Refer to WindowManager::create_frame
				container	tabstop;
				std::vector<edge_nimbus_action> effects_edge_nimbus;
				basic_window*	focus{nullptr};
				basic_window*	menubar{nullptr};
				bool			ime_enabled{false};
#if defined(NANA_WINDOWS)
				cursor			running_cursor{ nana::cursor::arrow };
#endif
				cursor			state_cursor{nana::cursor::arrow};
				basic_window*	state_cursor_window{ nullptr };

				std::function<void()> draw_through;	// A draw through renderer for root widgets.
			};

			const category::flags category;
			basic_window *active_window;	//if flags.take_active is false, the active_window still keeps the focus,
											//if the active_window is null, the parent of this window keeps focus.
			paint::graphics glass_buffer;	//if effect.bground is avaiable. Refer to window_layout::make_bground.
			update_state	upd_state;

			union
			{
				attr_root_tag * root;
				attr_frame_tag * frame;
			}attribute;

			other_tag(category::flags);
			~other_tag();
		}other;

		native_window_type	root;		//root Window handle
		unsigned			thread_id;		//the identifier of the thread that created the window.
		unsigned			index;
		container			children;
	};

}//end namespace detail
}//end namespace nana
#endif

