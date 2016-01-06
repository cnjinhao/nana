/*
 *	A Tooltip Implementation
 *	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/tooltip.hpp
 */

#ifndef NANA_GUI_WIDGETS_TOOLTIP_HPP
#define NANA_GUI_WIDGETS_TOOLTIP_HPP
#include "widgets/widget.hpp"

namespace nana
{
	///tooltip_interface
	///An interface for user-defined tooltip window.
	class tooltip_interface
	{
	public:
		virtual ~tooltip_interface(){}

		virtual bool tooltip_empty() const = 0;
		virtual nana::size tooltip_size() const = 0;
		virtual void tooltip_text(const ::std::string&)	= 0;
		virtual void tooltip_move(const nana::point& screen_pos, bool ignore_pos)	= 0;
		virtual void duration(std::size_t) = 0;
	};

	class tooltip
	{
		class factory_interface
		{
		public:
			virtual ~factory_interface(){}
			virtual tooltip_interface* create() = 0;
			virtual void destroy(tooltip_interface*) = 0;
		};

		template<typename TooltipWindow>
		class factory
			: public factory_interface
		{
			tooltip_interface * create() override
			{
				return new TooltipWindow;
			}

			void destroy(tooltip_interface* p) override
			{
				delete p;
			}
		};
	public:
		typedef factory_interface factory_if_type;

		template<typename TooltipWindow>
		static void make_factory()
		{
			_m_hold_factory(new factory<TooltipWindow>);
		}

		tooltip(){}
		tooltip(window w, const ::std::string &tip){set(w,tip);}


		static void set(window, const ::std::string&);
		static void show(window, point pos, const ::std::string&, std::size_t duration);
		static void close();
	private:
		static void _m_hold_factory(factory_interface*);
	};//class tooltip
}//namespace nana
#endif
