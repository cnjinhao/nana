/*
 *	A Picture Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/picture.hpp
 *
 *	Used for showing a picture
 */
#ifndef NANA_GUI_WIDGET_PICTURE_HPP
#define NANA_GUI_WIDGET_PICTURE_HPP
#include "widget.hpp"

namespace nana
{
	namespace drawerbase
	{
		namespace picture
		{
			struct implement;

			class drawer : public drawer_trigger
			{
			public:
				drawer();
				~drawer();
				void attached(widget_reference, graph_reference)	override;
			private:
				void refresh(graph_reference)	override;
				void _m_draw_background();
			public:
				implement * const impl_;
			};
		}//end namespace picture
	}//end namespace drawerbase

       /// Rectangle area for displaying a bitmap file
	class picture
		: public widget_object<category::widget_tag, drawerbase::picture::drawer>
	{
	public:
		picture();
		picture(window, bool visible);
		picture(window, const rectangle& ={}, bool visible = true);

		void load(nana::paint::image);

		void stretchable(unsigned left, unsigned top, unsigned right, unsigned bottom);

        /// Fills a gradual-change color in background. If One of colors is invisible or clr_from is equal to clr_to, it draws background in bgcolor.
		void set_gradual_background(const ::nana::color& clr_from, const ::nana::color& clr_to, bool horizontal);
		void transparent(bool);
		bool transparent() const;
	};
}//end namespace nana
#endif
