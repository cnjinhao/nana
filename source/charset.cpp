/**
 *	A Character Encoding Set Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2018 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file nana/charset.cpp
 *	@brief A conversion between unicode characters and multi bytes characters
 *	@contributions
 *		UTF16 4-byte decoding issue by Renke Yan.
 *		Pr0curo(pr#98)
 *		crillion
 */

#include <nana/charset.hpp>
#include <utility>
#include <nana/deploy.hpp>
#include <cwchar>
#include <clocale>
#include <cstring>	//Added by Pr0curo(pr#98)
#include <memory>
#include <locale>	//Added by crillion

//GCC 4.7.0 does not implement the <codecvt> and codecvt_utfx classes
#ifndef STD_CODECVT_NOT_SUPPORTED
	#include <codecvt>
#endif

#if defined(NANA_WINDOWS)
	#include <windows.h>
#endif

namespace nana
{
	namespace utf
	{
		/// return a pointer to the code unit of the character at pos
		const char* char_ptr(const char* text, unsigned pos)
		{
			auto ustr = reinterpret_cast<const unsigned char*>(text);
			auto const end = ustr + std::strlen(text);

			for (unsigned i = 0; i != pos; ++i)
			{
				const auto uch = *ustr;
				if (uch < 0x80)
				{
					++ustr;
					continue;
				}

				if (uch < 0xC0)        // use police ?
					return nullptr;

				if ((uch < 0xE0) && (ustr + 1 < end)) //? *(ustr + 1) < 0xE0
					ustr += 2;
				else if (uch < 0xF0 && (ustr + 2 <= end))
					ustr += 3;
				else if (uch < 0x1F && (ustr + 3 <= end))
					ustr += 4;
				else
					return nullptr;
			}

			return reinterpret_cast<const char*>(ustr);
		}

		/// return a pointer to the code unit of the character at pos - reuse ^ ?
		const char* char_ptr(const std::string& text_utf8, unsigned pos)
		{
			auto ustr = reinterpret_cast<const unsigned char*>(text_utf8.c_str());
			auto const end = ustr + text_utf8.size();

			for (unsigned i = 0; i != pos; ++i)
			{
				const auto uch = *ustr;
				if (uch < 0x80)
				{
					++ustr;
					continue;
				}

				if (uch < 0xC0)
					return nullptr;

				if ((uch < 0xE0) && (ustr + 1 < end))
					ustr += 2;
				else if (uch < 0xF0 && (ustr + 2 <= end))
					ustr += 3;
				else if (uch < 0x1F && (ustr + 3 <= end))
					ustr += 4;
				else
					return nullptr;
			}

			return reinterpret_cast<const char*>(ustr);
		}

		/// return a code point (max 16 bits?) and the len in code units of the character at pos
		wchar_t char_at(const char* text_utf8, unsigned pos, unsigned * len)
		{
			if (!text_utf8)
				return 0;

			if (pos)
			{
				text_utf8 = char_ptr(text_utf8, pos);
				if (!text_utf8)
					return 0;
			}

			const wchar_t uch = *reinterpret_cast<const unsigned char*>(text_utf8);
			if (uch < 0x80)
			{
				if (len)
					*len = 1;

				return *text_utf8;  // uch ?
			}

			if (uch < 0xC0)    // use police or ??
			{
				if (len)
					*len = 0;

				return 0;
			}

			const auto end = text_utf8 + std::strlen(text_utf8);

			if (uch < 0xE0 && (text_utf8 + 1 <= end))
			{
				if (len)
					*len = 2;
				return (wchar_t(uch & 0x1F) << 6) | (reinterpret_cast<const unsigned char*>(text_utf8)[1] & 0x3F);
			}
			else if (uch < 0xF0 && (text_utf8 + 2 <= end))
			{
				if (len)
					*len = 3;

				return ((((uch & 0xF) << 6) | (reinterpret_cast<const unsigned char*>(text_utf8)[1] & 0x3F)) << 6) | (reinterpret_cast<const unsigned char*>(text_utf8)[2] & 0x3F);
			}
			else if (uch < 0x1F && (text_utf8 + 3 <= end))
			{
				if (len)
					*len = 4;
				return ((((((uch & 0x7) << 6) | (reinterpret_cast<const unsigned char*>(text_utf8)[1] & 0x3F)) << 6) | (reinterpret_cast<const unsigned char*>(text_utf8)[2] & 0x3F)) << 6) | (reinterpret_cast<const unsigned char*>(text_utf8)[3] & 0x3F);
			}

			if (len)
				*len = 0;

			return 0;
		}

