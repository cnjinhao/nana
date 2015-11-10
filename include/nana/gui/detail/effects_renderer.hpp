#ifndef NANA_GUI_DETAIL_EFFECTS_RENDERER_HPP
#define NANA_GUI_DETAIL_EFFECTS_RENDERER_HPP
#include <nana/gui/effects.hpp>
#include <nana/paint/graphics.hpp>
#include <nana/paint/pixel_buffer.hpp>
#include <nana/gui/layout_utility.hpp>
#include <nana/gui/detail/window_layout.hpp>

namespace nana{
	namespace detail
	{
		template<typename CoreWindow>
		class edge_nimbus_renderer
		{
			edge_nimbus_renderer() = default;
		public:
			using core_window_t = CoreWindow;
			using window_layer = ::nana::detail::window_layout;
			using graph_reference = ::nana::paint::graphics&;

			static edge_nimbus_renderer& instance()
			{
				static edge_nimbus_renderer object;
				return object;
			}

			unsigned weight() const
			{
				return 2;
			}

			void erase(core_window_t* wd)
			{
				if (effects::edge_nimbus::none == wd->effect.edge_nimbus)
					return;

				core_window_t * root_wd = wd->root_widget;
				auto & nimbus = root_wd->other.attribute.root->effects_edge_nimbus;

				for (auto i = nimbus.begin(); i != nimbus.end(); ++i)
				{
					if (i->window == wd)
					{
						auto pixels = weight();
						rectangle r{wd->pos_root, wd->dimension};
						r.x -= static_cast<int>(pixels);
						r.y -= static_cast<int>(pixels);
						r.width += static_cast<unsigned>(pixels << 1);
						r.height += static_cast<unsigned>(pixels << 1);

						root_wd->root_graph->paste(root_wd->root, r, r.x, r.y);

						nimbus.erase(i);
						break;
					}
				}
			}

			void render(core_window_t * wd, bool forced, const rectangle* update_area = nullptr)
			{
				bool copy_separately = true;
				std::vector<std::pair<rectangle, core_window_t*>>	rd_set;

				if (wd->root_widget->other.attribute.root->effects_edge_nimbus.size())
				{
					auto root_wd = wd->root_widget;

					auto & nimbus = root_wd->other.attribute.root->effects_edge_nimbus;

					auto focused = root_wd->other.attribute.root->focus;

					const unsigned pixels = weight();

					auto graph = root_wd->root_graph;

					nana::rectangle r;
					for(auto & action : nimbus)
					{
						if(_m_edge_nimbus(focused, action.window) && window_layer::read_visual_rectangle(action.window, r))
						{
							if (action.window == wd)
							{
								if (update_area)
									::nana::overlap(*update_area, rectangle(r), r);
								copy_separately = false;
							}

							//Avoiding duplicated rendering. If the window is declared to lazy refresh, it should be rendered.
							if ((forced && (action.window == wd)) || (focused == action.window) || !action.rendered || (action.window->other.upd_state == core_window_t::update_state::refresh))
							{
								rd_set.emplace_back(r, action.window);
								action.rendered = true;
							}
						}
						else if(action.rendered)
						{
							action.rendered = false;

							if (action.window == wd)
								copy_separately = false;

							::nana::rectangle erase_r(
									action.window->pos_root.x - static_cast<int>(pixels),
									action.window->pos_root.y - static_cast<int>(pixels),
									static_cast<unsigned>(action.window->dimension.width + (pixels << 1)),
									static_cast<unsigned>(action.window->dimension.height + (pixels << 1))
								);

							graph->paste(root_wd->root, erase_r, erase_r.x, erase_r.y);
						}
					}
				}

				if (copy_separately)
				{
					rectangle vr;
					if (window_layer::read_visual_rectangle(wd, vr))
					{
						if (update_area)
							::nana::overlap(*update_area, rectangle(vr), vr);
						wd->root_graph->paste(wd->root, vr, vr.x, vr.y);
					}
				}

				//Render
				for (auto & rd : rd_set)
					_m_render_edge_nimbus(rd.second, rd.first);
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
					if ((good_r.x < wd->pos_root.x) || (good_r.y < wd->pos_root.y) || 
						(good_r.right() > visual.right()) || (good_r.bottom() > visual.bottom()))
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

						pixbuf.paste(wd->root, { r.x, r.y });

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
