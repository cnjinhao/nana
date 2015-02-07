/*
*	Screen Informations
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file: nana/gui/screen.hpp
*/

#ifndef NANA_GUI_SCREEN_HPP
#define NANA_GUI_SCREEN_HPP
#include "basis.hpp"
#include <functional>
#include <memory>

namespace nana
{
	/// The monitor display metrics
	class display
	{
	public:
		virtual ~display() = default;

		/// The index of monitor.
		virtual std::size_t get_index() const = 0;

		/// Returns the positional coordinates and size of the display device in reference to the desktop area
		virtual const ::nana::rectangle& area() const = 0;
	};

	class screen
	{
	public:
		static ::nana::size desktop_size();
		static ::nana::size primary_monitor_size();
		static std::shared_ptr<display>	from_point(const point&);
		static std::shared_ptr<display> from_window(window);

		/// Returns the number of display monitors
		std::size_t count() const;

		std::shared_ptr<display> get_display(std::size_t index) const;
		std::shared_ptr<display> get_primary() const;

		void for_each(std::function<void(display&)>) const;
	};
}//end namespace nana

#endif
