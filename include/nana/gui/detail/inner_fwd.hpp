/*
*	Inner Forward Declaration
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2014 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file: nana/gui/detail/inner_fwd.hpp
*
*/

#ifndef NANA_GUI_INNER_FWD_HPP
#define NANA_GUI_INNER_FWD_HPP

#include <nana/deploy.hpp>

namespace nana{
	namespace detail
	{
		struct signals
		{
			enum class code
			{
				caption,
				read_caption,
				destroy,
				size,
				end
			};

			union
			{
				const nana::char_t* caption;
				nana::string * str;
				struct
				{
					unsigned width;
					unsigned height;
				}size;
			}info;
		};

		class signal_invoker_interface
		{
		public:
			virtual ~signal_invoker_interface()
			{}

			virtual void call_signal(signals::code, const signals&) = 0;
		};
	}
}
#endif	//NANA_GUI_INNER_FWD_HPP
