/**
 *	A Picture Implementation
 *	Nana C++ Library(https://nana.acemind.cn)
 *	Copyright(C) 2003-2020 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/picture.hpp
 *
 *	@brief Used for showing a picture
 */
#ifndef NANA_GUI_WIDGET_PICTURE_HPP
#define NANA_GUI_WIDGET_PICTURE_HPP
#include <nana/push_ignore_diagnostic>

#include "widget.hpp"

namespace nana
{
	class picture;

	namespace drawerbase::picture
	{
			struct implement;

			class drawer : public drawer_trigger
			{
				friend class ::nana::picture;
			public:
				drawer();
				~drawer();
				void attached(widget_reference, graph_reference)	override;
			private:
				void refresh(graph_reference)	override;
			private:
				implement * const impl_;
			};
	}//end namespace drawerbase

       /// Rectangle area for displaying an image
	class picture
		: public widget_object<category::widget_tag, drawerbase::picture::drawer>
	{
	public:
		picture() = default;
		picture(window, const rectangle& ={}, bool visible = true);

		bool load(::nana::paint::image, const rectangle& valid_area = {});
		void clear();

		/// Sets the align of image.
		void align(align, align_v);

		/// Enables the image to be stretched to the widget size.
		void stretchable(unsigned left, unsigned top, unsigned right, unsigned bottom);

		/// Enables/disables the image to be stretched without changing aspect ratio.
		void stretchable(bool);

        /// Fills a gradual-change color in background. If one of colors is invisible or clr_from is equal to clr_to, it draws background in bgcolor.
		void set_gradual_background(const color& clr_from, const color& clr_to, bool horizontal);
		void transparent(bool);
		bool transparent() const;
	};
}//end namespace nana
#include <nana/pop_ignore_diagnostic>

#endif
