/*
*	Window Layout Implementation
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file: nana/gui/detail/window_layout.hpp
*
*/

#include <nana/gui/detail/window_layout.hpp>
#include <nana/gui/detail/basic_window.hpp>
#include <nana/gui/detail/native_window_interface.hpp>
#include <nana/gui/layout_utility.hpp>
#include <algorithm>

namespace nana
{
	namespace detail
	{
		//class window_layout
			void window_layout::paint(core_window_t* wd, bool is_redraw, bool is_child_refreshed)
			{
				if (wd->flags.refreshing)
					return;

				if (nullptr == wd->effect.bground)
				{
					if (is_redraw && (!wd->drawer.graphics.empty()))
					{
						wd->flags.refreshing = true;
						wd->drawer.refresh();
						wd->flags.refreshing = false;
					}
					maproot(wd, is_redraw, is_child_refreshed);
				}
				else
					_m_paint_glass_window(wd, is_redraw, is_child_refreshed, false, true);
			}

			bool window_layout::maproot(core_window_t* wd, bool have_refreshed, bool is_child_refreshed)
			{
				auto check_opaque = wd->seek_non_lite_widget_ancestor();
				if (check_opaque && check_opaque->flags.refreshing)
					return true;

				nana::rectangle vr;
				if (read_visual_rectangle(wd, vr))
				{
					//get the root graphics
					auto& graph = *(wd->root_graph);

					if (wd->other.category != category::lite_widget_tag::value)
						graph.bitblt(vr, wd->drawer.graphics, nana::point(vr.x - wd->pos_root.x, vr.y - wd->pos_root.y));

					_m_paste_children(wd, is_child_refreshed, have_refreshed, vr, graph, nana::point());

					if (wd->parent)
					{
						std::vector<wd_rectangle>	blocks;
						if (read_overlaps(wd, vr, blocks))
						{
							nana::point p_src;
							for (auto & el : blocks)
							{
								if (el.window->other.category == category::frame_tag::value)
								{
									native_window_type container = el.window->other.attribute.frame->container;
									native_interface::refresh_window(container);
									graph.bitblt(el.r, container);
								}
								else
								{
									p_src.x = el.r.x - el.window->pos_root.x;
									p_src.y = el.r.y - el.window->pos_root.y;
									graph.bitblt(el.r, (el.window->drawer.graphics), p_src);
								}

								_m_paste_children(el.window, is_child_refreshed, false, el.r, graph, nana::point{});
							}
						}
					}
					_m_notify_glasses(wd, vr);
					return true;
				}
				return false;
			}

			void window_layout::paste_children_to_graphics(core_window_t* wd, nana::paint::graphics& graph)
			{
				_m_paste_children(wd, false, false, rectangle{ wd->pos_root, wd->dimension }, graph, wd->pos_root);
			}

			//read_visual_rectangle
			//@brief:	Reads the visual rectangle of a window, the visual rectangle's reference frame is to root widget,
			//			the visual rectangle is a rectangular block that a window should be displayed on screen.
			//			The result is a rectangle that is a visible area for its ancesters.
			bool window_layout::read_visual_rectangle(core_window_t* wd, nana::rectangle& visual)
			{
				if (! wd->displayed())	return false;

				visual = rectangle{ wd->pos_root, wd->dimension };

				if (wd->root_widget != wd)
				{
					//Test if the root widget is overlapped the specified widget
					//the pos of root widget is (0, 0)
					if (overlap(visual, rectangle{ wd->root_widget->pos_owner, wd->root_widget->dimension }) == false)
						return false;
				}

				for (auto* parent = wd->parent; parent; parent = parent->parent)
				{
					overlap(rectangle{ parent->pos_root, parent->dimension }, visual, visual);
				}

				return true;
			}

			//read_overlaps
			//	reads the overlaps that are overlapped a rectangular block
			bool window_layout::read_overlaps(core_window_t* wd, const nana::rectangle& vis_rect, std::vector<wd_rectangle>& blocks)
			{
				wd_rectangle block;
				while (wd->parent)
				{
					auto & siblings = wd->parent->children;
					//It should be checked that whether the window is still a chlid of its parent.
					if (siblings.size())
					{
						auto i = &(siblings[0]);
						auto *end = i + siblings.size();
						i = std::find(i, end, wd);
						if (i != end)
						{
							//find the widget that next to wd.
							for (++i; i < end; ++i)
							{
								core_window_t* cover = *i;
								if ((category::flags::root != cover->other.category) && cover->visible && (nullptr == cover->effect.bground))
								{
									if (overlap(vis_rect, rectangle{ cover->pos_root, cover->dimension }, block.r))
									{
										block.window = cover;
										blocks.push_back(block);
									}
								}
							}
						}
					}
					wd = wd->parent;
				}
				return (!blocks.empty());
			}

