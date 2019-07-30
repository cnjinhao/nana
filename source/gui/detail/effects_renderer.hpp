/*
*	Effects Renderer
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file: nana/gui/detail/effects_renderer.cpp
*/

#ifndef NANA_GUI_DETAIL_EFFECTS_RENDERER_HPP
#define NANA_GUI_DETAIL_EFFECTS_RENDERER_HPP
#include "basic_window.hpp"
#include <nana/gui/effects.hpp>
#include <nana/paint/graphics.hpp>
#include <nana/paint/pixel_buffer.hpp>
#include <nana/gui/layout_utility.hpp>
#include <nana/gui/detail/window_layout.hpp>

namespace nana{
	namespace detail
	{
		/// Effect edige nimbus renderer
		class edge_nimbus_renderer
		{
			edge_nimbus_renderer() = default;
		public:
			using window_layer = ::nana::detail::window_layout;
			using graph_reference = ::nana::paint::graphics&;

			static edge_nimbus_renderer& instance();

			constexpr unsigned weight() const
			{
				return 2;
			}

			void erase(basic_window* wd);

			void render(basic_window* wd, bool forced, const rectangle* update_area = nullptr);
		private:
			/// Determines whether the effect will be rendered for the given window.
			static bool _m_edge_nimbus(basic_window * const wd, basic_window * const focused_wd);

			void _m_render_edge_nimbus(basic_window* wd, const nana::rectangle & visual);
		};
	}
}//end namespace nana

#endif
