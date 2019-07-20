/*
 *	Implementations of Inner Forward Declaration
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2018 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/detail/inner_fwd_implement.hpp
 *
 */

#ifndef NANA_GUI_INNER_FWD_IMPLEMENT_HPP
#define NANA_GUI_INNER_FWD_IMPLEMENT_HPP

#include <nana/push_ignore_diagnostic>
#include "basic_window.hpp"
#include <nana/gui/detail/inner_fwd.hpp>
#include <nana/paint/graphics.hpp>

#include <map>

namespace nana{
	namespace detail
	{
		class shortkey_container
		{
			struct item_type;

			//Noncopyable
			shortkey_container(const shortkey_container&) = delete;
			shortkey_container& operator=(const shortkey_container&) = delete;

			shortkey_container& operator=(shortkey_container&&) = delete;
		public:
			shortkey_container();

			shortkey_container(shortkey_container&&);

			~shortkey_container();

			void clear();

			bool make(window wd, unsigned long key);

			void umake(window wd);

			const std::vector<unsigned long>* keys(window wd) const;

			window find(unsigned long key) const;
		private:
			struct implementation;
			implementation * impl_;
		};

		struct window_platform_assoc;

		struct root_misc
		{
			basic_window * window;
			window_platform_assoc * wpassoc{ nullptr };

			nana::paint::graphics	root_graph;
			shortkey_container		shortkeys;

			struct condition_rep
			{
				bool			ignore_tab;			//ignore tab when the focus is changed by TAB key.
				basic_window*	pressed;			//The handle to a window which has been pressed by mouse left button.
				basic_window*	pressed_by_space;	//The handle to a window which has been pressed by SPACEBAR key.
				basic_window*	hovered;			//the latest window that mouse moved
			}condition;

			root_misc(root_misc&&);
			root_misc(basic_window * wd, unsigned width, unsigned height);
			~root_misc();
		private:
			root_misc(const root_misc&) = delete;
			root_misc& operator=(const root_misc&) = delete;
		};//end struct root_misc



		class root_register
		{
			//Noncopyable
			root_register(const root_register&) = delete;
			root_register& operator=(const root_register&) = delete;

			//Nonmovable
			root_register(root_register&&) = delete;
			root_register& operator=(root_register&&) = delete;
		public:
			root_register();
			~root_register();

			root_misc* insert(native_window_type, root_misc&&);

			root_misc * find(native_window_type);

			void erase(native_window_type);
		private:
			struct implementation;
			implementation * const impl_;
		};
	}
}//end namespace nana

#include <nana/pop_ignore_diagnostic>

#endif	//NANA_GUI_INNER_FWD_IMPLEMENT_HPP
