/**
 *	Nana GUI Library Definition
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file nana/gui/compact.hpp
 *	@description
 *		the header file contains the files required for running of Nana.GUI
 */

#ifndef NANA_GUI_WVL_HPP
#define NANA_GUI_WVL_HPP

#include "programming_interface.hpp"

namespace nana
{
	namespace detail
	{
		struct form_loader_private
		{
			template<typename, bool> friend class form_loader;
		private:
			static void insert_form(::nana::widget*);
		};

		template<typename Form, bool IsVisible>
		class form_loader
		{
        public:
            template<typename... Args>
            Form & operator()(Args &&... args) const
            {
                auto p = new Form(std::forward<Args>(args)...);

                if (p->empty())
                    throw std::runtime_error("form_loader failed to create the form");


                detail::form_loader_private::insert_form(p);
                if (IsVisible)
                    p->show();

                return *p;
            }

        };
	}

    template<typename Form, bool IsVisible = true>
    using form_loader = detail::form_loader<Form, IsVisible>;
}//end namespace nana
#endif
