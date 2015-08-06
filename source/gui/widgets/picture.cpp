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
#include <nana/gui/layout_utility.hpp>
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
					rectangle		valid_area;
					::nana::align	align_horz{ ::nana::align::left };
					::nana::align_v align_vert{ ::nana::align_v::top };
					std::unique_ptr<element::bground> bground;	//If it is not a null ptr, the widget is stretchable mode
					bool			stretchable{ false };		//If it is true, the widget is stretchable mode without changing aspect ratio.
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

				auto graphsize = graph.size();

				auto & backimg = impl_->backimg;

				if (!backimg.bground)
				{
					auto valid_area = backimg.valid_area;
					if (valid_area.empty())
						valid_area = backimg.image.size();

					if (backimg.stretchable)
					{
						auto fit_size = fit_zoom({ valid_area.width, valid_area.height }, graphsize);
						::nana::point pos;

						if (fit_size.width != graphsize.width)
						{
							switch (backimg.align_horz)
							{
							case ::nana::align::left: break;
							case ::nana::align::center:
								pos.x = (int(graphsize.width) - int(fit_size.width)) / 2;
								break;
							case ::nana::align::right:
								pos.x = int(graphsize.width) - int(fit_size.width);
								break;
							}
						}
						else if (fit_size.height != graphsize.height)
						{
							switch (backimg.align_vert)
							{
							case ::nana::align_v::top: break;
							case ::nana::align_v::center:
								pos.y = (int(graphsize.height) - int(fit_size.height)) / 2;
								break;
							case ::nana::align_v::bottom:
								pos.y = int(graphsize.height) - int(fit_size.height);
								break;
							}
						}

						_m_draw_background(fit_size.width, fit_size.height);

						backimg.image.stretch(valid_area, graph, ::nana::rectangle{ pos, fit_size });
					}
					else
					{
						//The point in which position the image to be drawn. 
						::nana::point pos;

						switch (backimg.align_horz)
						{
						case ::nana::align::left: break;
						case ::nana::align::center:
							pos.x = (int(graphsize.width) - int(valid_area.width)) / 2;
							break;
						case ::nana::align::right:
							pos.x = int(graphsize.width) - int(valid_area.width);
							break;
						}

						switch (backimg.align_vert)
						{
						case ::nana::align_v::top: break;
						case ::nana::align_v::center:
							pos.y = (int(graphsize.height) - int(valid_area.height)) / 2;
							break;
						case ::nana::align_v::bottom:
							pos.y = int(graphsize.height) - int(valid_area.height);
							break;
						}

						_m_draw_background(valid_area.width, valid_area.height);

						if ( ! backimg.image.empty())
							backimg.image.paste(valid_area, graph, pos);
					}
				}
				else
				{
					_m_draw_background(graphsize.width, graphsize.height);

					color invalid_clr_for_call;
					backimg.bground->draw(graph, invalid_clr_for_call, invalid_clr_for_call, rectangle{ graphsize }, element_state::normal);
				}

				graph.setsta();
			}

			void drawer::_m_draw_background(unsigned w, unsigned h)
			{
				auto graph = impl_->graph_ptr;

				if (graph && (bground_mode::basic != API::effects_bground_mode(*impl_->wdg_ptr)))
				{
					if (w < graph->size().width || h < graph->size().height /*  .width   ???  */ || impl_->backimg.image.alpha())
					{
						auto & bground = impl_->gradual_bground;
						if (bground.gradual_from.invisible() || bground.gradual_to.invisible())
							graph->rectangle(true, impl_->wdg_ptr->bgcolor());
						else if (bground.gradual_from == bground.gradual_to)
							graph->rectangle(true, bground.gradual_from);
						else
							graph->gradual_rectangle(::nana::rectangle{ graph->size() }, bground.gradual_from, bground.gradual_to, !bground.horizontal);
					}
				}
			}
			//end class drawer
		}//end namespace picture
	}//end namespace drawerbase

	//class picture
		picture::picture(window wd, bool visible)
		{
			create(wd, rectangle(), visible);
		}

		picture::picture(window wd, const nana::rectangle& r, bool visible)
		{
			create(wd, r, visible);
		}

		void picture::load(::nana::paint::image img, const ::nana::rectangle& valid_area)
		{
			internal_scope_guard lock;
			auto& backimg = get_drawer_trigger().impl_->backimg;
			backimg.image = std::move(img);
			backimg.valid_area = valid_area;

			if (backimg.bground)
				backimg.bground->image(backimg.image, true, valid_area);

			if (handle())
			{
				get_drawer_trigger().impl_->graph_ptr->set_changed();
				API::refresh_window(*this);
			}
		}

		void picture::align(::nana::align horz, align_v vert)
		{
			internal_scope_guard lock;

			auto& backimg = get_drawer_trigger().impl_->backimg;

			if (backimg.align_horz == horz && backimg.align_vert == vert)
				return;

			backimg.align_horz = horz;
			backimg.align_vert = vert;

			if (handle())
			{
				get_drawer_trigger().impl_->graph_ptr->set_changed();
				API::refresh_window(*this);
			}
		}

		void picture::stretchable(unsigned left, unsigned top, unsigned right, unsigned bottom)
		{
			if (!handle())
				return;

			internal_scope_guard lock;
			auto & backimg = get_drawer_trigger().impl_->backimg;
			if (!backimg.bground)
			{
				backimg.bground.reset(new element::bground);
				backimg.bground->states({ element_state::normal });
				backimg.bground->image(backimg.image, true, backimg.valid_area);
			}

			backimg.bground->stretch_parts(left, top, right, bottom);
			backimg.stretchable = false;
			if (handle())
			{
				get_drawer_trigger().impl_->graph_ptr->set_changed();
				API::refresh_window(*this);
			}
		}

		void picture::stretchable(bool enables)
		{
			internal_scope_guard lock;

			auto & backimg = get_drawer_trigger().impl_->backimg;
			backimg.bground.reset();

			backimg.stretchable = enables;
			if (handle())
			{
				get_drawer_trigger().impl_->graph_ptr->set_changed();
				API::refresh_window(*this);
			}
		}

		void picture::set_gradual_background(const ::nana::color& from, const ::nana::color& to, bool horizontal)
		{
			auto & bground = get_drawer_trigger().impl_->gradual_bground;
			bground.gradual_from = from;
			bground.gradual_to = to;
			bground.horizontal = horizontal;
			if (handle())
			{
				get_drawer_trigger().impl_->graph_ptr->set_changed();
				API::refresh_window(*this);
			}
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
