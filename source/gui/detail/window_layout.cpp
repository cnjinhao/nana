/*
*	Window Layout Implementation
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file: nana/gui/detail/window_layout.hpp
*
*/

#include "basic_window.hpp"
#include <nana/gui/detail/window_layout.hpp>
#include <nana/gui/detail/native_window_interface.hpp>
#include <nana/gui/layout_utility.hpp>
#include <algorithm>

namespace nana
{
	namespace detail
	{
		//class window_layout
			void window_layout::paint(basic_window* wd, paint_operation operation, bool req_refresh_children)
			{
				if (wd->flags.refreshing && (paint_operation::try_refresh == operation))
					return;

				if (nullptr == wd->effect.bground)
				{
					if ((paint_operation::try_refresh == operation) && (!wd->drawer.graphics.empty()))
					{
						wd->flags.refreshing = true;
						wd->drawer.refresh();
						wd->flags.refreshing = false;
					}
					maproot(wd, (paint_operation::none != operation), req_refresh_children);
				}
				else
					_m_paint_glass_window(wd, (paint_operation::try_refresh == operation), req_refresh_children, false, true);
			}

			bool window_layout::maproot(basic_window* wd, bool have_refreshed, bool req_refresh_children)
			{
				auto check_opaque = wd->seek_non_lite_widget_ancestor();
				if (check_opaque && check_opaque->flags.refreshing)
					return true;

				nana::rectangle vr;
				if (read_visual_rectangle(wd, vr))
				{
					//get the root graphics
					auto& graph = *(wd->root_graph);

					if (category::flags::lite_widget != wd->other.category)
						graph.bitblt(vr, wd->drawer.graphics, nana::point(vr.x - wd->pos_root.x, vr.y - wd->pos_root.y));

					_m_paste_children(wd, have_refreshed, req_refresh_children, vr, graph, nana::point());

					if (wd->parent)
					{
						std::vector<wd_rectangle>	blocks;
						if (read_overlaps(wd, vr, blocks))
						{
							nana::point p_src;
							for (auto & el : blocks)
							{
								p_src.x = el.r.x - el.window->pos_root.x;
								p_src.y = el.r.y - el.window->pos_root.y;
								graph.bitblt(el.r, (el.window->drawer.graphics), p_src);

								_m_paste_children(el.window, false, req_refresh_children, el.r, graph, nana::point{});
							}
						}
					}
					_m_notify_glasses(wd);
					return true;
				}
				return false;
			}

			void window_layout::paste_children_to_graphics(basic_window* wd, nana::paint::graphics& graph)
			{
				_m_paste_children(wd, false, false, rectangle{ wd->pos_root, wd->dimension }, graph, wd->pos_root);
			}

			//read_visual_rectangle
			///@brief 	Reads the visual rectangle of a window, the visual rectangle's reference frame is to root widget,
			///			the visual rectangle is a rectangular block that a window should be displayed on screen.
			///			The result is a rectangle that is a visible area for its ancestors.
			bool window_layout::read_visual_rectangle(basic_window* wd, nana::rectangle& visual)
			{
				if (! wd->displayed())	return false;

				visual = rectangle{ wd->pos_root, wd->dimension };

				if (category::flags::root != wd->other.category)
				{
					//Test if the root widget is overlapped the specified widget
					//the pos of root widget is (0, 0)
					if (overlapped(visual, rectangle{ wd->root_widget->pos_owner, wd->root_widget->dimension }) == false)
						return false;

					for (auto parent = wd->parent; parent; parent = parent->parent)
					{
						if (category::flags::root == parent->other.category)
						{
							wd = parent;
							break;
						}

						if (!overlap(rectangle{ parent->pos_root, parent->dimension }, visual, visual))
							return false;
					}
				}

				//Now, wd actually is the root widget of original parameter wd
				if (nullptr == wd->parent)
					return true;
				
				auto parent_rw = wd->parent->root_widget;
				//visual rectangle of wd's parent
				rectangle vrt_parent{ parent_rw->pos_root, parent_rw->dimension };

				point pos_root;
				while (parent_rw->parent)
				{
					pos_root -= native_interface::window_position(parent_rw->root);

					if (!overlap(rectangle{ pos_root, parent_rw->parent->root_widget->dimension }, vrt_parent, vrt_parent))
						return false;

					parent_rw = parent_rw->parent->root_widget;
				}

				return overlap(vrt_parent, visual, visual);
			}

			//read_overlaps
			//	reads the overlaps that are overlapped a rectangular block
			bool window_layout::read_overlaps(basic_window* wd, const nana::rectangle& vis_rect, std::vector<wd_rectangle>& blocks)
			{
				auto const is_wd_root = (category::flags::root == wd->other.category);
				wd_rectangle block;
				while (wd->parent)
				{
					auto i = std::find(wd->parent->children.cbegin(), wd->parent->children.cend(), wd);
					if (i != wd->parent->children.cend())
					{
						for (++i; i != wd->parent->children.cend(); ++i)
						{
							basic_window* cover = *i;

							if (!cover->visible)
								continue;

							if (is_wd_root)
							{
								if(category::flags::root == cover->other.category)
								{
									if (overlap(vis_rect, rectangle{ native_interface::window_position(cover->root), cover->dimension }, block.r))
									{
										block.window = cover;
										blocks.push_back(block);
									}
								}
							}
							else if((category::flags::root != cover->other.category) && (nullptr == cover->effect.bground))
							{
								if (overlap(vis_rect, rectangle{ cover->pos_root, cover->dimension }, block.r))
								{
									block.window = cover;
									blocks.push_back(block);
								}
							}
						}
					}

					wd = wd->parent;
				}
				return (!blocks.empty());
			}

