/*
*	Virtual Keyboard Implementations
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2023 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file: nana/gui/detail/keyboard.hpp
*/
#ifndef NANA_GUI_DETAIL_KEYBOARD_INCLUDED
#define NANA_GUI_DETAIL_KEYBOARD_INCLUDED
#include "../basis.hpp"

namespace nana::detail
{
	class virtual_keyboard
	{
		struct implementation;

		virtual_keyboard(const virtual_keyboard&) = delete;
		virtual_keyboard& operator=(const virtual_keyboard&) = delete;

		virtual_keyboard(virtual_keyboard&&) = delete;
		virtual_keyboard& operator=(virtual_keyboard&&) = delete;
	public:
		enum class modes
		{
			letter_lower,
			letter_upper,
			digital,
			symbol
		};

		enum class behaves
		{
			done
		};

		virtual_keyboard();
		~virtual_keyboard();

		void attach(window);
		bool qwerty(window, std::vector<std::string> langs, behaves, modes);
		bool numeric(window);
	private:
		implementation* const impl_;
	};
}


#endif