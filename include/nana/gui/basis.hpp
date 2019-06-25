/**
 *  \file basis.hpp
 *  \brief This file provides basis class and data structures required by the GUI
 *
 *	Basis Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 */

#ifndef NANA_GUI_BASIS_HPP
#define NANA_GUI_BASIS_HPP

#include <nana/push_ignore_diagnostic>

#include "../basic_types.hpp"
#include "../traits.hpp"	//metacomp::fixed_type_set

namespace nana
{
	namespace detail
	{
		struct basic_window;

		struct native_window_handle_impl;
		struct native_drawable_impl;
		struct event_handle_impl;
	}

	struct accel_key
	{
		char key;
		bool case_sensitive{ false };
		bool alt{ false };
		bool ctrl{ false };
		bool shift{ false };
	};

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

	enum class window_relationship
	{
		owner,		///< Owner window.
		parent,		///< Parent window.
		either_po 	///< One between owner and parent.
	};

	enum class bground_mode
	{
		none,
		basic,
		blend
	};

	enum class dragdrop_status
	{
		not_ready,
		ready,
		in_progress
	};

	namespace category
	{
		enum class flags
		{
			super,
			widget = 0x1,
			lite_widget = 0x3,
			root = 0x5
		};
		//wait for constexpr
		struct widget_tag{ static const flags value = flags::widget; };
		struct lite_widget_tag : public widget_tag{ static const flags value = flags::lite_widget;  };
		struct root_tag : public widget_tag{ static const flags value = flags::root;  };
	}// end namespace category

	using window = detail::basic_window*;							///< The window handle type representing nana window objects
	using native_window_type = detail::native_window_handle_impl*;	///< The native window handle type representing system native windows. E.g, HWND in windows, Window in X11

	using event_handle = detail::event_handle_impl*;				///< The event handle type representing nana window events
	using native_drawable_type = detail::native_drawable_impl*;		///< The drawable handle type representing system native drawable objects.	E.g. HDC in windows, Drawable in X11


	struct keyboard
	{
		enum{
			//Control Code for ASCII
			start_of_headline = 0x1,	//Ctrl+A
			end_of_text = 0x3,	//Ctrl+C
			backspace = 0x8, tab = 0x9,
			alt = 0x12,
			enter_n = 0xA, enter = 0xD, enter_r = 0xD,
			sync_idel = 0x16,	//Ctrl+V
			cancel = 0x18,	//Ctrl+X
			end_of_medium = 0x19,	//Ctrl+Y
			substitute = 0x1A,	//Ctrl+Z
			escape = 0x1B,
			space = 0x20,	//Space
			del = 0x7F,		//Delete
			os_del = del,	//Deprecated

			//The following names are intuitive name of ASCII control codes
			select_all = start_of_headline,
			copy = end_of_text,
			paste = sync_idel,
			cut = cancel,
			redo = end_of_medium,
			undo = substitute,

			//System Code for OS
			os_tab = 0x09,
			os_shift = 0x10,
			os_ctrl = 0x11,
			os_pageup = 0x21, os_pagedown,
			os_arrow_left = 0x25, os_arrow_up, os_arrow_right, os_arrow_down,
			os_insert = 0x2D,
            os_end = 0x23, os_home //Pos 1
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
	
	
/** @brief Provided to generate an appearance object with better readability and understandability   
 
A window has an appearance. This appearance can be specified when a window is being created. 
To determine the appearance of a window there is a structure named nana::appearance with 
a bool member for each feature with can be included or excluded in the "appearance" of the windows form. 
But in practical development is hard to describe the style of the appearance using the struct nana::appearance.
If a form would to be defined without min/max button and sizable border, then

\code{.CPP}
    nana::form form(x, y, width, height, nana::appearance(false, false, false, true, false));
\endcode

This piece of code may be confusing because of the 5 parameters of the constructor of `nana::form`. So the library provides a helper class for making it easy.  
For better readability and understandability Nana provides three templates classes to generate an appearance object: 
nana::appear::decorate, nana::appear::bald and nana::appear::optional. Each provide an operator 
that return a corresponding nana::appearance with predefined values. 
*/
	struct appear
	{
		struct minimize{};
		struct maximize{};
		struct sizable{};
		struct taskbar{};
		struct floating{};
		struct no_activate{};
		
        /** @brief Create an appearance of a window with "decoration" in non-client area, such as title bar
         *  
         *  We can create a form without min/max button and sizable border like this:  
         * \code{.CPP}
         * using nana::appear;
         * nana::form form(x, y, width, height, appear::decorate<appear::taskbar>());
         * \endcode
		 * The appearance created by appear::decorate<>() has a titlebar and borders that are draw by the 
		 * platform- window manager. If a window needs a minimize button, it should be:
         * \code{.CPP}
         * appear::decorate<appear::minimize, appear::taskbar>()
         * \endcode
         */
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
		
        /// Create an appearance of a window without "decoration" with no titlebar and no 3D-look borders.
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
	};//end namespace appear

	/// Interface for caret operations
	class caret_interface
	{
	public:
		virtual ~caret_interface() = default;

		virtual bool activated() const = 0;
		virtual void disable_throw() noexcept = 0;

		virtual void effective_range(const rectangle& range) = 0;

		virtual void position(const point& pos) = 0;
		virtual point position() const = 0;

		virtual void dimension(const size& size) = 0;
		virtual size dimension() const  = 0;

		virtual void visible(bool visibility) = 0;
		virtual bool visible() const = 0;
	};//end class caret_interface

	/// Interface for scroll operations
	/**
	 * This interface provides methods to operate the scrollbars that are contained
	 * in a specific widget, such as listbox and treebox
	 */
	class scroll_operation_interface
	{
	public:
		virtual ~scroll_operation_interface() = default;

		virtual bool visible(bool vert) const = 0;
	};

	namespace parameters
	{
		/// The system-wide parameters for mouse wheel
		struct mouse_wheel
		{
			unsigned lines;			///< The number of lines to scroll when the vertical mouse wheel is moved.
			unsigned characters;	///< The number of characters to scroll when the horizontal mouse wheel is moved.

			mouse_wheel();
		};
	}
}//end namespace nana

#include <nana/pop_ignore_diagnostic>
#endif
