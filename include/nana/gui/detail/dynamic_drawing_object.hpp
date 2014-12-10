/*
 *	Dynamic Drawing Object Implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/detail/dynamic_drawing_object.hpp
 *
 *	!DON'T INCLUDE THIS HEADER FILE IN YOUR SOURCE CODE
 */

#ifndef NANA_GUI_DETAIL_DYNAMIC_DRAWING_OBJECT_HPP
#define NANA_GUI_DETAIL_DYNAMIC_DRAWING_OBJECT_HPP

#include <nana/paint/graphics.hpp>
#include <nana/paint/image.hpp>

namespace nana
{
	namespace detail
	{
		namespace dynamic_drawing
		{
			class object
			{
				object(const object&) = delete;
				object& operator=(const object&) = delete;
			public:
				object(){}
				virtual ~object(){}

				virtual bool diehard() const
				{
					return false;
				}

				virtual void draw(nana::paint::graphics&) const = 0;
			};

			//user_draw_function
			class user_draw_function
				: public object
			{
			public:
				user_draw_function(std::function<void(paint::graphics&)> && f, bool diehard)
					: diehard_(diehard), fn_(std::move(f))
				{}

				void draw(paint::graphics& graph) const
				{
					fn_(graph);
				}

				bool diehard() const
				{
					return diehard_;
				}
			private:
				bool diehard_;
				std::function<void(paint::graphics&)> fn_;
			};
		}//end namespace dynamic_drawing
	}
}//end namespace nana

#endif

