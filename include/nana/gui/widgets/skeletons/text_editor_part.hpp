
#ifndef NANA_WIDGETS_SKELETONS_TEXT_EDITOR_SCHEME_HPP
#define NANA_WIDGETS_SKELETONS_TEXT_EDITOR_SCHEME_HPP

#include "../../detail/widget_geometrics.hpp"
#include <vector>

namespace nana
{
	namespace widgets
	{
		namespace skeletons
		{
			enum class text_focus_behavior
			{
				none,
				select,
				select_if_tabstop,
				select_if_click,
				select_if_tabstop_or_click
			};

			//forward declaration
			class text_editor;

			struct text_editor_scheme
				: public ::nana::widget_geometrics
			{
				color_proxy selection{static_cast<color_rgb>(0x3399FF)};
				color_proxy selection_text{colors::white};

				parameters::mouse_wheel mouse_wheel;	///< The number of lines/characters to scroll when the vertical/horizontal mouse wheel is moved.
			};

			class text_editor_event_interface
			{
			public:
				virtual ~text_editor_event_interface() = default;

				virtual void text_exposed(const std::vector<upoint>&) = 0;
			};
		}
	}
}
#endif