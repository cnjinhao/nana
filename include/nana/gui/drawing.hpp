/*
 *	A Drawing Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2014 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/drawing.hpp
 */
#ifndef NANA_GUI_DRAWING_HPP
#define NANA_GUI_DRAWING_HPP
#include "widgets/widget.hpp"
#include "../traits.hpp"
namespace nana
{
	/// \brief Draw pictures on a widget by specifying a drawing method that will be employed everytime the widget refreshes. 
    /// By the end of drawing, the picture may not be displayed immediately. 
    /// If a picture need to be displayed immediately call nana::gui::API::refresh_window() .
	class drawing
		:private nana::noncopyable
	{
	public:
		typedef struct{}* diehard_t;                                ///< A handle to a drawing method
		typedef std::function<void(paint::graphics&)> draw_fn_t;    ///< A function to draw

		drawing(window w);              ///< Create a drawing object for a widget w
		
		virtual ~drawing();             ///< Just for polymorphism

		bool empty() const;             ///< Returns true if the drawing object is invalid. 
		void update() const;

        void draw(const draw_fn_t&);         ///< Draws things that are defined by draw_fn_t.
		void draw(draw_fn_t&&);              ///< Draws things that are defined by draw_fn_t.

                        /// Draws things that are defined by draw_fn_t but will not be deleted when clear() is called.
		diehard_t draw_diehard(const draw_fn_t&);
                        /// Draws things that are defined by draw_fn_t but will not be deleted when clear() is called.
		diehard_t draw_diehard(draw_fn_t&&);
		void erase(diehard_t);              ///< Erases a diehard drawing method.

		void clear();                       ///< Erases all.
	private:
		window handle_;
	};//end class drawing
}//end namespace nana
#endif