			bool window_layout::enable_effects_bground(core_window_t * wd, bool enabled)
			{
				if (wd->other.category != category::widget_tag::value)
					return false;

				if (false == enabled)
				{
					delete wd->effect.bground;
					wd->effect.bground = nullptr;
					wd->effect.bground_fade_rate = 0;
				}

				//Find the window whether it is registered for the bground effects
				auto i = std::find(data_sect.effects_bground_windows.begin(), data_sect.effects_bground_windows.end(), wd);
				if (i != data_sect.effects_bground_windows.end())
				{
					//If it has already registered, do nothing.
					if (enabled)
						return false;

					//Disable the effect.
					data_sect.effects_bground_windows.erase(i);
					wd->other.glass_buffer.release();
					return true;
				}
				//No such effect has registered.
				if (false == enabled)
					return false;

				//Enable the effect.
				data_sect.effects_bground_windows.push_back(wd);
				wd->other.glass_buffer.make(wd->dimension);
				make_bground(wd);
				return true;
			}

			//make_bground
			//		update the glass buffer of a glass window.
			void window_layout::make_bground(core_window_t* const wd)
			{
				nana::point rpos{ wd->pos_root };
				auto & glass_buffer = wd->other.glass_buffer;

				if (wd->parent->other.category == category::lite_widget_tag::value)
				{
					std::vector<core_window_t*> layers;
					core_window_t * beg = wd->parent;
					while (beg && (beg->other.category == category::lite_widget_tag::value))
					{
						layers.push_back(beg);
						beg = beg->parent;
					}

					glass_buffer.bitblt(::nana::rectangle{ wd->dimension }, beg->drawer.graphics, wd->pos_root - beg->pos_root);
					
					nana::rectangle r(wd->pos_owner, wd->dimension);
					for (auto i = layers.rbegin(), layers_rend = layers.rend(); i != layers_rend; ++i)
					{
						core_window_t * pre = *i;
						if (false == pre->visible)
							continue;

						core_window_t * term = ((i + 1 != layers_rend) ? *(i + 1) : wd);
						r.x = wd->pos_root.x - pre->pos_root.x;
						r.y = wd->pos_root.y - pre->pos_root.y;
						for (auto child : pre->children)
						{
							if (child->index >= term->index)
								break;

							nana::rectangle ovlp;
							if (child->visible && overlap(r, rectangle(child->pos_owner, child->dimension), ovlp))
							{
								if (child->other.category != category::lite_widget_tag::value)
									glass_buffer.bitblt(nana::rectangle(ovlp.x - pre->pos_owner.x, ovlp.y - pre->pos_owner.y, ovlp.width, ovlp.height), child->drawer.graphics, nana::point(ovlp.x - child->pos_owner.x, ovlp.y - child->pos_owner.y));
								ovlp.x += pre->pos_root.x;
								ovlp.y += pre->pos_root.y;
								_m_paste_children(child, false, false, ovlp, glass_buffer, rpos);
							}
						}
					}
				}
				else
					glass_buffer.bitblt(::nana::rectangle{ wd->dimension }, wd->parent->drawer.graphics, wd->pos_owner);

				const rectangle r_of_wd{ wd->pos_owner, wd->dimension };
				for (auto child : wd->parent->children)
				{
					if (child->index >= wd->index)
						break;

					nana::rectangle ovlp;
					if (child->visible && overlap(r_of_wd, rectangle{ child->pos_owner, child->dimension }, ovlp))
					{
						if (child->other.category != category::lite_widget_tag::value)
							glass_buffer.bitblt(nana::rectangle{ ovlp.x - wd->pos_owner.x, ovlp.y - wd->pos_owner.y, ovlp.width, ovlp.height }, child->drawer.graphics, nana::point(ovlp.x - child->pos_owner.x, ovlp.y - child->pos_owner.y));

						ovlp.x += wd->pos_root.x;
						ovlp.y += wd->pos_root.y;
						_m_paste_children(child, false, false, ovlp, glass_buffer, rpos);
					}
				}

				if (wd->effect.bground)
					wd->effect.bground->take_effect(reinterpret_cast<window>(wd), glass_buffer);
			}

