/*
 *	Basis Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2014 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/basis.hpp
 *
 *	This file provides basis class and data structrue that required by gui
 */

#ifndef NANA_GUI_BASIS_HPP
#define NANA_GUI_BASIS_HPP

#include "../basic_types.hpp"
#include "../traits.hpp"	//metacomp::fixed_type_set

namespace nana
{
	namespace detail
	{
		struct native_window_handle_impl{};
		struct window_handle_impl{};
		struct event_handle_impl{};
	}

	enum class checkstate
	{
		unchecked, checked, partial
	};

	enum class window_border
	{
		none,
		left, right, top, bottom,
		top_left, top_right, bottom_left, bottom_right
	};

	enum class bground_mode
	{
		none,
		basic,
		blend
	};

	namespace category
	{
		enum class flags
		{
			super,
			widget = 0x1,
			lite_widget = 0x3,
			root = 0x5,
			frame = 0x9
		};
		//wait for constexpr
		struct widget_tag{ static const flags value = flags::widget; };
		struct lite_widget_tag : widget_tag{ static const flags value = flags::lite_widget;};
		struct root_tag : widget_tag{ static const flags value = flags::root; };
		struct frame_tag: widget_tag{ static const flags value = flags::frame; };
	}// end namespace category

	typedef detail::native_window_handle_impl * native_window_type;

	typedef detail::window_handle_impl*	window; ///< \see [What is window class ](https://sourceforge.net/p/nanapro/discussion/general/thread/bd0fabfb/) 
	typedef detail::event_handle_impl*	event_handle;


	struct keyboard
	{
		enum t{
			//Control Code for ASCII
			select_all	= 0x1,
			end_of_text		= 0x3,	//Ctrl+C
			backspace	= 0x8,	tab		= 0x9,
			enter_n		= 0xA,	enter	= 0xD,	enter_r = 0xD,
			alt			= 0x12,
			sync_idel		= 0x16,	//Ctrl+V
			cancel			= 0x18,	//Ctrl+X
			end_of_medium	= 0x19,	//Ctrl+Y
			substitute		= 0x1A,	//Ctrl+Z
			escape		= 0x1B,

			//The following names are intuitive name of ASCII control codes
			copy	= 0x3,	//end_of_text
			paste	= 0x16,	//sync_idel
			cut		= 0x18,	//cancel
			redo	= 0x19,	//end_of_medium
			undo	= 0x1A,	//substitue

			//System Code for OS
			os_pageup		= 0x21,	os_pagedown,
			os_arrow_left	= 0x25, os_arrow_up, os_arrow_right, os_arrow_down,
			os_insert		= 0x2D, os_del
		};
	};

	namespace color
	{
		enum
		{
			white	= 0xFFFFFF,
			blue	= 0x0000FF,
			green	= 0x00FF00,
			red		= 0xFF0000,

			button_face_shadow_start = 0xF5F4F2,
			button_face_shadow_end = 0xD5D2CA,
			button_face = 0xD4D0C8,
			dark_border	= 0x404040,
			gray_border	= 0x808080,
			highlight = 0x1CC4F7
		};
	};

	enum class cursor
	{
		hand	= 60,     ///< displays a hand to indicate a text or an element is clickable
		arrow	= 68,     ///< the default shape
		wait	= 150,    ///< indicates the system is currently busy
		iterm	= 152,    ///< A text caret. Displays a caret to indicate the UI is input able
		size_we	= 108,
		size_ns	= 116,
		size_top_left = 134,
		size_top_right = 136,
		size_bottom_left = 12,
		size_bottom_right = 14
	};

	enum class mouse
	{
		any_button, 
		left_button, 
		middle_button, 
		right_button
	};

	enum class z_order_action
	{
		none, 
		bottom,       ///< brings a window at the bottom of z-order.
		top,          ///< brings a widget at the top of the z-order.
		topmost,      ///< brings a window at the top of the z-order and stays here.
		foreground    ///< brings a window to the foreground.
	};

	/// Window appearance structure defined to specify the appearance of a form
	struct appearance
	{
		bool taskbar;
		bool floating;
		bool no_activate;

		bool minimize;
		bool maximize;
		bool sizable;

		bool decoration;

		appearance();
		appearance(bool has_decoration, bool taskbar, bool floating, bool no_activate, bool min, bool max, bool sizable);
	};
    /// Provided to generate an appearance object with better readability and understandability   
	struct appear
	{
		struct minimize{};
		struct maximize{};
		struct sizable{};
		struct taskbar{};
		struct floating{};
		struct no_activate{};
        /// Create an appearance of a window with "decoration"
		template<   typename Minimize = null_type,
					typename Maximize = null_type,
					typename Sizable = null_type,
					typename Floating = null_type,
					typename NoActive = null_type>
		struct decorate
		{
			typedef meta::fixed_type_set<Minimize, Maximize, Sizable, Floating, NoActive> set_type;

			operator appearance() const
			{
				return appearance(	true, true,
									set_type::template count<floating>::value,
									set_type::template count<no_activate>::value,
									set_type::template count<minimize>::value,
									set_type::template count<maximize>::value,
									set_type::template count<sizable>::value
									);
			}
		};
        /// Create an appearance of a window without "decoration"
		template < typename Taskbar  = null_type, 
                   typename Floating = null_type, 
                   typename NoActive = null_type, 
                   typename Minimize = null_type, 
                   typename Maximize = null_type, 
                   typename Sizable  = null_type>
		struct bald
		{
			typedef meta::fixed_type_set<Taskbar, Floating, NoActive, Minimize, Maximize, Sizable> set_type;

			operator appearance() const
			{
				return appearance(	false,
									set_type::template count<taskbar>::value,
									set_type::template count<floating>::value,
									set_type::template count<no_activate>::value,
									set_type::template count<minimize>::value,
									set_type::template count<maximize>::value,
									set_type::template count<sizable>::value
									);
			}
		};
        /// Create a window with decoration depending on the first non-type template parameter
		template < bool HasDecoration = true, 
                   typename Sizable = null_type, 
                   typename Taskbar = null_type, 
                   typename Floating = null_type, 
                   typename NoActive = null_type>
		struct optional
		{
			typedef meta::fixed_type_set<Taskbar, Floating, NoActive> set_type;

			operator appearance() const
			{
				return appearance(HasDecoration,
									set_type::template count<taskbar>::value,
									set_type::template count<floating>::value,
									set_type::template count<no_activate>::value,
									true, true,
									set_type::template count<sizable>::value);
			}
		};
	};//end namespace apper
}//end namespace nana
#endif
