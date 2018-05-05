/*
 *	A Content View Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2017-2018 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/skeletons/content_view.hpp
 *	@author: Jinhao
 */

#ifndef NANA_WIDGETS_SKELETONS_CONTENT_VIEW_INCLUDED
#define NANA_WIDGETS_SKELETONS_CONTENT_VIEW_INCLUDED

#include <nana/gui/basis.hpp>
#include <functional>
#include <memory>

namespace nana
{
	namespace paint
	{
		class graphics;
	}
}

namespace nana { namespace widgets {
namespace skeletons
{
	class content_view
	{
		struct implementation;

		content_view(const content_view&) = delete;
		content_view& operator=(const content_view&) = delete;

		content_view(content_view&&) = delete;
		content_view& operator=(content_view&&) = delete;
	public:
		using graph_reference = paint::graphics&;

		enum class scrolls
		{
			none, horz, vert, both
		};

		struct events_type
		{
			::std::function<void(const point&)> hover_outside;
			::std::function<void()> scrolled;
		};

		content_view(window handle);
		~content_view();

		events_type& events();

		bool enable_scrolls(scrolls which);

		std::shared_ptr<scroll_operation_interface> scroll_operation() const;

		void step(unsigned step_value, bool horz);
		bool scroll(bool forwards, bool horz);
		bool turn_page(bool forwards, bool horz);

		void disp_area(const rectangle& da, const point& skew_horz_bar, const point& skew_vert_bar, const size& extra_px, bool try_update = true);

		void content_size(const size& sz, bool try_update = true);
		const size& content_size() const;

		const point& origin() const;
		rectangle corner() const;
		void draw_corner(graph_reference);

		rectangle view_area() const;
		rectangle view_area(const size& alt_content_size) const;

		unsigned extra_space(bool horz) const;

		void change_position(int pos, bool aligned, bool horz);

		/// Returns true if the origin is moved
		bool move_origin(const point& skew);

		void sync(bool passive);

		void pursue(const point& cursor);

		void set_wheel_speed(std::function<unsigned()> fn);

		static constexpr unsigned space()
		{
			return 16;
		}
	private:
		implementation* const impl_;
	};
}
}
}

#endif