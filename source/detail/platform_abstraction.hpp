/*
 *	Platform Abstraction
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2017-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/detail/platform_spec.hpp
 *
 *	The platform_abstraction provides some functions and types for the abstract
 *	system platform.
 */
#ifndef NANA_DETAIL_PLATFORM_ABSTRACTION_HEADER_INCLUDED
#define NANA_DETAIL_PLATFORM_ABSTRACTION_HEADER_INCLUDED

#include "platform_abstraction_types.hpp"
#include <memory>
#include <nana/filesystem/filesystem.hpp>

namespace nana
{


	class platform_abstraction
	{
	public:
		using font = font_interface;

		using path_type = ::std::filesystem::path;

		static void initialize();
		/// Shutdown before destruction of platform_spec 
		static void shutdown();
		static double font_default_pt();
		static void font_languages(const std::string&);
		static ::std::shared_ptr<font> default_font(const ::std::shared_ptr<font>&);
		static ::std::shared_ptr<font> make_font(const ::std::string& font_family, double size_pt, const font::font_style& fs);
		static ::std::shared_ptr<font> make_font_from_ttf(const path_type& ttf, double size_pt, const font::font_style& fs);
		static void font_resource(bool try_add, const path_type& ttf);

		static unsigned screen_dpi(bool x_requested);
	};
}

#endif