		/// return a code point (max 16 bits?) and the len in code units of the character at pos
		wchar_t char_at(const ::std::string& text_utf8, unsigned pos, unsigned * len)
		{
			const char* ptr;
			if (pos)
			{
				ptr = char_ptr(text_utf8, pos);
				if (!ptr)
					return 0;
			}
			else
				ptr = text_utf8.c_str();

			const wchar_t uch = *reinterpret_cast<const unsigned char*>(ptr);
			if (uch < 0x80)
			{
				if (len)
					*len = 1;

				return *ptr;
			}

			if (uch < 0xC0)
			{
				if (len)
					*len = 0;

				return 0;
			}

			const auto end = text_utf8.c_str() + text_utf8.size();

			if (uch < 0xE0 && (ptr + 1 <= end))
			{
				if (len)
					*len = 2;
				return (wchar_t(uch & 0x1F) << 6) | (reinterpret_cast<const unsigned char*>(ptr)[1] & 0x3F);
			}
			else if (uch < 0xF0 && (ptr + 2 <= end))
			{
				if (len)
					*len = 3;

				return ((((uch & 0xF) << 6) | (reinterpret_cast<const unsigned char*>(ptr)[1] & 0x3F)) << 6) | (reinterpret_cast<const unsigned char*>(ptr)[2] & 0x3F);
			}
			else if (uch < 0x1F && (ptr + 3 <= end))
			{
				if (len)
					*len = 4;
				return ((((((uch & 0x7) << 6) | (reinterpret_cast<const unsigned char*>(ptr)[1] & 0x3F)) << 6) | (reinterpret_cast<const unsigned char*>(ptr)[2] & 0x3F)) << 6) | (reinterpret_cast<const unsigned char*>(ptr)[3] & 0x3F);
			}

			if (len)
				*len = 0;

			return 0;
		}
	}

	namespace detail
	{
		/// candidate to be more general??
		class locale_initializer
		{
		public:
			static void init()
			{
				static bool initialized = false;
				if (initialized) return;

				initialized = true;
				//Only set the C library locale
				std::setlocale(LC_CTYPE, "");
			}
		};

		/// convert wchar C string from ? ANSI code page CP_ACP (windows) or LC_CTYPE c locale (-nix) into utf8 std::string
		static bool wc2mb(std::string& mbstr, const wchar_t * s)
		{
			if(nullptr == s || *s == 0)
			{
				mbstr.clear();
				return true;
			}
#if defined(NANA_WINDOWS)
			int bytes = ::WideCharToMultiByte(CP_ACP, 0, s, -1, 0, 0, 0, 0);
			if(bytes > 1)
			{
				mbstr.resize(bytes - 1);
				::WideCharToMultiByte(CP_ACP, 0, s, -1, &(mbstr[0]), bytes - 1, 0, 0);
			}
			return true;
#else
			locale_initializer::init();
			std::mbstate_t mbstate = std::mbstate_t();
			std::size_t len = std::wcsrtombs(nullptr, &s, 0, &mbstate);
			if(len == static_cast<std::size_t>(-1))
				return false;

			if(len)
			{
				mbstr.resize(len);
				std::wcsrtombs(&(mbstr[0]), &s, len, &mbstate);
			}
			else
				mbstr.clear();
#endif
			return true;
		}

		/// convert a char C-string from The system default Windows ANSI code page CP_ACP or from LC_CTYPE c locale (-nix) into utf16 std::wstring
		static bool mb2wc(std::wstring& wcstr, const char* s)
		{
			if(nullptr == s || *s == 0)
			{
				wcstr.clear();
				return true;
			}
#if defined(NANA_WINDOWS)
			int chars = ::MultiByteToWideChar(CP_ACP, 0, s, -1, 0, 0);
			if(chars > 1)
			{
				wcstr.resize(chars - 1);
				::MultiByteToWideChar(CP_ACP, 0, s, -1, &wcstr[0], chars - 1);
			}
#else
			locale_initializer::init();
			std::mbstate_t mbstate = std::mbstate_t();
			std::size_t len = std::mbsrtowcs(nullptr, &s, 0, &mbstate);
			if(len == static_cast<std::size_t>(-1))
				return false;

			if(len)
			{
				wcstr.resize(len);
				std::mbsrtowcs(&wcstr[0], &s, len, &mbstate);
			}
			else
				wcstr.clear();
#endif
			return true;
		}

		/// convert a char C string from The system default Windows ANSI code page CP_ACP or LC_CTYPE c locale (-nix) into utf16 std::string
		bool mb2wc(std::string& wcstr, const char* s)
		{
			if(nullptr == s || *s == 0)
			{
				wcstr.clear();
				return true;
			}
#if defined(NANA_WINDOWS)
			int chars = ::MultiByteToWideChar(CP_ACP, 0, s, -1, 0, 0);
			if(chars > 1)
			{
				wcstr.resize((chars - 1) * sizeof(wchar_t));
				::MultiByteToWideChar(CP_ACP, 0, s, -1, reinterpret_cast<wchar_t*>(&wcstr[0]), chars - 1);
				                                      // ^ the trick !
			}
#else
			locale_initializer::init();
			std::mbstate_t mbstate = std::mbstate_t();
			std::size_t len = std::mbsrtowcs(nullptr, &s, 0, &mbstate);
			if(len == static_cast<std::size_t>(-1))
				return false;

			if(len)
			{
				wcstr.resize(sizeof(wchar_t) * len);
				std::mbsrtowcs(reinterpret_cast<wchar_t*>(&wcstr[0]), &s, len, &mbstate);
			}
			else
				wcstr.clear();
#endif
			return true;
		}

