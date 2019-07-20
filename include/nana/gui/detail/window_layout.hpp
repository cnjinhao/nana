/*
 *	Window Layout Implementation
 *	Copyright(C) 2003-2017 Jinhao(cnjinhao@hotmail.com)
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
#include <nana/push_ignore_diagnostic>

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
		struct wd_rectangle
		{
			basic_window * window;
			rectangle r;
		};

		enum class paint_operation {
			none,
			have_refreshed,
			try_refresh
		};
	public:
		static void paint(basic_window*, paint_operation, bool request_refresh_children);

		static bool maproot(basic_window*, bool have_refreshed, bool request_refresh_children);

		static void paste_children_to_graphics(basic_window*, nana::paint::graphics& graph);

		//read_visual_rectangle
		//@brief:	Reads the visual rectangle of a window, the visual rectangle's reference frame is to root widget,
		//			the visual rectangle is a rectangular block that a window should be displayed on screen.
		//			The result is a rectangle that is a visible area for its ancesters.
		static bool read_visual_rectangle(basic_window*, nana::rectangle& visual);

		//read_overlaps
		//	reads the overlaps that are overlapped a rectangular block
		static bool read_overlaps(basic_window*, const nana::rectangle& vis_rect, std::vector<wd_rectangle>& blocks);

		static bool enable_effects_bground(basic_window *, bool enabled);

		//make_bground
		//		update the glass buffer of a glass window.
		static void make_bground(basic_window* const);
	private:

		/// _m_paste_children
		/**
		 * Pastes children window to the root graphics directly. just paste the visual rectangle
		 * @param window A handle to the window whose child windows will be pasted to the graph.
		 * @param has_refreshed Indicates whethere the window has been refreshed.
		 * @param request_refresh_children A flag indicates whether to refresh its child windows.
		 * @param parent_rect  The child windows which are overlapped with the rectangle will be pasted
		 * @param graph A graphics object to which the child windows are pasted.
		 * @param graph_rpos The reference point to the graph.
		 */
		static void _m_paste_children(basic_window* window, bool has_refreshed, bool request_refresh_children, const nana::rectangle& parent_rect, nana::paint::graphics& graph, const nana::point& graph_rpos);

		static void _m_paint_glass_window(basic_window*, bool is_redraw, bool is_child_refreshed, bool called_by_notify, bool notify_other);

		//Notify the windows which have brground to update their background buffer.
		static void _m_notify_glasses(basic_window* const sigwd);
	private:
		struct data_section
		{
			std::vector<basic_window*> 	effects_bground_windows;
		};
		static data_section	data_sect;
	};//end class window_layout
}//end namespace detail
}//end namespace nana

#include <nana/pop_ignore_diagnostic>

#endif //NANA_GUI_DETAIL_WINDOW_LAYOUT_HPP

