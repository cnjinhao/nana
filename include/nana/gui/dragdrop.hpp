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
#include <nana/filesystem/filesystem.hpp>

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
		explicit simple_dragdrop(window source);
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

	namespace detail
	{
		struct dragdrop_data;
	}

	class dragdrop
	{
		struct implementation;

		/// Non-copyable
		dragdrop(const dragdrop&) = delete;
		dragdrop& operator=(const dragdrop&) = delete;

		/// Non-movable
		dragdrop(dragdrop&&) = delete;
		dragdrop& operator=(dragdrop&&) = delete;
	public:
		class data
		{
			friend struct dragdrop::implementation;

			/// Non-copyable
			data(const data&) = delete;
			data& operator=(const data&) = delete;
		public:
			data();
			data(data&&);
			~data();

			data& operator=(data&& rhs);

			void insert(std::filesystem::path);
		private:
			detail::dragdrop_data* real_data_;
		};

		dragdrop(window source);
		~dragdrop();

		void condition(std::function<bool()> predicate_fn);
		void prepare_data(std::function<data()> generator);
		void drop_finished(std::function<void(bool)> finish_fn);
	private:
		implementation* const impl_;
	};
}

#endif