		class charset_encoding_interface
		{
		public:
			virtual ~charset_encoding_interface(){}

			virtual charset_encoding_interface * clone() const = 0;

			virtual std::string str() const = 0;
			virtual std::string&& str_move() = 0;
			virtual std::string str(unicode) const = 0;

			virtual std::wstring wstr() const = 0;
			virtual std::wstring&& wstr_move() = 0;
		};

		/// playing with the idea - we need a mechanism to set a user selected police - Testing an abstract interface
		struct encoding_error_police
		{
			virtual unsigned long next_code_point(const unsigned char*& current_code_unit, const unsigned char* end) = 0;
			virtual ~encoding_error_police() = default;
		};

		/// the current nana default: it is safe - you may want to keep it ! use the other at your risk: mainly for debugging
		struct utf8_error_police : public encoding_error_police
		{
			unsigned long next_code_point(const unsigned char*& current_code_unit, const unsigned char* end) override
			{
				current_code_unit = end;
				return 0;
			}

		};

		///
		struct utf8_error_police_def_char : public encoding_error_police
		{
			static unsigned long def_error_mark ;

			unsigned long error_mark{ def_error_mark };
			utf8_error_police_def_char() = default;
			utf8_error_police_def_char( unsigned long mark): error_mark{mark}{}
			unsigned long next_code_point(const unsigned char*& current_code_unit, const unsigned char* end) override
			{
				if(current_code_unit < end)
					++current_code_unit;
				return error_mark;
			}

		};

		unsigned long utf8_error_police_def_char::def_error_mark{ '*' };

		///
		struct utf8_error_police_throw : public encoding_error_police
		{
			unsigned long next_code_point(const unsigned char*& current_code_unit, const unsigned char* end) override
			{
				//utf8_Error::use_throw = true;
				utf8_Error(std::string("The text is not encoded in UTF8: ") +
					reinterpret_cast<const char*>( current_code_unit) ).emit();;
				current_code_unit = end;
				return 0;
			}

		};

		struct utf8_error_police_latin : public encoding_error_police
		{
			unsigned long next_code_point(const unsigned char*& current_code_unit, const unsigned char* /*end*/) override
			{
				return *(current_code_unit++) ;
			}
		};

