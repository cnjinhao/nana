
#ifndef NANA_WIDGETS_SKELETONS_TEXT_EDITOR_SCHEME_HPP
#define NANA_WIDGETS_SKELETONS_TEXT_EDITOR_SCHEME_HPP

#include "../../detail/widget_colors.hpp"

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
		}
	}
}
#endif