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

#include <nana/push_ignore_diagnostic>
#include "widgets/widget.hpp"
#include "../traits.hpp"
namespace nana
{
	/// \brief Draw pictures on a widget by specifying a drawing method that will be employed every time the widget refreshes. 
    /// By the end of drawing, the picture may not be displayed immediately. 
    /// If a picture need to be displayed immediately call nana::gui::API::refresh_window() .
	class drawing
		:private nana::noncopyable
	{
		struct draw_fn_handle;
	public:
		using diehard_t = draw_fn_handle * ;						///< A handle to a drawing method
		using draw_fn_t = std::function<void(paint::graphics&)>;    ///< A function to draw

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

#include <nana/pop_ignore_diagnostic>
#endif