		/// buggie?
		struct utf8_error_police_system : public encoding_error_police
		{
			unsigned long next_code_point(const unsigned char*& current_code_unit, const unsigned char* /*end*/) override
			{
				std::wstring wc;
				mb2wc(wc, reinterpret_cast<const char*>(current_code_unit));
				current_code_unit++;

				return wc[0];      // use utf16char but what endian?
			}
		};


//		auto def_encoding_error_police = std::make_unique<utf8_error_police>();  // the nana default
//		auto def_encoding_error_police = std::make_unique<utf8_error_police_latin>();
//		auto def_encoding_error_police = std::make_unique<utf8_error_police_throw>();
//		auto def_encoding_error_police = std::make_unique<utf8_error_police_def_char>('X');
		auto def_encoding_error_police = std::make_unique<utf8_error_police_system>();



#ifndef STD_CODECVT_NOT_SUPPORTED
		class charset_string
			: public charset_encoding_interface
		{
		public:
			charset_string(const std::string& s)
				: data_(s)
			{}

			charset_string(std::string&& s)
				: data_(std::move(s))
			{}

			charset_string(const std::string& s, unicode encoding)
				: data_(s), is_unicode_(true), utf_x_(encoding)
			{}

			charset_string(std::string&& s, unicode encoding)
				: data_(std::move(s)), is_unicode_(true), utf_x_(encoding)
			{}
		private:
			virtual charset_encoding_interface * clone() const
			{
				return new charset_string(*this);
			}

			virtual std::string str() const
			{
				if(is_unicode_)
				{
					std::wstring wcstr;
					switch(utf_x_)
					{
					case unicode::utf8:
						wcstr = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(data_);
						break;
					case unicode::utf16:
						wcstr = std::wstring_convert<std::codecvt_utf16<wchar_t, 0x10FFFF, std::little_endian>>().from_bytes(data_);
						break;
					case unicode::utf32:
						wcstr.append(reinterpret_cast<const wchar_t*>(data_.c_str()), data_.size() / sizeof(wchar_t));
						break;
					}

					std::string mbstr;
					wc2mb(mbstr, wcstr.c_str());
					return mbstr;
				}
				return data_;
			}

			virtual std::string&& str_move()
			{
				if(is_unicode_)
					data_ = str();
				return std::move(data_);
			}

			virtual std::string str(unicode encoding) const
			{
				if(is_unicode_ && (utf_x_ != encoding))
				{
					switch(utf_x_)
					{
					case unicode::utf8:
						switch(encoding)
						{
						case unicode::utf16:
							return std::wstring_convert<std::codecvt_utf16<char16_t>, char16_t>().to_bytes(
								std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>().from_bytes(data_)
								);
						case unicode::utf32:
							{
								std::u32string u32str = std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t>().from_bytes(data_);
								return std::string(reinterpret_cast<const char*>(u32str.c_str()), u32str.size() * sizeof(char32_t));
							}
						default:
							break;	//no conversion
						}
						break;
					case unicode::utf16:
						switch(encoding)
						{
						case unicode::utf8:
							return std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>().to_bytes(
								std::wstring_convert<std::codecvt_utf16<char16_t>, char16_t>().from_bytes(data_)
								);
						case unicode::utf32:
							{
								std::u32string u32str = std::wstring_convert<std::codecvt_utf16<char32_t>, char32_t>().from_bytes(data_);
								return std::string(reinterpret_cast<const char*>(u32str.c_str()), u32str.size() * sizeof(char32_t));
							}
						default:
							break;	//no conversion
						}
						break;
					case unicode::utf32:
						switch(encoding)
						{
						case unicode::utf8:
							return std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t>().to_bytes(
									std::u32string(reinterpret_cast<const char32_t*>(data_.c_str()), data_.size() / sizeof(char32_t))
								);
						case unicode::utf16:
							return std::wstring_convert<std::codecvt_utf16<char32_t>, char32_t>().to_bytes(
									std::u32string(reinterpret_cast<const char32_t*>(data_.c_str()), data_.size() / sizeof(char32_t))
								);
						default:
							break; //no conversion
						}
						break;
					}
					return{};
				}

				std::wstring wcstr;
				if(mb2wc(wcstr, data_.c_str()))
				{
					switch(encoding)
					{
					case unicode::utf8:
						return std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(wcstr);
					case unicode::utf16:
						return std::wstring_convert<std::codecvt_utf16<wchar_t, 0x10FFFF, std::little_endian>>().to_bytes(wcstr);
					case unicode::utf32:
	#if defined(NANA_WINDOWS)
						{
							const char * bytes = reinterpret_cast<const char*>(wcstr.c_str());
							std::u32string utf32str = std::wstring_convert<std::codecvt_utf16<char32_t>, char32_t>().from_bytes(bytes, bytes + sizeof(wchar_t) * wcstr.size());
							return std::string(reinterpret_cast<const char*>(utf32str.c_str()), sizeof(char32_t) * utf32str.size());
						}
	#elif defined(NANA_POSIX)
						return std::string(reinterpret_cast<const char*>(wcstr.c_str()), sizeof(wchar_t) * wcstr.size());
	#else
						throw std::runtime_error("Bad charset");
	#endif
					}
				}
				return{};
			}

			virtual std::wstring wstr() const
			{
				if(is_unicode_)
				{
					switch(utf_x_)
					{
					case unicode::utf8:
						return std::wstring_convert<std::codecvt_utf8<wchar_t, 0x10FFFF, std::little_endian>>().from_bytes(data_);
					case unicode::utf16:
						return std::wstring_convert<std::codecvt_utf16<wchar_t, 0x10FFFF, std::little_endian>>().from_bytes(data_);
					case unicode::utf32:
						return std::wstring(reinterpret_cast<const wchar_t*>(data_.c_str()), data_.size() * sizeof(wchar_t));
					}
					return{};
				}

				std::wstring wcstr;
				mb2wc(wcstr, data_.c_str());
				return wcstr;
			}

			virtual std::wstring && wstr_move()
			{
				wdata_for_move_ = wstr();
				return std::move(wdata_for_move_);
			}
		private:
			std::string data_;
			std::wstring wdata_for_move_{};
			bool is_unicode_{ false };
			unicode utf_x_{ unicode::utf8 };
		};

		class charset_wstring
			: public charset_encoding_interface
		{
		public:
			charset_wstring(const std::wstring& s)
				: data_(s)
			{}

			charset_wstring(std::wstring&& s)
				: data_(std::move(s))
			{}

			virtual charset_encoding_interface * clone() const
			{
				return new charset_wstring(*this);
			}

			virtual std::string str() const
			{
				if(data_.size())
				{
					std::string mbstr;
					wc2mb(mbstr, data_.c_str());
					return mbstr;
				}
				return{};
			}

			virtual std::string&& str_move()
			{
				data_for_move_ = str();
				return std::move(data_for_move_);
			}

			virtual std::string str(unicode encoding) const
			{
				switch(encoding)
				{
				case unicode::utf8:
					return std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(data_);
				case unicode::utf16:
					return std::wstring_convert<std::codecvt_utf16<wchar_t, 0x10FFFF, std::little_endian>>().to_bytes(data_);
				case unicode::utf32:
	#if defined (NANA_WINDOWS)
					{
						const char* bytes = reinterpret_cast<const char*>(data_.c_str());
						std::u32string utf32str = std::wstring_convert<std::codecvt_utf16<char32_t>, char32_t>().from_bytes(bytes, bytes + sizeof(wchar_t) * data_.size());
						return std::string(reinterpret_cast<const char*>(utf32str.c_str()), sizeof(char32_t) * utf32str.size());
					}
	#elif defined(NANA_POSIX)
					return std::string(reinterpret_cast<const char*>(data_.c_str()), data_.size() * sizeof(wchar_t));
	#else
					throw std::runtime_error("Bad charset");
	#endif
				}
				return std::string();
			}

			virtual std::wstring wstr() const
			{
				return data_;
			}

			virtual std::wstring&& wstr_move()
			{
				return std::move(data_);
			}
		private:
			std::wstring data_;
			std::string data_for_move_;
		};
#else


