#ifndef NANA_PAINT_PTDEFS_INCLUDED
#define NANA_PAINT_PTDEFS_INCLUDED

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

			font_style() = default;
			font_style(unsigned weight, bool italic = false, bool underline = false, bool strike_out = false);
		};
	}//end namespace detail

	namespace paint
	{
		using native_font_type = ::nana::detail::native_font_signature*;
	}
}

#endif