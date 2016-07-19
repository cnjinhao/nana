/*
*	Color Schemes
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
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

#include "widget_geometrics.hpp"

namespace nana
{
	namespace detail
	{
		class scheme_factory_interface
		{
		public:
			struct factory_identifier{};
			virtual ~scheme_factory_interface() = default;

			virtual factory_identifier* get_id() const = 0;
			virtual	widget_geometrics* create() = 0;
			virtual widget_geometrics* create(widget_geometrics&) = 0;
		};
		

		template<typename Scheme>
		class scheme_factory
			: public scheme_factory_interface
		{
		private:
			factory_identifier* get_id() const override
			{
				return &fid_;
			}

			widget_geometrics* create() override
			{
				return (new Scheme);
			}

			widget_geometrics* create(widget_geometrics& other) override
			{
				return (new Scheme(static_cast<Scheme&>(other)));
			}
		private:
			static factory_identifier fid_;
		};

		template<typename Scheme>
		scheme_factory_interface::factory_identifier scheme_factory<Scheme>::fid_;

		class color_schemes
		{
			struct implement;
			color_schemes(const color_schemes&) = delete;
			color_schemes(color_schemes&&) = delete;
			color_schemes& operator=(const color_schemes&) = delete;
			color_schemes& operator=(color_schemes&&) = delete;
		public:
			using scheme = widget_geometrics;

			color_schemes();
			~color_schemes();

			scheme&	scheme_template(scheme_factory_interface&&);
			scheme* create(scheme_factory_interface&&);
		private:
			implement * impl_;
		};
	}//end namespace detail;
}//end namespace nana
#endif