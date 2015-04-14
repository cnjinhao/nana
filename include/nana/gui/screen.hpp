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

		virtual bool is_primary_monitor() const = 0;

		/// Returns the positional coordinates and size of the display device in reference to the desktop area
		virtual const ::nana::rectangle& area() const = 0;
		virtual const ::nana::rectangle& workarea() const = 0;
	};

	class screen
	{
		struct implement;
	public:
		static ::nana::size desktop_size();
		static ::nana::size primary_monitor_size();

		screen();

		/// Reload has no preconditions, it's safe to call on moved-from
		void reload();

		/// Returns the number of display monitors
		std::size_t count() const;

		display& from_point(const point&);
		display& from_window(window);

		display& get_display(std::size_t index) const;
		display& get_primary() const;

		void for_each(std::function<void(display&)>) const;
	private:
		std::shared_ptr<implement> impl_;
	};
}//end namespace nana

#endif
