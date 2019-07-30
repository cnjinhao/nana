/*
 *	Nana GUI Library Definition
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/wvl.cpp
 *	@description:
 *		the file contains the files required for running of Nana.GUI 
 */

#include <nana/gui/compact.hpp>
#include <nana/gui/widgets/widget.hpp>
#include <nana/gui/detail/bedrock.hpp>
#include <nana/std_thread.hpp>
#include <iostream> 

#ifdef STD_THREAD_NOT_SUPPORTED
#	include <boost/chrono.hpp>
#else
#	include <chrono>
#endif

//#define NANA_AUTOMATIC_GUI_TESTING
namespace nana
{
	namespace detail
	{
		void form_loader_private::insert_form(::nana::widget* p)
		{
			bedrock::instance().manage_form_loader(p->handle(), true);
		}
	}

#ifdef NANA_AUTOMATIC_GUI_TESTING

	void click(widget& w)
	{
		std::cout << "Automatically clicking widget "<<w.caption()<<":\n";
		arg_click arg;
		arg.window_handle = w.handle();
		w.events().click.emit(arg, w.handle());
	}

	/// in seconds
	void Wait(unsigned wait)
	{
		if (!wait) return;
		std::cout << "waiting " << wait << " sec...\n";
#	ifdef STD_THREAD_NOT_SUPPORTED
		boost::this_thread::sleep_for(boost::chrono::seconds{ wait });
#	else
		std::this_thread::sleep_for(std::chrono::seconds{ wait });
#	endif
	}

	void pump()
	{
		internal_scope_guard lock;
		detail::bedrock::instance().pump_event(nullptr, false);
	}


	void exec(
		unsigned wait,         // = 1,      ///< for the GUI to be constructed, in seconds  
		unsigned wait_end,     // = 1,      ///< for the GUI to be destructed, in seconds
		std::function<void()>f // = {}      ///< emit events to mimics user actions and may assert results
	)
	{
			
	    std::cout << "Will wait " << wait << " sec...\n";

	    std::thread t([wait, &f, wait_end]()
			{ 
				Wait( wait );
				std::cout << "running... \n"  ;
				if (f)
				{
					f();
					std::cout << "\nCongratulations, this was not trivial !" << std::endl;
				}else
				{
					std::cout << "\nJust a trivial test." << std::endl;
				}
				std::cout << "Done... \n";
				std::cout << "Now again ";
				Wait(wait_end);
				std::cout << "Done... Now API::exit all ...\n";
				API::exit_all();   
			});		

		pump();
		if (t.joinable())
			t.join();
	}
#else
	void exec()
	{
		internal_scope_guard lock;
		detail::bedrock::instance().pump_event(nullptr, false);
	}
#endif
}//end namespace nana
