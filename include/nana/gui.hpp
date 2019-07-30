/**
 *	Nana GUI Header
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file nana/gui.hpp
 *	@description
 *		the header file contains the files required for running of Nana.GUI
 */

#ifndef NANA_GUI_HPP
#define NANA_GUI_HPP

#include "gui/compact.hpp"
#include "gui/screen.hpp"
#include "gui/widgets/form.hpp"
#include "gui/drawing.hpp"
#include "gui/msgbox.hpp"
#include "gui/place.hpp"


namespace nana
{
#ifdef NANA_AUTOMATIC_GUI_TESTING

	/// @brief  Take control of the GUI and optionally automatically tests it.
	///
	/// @detail It transfers to nana the program flow control, which begin pumping messages 
	///         from the underlying OS, interpreting and sending it with suitable arguments 
	///         to the nana widgets that registered a response in the corresponding event.	
	///         It also accept arguments to be used in case of automatic GUI testing.	
	///         Other Way the arguments are ignored. 
	void exec(
		unsigned wait = 1,         ///< for the GUI to be constructed, in seconds  
		unsigned wait_end = 1,     ///< for the GUI to be destructed, in seconds
		std::function<void()> = {} ///< emit events to mimics user actions and may assert results
	);

	/// send a click message to this widget - useful in GUI testing
	void click(widget& w);

	/// in seconds
	void Wait(unsigned wait = 0);
#else
	void exec();
#endif


}//end namespace nana
#endif
