/*
 *	A Picture Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/picture.cpp
 *	@description:
 *		Used for showing a picture
 */

#include <nana/gui/widgets/picture.hpp>
#include <nana/paint/image.hpp>
#include <nana/gui/element.hpp>

namespace nana
{
	namespace drawerbase
	{
		namespace picture
		{
			struct implement
			{
				widget* wdg_ptr{nullptr};
				paint::graphics* graph_ptr{nullptr};


				struct gradual_bground_tag
				{
					::nana::color gradual_from;
					::nana::color gradual_to;
					bool	horizontal{true};
				}gradual_bground;

				struct back_image_tag
				{
					paint::image	image;
					std::unique_ptr<element::bground> bground;
				}backimg;
			};

			//class drawer
			drawer::drawer() :impl_(new implement)
			{
			}

			drawer::~drawer()
			{
				delete impl_;
			}

			void drawer::attached(widget_reference& wdg, graph_reference graph)
			{
				impl_->wdg_ptr = &wdg;
				impl_->graph_ptr = &graph;
			}

			void drawer::refresh(graph_reference graph)
			{
				if (!graph.changed())
					return;

				if (!impl_->backimg.bground)
				{
					_m_draw_background();
					impl_->backimg.image.paste(graph, 0, 0);
				}
				else
					impl_->backimg.bground->draw(graph, {}, {}, graph.size(), element_state::normal);

				graph.setsta();
			}

			void drawer::_m_draw_background()
			{
				auto graph = impl_->graph_ptr;
				if (graph && (bground_mode::basic != API::effects_bground_mode(*impl_->wdg_ptr)))
				{
					auto & bground = impl_->gradual_bground;
					if (bground.gradual_from.invisible() || bground.gradual_to.invisible())
						graph->rectangle(true, impl_->wdg_ptr->bgcolor());
					else if (bground.gradual_from == bground.gradual_to)
						graph->rectangle(true, bground.gradual_from);
					else
						graph->gradual_rectangle(graph->size(), bground.gradual_from, bground.gradual_to, !bground.horizontal);
				}
			}
			//end class drawer
		}//end namespace picture
	}//end namespace drawerbase

	//class picture
		picture::picture(){}

		picture::picture(window wd, bool visible)
		{
			create(wd, rectangle(), visible);
		}

		picture::picture(window wd, const nana::rectangle& r, bool visible)
		{
			create(wd, r, visible);
		}

		void picture::load(nana::paint::image img)
		{
			auto& backimg = get_drawer_trigger().impl_->backimg;
			backimg.image = std::move(img);

			if (backimg.bground)
				backimg.bground->image(backimg.image, true, {});

			get_drawer_trigger().impl_->graph_ptr->set_changed();
			API::refresh_window(*this);
		}

		void picture::stretchable(unsigned left, unsigned top, unsigned right, unsigned bottom)
		{
			if (!handle())
				return;

			auto & backimg = get_drawer_trigger().impl_->backimg;
			if (!backimg.bground)
			{
				backimg.bground.reset(new element::bground);
				backimg.bground->states({ element_state::normal });
				backimg.bground->image(backimg.image, true, {});
			}

			backimg.bground->stretch_parts(left, top, right, bottom);
			get_drawer_trigger().impl_->graph_ptr->set_changed();
			API::refresh_window(*this);
		}

		void picture::set_gradual_background(const ::nana::color& from, const ::nana::color& to, bool horizontal)
		{
			auto & bground = get_drawer_trigger().impl_->gradual_bground;
			bground.gradual_from = from;
			bground.gradual_to = to;
			bground.horizontal = horizontal;
			get_drawer_trigger().impl_->graph_ptr->set_changed();
			API::refresh_window(*this);
		}

		void picture::transparent(bool enabled)
		{
			if(enabled)
				API::effects_bground(*this, effects::bground_transparent(0), 0.0);
			else
				API::effects_bground_remove(*this);
		}

		bool picture::transparent() const
		{
			return (bground_mode::basic == API::effects_bground_mode(*this));
		}
	//end class picture
}//end namespace nana
