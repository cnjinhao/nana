/*
*	Color Schemes
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2014 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file: nana/gui/color_schemes.hpp
*	@description:
*/
#ifndef NANA_DETAIL_COLOR_SCHEMES_HPP
#define NANA_DETAIL_COLOR_SCHEMES_HPP

#include "widget_colors.hpp"

namespace nana
{
	namespace detail
	{
		class scheme_factory_base
		{
		public:
			struct factory_identifier{};
			virtual ~scheme_factory_base() = default;

			virtual factory_identifier* get_id() const = 0;
			virtual	widget_colors* create() = 0;
			virtual widget_colors* create(widget_colors&) = 0;
		};

		template<typename Scheme>
		class scheme_factory
			: public scheme_factory_base
		{
		private:
			factory_identifier* get_id() const override
			{
				return &fid_;
			}

			widget_colors* create() override
			{
				return (new Scheme);
			}

			widget_colors* create(widget_colors& other) override
			{
				return (new Scheme(static_cast<Scheme&>(other)));
			}
		private:
			static factory_identifier fid_;
		};

		template<typename Scheme>
		scheme_factory_base::factory_identifier scheme_factory<Scheme>::fid_;

		class color_schemes
		{
			struct implement;
			color_schemes(const color_schemes&) = delete;
			color_schemes(color_schemes&&) = delete;
			color_schemes& operator=(const color_schemes&) = delete;
			color_schemes& operator=(color_schemes&&) = delete;
		public:
			using scheme = widget_colors;

			color_schemes();
			~color_schemes();

			scheme&	scheme_template(scheme_factory_base&&);
			scheme* create(scheme_factory_base&&);
		private:
			implement * impl_;
		};
	}//end namespace detail;
}//end namespace nana
#endif