        /// return the first code point and move the pointer to next character, springing to the end by errors
		unsigned long utf8char(const unsigned char*& p, const unsigned char* end)
		{
			if(p != end)
			{
				if(*p < 0x80)        // ASCII char   0-127 or 0-0x80
				{
					return *(p++);
				}
				unsigned ch = *p;
				unsigned long code;
				if(ch < 0xC0)       // error? - move to end. Possible ANSI or ISO code-page
				{
					//return *(p++); // temp: assume equal
					//p = end;
					//return 0;
					return def_encoding_error_police->next_code_point(p, end);
				}
				else if(ch < 0xE0 && (p + 1 <= end))      // two byte character
				{
					code = ((ch & 0x1F) << 6) | (p[1] & 0x3F);
					p += 2;
				}
				else if(ch < 0xF0 && (p + 2 <= end))     // 3 byte character
				{
					code = ((((ch & 0xF) << 6) | (p[1] & 0x3F)) << 6) | (p[2] & 0x3F);
					p += 3;
				}
				else if(ch < 0x1F && (p + 3 <= end))   // 4 byte character
				{
					code = ((((((ch & 0x7) << 6) | (p[1] & 0x3F)) << 6) | (p[2] & 0x3F)) << 6) | (p[3] & 0x3F);
					p += 4;
				}
				else    //  error, go to end
				{
					p = end;
					return 0;
				}
				return code;
			}
			return 0;
		}

		unsigned long utf16char(const unsigned char* & bytes, const unsigned char* end, bool le_or_be)
		{
			unsigned long code;
			if(le_or_be)
			{
				if((end - bytes >= 4) && ((bytes[1] & 0xFC) == 0xD8))
				{
					//32bit encoding
					unsigned long ch0 = bytes[0] | (bytes[1] << 8);
					unsigned long ch1 = bytes[2] | (bytes[3] << 8);

					code = ((ch0 & 0x3FF) << 10) | (ch1 & 0x3FF);
					bytes += 4;
				}
				else if(end - bytes >= 2)
				{
					code = bytes[0] | (bytes[1] << 8);
					bytes += 2;
				}
				else
				{
					bytes = end;
					return 0;
				}
			}
			else
			{
				if((end - bytes >= 4) && ((bytes[0] & 0xFC) == 0xD8))
				{
					//32bit encoding
					unsigned long ch0 = (bytes[0] << 8) | bytes[1];
					unsigned long ch1 = (bytes[2] << 8) | bytes[3];
					code = (((ch0 & 0x3FF) << 10) | (ch1 & 0x3FF)) + 0x10000;
					bytes += 4;
				}
				else if(end - bytes >= 2)
				{
					code = (bytes[0] << 8) | bytes[1];
					bytes += 2;
				}
				else
				{
					bytes = end;
					return 0;
				}
			}
			return code;
		}

		unsigned long utf32char(const unsigned char* & bytes, const unsigned char* end, bool le_or_be)
		{
			if(end - bytes >= 4)
			{
				unsigned long code;
				if(le_or_be)
					code = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
				else
					code = bytes[3] | (bytes[2] << 8) | (bytes[1] << 16) | (bytes[0] << 24);
				bytes += 4;
				return code;
			}
			bytes = end;
			return 0;
		}

		void put_utf8char(std::string& s, unsigned long code)
		{
			if(code < 0x80)
			{
				s += static_cast<char>(code);
			}
			else if(code < 0x800)
			{
				s += static_cast<char>(0xC0 | (code >> 6));
				s += static_cast<char>(0x80 | (code & 0x3F));
			}
			else if(code < 0x10000)
			{
				s += static_cast<char>(0xE0 | (code >> 12));
				s += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
				s += static_cast<char>(0x80 | (code & 0x3F));
			}
			else
			{
				s += static_cast<char>(0xF0 | (code >> 18));
				s += static_cast<char>(0x80 | ((code >> 12) & 0x3F));
				s += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
				s += static_cast<char>(0x80 | (code & 0x3F));
			}
		}

		//le_or_be, true = le, false = be
		void put_utf16char(std::string& s, unsigned long code, bool le_or_be)
		{
			if(code <= 0xFFFF)
			{
				if(le_or_be)
				{
					s += static_cast<char>(code & 0xFF);
					s += static_cast<char>((code & 0xFF00) >> 8);
				}
				else
				{
					s += static_cast<char>((code & 0xFF00) >> 8);
					s += static_cast<char>(code & 0xFF);
				}
			}
			else
			{
				unsigned long ch0 = (0xD800 | ((code - 0x10000) >> 10));
				unsigned long ch1 = (0xDC00 | ((code - 0x10000) & 0x3FF));

				if(le_or_be)
				{
					s += static_cast<char>(ch0 & 0xFF);
					s += static_cast<char>((ch0 & 0xFF00) >> 8);

					s += static_cast<char>(ch1 & 0xFF);
					s += static_cast<char>((ch1 & 0xFF00) >> 8);
				}
				else
				{
					s += static_cast<char>((ch0 & 0xFF00) >> 8);
					s += static_cast<char>(ch0 & 0xFF);

					s += static_cast<char>((ch1 & 0xFF00) >> 8);
					s += static_cast<char>(ch1 & 0xFF);
				}
			}
		}

