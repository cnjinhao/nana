#ifndef NANA_DETAIL_PLATFORM_ABSTRACTION_TYPES_HEADER_INCLUDED
#define NANA_DETAIL_PLATFORM_ABSTRACTION_TYPES_HEADER_INCLUDED
#include <nana/config.hpp>
#include <nana/paint/detail/ptdefs.hpp>

#include <string>

#ifdef NANA_X11
#	define NANA_USE_XFT
#endif

namespace nana
{
	class font_interface
	{
	public:
		using font_style = detail::font_style;
		using native_font_type = paint::native_font_type;

		virtual ~font_interface() = default;

		virtual native_font_type native_handle() const = 0;

		virtual const paint::font_info& font_info() const = 0;
	};
}

#endif