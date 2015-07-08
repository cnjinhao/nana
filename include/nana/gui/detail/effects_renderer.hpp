#ifndef NANA_GUI_DETAIL_EFFECTS_RENDERER_HPP
#define NANA_GUI_DETAIL_EFFECTS_RENDERER_HPP
#include <nana/gui/effects.hpp>
#include <nana/paint/graphics.hpp>
#include <nana/paint/pixel_buffer.hpp>
#include <nana/gui/layout_utility.hpp>

namespace nana{
	namespace detail
	{
		template<typename CoreWindow>
		class edge_nimbus_renderer
		{
			edge_nimbus_renderer() = default;
		public:
			typedef CoreWindow core_window_t;
			typedef window_layout window_layer;
			typedef nana::paint::graphics & graph_reference;

			static edge_nimbus_renderer& instance()
			{
				static edge_nimbus_renderer object;
				return object;
			}

			std::size_t weight() const
			{
				return 2;
			}

			bool render(core_window_t * wd, bool forced)
			{
				bool rendered = false;
				core_window_t * root_wd = wd->root_widget;
				auto & nimbus = root_wd->other.attribute.root->effects_edge_nimbus;

				if(nimbus.size())
				{
					core_window_t * focused = root_wd->other.attribute.root->focus;
					native_window_type native = root_wd->root;
					std::size_t pixels = weight();

					auto graph = root_wd->root_graph;

					std::vector<core_window_t*> erase;
					std::vector<std::pair<rectangle,core_window_t*>>	rd_set;
					nana::rectangle r;
					for(auto & action : nimbus)
					{
						if(_m_edge_nimbus(focused, action.window) && window_layer::read_visual_rectangle(action.window, r))
						{
							if(action.window == wd)
								rendered = true;

							//Avoiding duplicated rendering. If the window is declared to lazy refresh, it should be rendered.
							if ((forced && (action.window == wd)) || !action.rendered || (action.window->other.upd_state == core_window_t::update_state::refresh))
							{
								rd_set.emplace_back(r, action.window);
								action.rendered = true;
							}
						}
						else if(action.rendered)
						{
							action.rendered = false;
							erase.push_back(action.window);
						}
					}

					//Erase
					for(auto el : erase)
					{
						if(el == wd)
							rendered = true;

						r.x = el->pos_root.x - static_cast<int>(pixels);
						r.y = el->pos_root.y - static_cast<int>(pixels);
						r.width = static_cast<unsigned>(el->dimension.width + (pixels << 1));
						r.height = static_cast<unsigned>(el->dimension.height + (pixels << 1));

						graph->paste(native, r, r.x, r.y);
					}

					//Render
					for (auto & rd : rd_set)
						_m_render_edge_nimbus(rd.second, rd.first);
				}
				return rendered;
			}
		private:
			static bool _m_edge_nimbus(core_window_t * focused_wd, core_window_t * wd)
			{
				if((focused_wd == wd) && (static_cast<unsigned>(wd->effect.edge_nimbus) & static_cast<unsigned>(effects::edge_nimbus::active)))
					return true;
				else if((static_cast<unsigned>(wd->effect.edge_nimbus) & static_cast<unsigned>(effects::edge_nimbus::over)) && (wd->flags.action == mouse_action::over))
					return true;
				return false;
			}

			void _m_render_edge_nimbus(core_window_t* wd, const nana::rectangle & visual)
			{
				auto r = visual;
				r.pare_off(-static_cast<int>(weight()));
				rectangle good_r;
				if (overlap(r, rectangle{ wd->root_graph->size() }, good_r))
				{
					if(	(good_r.x < wd->pos_root.x) || (good_r.y < wd->pos_root.y) ||
						(good_r.x + good_r.width > visual.x + visual.width) || (good_r.y + good_r.height > visual.y + visual.height))
					{
						auto graph = wd->root_graph;
						nana::paint::pixel_buffer pixbuf(graph->handle(), r);

						pixel_argb_t px0, px1, px2, px3;
						
						px0 = pixbuf.pixel(0, 0);
						px1 = pixbuf.pixel(r.width - 1, 0);
						px2 = pixbuf.pixel(0, r.height - 1);
						px3 = pixbuf.pixel(r.width - 1, r.height - 1);

						good_r.x = good_r.y = 1;
						good_r.width = r.width - 2;
						good_r.height = r.height - 2;
						pixbuf.rectangle(good_r, wd->scheme->activated.get_color(), 0.95, false);

						good_r.x = good_r.y = 0;
						good_r.width = r.width;
						good_r.height = r.height;
						pixbuf.rectangle(good_r, wd->scheme->activated.get_color(), 0.4, false);

						pixbuf.pixel(0, 0, px0);
						pixbuf.pixel(r.width - 1, 0, px1);
						pixbuf.pixel(0, r.height - 1, px2);
						pixbuf.pixel(r.width - 1, r.height - 1, px3);

						pixbuf.paste(wd->root, r.x, r.y);

						std::vector<typename window_layer::wd_rectangle> overlaps;
						if(window_layer::read_overlaps(wd, visual, overlaps))
						{
							for(auto & wdr : overlaps)
								graph->paste(wd->root, wdr.r, wdr.r.x, wdr.r.y);
						}
					}
					else
						wd->root_graph->paste(wd->root, visual, visual.x, visual.y);
				}
			}
		};
	}
}//end namespace nana

#endif
