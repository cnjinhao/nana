/**
 *	A Frame Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/frame.hpp
 *
 *	@brief A frame provides a way to contain the platform window in a stdex GUI Window
 */

#ifndef NANA_GUI_WIDGET_FRAME_HPP
#define NANA_GUI_WIDGET_FRAME_HPP

#include "widget.hpp"
namespace nana
{
	/** 
	\brief Container for system native windows. Provides an approach to 
	display a control that is not written with Nana.GUI in a Nana.GUI window.

	Notes:
	  
	1. nana::native_window_type is a type of system handle of windows.
	2. all the children windows of a nana::frame is topmost to Nana.GUI windows.
	3. a simple example. Displaying a Windows Edit Control.

			nana::frame frame(parent, 0, 0 200, 100);
			HWND frame_handle = reinterpret_cast<HWND>(frame.container());
			HWND edit = ::CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"Test",
																WS_CHILD | WS_VISIBLE | WS_BORDER, 0, 0, 200, 100,
																frame_handle, 0, ::GetModuleHandle(0), 0);
			if(edit)
				frame.insert(edit);
 
     */
	class frame: public widget_object<category::frame_tag, int, nana::general_events>
	{
		typedef widget_object<category::frame_tag, int> base_type;
	public:
		frame();
		frame(window, bool visible);
		frame(window, const rectangle& = rectangle(), bool visible = true);
		bool insert(native_window_type);					///< Inserts a platform native window.
		native_window_type element(unsigned index);		    ///< Returns the child window through index.

		native_window_type container() const;	     	    ///< Returns the frame container native window handle.
	};
}//end namespace nana
#endif
