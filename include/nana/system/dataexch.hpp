/*
 *	Data Exchanger Implementation
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
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

namespace nana{

namespace paint{
	class graphics;
}

namespace system{

	/// a data exchange mechanism through Windows Clipboard, X11 Selection.
	class dataexch
	{
	public:
		enum class format
		{
			text, pixmap
		};

		void set(const std::string & text_utf8);
		void set(const std::wstring& text);

		bool set(const nana::paint::graphics& g);

		void get(std::string& text_utf8);
		void get(std::wstring& text);
	private:
		bool _m_set(format, const void* buf, std::size_t size);
		void* _m_get(format, size_t& size);
	};

}//end namespace system
}//end namespace nana

#endif
