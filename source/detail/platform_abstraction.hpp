/*
 *	Platform Abstraction
 *	Nana C++ Library 
 *  Documentation https://nana.acemind.cn/documentation
 *  Sources: https://github.com/cnjinhao/nana
 *	Copyright(C) 2003-2024 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file nana/detail/platform_spec.hpp
 *
 *	The platform_abstraction provides some functions and types for the abstract
 *	system platform.
 */

#ifndef NANA_DETAIL_PLATFORM_ABSTRACTION_HEADER_INCLUDED
#define NANA_DETAIL_PLATFORM_ABSTRACTION_HEADER_INCLUDED

#include "platform_abstraction_types.hpp"
#include <memory>
#include <filesystem>
#include <nana/basic_types.hpp>
#include <nana/gui/basis.hpp>

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

		/// \todo: generalize dpi to v2 awareness

		/// 'manuallay' set the current system DPI, this is used for DPI scaling.
		static void set_current_dpi(int dpi);
		static int current_dpi();

		static int         dpi_scale(int scalar);
		static nana::size  dpi_scale(const nana::size&   size);
		static nana::point dpi_scale(const nana::point& point);

		static int          dpi_scale(window wd, int          scalar);
		static unsigned int dpi_scale(window wd, unsigned int scalar);
		static nana::size   dpi_scale(window wd, nana::size     size);
		static nana::point  dpi_scale(window wd, nana::point   point);

		/// dpi scaling for int
        static int          dpi_scale      (const int  scalar, int dpi);
		static int          dpi_transform  (int&       scalar, int dpi);
		static int          unscale_dpi    (const int  scalar, int dpi);
		static int          untransform_dpi(int&       scalar, int dpi);

		/// dpi scaling for unsigned int
		static unsigned int dpi_scale      (const unsigned int scalar, int dpi);
		static unsigned int dpi_transform  (unsigned int&      scalar, int dpi);
		static unsigned int unscale_dpi    (const unsigned int scalar, int dpi);
		static unsigned int untransform_dpi(unsigned int&      scalar, int dpi);

		/// dpi scaling for nana::size
		static nana::size   dpi_scale      (const nana::size& size, int dpi);
		static nana::size   dpi_transform  (nana::size&       size, int dpi);
		static nana::size   unscale_dpi    (const nana::size& size, int dpi);
		static nana::size   untransform_dpi(nana::size&       size, int dpi);

		/// dpi scaling for nana::point
		static nana::point  dpi_scale      (const nana::point& point, int dpi);
		static nana::point  dpi_transform  (nana::point&       point, int dpi);
		static nana::point  unscale_dpi    (const nana::point& point, int dpi);
		static nana::point  untransform_dpi(nana::point&       point, int dpi);

		/// dpi scaling for nana::rectangle
		static nana::rectangle dpi_scale      (const nana::rectangle& rect, int dpi);
		static nana::rectangle dpi_transform  (nana::rectangle&       rect, int dpi);
		static nana::rectangle unscale_dpi    (const nana::rectangle& rect, int dpi);
		static nana::rectangle untransform_dpi(nana::rectangle&       rect, int dpi);


		/// Open the font, if ttf is specified, it ignores the font family name of font_info and creates the font using truetype file.
		static std::shared_ptr<font> open_font(const font_info&, int dpi, const path_type& ttf);
		static void font_resource(bool try_add, const path_type& ttf);

		static unsigned screen_dpi(bool x_requested);
	};
}

#endif