		void put_utf32char(std::string& s, unsigned long code, bool le_or_be)
		{
			if(le_or_be)
			{
				s += static_cast<char>(code & 0xFF);
				s += static_cast<char>((code & 0xFF00) >> 8);
				s += static_cast<char>((code & 0xFF0000) >> 16);
				s += static_cast<char>((code & 0xFF000000) >> 24);
			}
			else
			{
				s += static_cast<char>((code & 0xFF000000) >> 24);
				s += static_cast<char>((code & 0xFF0000) >> 16);
				s += static_cast<char>((code & 0xFF00) >> 8);
				s += static_cast<char>(code & 0xFF);
			}
		}

		std::string utf8_to_utf16(const std::string& s, bool le_or_be)
		{
			const unsigned char * bytes = reinterpret_cast<const unsigned char*>(s.c_str());
			const unsigned char * end = bytes + s.size();

			std::string utf16str;

			//If there is a BOM, ignore it.
			if(s.size() >= 3)
			{
				if(bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF)
				{
					bytes += 3;
					put_utf16char(utf16str, 0xFEFF, le_or_be);
				}
			}

			while(bytes != end)
			{
				put_utf16char(utf16str, utf8char(bytes, end), le_or_be);
			}
			return utf16str;
		}

		std::string utf8_to_utf32(const std::string& s, bool le_or_be)
		{
			const unsigned char * bytes = reinterpret_cast<const unsigned char*>(s.c_str());
			const unsigned char * end = bytes + s.size();

			std::string utf32str;
			//If there is a BOM, ignore it.
			if(s.size() >= 3)
			{
				if(bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF)
				{
					bytes += 3;
					put_utf32char(utf32str, 0xFEFF, le_or_be);
				}
			}

			while(bytes != end)
			{
				put_utf32char(utf32str, utf8char(bytes, end), le_or_be);
			}
			return utf32str;
		}

		std::string utf16_to_utf8(const std::string& s)
		{
			const unsigned char * bytes = reinterpret_cast<const unsigned char*>(s.c_str());
			const unsigned char * end = bytes + s.size();
			bool le_or_be = true;
			std::string utf8str;
			//If there is a BOM, ignore it
			if(s.size() >= 2)
			{
				if(bytes[0] == 0xFF && bytes[1] == 0xFE)
				{
					bytes += 2;
					le_or_be = true;

					utf8str += (char)0xEF;
					utf8str += (char)0xBB;
					utf8str += (char)0xBF;
				}
				else if(bytes[0] == 0xFE && bytes[1] == 0xFF)
				{
					bytes += 2;
					le_or_be = false;
					utf8str += (char)(0xEF);
					utf8str += (char)(0xBB);
					utf8str += (char)(0xBF);
				}
			}

			while(bytes != end)
			{
				put_utf8char(utf8str, utf16char(bytes, end, le_or_be));
			}
			return utf8str;
		}

		std::string utf16_to_utf32(const std::string& s)
		{
			const unsigned char * bytes = reinterpret_cast<const unsigned char*>(s.c_str());
			const unsigned char * end = bytes + s.size();
			bool le_or_be = true;
			std::string utf32str;
			//If there is a BOM, ignore it
			if(s.size() >= 2)
			{
				if(bytes[0] == 0xFF && bytes[1] == 0xFE)
				{
					bytes += 2;
					le_or_be = true;
					put_utf32char(utf32str, 0xFEFF, true);
				}
				else if(bytes[0] == 0xFE && bytes[1] == 0xFF)
				{
					bytes += 2;
					le_or_be = false;
					put_utf32char(utf32str, 0xFEFF, false);
				}
			}

			while(bytes != end)
			{
				put_utf32char(utf32str, utf16char(bytes, end, le_or_be), le_or_be);
			}
			return utf32str;
		}

		std::string utf32_to_utf8(const std::string& s)
		{
			const unsigned char * bytes = reinterpret_cast<const unsigned char*>(s.c_str());
			const unsigned char * end = bytes + (s.size() & (~4 + 1));

			std::string utf8str;
			bool le_or_be = true;
			//If there is a BOM, ignore it
			if(s.size() >= 4)
			{
				if(bytes[0] == 0 && bytes[1] == 0 && bytes[2] == 0xFE && bytes[3] == 0xFF)
				{
					le_or_be = false;
					bytes += 4;
					utf8str += (char)0xEF;
					utf8str += (char)0xBB;
					utf8str += (char)0xBF;
				}
				else if(bytes[0] == 0xFF && bytes[1] == 0xFE && bytes[2] == 0 && bytes[3] == 0)
				{
					le_or_be = true;
					bytes += 4;
					utf8str += (char)0xEF;
					utf8str += (char)0xBB;
					utf8str += (char)0xBF;
				}
			}

			while(bytes < end)
			{
				put_utf8char(utf8str, utf32char(bytes, end, le_or_be));
			}
			return utf8str;
		}

