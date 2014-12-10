/*
 *	Data Exchanger Implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file:			nana/system/dataexch.hpp
 *	@description:	An implementation of a data exchange mechanism through Windows Clipboard, X11 Selection. 
 */

#ifndef NANA_SYSTEM_DATAEXCH_HPP
#define NANA_SYSTEM_DATAEXCH_HPP
#include <nana/basic_types.hpp>

namespace nana{ namespace system{
            /// a data exchange mechanism through Windows Clipboard, X11 Selection.
	class dataexch
	{
	public:
		struct format
		{
			enum{ text, unicode, pixmap, end};
		};

		void set(const nana::char_t* text);
		void set(const nana::string& text);
		void get(nana::string& str);
	private:
		bool _m_set(unsigned type, const void* buf, std::size_t size);
		void* _m_get(unsigned type, size_t& size);
	};

}//end namespace system
}//end namespace nana

#endif
