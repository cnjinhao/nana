/**
 *	A Basic Window Widget Definition
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file nana/gui/detail/basic_window.hpp
 *  @brief A Basic Window Widget Definition
 */

#ifndef NANA_GUI_DETAIL_BASIC_WINDOW_HPP
#define NANA_GUI_DETAIL_BASIC_WINDOW_HPP
#include <nana/push_ignore_diagnostic>
#include <nana/gui/detail/drawer.hpp>
#include <nana/gui/detail/events_holder.hpp>
#include <nana/gui/detail/widget_geometrics.hpp>
#include <nana/gui/detail/widget_content_measurer_interface.hpp>
#include <nana/gui/detail/widget_notifier_interface.hpp>
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

	class caret
		: public caret_interface
	{
	public:
		caret(basic_window* owner, const size& size);
		~caret();

		void activate(bool activity);
		basic_window* owner() const noexcept;
		void update();
	public:
		//Implement caret_interface functions

		//This function is useless for class caret, see caret_proxy.
		void disable_throw() noexcept override;
		void effective_range(const rectangle& r) override;
		void position(const point& pos) override;
		nana::point position() const override;
		size dimension() const override;
		void dimension(const size& s) override;
		void visible(bool visibility) override;
		bool visible() const override;
		bool activated() const override;
	private:
		basic_window * owner_;
		point	position_;
		size	size_;
		size	visual_size_;
		visible_state visibility_{ visible_state::invisible };
		bool	out_of_range_{ false };
		rectangle effect_range_;
	};//end class caret


	/// Define some constant about tab category, these flags can be combine with operator |
	struct tab_type
	{
		enum t
		{
			none,		///< process by nana
			tabstop,	///< move to the next tabstop window
			eating,		///< process by current window
		};
	};


	/// a window data structure descriptor
	struct basic_window
		: public events_holder
	{
		using container = std::vector<basic_window*>;

		enum class update_state
		{
			none, lazy, refreshed, request_refresh
		};

		struct edge_nimbus_action
		{
			basic_window * window;
			bool rendered;
		};

		/// constructor for the root window
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

		/// bind a native window and baisc_window
		void bind_native_window(native_window_type, unsigned width, unsigned height, unsigned extra_width, unsigned extra_height, paint::graphics&);

		bool is_ancestor_of(const basic_window* wd) const;
		bool visible_parents() const;
		bool displayed() const;
		bool belong_to_lazy() const;
		const basic_window * child_caret() const; ///< Returns the child which owns the caret

		bool is_draw_through() const;	///< Determines whether it is a draw-through window.

		basic_window * seek_non_lite_widget_ancestor() const;

		void set_action(mouse_action);

		/// Only refresh when the root of window is in lazy-updating mode
		bool try_lazy_update(bool try_refresh);
	public:
		/// Override event_holder
		bool set_events(const std::shared_ptr<general_events>&) override;
		general_events * get_events() const override;
	private:
		void _m_init_pos_and_size(basic_window* parent, const rectangle&);
		void _m_initialize(basic_window* parent);
	public:
#if defined(NANA_POSIX)
		point	pos_native;
#endif
		point	pos_root;	///< coordinates of the root window
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
		::nana::detail::drawer	drawer;	    ///< Self Drawer with owen graphics
		basic_window*		root_widget;	///< A pointer refers to the root basic window, if the window is a root, the pointer refers to itself.
		paint::graphics*	root_graph;		///< Refer to the root buffer graphics
		cursor	predef_cursor;
		std::unique_ptr<widget_notifier_interface> widget_notifier;

		struct flags_type
		{
			bool enabled	:1;
			bool dbl_click	:1;
			bool captured	:1;	///< if mouse button is down, it always receive mouse move even the mouse is out of its rectangle
			bool modal		:1;
			bool take_active:1;	///< If take_active is false, other.active_window still keeps the focus.
			bool refreshing	:1;
			bool destroying	:1;
			bool dropable	:1; ///< Whether the window has make mouse_drop event.
			bool fullscreen	:1;	///< When the window is maximizing whether it fit for fullscreen.
			bool borderless :1;
			bool make_bground_declared	: 1;	///< explicitly make bground for bground effects
			bool ignore_menubar_focus	: 1;	///< A flag indicates whether the menubar sets the focus.
			bool ignore_mouse_focus		: 1;	///< A flag indicates whether the widget accepts focus when clicking on it
			bool space_click_enabled : 1;		///< A flag indicates whether enable mouse_down/click/mouse_up when pressing and releasing whitespace key.
			bool draggable : 1;
			unsigned Reserved : 17;
			unsigned char tab;		///< indicate a window that can receive the keyboard TAB
			mouse_action	action;
			mouse_action	action_before;
		}flags;


		struct annex_components
		{
			caret* caret_ptr{ nullptr };

			//The following pointers refer to the widget's object.
			std::shared_ptr<general_events> events_ptr;
			widget_geometrics* scheme{ nullptr };
			::nana::dev::widget_content_measurer_interface* content_measurer{ nullptr };
		}annex;

		struct
		{
			effects::edge_nimbus	edge_nimbus;
			effects::bground_interface * bground;
			double	bground_fade_rate;
		}effect;

		struct other_tag
		{
			struct	attr_root_tag
			{
				bool ime_enabled{ false };
				bool lazy_update{ false };	///< Indicates whether the window is in lazy-updating mode.

				container	update_requesters;	///< Container for lazy-updating requesting windows.
				container	tabstop;
				std::vector<edge_nimbus_action> effects_edge_nimbus;
				basic_window*	focus{nullptr};
				basic_window*	menubar{nullptr};
				cursor			state_cursor{nana::cursor::arrow};
				basic_window*	state_cursor_window{ nullptr };

				std::function<void()> draw_through;	///< A draw through renderer for root widgets.
			};

			const category::flags category;
			basic_window *active_window;	///< if flags.take_active is false, the active_window still keeps the focus,
											///< if the active_window is null, the parent of this window keeps focus.
			paint::graphics glass_buffer;	///< if effect.bground is avaiable. Refer to window_layout::make_bground.
			update_state	upd_state;
			dragdrop_status	dnd_state{ dragdrop_status::not_ready };

			union
			{
				attr_root_tag * root;
			}attribute;

			other_tag(category::flags);
			~other_tag();
		}other;

		native_window_type	root;		    ///< root Window handle
		thread_t			thread_id;		///< the identifier of the thread that created the window.
		unsigned			index;
		container			children;
	};

}//end namespace detail
}//end namespace nana
#include <nana/pop_ignore_diagnostic>
#endif

