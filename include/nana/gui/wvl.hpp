/**
 *	Nana GUI Library Definition
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2017 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file nana/gui/wvl.hpp
 *	@description
 *		the header file contains the files required for running of Nana.GUI
 */

#ifndef NANA_GUI_WVL_HPP
#define NANA_GUI_WVL_HPP

#include "programming_interface.hpp"
#include "screen.hpp"
#include "widgets/form.hpp"
#include "drawing.hpp"
#include "msgbox.hpp"
#include "place.hpp"


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
                    throw std::logic_error("form_loader failed to create the form");

                detail::form_loader_private::insert_form(p);
                if (IsVisible)
                    p->show();

                return *p;
            }

        };
	}

    template<typename Form, bool IsVisible = true>
    using form_loader = detail::form_loader<Form, IsVisible>;

#ifdef NANA_AUTOMATIC_GUI_TESTING

	/// @brief  Take control of the GUI and optionaly automaticaly tests it.
	///
	/// @detail It transfers to nana the program flow control, which begin pumping messages 
	///         from the underlying OS, interpreting and sending it with suitable arguments 
	///         to the nana widgets that registered a response in the corresponding event.	
	///         It also accept arguments to be used in case of automatic GUI testing.	
	///         Other Way the arguments are ignored. 
	void exec(
		      unsigned wait = 1,         ///< for the GUI to be constructed, in seconds  
		      unsigned wait_end = 1,     ///< for the GUI to be destructed, in seconds
		      std::function<void()> = {} ///< emit events to mimics user actions and may asert results
	         );

	/// send a click message to this widget - useffull in GUI testing
	void click(widget& w);

	/// in seconds
	void Wait(unsigned wait = 0);
#else
	void exec();
#endif

 
}//end namespace nana
#endif
