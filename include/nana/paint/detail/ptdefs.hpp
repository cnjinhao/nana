#ifndef NANA_PAINT_PTDEFS_INCLUDED
#define NANA_PAINT_PTDEFS_INCLUDED
#include <string>

namespace nana
{
	namespace detail
	{
		struct native_font_signature;

		
		struct font_style
		{
			unsigned weight{ 400 };	//normal
			bool italic{ false };
			bool underline{ false };
			bool strike_out{ false };
			bool antialiasing{ true };

			font_style() = default;
			font_style(unsigned weight, bool italic = false, bool underline = false, bool strike_out = false);


			font_style& change_weight(unsigned);
			font_style& change_italic(bool);
			font_style& change_underline(bool);
			font_style& change_strikeout(bool);
			font_style& change_antialiasing(bool);
		};
	}//end namespace detail

	namespace paint
	{
		using native_font_type = ::nana::detail::native_font_signature*;

		struct font_info
		{
			std::string family;	///< Font family
			double size_pt;		///< Font Size, in pt.
			nana::detail::font_style style;	///< Font Styles
		};
	}
}

#endif