			bool window_layout::enable_effects_bground(basic_window * wd, bool enabled)
			{
				if (category::flags::widget != wd->other.category)
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
			void window_layout::make_bground(basic_window* const wd)
			{
				nana::point rpos{ wd->pos_root };
				auto & glass_buffer = wd->other.glass_buffer;

				if (category::flags::lite_widget == wd->parent->other.category)
				{
					std::vector<basic_window*> layers;
					auto beg = wd->parent;
					while (beg && (category::flags::lite_widget == beg->other.category))
					{
						layers.push_back(beg);
						beg = beg->parent;
					}

					glass_buffer.bitblt(::nana::rectangle{ wd->dimension }, beg->drawer.graphics, wd->pos_root - beg->pos_root);
					
					nana::rectangle r(wd->pos_owner, wd->dimension);
					for (auto i = layers.rbegin(), layers_rend = layers.rend(); i != layers_rend; ++i)
					{
						auto pre = *i;
						if (false == pre->visible)
							continue;

						auto term = ((i + 1 != layers_rend) ? *(i + 1) : wd);
						r.position(wd->pos_root - pre->pos_root);

						for (auto child : pre->children)
						{
							if (child->index >= term->index)
								break;

							nana::rectangle ovlp;
							if (child->visible && overlap(r, rectangle(child->pos_owner, child->dimension), ovlp))
							{
								if (category::flags::lite_widget != child->other.category)
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
						if (category::flags::lite_widget != child->other.category)
							glass_buffer.bitblt(nana::rectangle{ ovlp.x - wd->pos_owner.x, ovlp.y - wd->pos_owner.y, ovlp.width, ovlp.height }, child->drawer.graphics, {ovlp.position() - child->pos_owner});

						ovlp.x += wd->parent->pos_root.x;
						ovlp.y += wd->parent->pos_root.y;
						_m_paste_children(child, false, false, ovlp, glass_buffer, rpos);
					}
				}

				if (wd->effect.bground)
					wd->effect.bground->take_effect(wd, glass_buffer);
			}

			void window_layout::_m_paste_children(basic_window* wd, bool have_refreshed, bool req_refresh_children, const nana::rectangle& parent_rect, nana::paint::graphics& graph, const nana::point& graph_rpos)
			{
				nana::rectangle rect;
				for (auto child : wd->children)
				{
					//it will not past children if no drawer and visible is false.
					if ((false == child->visible) || ((category::flags::lite_widget != child->other.category) && child->drawer.graphics.empty()))
						continue;

					if (category::flags::root == child->other.category)
					{
						paint(child, (req_refresh_children ? paint_operation::try_refresh : paint_operation::none), req_refresh_children);
						continue;
					}

					if (nullptr == child->effect.bground)
					{
						if (overlap(nana::rectangle{ child->pos_root, child->dimension }, parent_rect, rect))
						{
							if (category::flags::lite_widget != child->other.category)
							{
								if (req_refresh_children && (false == child->flags.refreshing))
								{
									child->flags.refreshing = true;
									child->drawer.refresh();
									child->flags.refreshing = false;
								}

								graph.bitblt(nana::rectangle(rect.x - graph_rpos.x, rect.y - graph_rpos.y, rect.width, rect.height),
									child->drawer.graphics, nana::point(rect.x - child->pos_root.x, rect.y - child->pos_root.y));
							}
							//req_refresh_children determines whether the child has been refreshed, and also determines whether
							//the children of child to be refreshed.
							_m_paste_children(child, req_refresh_children, req_refresh_children, rect, graph, graph_rpos);
						}
					}
					else
					{
						//Update the glass window's background if the parent have_refreshed.
						_m_paint_glass_window(child, have_refreshed, req_refresh_children, true, false);
					}
				}
			}

			void window_layout::_m_paint_glass_window(basic_window* wd, bool is_redraw, bool is_child_refreshed, bool called_by_notify, bool notify_other)
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
						_m_notify_glasses(wd);
				}
			}

			/// Notify the glass windows that are overlapped with the specified visual rectangle.
			/// If a child window of sigwd is a glass window, it doesn't to be notified.
			void window_layout::_m_notify_glasses(basic_window* const sigwd)
			{
				nana::rectangle r_of_sigwd(sigwd->pos_root, sigwd->dimension);
				for (auto wd : data_sect.effects_bground_windows)
				{
					//Don't notify the window if both native root windows are not same(e.g. wd and sigwd have
					//a some parent). Otherwise, _m_paint_glass_window() recursively paints sigwd to make stack overflow.
					//On the other hand, a nested root window is always floating on its parent's child widgets, it's unnecessary to
					//notify the wd if they haven't a same native root window.
					if (sigwd->root != wd->root)
						continue;

					if (wd == sigwd || !wd->displayed() ||
						(false == overlapped(nana::rectangle{ wd->pos_root, wd->dimension }, r_of_sigwd)))
						continue;

					if (sigwd->parent == wd->parent)
					{
						if (sigwd->index >= wd->index)
							continue;
					}
					else if (sigwd != wd->parent)
					{
						using cat_flags = category::flags;

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
							basic_window *p = wd->parent, *signode = sigwd;
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
