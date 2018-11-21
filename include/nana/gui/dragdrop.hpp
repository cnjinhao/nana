/**
*	Drag and Drop Implementation
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2018 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file: nana/gui/dragdrop.hpp
*	@author: Jinhao(cnjinhao@hotmail.com)
*/
#ifndef NANA_GUI_DRAGDROP_INCLUDED
#define NANA_GUI_DRAGDROP_INCLUDED

#include <nana/push_ignore_diagnostic>
#include <functional>
#include "basis.hpp"

#include <memory>

namespace nana
{
	class simple_dragdrop
	{
		struct implementation;

		simple_dragdrop(const simple_dragdrop&) = delete;
		simple_dragdrop& operator=(const simple_dragdrop&) = delete;

		simple_dragdrop(simple_dragdrop&&) = delete;
		simple_dragdrop& operator=(simple_dragdrop&&) = delete;
	public:
		explicit simple_dragdrop(window drag_wd);
		simple_dragdrop(window                drag_origin,
						std::function<bool()> when,
						window                drop_target,
						std::function<void()> how)
		: simple_dragdrop{drag_origin}
		{
			condition(when);
			make_drop(drop_target, how);
		}
		~simple_dragdrop();

		/// Sets a condition that determines whether the drag&drop can start
		void condition(std::function<bool()> predicate_fn);
		void make_drop(window target, std::function<void()> drop_fn);
	private:
		implementation* const impl_;
	};

	class dragdrop
	{
		struct implementation;

		dragdrop(const dragdrop&) = delete;
		dragdrop& operator=(const dragdrop&) = delete;

		dragdrop(dragdrop&&) = delete;
		dragdrop& operator=(dragdrop&&) = delete;
	public:
		dragdrop(window source);
		~dragdrop();

		void condition(std::function<bool()> predicate_fn);

		void make_data(std::function<void()> generator);
	private:
		implementation* const impl_;
	};
}

#endif