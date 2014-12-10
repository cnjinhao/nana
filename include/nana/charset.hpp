#ifndef NANA_CHARSET_HPP
#define NANA_CHARSET_HPP
#include <string>

namespace nana
{
	enum class unicode
	{
		utf8, utf16, utf32
	};

	namespace detail
	{
		class charset_encoding_interface;
	}
	/// An intelligent charset class for character code conversion.
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

/*!\class charset

Example
1. A UTF-8 string from the socket.

		int len = ::recv(sd, buf, buflen, 0);
		textbox.caption(nana::charset(std::string(buf, len), nana::unicode::utf8));

2. Send the string in text to the socket as UTF-8.

		std::string utf8str = nana::charset(textbox.caption()).to_bytes(nana::unicode::utf8);
		::send(sd, utf8str.c_str(), utf8str.size(), 0);

3, Convert a string to the specified multi-byte character code.

			//Convert to a multibytes string through default system language.
		std::string mbstr = nana::charset(a_wstring);
			//If the default system language is English and convert
			//a Chinese unicode string to multibytes string through GB2312
		std::setlocale(LC_CTYPE, "zh_CN.GB2312"); 
			//set::setlocale(LC_CTYPE, ".936"); call it in Windows
		std::string mbstr = nana::charset(a_wstring_with_chinese);

*/