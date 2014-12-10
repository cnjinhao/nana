/*
 *	Window Layout Implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/detail/window_layout.hpp
 *
 */

#ifndef NANA_GUI_DETAIL_WINDOW_LAYOUT_HPP
#define NANA_GUI_DETAIL_WINDOW_LAYOUT_HPP

#include <nana/gui/basis.hpp>
#include <vector>

namespace nana
{
	namespace paint
	{
		class image;
		class graphics;
	}
}

namespace nana{
namespace detail
{
	struct basic_window;

	//class window_layout
	class window_layout
	{
	public:
		typedef basic_window core_window_t;

		struct wd_rectangle
		{
			core_window_t * window;
			rectangle r;
		};
	public:
		static void paint(core_window_t*, bool is_redraw, bool is_child_refreshed);

		static bool maproot(core_window_t*, bool have_refreshed, bool is_child_refreshed);

		static void paste_children_to_graphics(core_window_t*, nana::paint::graphics& graph);

		//read_visual_rectangle
		//@brief:	Reads the visual rectangle of a window, the visual rectangle's reference frame is to root widget,
		//			the visual rectangle is a rectangular block that a window should be displayed on screen.
		//			The result is a rectangle that is a visible area for its ancesters.
		static bool read_visual_rectangle(core_window_t*, nana::rectangle& visual);

		//read_overlaps
		//	reads the overlaps that are overlapped a rectangular block
		static bool read_overlaps(core_window_t*, const nana::rectangle& vis_rect, std::vector<wd_rectangle>& blocks);

		static bool enable_effects_bground(core_window_t *, bool enabled);

		//make_bground
		//		update the glass buffer of a glass window.
		static void make_bground(core_window_t* const);
	private:

		//_m_paste_children
		//@brief:paste children window to the root graphics directly. just paste the visual rectangle
		static void _m_paste_children(core_window_t*, bool is_child_refreshed, bool have_refreshed, const nana::rectangle& parent_rect, nana::paint::graphics& graph, const nana::point& graph_rpos);

		static void _m_paint_glass_window(core_window_t*, bool is_redraw, bool is_child_refreshed, bool called_by_notify, bool notify_other);

		//_m_notify_glasses
		//@brief:	Notify the glass windows that are overlapped with the specified vis_rect
		static void _m_notify_glasses(core_window_t* const sigwd, const nana::rectangle& r_visual);
	private:
		struct data_section
		{
			std::vector<core_window_t*> 	effects_bground_windows;
		};
		static data_section	data_sect;
	};//end class window_layout
}//end namespace detail
}//end namespace nana

#endif //NANA_GUI_DETAIL_WINDOW_LAYOUT_HPP