			//_m_paste_children
			//@brief:paste children window to the root graphics directly. just paste the visual rectangle
			void window_layout::_m_paste_children(core_window_t* wd, bool is_child_refreshed, bool have_refreshed, const nana::rectangle& parent_rect, nana::paint::graphics& graph, const nana::point& graph_rpos)
			{
				nana::rectangle rect;
				for (auto child : wd->children)
				{
					//it will not past children if no drawer and visible is false.
					if ((false == child->visible) || ((category::flags::lite_widget != child->other.category) && child->drawer.graphics.empty()))
						continue;

					if (category::flags::root == child->other.category)
					{
						paint(child, is_child_refreshed, is_child_refreshed);
						continue;
					}

					if (nullptr == child->effect.bground)
					{
						if (overlap(nana::rectangle{ child->pos_root, child->dimension }, parent_rect, rect))
						{
							bool have_child_refreshed = false;
							if (child->other.category != category::lite_widget_tag::value)
							{
								if (is_child_refreshed && (false == child->flags.refreshing))
								{
									have_child_refreshed = true;
									child->flags.refreshing = true;
									child->drawer.refresh();
									child->flags.refreshing = false;
								}

								graph.bitblt(nana::rectangle(rect.x - graph_rpos.x, rect.y - graph_rpos.y, rect.width, rect.height),
									child->drawer.graphics, nana::point(rect.x - child->pos_root.x, rect.y - child->pos_root.y));
							}
							_m_paste_children(child, is_child_refreshed, have_child_refreshed, rect, graph, graph_rpos);
						}
					}
					else
					{
						//If have_refreshed, the glass should be notified.
						_m_paint_glass_window(child, false, is_child_refreshed, have_refreshed, false);
					}
				}
			}

			void window_layout::_m_paint_glass_window(core_window_t* wd, bool is_redraw, bool is_child_refreshed, bool called_by_notify, bool notify_other)
			{
				//A window which has an empty graphics(and lite-widget) does not notify
				//glass windows for updating their background.
				if ((wd->flags.refreshing && is_redraw) || wd->drawer.graphics.empty())
					return;

				nana::rectangle vr;
				if (read_visual_rectangle(wd, vr))
				{
					if (is_redraw || called_by_notify)
					{
						//The background is made by more than calling by notification(such as redraw of parent,
						//redraw of siblings which are covered by wd), sometimes it should be remade when an attribute
						//of the wd is changed(such as its background color is changed).
						if (called_by_notify || wd->flags.make_bground_declared)
						{
							make_bground(wd);
							wd->flags.make_bground_declared = false;
						}

						wd->flags.refreshing = true;
						wd->drawer.refresh();
						wd->flags.refreshing = false;
					}

					auto & root_graph = *(wd->root_graph);
					//Map root
					root_graph.bitblt(vr, wd->drawer.graphics, nana::point(vr.x - wd->pos_root.x, vr.y - wd->pos_root.y));
					_m_paste_children(wd, is_child_refreshed, (is_redraw || called_by_notify), vr, root_graph, nana::point());

					if (wd->parent)
					{
						std::vector<wd_rectangle>	blocks;
						read_overlaps(wd, vr, blocks);
						for (auto & n : blocks)
						{
							root_graph.bitblt(n.r, (n.window->drawer.graphics), nana::point(n.r.x - n.window->pos_root.x, n.r.y - n.window->pos_root.y));
						}
					}

					if (notify_other)
						_m_notify_glasses(wd, vr);
				}
			}

			//_m_notify_glasses
			//@brief:	Notify the glass windows that are overlapped with the specified vis_rect
			void window_layout::_m_notify_glasses(core_window_t* const sigwd, const nana::rectangle& r_visual)
			{
				typedef category::flags cat_flags;

				nana::rectangle r_of_sigwd(sigwd->pos_root, sigwd->dimension);
				for (auto wd : data_sect.effects_bground_windows)
				{
					if (wd == sigwd || !wd->displayed() ||
						(false == overlap(nana::rectangle{ wd->pos_root, wd->dimension }, r_of_sigwd)))
						continue;

					if (sigwd->parent == wd->parent)
					{
						if (sigwd->index >= wd->index)
							continue;
					}
					else if (sigwd != wd->parent)
					{
						if (wd->parent && (cat_flags::lite_widget == wd->parent->other.category))
						{
							//Test if sigwd is an ancestor of the glass window, and there are lite widgets
							//between sigwd and glass window.
							auto ancestor = wd->parent->parent;
							while (ancestor && (ancestor != sigwd) && (cat_flags::lite_widget == ancestor->other.category))
								ancestor = ancestor->parent;

							if ((ancestor != sigwd) || (cat_flags::lite_widget == ancestor->other.category))
								continue;
						}
						else
						{
							//test if sigwnd is a parent of glass window x, or a slibing of the glass window, or a child of the slibing of the glass window.
							core_window_t *p = wd->parent, *signode = sigwd;
							while (signode->parent && (signode->parent != p))
								signode = signode->parent;

							if ((!signode->parent) || (signode->index >= wd->index))
								continue;
						}
					}
					else
						continue;

					_m_paint_glass_window(wd, true, false, true, true);
				}
			}
		//end class window_layout

		window_layout::data_section window_layout::data_sect;
	}//end namespace detail
}//end namespace nana
