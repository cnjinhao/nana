/**
 *	The charset Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2017 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/charset.hpp
 */

#ifndef NANA_CHARSET_HPP
#define NANA_CHARSET_HPP
#include <string>

namespace nana
{
	namespace utf
	{
		/// Attempt to get a pointer to a character of UTF-8 string by a specified character index.
		/// @param text_utf8 A string encoded as UTF-8.
		/// @param pos The unicode character index.
		/// @returns A pointer to the unicode character. It returns a null if pos is out of range. 
		const char* char_ptr(const char* text_utf8, unsigned pos);
		const char* char_ptr(const ::std::string& text_utf8, unsigned pos);

		/// Get the unicode character by a specified character index.
		/// @param text_utf8 A string encoded as UTF-8.
		/// @param pos The unicode character index.
		/// @param len A unsigned pointer to receive the number of bytes it takes in UTF-8 encoded. If len is a nullptr, it is ignored.
		/// @returns A unicode character. '\0' if pos is out of range.
		wchar_t char_at(const char* text_utf8, unsigned pos, unsigned * len);
		wchar_t char_at(const ::std::string& text_utf8, unsigned pos, unsigned * len);
	}

	enum class unicode
	{
		utf8, utf16, utf32
	};

	namespace detail
	{
		class charset_encoding_interface;
	}

	/*!\class charset
	\brief An intelligent charset class for character code conversion.
	Example:
	1. A UTF-8 string from the socket.

	    int len = ::recv(sd, buf, buflen, 0);
	    textbox.caption(nana::charset(std::string(buf, len), nana::unicode::utf8));

	2. Send the string in text to the socket as UTF-8.

	    std::string utf8str = nana::charset(textbox.caption()).to_bytes(nana::unicode::utf8);
	    ::send(sd, utf8str.c_str(), utf8str.size(), 0);

	3, Convert a string to the specified multi-byte character code.

	    // Convert to a multibytes string through default system language.
	    std::string mbstr = nana::charset(a_wstring);

	    // If the default system language is English and convert
	    // a Chinese unicode string to multibytes string through GB2312
	    std::setlocale(LC_CTYPE, "zh_CN.GB2312");
	               //set::setlocale(LC_CTYPE, ".936"); call it in Windows
	    std::string mbstr = nana::charset(a_wstring_with_chinese);

	*/
	class charset
	{
	public:
		charset(const charset&);
		charset & operator=(const charset&);
		charset(charset&&);
		charset & operator=(charset&&);

		charset(const std::string&);         ///<Attempt to convert a multibytes string.
		charset(std::string&&);              ///<Attempt to convert a multibytes string.
		charset(const std::string&, unicode);///<Attempt to convert a unicode string in byte sequence.
		charset(std::string&&, unicode);     ///<Attempt to convert a unicode string in byte sequence.
		charset(const std::wstring&);        ///<Attempt to convert a UCS2/UCS4 string.
		charset(std::wstring&&);             ///<Attempt to convert a UCS2/UCS4 string.
		~charset();
		operator std::string() const;        ///<Converts the string to multibytes string.
		operator std::string&&();            ///<Converts the string to multibytes string.
		operator std::wstring() const;       ///<Converts the string to UCS2/UCS4 string.
		operator std::wstring&&();           ///<Converts the string to UCS2/UCS4 string.
		std::string to_bytes(unicode) const; ///<Converts the string to a unicode in bytes sequenece width a specified unicode transformation format.
	private:
		detail::charset_encoding_interface* impl_;
	};

}//end namespace nana
#endif

