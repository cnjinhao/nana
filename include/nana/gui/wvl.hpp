/*
 *	Nana GUI Library Definition
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/wvl.hpp
 *	@description:
 *		the header file contains the files required for running of Nana.GUI
 */

#ifndef NANA_GUI_WVL_HPP
#define NANA_GUI_WVL_HPP

#include "programming_interface.hpp"
#include "widgets/form.hpp"
#include "drawing.hpp"
#include "msgbox.hpp"
#include "../exceptions.hpp"

namespace nana
{
	template<typename Form, bool IsMakeVisible = true>
	class form_loader
	{
	public:
		template<typename... Args>
		Form & operator()(Args &&... args) const
		{
			Form* res = detail::bedrock::instance().rt_manager.create_form<Form>(std::forward<Args>(args)...);
			if (nullptr == res)
				throw nana::bad_window("form_loader.operator(): failed to create a window");

			if (IsMakeVisible) res->show();

			return *res;
		}

	};

	void exec();
}//end namespace nana
#endif
