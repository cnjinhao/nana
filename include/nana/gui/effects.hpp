/*
 *	Background Effects Implementation
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/effects.hpp
 *
 */
#ifndef NANA_GUI_EFFECTS_HPP
#define NANA_GUI_EFFECTS_HPP
#include <cstddef>
#include <nana/paint/graphics.hpp>

namespace nana
{
	namespace effects
	{
		enum class edge_nimbus
		{
			none, active = 0x1, over = 0x2
		};

		class bground_interface
		{
		public:
			typedef paint::graphics & graph_reference;

			virtual ~bground_interface() = 0;
			virtual void take_effect(window, graph_reference) const = 0;
		};

		class bground_factory_interface
		{
			friend class effects_accessor;
		public:
			virtual ~bground_factory_interface() = 0;
		private:
			virtual bground_interface * create() const = 0;
		};

		class bground_transparent
			: public bground_factory_interface
		{
		public:
			explicit bground_transparent(std::size_t percent);
		private:
			bground_interface* create() const override;
		private:
			std::size_t percent_;
		};

		class bground_blur
			: public bground_factory_interface
		{
		public:
			bground_blur(std::size_t radius);
		private:
			bground_interface * create() const override;
		private:
			std::size_t radius_;
		};
	}
}//end namespace nana
#endif
