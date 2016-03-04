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
#include <thread>
#include <iostream> 

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
		arg_click arg;
		arg.window_handle = w.handle();
		w.events().click.emit(arg);
	}

	/// in seconds
	void Wait(unsigned wait)
	{
		if (wait)
			std::this_thread::sleep_for(std::chrono::seconds{ wait });
	}

	void pump()
	{
		detail::bedrock::instance().pump_event(nullptr, false);
	}


	void exec(form *main_form, //= nullptr, ///< used to close the program
		unsigned wait,         // = 1,      ///< for the GUI to be constructed, in seconds  
		unsigned wait_end,     // = 1,      ///< for the GUI to be destructed, in seconds
		std::function<void()>f // = {}      ///< emit events to mimics user actions and may asert results
	)
	{
	#ifdef NANA_AUTOMATIC_GUI_TESTING
		//if (!wait)	
		//	wait = 1;
		//if (!main_form && !f)
		//	f = []() {API::exit(); };
			
	    std::cout << "Will wait " << wait << " sec...\n";
	    std::thread t([wait, &f, wait_end, main_form]()
		            { if (wait) 
		                {   
							std::cout << "Waiting " << wait << " sec...\n";
							Wait( wait );
							std::cout << "running... \n"  ;
		                    if (f) f(); 
							std::cout << "Done... \n";
							std::cout << "Now waiting anothers " << wait_end << " sec...\n";
							Wait(wait_end);
							std::cout << "Done... \n";
							if (main_form)
								 main_form->close();
							API::exit();    // why not works?
						}
		            });		
		pump();
		if (t.joinable())
			t.join();
    #else
			pump();
	#endif
	}
}//end namespace nana
