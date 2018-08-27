/*
 *	A Widget Iterator Template
 *	Copyright(C) 2017-2018 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/detail/widget_iterator.hpp
 *  @description: widget_iterator is the base class provided to simplify definitions of the required types for widget iterators.
 *	It is provided because of deprecation of std::iterator in C++17
 */
#ifndef NANA_GUI_WIDGET_ITERATOR_INCLUDED
#define NANA_GUI_WIDGET_ITERATOR_INCLUDED

#include <cstddef>	//provides std::ptrdiff_t


namespace nana {
	namespace widgets {
		namespace detail
		{
			template<typename Category, typename T>
			class widget_iterator
			{
			public:
				using iterator_category = Category;
				using value_type = T;
				using difference_type = std::ptrdiff_t;
				using pointer = T * ;
				using reference = T & ;
			};
		}
	}
}

#endif
