/*
 *	A Drawing Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2014 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/drawing.cpp
 */

#include <nana/gui/drawing.hpp>
#include <nana/gui/programming_interface.hpp>
#include <nana/gui/detail/basic_window.hpp>

namespace nana
{
	//restrict
	//@brief:	This name is only visible for this compiling-unit
	namespace restrict
	{
		typedef detail::bedrock::core_window_t core_window_t;
		extern detail::bedrock& bedrock;

		inline detail::drawer& get_drawer(window wd)
		{
			return reinterpret_cast<core_window_t*>(wd)->drawer;
		}
	}
    
    //class drawing
  		drawing::drawing(window wd)
			:handle_(wd)
  		{}
  		
  		drawing::~drawing(){} //Just for polymorphism

		bool drawing::empty() const
		{
			return API::empty_window(handle_) ||  reinterpret_cast<restrict::core_window_t*>(handle_)->root_graph->empty();
		}

		void drawing::update() const
		{
			API::refresh_window(handle_);
		}

		void drawing::draw(const draw_fn_t& f)
		{
			if(API::empty_window(handle_))	return;
			restrict::get_drawer(handle_).draw(draw_fn_t(f), false);		
		}

		void drawing::draw(draw_fn_t&& f)
		{
			if(API::empty_window(handle_))	return;
			restrict::get_drawer(handle_).draw(std::move(f), false);
		}

		drawing::diehard_t drawing::draw_diehard(const draw_fn_t& f)
		{
			if(API::empty_window(handle_)) return nullptr;
			return reinterpret_cast<diehard_t>(restrict::get_drawer(handle_).draw(draw_fn_t(f), true));
		}

		drawing::diehard_t drawing::draw_diehard(draw_fn_t&& f)
		{
			if(API::empty_window(handle_))	return nullptr;
			return reinterpret_cast<diehard_t>(restrict::get_drawer(handle_).draw(std::move(f), true));
		}

		void drawing::erase(diehard_t d)
		{
			if(API::empty_window(handle_))
				restrict::get_drawer(handle_).erase(d);
		}

		void drawing::clear()
		{
			if(API::empty_window(handle_))	return;
			restrict::get_drawer(handle_).clear();
		}
	//end class drawing
}//end namespace nana

