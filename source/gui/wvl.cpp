/*
 *	Nana GUI Library Definition
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/wvl.cpp
 *	@description:
 *		the file contains the files required for running of Nana.GUI 
 */

#include <nana/gui/wvl.hpp>

namespace nana
{
	void exec()
	{
		detail::bedrock::instance().pump_event(nullptr, false);
	}
}//end namespace nana