		std::string utf32_to_utf16(const std::string& s)
		{
			const unsigned char * bytes = reinterpret_cast<const unsigned char*>(s.c_str());
			const unsigned char * end = bytes + (s.size() & (~4 + 1));

			std::string utf16str;
			bool le_or_be = true;
			//If there is a BOM, ignore it
			if(s.size() >= 4)
			{
				if(bytes[0] == 0 && bytes[1] == 0 && bytes[2] == 0xFE && bytes[3] == 0xFF)
				{
					le_or_be = false;
					bytes += 4;
					put_utf16char(utf16str, 0xFEFF, false);
				}
				else if(bytes[0] == 0xFF && bytes[1] == 0xFE && bytes[2] == 0 && bytes[3] == 0)
				{
					le_or_be = true;
					bytes += 4;
					put_utf16char(utf16str, 0xFEFF, true);
				}
			}

			while(bytes < end)
			{
				put_utf16char(utf16str, utf32char(bytes, end, le_or_be), le_or_be);
			}
			return utf16str;
		}

		class charset_string
			: public charset_encoding_interface
		{
		public:
			charset_string(const std::string& s)
				: data_(s)
			{}

			charset_string(std::string&& s)
				: data_(std::move(s))
			{}

			charset_string(const std::string& s, unicode encoding)
				: data_(s), is_unicode_(true), utf_x_(encoding)
			{}

			charset_string(std::string&& s, unicode encoding)
				: data_(std::move(s)), is_unicode_(true), utf_x_(encoding)
			{}
		private:
			virtual charset_encoding_interface * clone() const
			{
				return new charset_string(*this);
			}

			virtual std::string str() const
			{
				if(is_unicode_)
				{
					std::string strbuf;
					switch(utf_x_)
					{
					case unicode::utf8:
#if defined(NANA_WINDOWS)
						strbuf = detail::utf8_to_utf16(data_, true);
						detail::put_utf16char(strbuf, 0, true);
#else
						strbuf = detail::utf8_to_utf32(data_, true);
						detail::put_utf32char(strbuf, 0, true);
#endif
						break;
					case unicode::utf16:
#if defined(NANA_WINDOWS)
						strbuf = data_;
						detail::put_utf16char(strbuf, 0, true);
#else
						strbuf = detail::utf16_to_utf32(data_);
						detail::put_utf32char(strbuf, 0, true);
#endif
						break;
					case unicode::utf32:
#if defined(NANA_WINDOWS)
						strbuf = detail::utf32_to_utf16(data_);
						detail::put_utf16char(strbuf, 0, true);
#else
						strbuf = data_;
						detail::put_utf32char(strbuf, 0, true);
#endif
						break;
					}

					std::string mbstr;
					wc2mb(mbstr, reinterpret_cast<const wchar_t*>(strbuf.c_str()));
					return mbstr;
				}
				return data_;
			}

			virtual std::string && str_move()
			{
				if(is_unicode_)
					data_ = std::move(str());
				return std::move(data_);
			}

			virtual std::string str(unicode encoding) const
			{
				if(is_unicode_ && (utf_x_ != encoding))
				{
					switch(utf_x_)
					{
					case unicode::utf8:
						switch(encoding)
						{
						case unicode::utf16:
							return detail::utf8_to_utf16(data_, true);
						case unicode::utf32:
							return detail::utf8_to_utf32(data_, true);
						default:
							break;
						}
						break;
					case unicode::utf16:
						switch(encoding)
						{
						case unicode::utf8:
							return detail::utf16_to_utf8(data_);
						case unicode::utf32:
							return detail::utf16_to_utf32(data_);
						default:
							break;
						}
						break;
					case unicode::utf32:
						switch(encoding)
						{
						case unicode::utf8:
							return detail::utf32_to_utf8(data_);
						case unicode::utf16:
							return detail::utf32_to_utf16(data_);
						default:
							break;
						}
						break;
					}
					return {};
				}
				std::string wcstr;
				if(mb2wc(wcstr, data_.c_str()))
				{
					switch(encoding)
					{
#if defined(NANA_WINDOWS)
					case unicode::utf8:
						return utf16_to_utf8(wcstr);
					case unicode::utf32:
						return utf16_to_utf32(wcstr);
					case unicode::utf16:
						return wcstr;
#else //POSIX
					case unicode::utf8:
						return utf32_to_utf8(wcstr);
					case unicode::utf16:
						return utf32_to_utf16(wcstr);
					case unicode::utf32:
						return wcstr;
#endif
					}
				}
				return {};
			}

			virtual std::wstring wstr() const
			{
				if(is_unicode_)
				{
					std::string bytes;
					switch(utf_x_)
					{
					case unicode::utf8:
#if defined(NANA_WINDOWS)
						bytes = detail::utf8_to_utf16(data_, true);
#else
						bytes = detail::utf8_to_utf32(data_, true);
#endif
						break;
					case unicode::utf16:
#if defined(NANA_WINDOWS)
						bytes = data_;
#else
						bytes = detail::utf16_to_utf32(data_);
#endif
						break;
					case unicode::utf32:
#if defined(NANA_WINDOWS)
						bytes = detail::utf32_to_utf16(data_);
#else
						bytes = data_;
#endif
						break;
					}
					return std::wstring(reinterpret_cast<const wchar_t*>(bytes.c_str()), bytes.size() / sizeof(wchar_t));
				}

				std::wstring wcstr;
				mb2wc(wcstr, data_.c_str());
				return wcstr;
			}

			virtual std::wstring&& wstr_move()
			{
				wdata_for_move_ = std::move(wstr());
				return std::move(wdata_for_move_);
			}
		private:
			std::string data_;
			std::wstring wdata_for_move_{};
			bool is_unicode_{ false };
			unicode utf_x_{ unicode::utf8 };
		};


