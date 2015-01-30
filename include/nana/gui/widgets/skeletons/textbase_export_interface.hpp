/*
 *	Definitions of textbase export interfaces
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/skeletons/textbase_export_interface.hpp
 *	@description: It is defined outside, some headers like textbox should not include a whole textbase for its ext_evtbase 
 *
 */

#ifndef NANA_GUI_WIDGET_TEXTBASE_EXPORT_INTERFACE_HPP
#define NANA_GUI_WIDGET_TEXTBASE_EXPORT_INTERFACE_HPP

namespace nana{	namespace widgets
{
	namespace skeletons
	{
		class textbase_event_agent_interface
		{
		public:
			virtual ~textbase_event_agent_interface() = default;

			virtual void first_change() = 0;	///< An event for the text first change after text has been opened or stored.
			virtual void text_changed() = 0;	///< An event for the change of text.
		};
	}//end namespace skeletons
}//end namespace widgets
}//end namespace nana
#endif	//NANA_GUI_WIDGET_TEXTBASE_EXPORT_INTERFACE_HPP
