/*
 *	A Drawing Implementation
 *	Nana C++ Library(https://nana.acemind.cn)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/drawing.cpp
 */

#include "detail/basic_window.hpp"
#include <nana/gui/drawing.hpp>
#include <nana/gui/programming_interface.hpp>

namespace nana
{
	//restrict
	//@brief:	This name is only visible for this compiling-unit
	namespace restrict
	{
		namespace
		{
			inline detail::drawer& get_drawer(window wd)
			{
				return wd->drawer;
			}
		}
	}
    
    //class drawing
  		drawing::drawing(window wd)
			:handle_(wd)
  		{
			if (!api::is_window(wd))
				throw std::invalid_argument("drawing: invalid window parameter");

			if (wd->is_draw_through())
				throw std::invalid_argument("drawing: the window is draw_through enabled");
		}
  		
  		drawing::~drawing(){} //Just for polymorphism

		bool drawing::empty() const
		{
			return api::empty_window(handle_) ||  handle_->root_graph->empty();
		}

		void drawing::update() const
		{
			api::refresh_window(handle_);
		}

		void drawing::draw(const draw_fn_t& f)
		{
			internal_scope_guard lock;
			if(api::empty_window(handle_))	return;
			restrict::get_drawer(handle_).drawing(draw_fn_t(f), false);		
		}

		void drawing::draw(draw_fn_t&& f)
		{
			internal_scope_guard lock;
			if(api::empty_window(handle_))	return;
			restrict::get_drawer(handle_).drawing(std::move(f), false);
		}

		drawing::diehard_t drawing::draw_diehard(const draw_fn_t& f)
		{
			internal_scope_guard lock;
			if(api::empty_window(handle_)) return nullptr;
			return reinterpret_cast<diehard_t>(restrict::get_drawer(handle_).drawing(draw_fn_t(f), true));
		}

		drawing::diehard_t drawing::draw_diehard(draw_fn_t&& f)
		{
			internal_scope_guard lock;
			if(api::empty_window(handle_))	return nullptr;
			return reinterpret_cast<diehard_t>(restrict::get_drawer(handle_).drawing(std::move(f), true));
		}

		void drawing::erase(diehard_t d)
		{
			internal_scope_guard lock;
			//Fixed by Tumiz
			//https://github.com/cnjinhao/nana/issues/153
			if(!api::empty_window(handle_))
				restrict::get_drawer(handle_).erase(reinterpret_cast<drawing_handle>(d));
		}

		void drawing::clear()
		{
			internal_scope_guard lock;
			if(api::empty_window(handle_))	return;
			restrict::get_drawer(handle_).clear();
		}
	//end class drawing
}//end namespace nana