		class charset_wstring
			: public charset_encoding_interface
		{
		public:
			charset_wstring(const std::wstring& s)
				: data_(s)
			{}

			virtual charset_encoding_interface * clone() const
			{
				return new charset_wstring(*this);
			}

			virtual std::string str() const
			{
				if(data_.size())
				{
					std::string mbstr;
					wc2mb(mbstr, data_.c_str());
					return mbstr;
				}
				return {};
			}

			virtual std::string && str_move()
			{
				data_for_move_ = std::move(str());
				return std::move(data_for_move_);
			}

			virtual std::string str(unicode encoding) const
			{
				switch(encoding)
				{
				case unicode::utf8:
#if defined(NANA_WINDOWS)
					return detail::utf16_to_utf8(std::string(reinterpret_cast<const char*>(data_.c_str()), data_.size() * sizeof(wchar_t)));
#else
					return detail::utf32_to_utf8(std::string(reinterpret_cast<const char*>(data_.c_str()), data_.size() * sizeof(wchar_t)));
#endif
				case unicode::utf16:
#if defined(NANA_WINDOWS)
					return std::string(reinterpret_cast<const char*>(data_.c_str()), data_.size() * sizeof(wchar_t));
#else
					return detail::utf32_to_utf16(std::string(reinterpret_cast<const char*>(data_.c_str()), data_.size() * sizeof(wchar_t)));
#endif
				case unicode::utf32:
#if defined(NANA_WINDOWS)
					return detail::utf16_to_utf32(std::string(reinterpret_cast<const char*>(data_.c_str()), data_.size() * sizeof(wchar_t)));
#else
					return std::string(reinterpret_cast<const char*>(data_.c_str()), data_.size() * sizeof(wchar_t));
#endif
				}
				return {};
			}

			virtual std::wstring wstr() const
			{
				return data_;
			}

			virtual std::wstring && wstr_move()
			{
				return std::move(data_);
			}
		private:
			std::wstring data_;
			std::string data_for_move_{};
		};
#endif
	}
	//class charset
		charset::charset(const charset& rhs)
			: impl_(rhs.impl_ ? rhs.impl_->clone() : 0)
		{}

		charset & charset::operator=(const charset& rhs)
		{
			if(this != &rhs)
			{
				delete impl_;
				impl_ = (rhs.impl_ ? rhs.impl_->clone() : 0);
			}
			return *this;
		}

		charset::charset(charset&& r)
			: impl_(r.impl_)
		{
			r.impl_ = 0;
		}

		charset & charset::operator=(charset&& r)
		{
			if(this != &r)
			{
				delete impl_;
				impl_ = r.impl_;
				r.impl_ = nullptr;
			}
			return *this;
		}

		charset::charset(const std::string& s)
			: impl_(new detail::charset_string(s))
		{}

		charset::charset(std::string&& s)
			: impl_(new detail::charset_string(std::move(s)))
		{}

		charset::charset(const std::string& s, unicode encoding)
			: impl_(new detail::charset_string(s, encoding))
		{}

		charset::charset(std::string&& s, unicode encoding)
			: impl_(new detail::charset_string(std::move(s), encoding))
		{}

		charset::charset(const std::wstring& s)
			: impl_(new detail::charset_wstring(s))
		{}

		charset::charset(std::wstring&& s)
			: impl_(new detail::charset_wstring(std::move(s)))
		{}

		charset::~charset()
		{
			delete impl_;
		}

		charset::operator std::string() const
		{
			return impl_->str();
		}

		charset::operator std::string&&()
		{
			return impl_->str_move();
		}

		charset::operator std::wstring() const
		{
			return impl_->wstr();
		}

		charset::operator std::wstring&&()
		{
			return impl_->wstr_move();
		}

		std::string charset::to_bytes(unicode encoding) const
		{
			return impl_->str(encoding);
		}
	//end class charset

}//end namespace nana
