
#ifndef NANA_WIDGETS_SKELETONS_TEXT_EDITOR_SCHEME_HPP
#define NANA_WIDGETS_SKELETONS_TEXT_EDITOR_SCHEME_HPP

#include "../../detail/widget_colors.hpp"
#include <vector>

namespace nana
{
	namespace widgets
	{
		namespace skeletons
		{
			struct text_editor_scheme
				: public ::nana::widget_colors
			{
				color_proxy selection{static_cast<color_rgb>(0x3399FF)};
				color_proxy selection_text{colors::white};
			};

			class text_editor_event_interface
			{
			public:
				virtual ~text_editor_event_interface() = default;

				virtual void text_position_changed(const std::vector<upoint>&) = 0;
			};
		}
	}
}
#endif