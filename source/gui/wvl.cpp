/*
 *	Nana GUI Library Definition
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
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
#include <nana/gui/detail/bedrock.hpp>
#include <nana/std_thread.hpp>
#include <iostream> 
#include <chrono>

//#define NANA_AUTOMATIC_GUI_TESTING
namespace nana
{
	namespace detail
	{
		void form_loader_private::insert_form(::nana::widget* p)
		{
			bedrock::instance().manage_form_loader(reinterpret_cast<basic_window*>(p->handle()), true);
		}
	}

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
		std::this_thread::sleep_for(std::chrono::seconds{ wait });
	}

	void pump()
	{
		detail::bedrock::instance().pump_event(nullptr, false);
	}


	void exec(
		unsigned wait,         // = 1,      ///< for the GUI to be constructed, in seconds  
		unsigned wait_end,     // = 1,      ///< for the GUI to be destructed, in seconds
		std::function<void()>f // = {}      ///< emit events to mimics user actions and may asert results
	)
	{
	#ifdef NANA_AUTOMATIC_GUI_TESTING
			
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

	#else
		static_cast<void>(wait);
		static_cast<void>(wait_end);
		static_cast<void>(f); //to eliminte unused parameter compiler warning.

		pump();
	#endif
	}
}//end namespace nana
