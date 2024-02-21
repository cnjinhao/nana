/*
 *	Platform Abstraction
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2017-2022 Jinhao(cnjinhao@hotmail.com)
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
#include <filesystem>

namespace nana
{
	class platform_abstraction
	{
	public:
		class revertible_mutex
		{
			revertible_mutex(const revertible_mutex&) = delete;
			revertible_mutex& operator=(const revertible_mutex&) = delete;
			revertible_mutex(revertible_mutex&&) = delete;
			revertible_mutex& operator=(revertible_mutex&&) = delete;
		public:
			revertible_mutex();
			~revertible_mutex();

			void lock();
			bool try_lock();
			void unlock();
			void revert();
			void forward();
		private:
			struct implementation;
			implementation* const impl_;
		};
	public:
		using font = font_interface;
		using font_info = paint::font_info;

		using path_type = ::std::filesystem::path;

		static void initialize();
		/// Shutdown before destruction of platform_spec 
		static void shutdown();

		static revertible_mutex& internal_mutex();

		static double font_default_pt();
		static void font_languages(const std::string&);
		static ::std::shared_ptr<font> default_font(const ::std::shared_ptr<font>&);

		static void set_current_dpi(std::size_t dpi);
		static std::size_t current_dpi();

		/// Open the font, if ttf is specified, it ignores the font family name of font_info and creates the font using truetype file.
		static std::shared_ptr<font> open_font(const font_info&, std::size_t dpi, const path_type& ttf);
		static void font_resource(bool try_add, const path_type& ttf);

		static unsigned screen_dpi(bool x_requested);
	};
}

#endif