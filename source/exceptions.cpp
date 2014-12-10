/*
 *	Exception Definition
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/exceptions.cpp
 */
#include <nana/exceptions.hpp>

namespace nana
{

	//class thrd_exit
		thrd_exit::thrd_exit(unsigned retval):retval_(retval){}
		thrd_exit::~thrd_exit() throw(){}
		const char* thrd_exit::what() const throw(){ return "Exit-Threading Exception"; }
		unsigned thrd_exit::retval() const {	return retval_;	}
	//end class thrd_exit

	//class bad_member
		bad_member::bad_member(const std::string& what):what_(what){}
		bad_member::~bad_member() throw(){}
		const char* bad_member::what() const throw()
		{
			return what_.c_str();
		}
	//end class bad_member

	//class bad_syntax
		bad_syntax::bad_syntax(const std::string& what):what_(what){}
		bad_syntax::~bad_syntax() throw(){}
		const char* bad_syntax::what() const throw()
		{
			return what_.c_str();
		}
	//end class bad_syntax

	//class bad_error
		bad_error::bad_error(const std::string& what):what_(what){}
		bad_error::~bad_error() throw(){}
		const char* bad_error::what() const throw()
		{
			return what_.c_str();
		}
	//end class bad_error

	//class bad_handle: public std::exception
		bad_handle::bad_handle(const std::string& what):what_(what){}
		bad_handle::~bad_handle() throw(){}
		const char* bad_handle::what() const throw()
		{
			return what_.c_str();
		}
	//end class bad_handle

	//class bad_window  
		bad_window::bad_window(const char* what)
			:what_(what)
		{}
		
		bad_window::~bad_window() throw(){}

		const char* bad_window::what() const throw()
		{
			return what_.c_str();
		}
	//end class bad_window
} //end namespace nana
