/*
 *	Exception Definition
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/exceptions.hpp
 */

#ifndef NANA_EXCEPTIONS_H
#define NANA_EXCEPTIONS_H
#include <exception>
#include <string>

namespace nana
{

	/**
	 *	nana::threads::thread::exit throws this exception to exit thread
	 *	this exception is not allowed to be catch by programmer,
	 *	otherwise the thread may not exit
	 */
	class thrd_exit: public std::exception
	{
	public:
		thrd_exit(unsigned retval);
		~thrd_exit() throw();
		const char* what() const throw();
		unsigned retval() const;
	private:
		unsigned retval_;
	};

	/**
	 * nana::text::settings_t throws this exception if it dose not found a given member
	 * in a scope
	 */
	class bad_member: public std::exception
	{
	public:
		bad_member(const std::string& what);
		~bad_member() throw();
		const char* what() const throw();
	private:
		std::string what_;
	};

	/**
	 * nana::text::settings_t throws this exception if there is a syntax error
	 */
	class bad_syntax: public std::exception
	{
	public:
		bad_syntax(const std::string& what);
        ~bad_syntax() throw();
		const char* what() const throw();
	private:
		std::string what_;
	};

	class bad_error: public std::exception
	{
	public:
		bad_error(const std::string& what);
        ~bad_error() throw();
		const char* what() const throw();
	private:
		std::string what_;
	};

	class bad_handle: public std::exception
	{
	public:
		bad_handle(const std::string& what);
        ~bad_handle() throw();
		const char* what() const throw();
	private:
		std::string what_;
	};

	class bad_window
		:public std::exception
	{
	public:   
		bad_window(const char* what);
		~bad_window() throw();
		const char* what() const throw();
	private:
		std::string what_;
	};
}// end namespace nana

#endif
