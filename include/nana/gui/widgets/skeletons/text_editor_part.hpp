
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
				color_proxy selection{ static_cast<color_rgb>(0x3399FF) };
				color_proxy selection_unfocused{ static_cast<color_rgb>(0xF0F0F0) };
				color_proxy selection_text{colors::white};

				parameters::mouse_wheel mouse_wheel;	///< The number of lines/characters to scroll when the vertical/horizontal mouse wheel is moved.
			};

			class text_editor_event_interface
			{
			public:
				virtual ~text_editor_event_interface() = default;

				virtual void text_exposed(const std::vector<upoint>&) = 0;
			};

			struct colored_area_type
			{
				const ::std::size_t begin;	///< The begin line position
				::std::size_t count;		///< The number of lines

				::nana::color bgcolor;
				::nana::color fgcolor;
			};

			class colored_area_access_interface
			{
			public:
				using colored_area_type = skeletons::colored_area_type;

				virtual ~colored_area_access_interface();

				virtual std::shared_ptr<colored_area_type> get(std::size_t line_pos) = 0;
				virtual bool clear() = 0;
				virtual bool remove(std::size_t line_pos) = 0;
				virtual std::size_t size() const = 0;
				virtual std::shared_ptr<colored_area_type> at(std::size_t index) = 0;
			};
		}
	}
}
#endif