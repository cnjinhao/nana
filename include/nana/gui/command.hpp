/**
 *	A command Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2021-2021 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *  @file: nana/gui/command.hpp
 *	@contributor
 *		qPCR4vir 
 */

#pragma once
#ifndef NANA_COMMAND_HPP
#define NANA_COMMAND_HPP

#include <string>
#include <functional>

#include <nana\paint\image.hpp>

namespace nana 
{
	namespace detail
	{

	}

	/// A command to be shared between menus, toolbars or button widgets.
	/// 
	/// Mostly state only, with actions and drawings controlled by the widgets.
	/// Some of the state-data are automatically used only at the momment of setting 
	/// the command at the widget.
	class command
	{
	public:
		using event_fn_t = std::function<void(command& me)>;

		command(std::string text,          ///<
			    event_fn_t  event_handler, ///< 
			    paint::image image = {}    ///<
		) 
			: text(text), event_handler(event_handler), image(image)
		{}

		//~command();

		enum class checks  // todo: see toolbar enum class tools
		{
			none,  // = toolbar button?
			option,   // = toolbar toggle?
			highlight
		};

		/// A callback functor type.  
	//private:
		std::string	    text;                   ///< always  shared
		std::string	    tooltip;                ///< shared only at the moment of setting at the widget
		event_fn_t	    event_handler;          ///< always  shared
		checks			style{ checks::none };  ///< always  shared
		paint::image	image;                  ///< always  shared
		bool            enabled{ true };        ///< always  shared
		bool            checked{ false };       ///< always  shared
		//mutable wchar_t	hotkey{ 0 };

	};

	using shared_command = std::shared_ptr<command>;

}

#endif