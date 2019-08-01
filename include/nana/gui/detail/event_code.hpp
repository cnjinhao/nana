/*
*	Definitions of General Events Code
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file: nana/gui/detail/event_code.hpp
*
*/

#ifndef NANA_DETAIL_EVENT_CODE_HPP
#define NANA_DETAIL_EVENT_CODE_HPP

namespace nana
{
	enum class event_code
	{
		click,				///< A mouse click event.
		dbl_click,			///< A mouse double click event.
		mouse_enter,		///< A mouse enters a widget.
		mouse_move,			///< A mouse moves over a widget.
		mouse_leave,		///< A mouse leaves a widget.
		mouse_down,			///< A mouse button is pressed on a widget.
		mouse_up,			///< A mouse button is released on a widget.
		mouse_wheel,		///< A mouse scrolls the wheel on a widget.
		mouse_drop,			///< A mouse release over a window that is registered as recipient of drag and drop.
		expose,				///< 
		resizing, 			///< A widget's size is sizing. In this event, A widget's size can be overridden with a new size.
		resized,			///< A widget's size is changing.
		move,				///< 
		unload,				///< A form is closed by clicking the X button, only works for root widget.
		destroy,			///< A widget is about to be destroyed.
		focus,				///< A widget's focus is changed.
		key_ime,
		key_press,			///< A keyboard is pressed on a focus widget.
		key_char,			///< The focus widget received a character.
		key_release,		///< A keyboard is released on a focus widget.
		shortkey,			///< The widgets received a shortkey message.

		elapse,				///< Unoperational events? . A widget received a tick that is sended by timer.

		end					///< End indicator, it's not an event.
	};
}//end namespace nana

#endif