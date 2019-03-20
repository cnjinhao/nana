/*
 *	A textbase class implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/skeletons/textbase.hpp
 *	@description: This class manages the multi-line text and provides some operation on text
 */

#ifndef NANA_GUI_WIDGET_DETAIL_TEXTBASE_HPP
#define NANA_GUI_WIDGET_DETAIL_TEXTBASE_HPP
#include <nana/push_ignore_diagnostic>

#include <nana/charset.hpp>
#include <nana/basic_types.hpp>
#include <nana/traits.hpp>
#include <nana/filesystem/filesystem.hpp>

#include "textbase_export_interface.hpp"

#include <deque>
#include <fstream>
#include <stdexcept>

namespace nana
{
namespace widgets
{
namespace skeletons
{
	template<typename CharT>
	class textbase
		: public ::nana::noncopyable
	{
	public:
		using char_type = CharT;
		using string_type = std::basic_string<CharT>;
		using size_type = typename string_type::size_type;
		using path_type = std::filesystem::path;

		textbase()
		{
			attr_max_.reset();
			//Insert an empty string for the first line of empty text.
			text_cont_.emplace_back();
		}

		void set_event_agent(textbase_event_agent_interface * evt)
		{
			evt_agent_ = evt;
		}

		bool empty() const
		{
			return (text_cont_.empty() ||
				((text_cont_.size() == 1) && text_cont_.front().empty()));
		}

		bool load(const path_type& file)
		{
			std::ifstream ifs{ file.string() };
			if (!ifs)
				return false;

			std::error_code err;
			auto const bytes = file_size(file, err);
			if (err)
				return false;

			if(bytes >= 2)
			{
				int ch = ifs.get();
				if(0xEF == ch && bytes >= 3)
				{
					//UTF8
					ch = ifs.get();
					if(0xBB == ch && 0xBF == ifs.get())
					{
						ifs.close();
						return load(file, nana::unicode::utf8);
					}
				}
				else if(0xFF == ch)
				{
					if(0xFE == ifs.get())
					{
						//UTF16,UTF32
						if((bytes >= 4) && (ifs.get() == 0 && ifs.get() == 0))
						{
							ifs.close();
							return load(file, nana::unicode::utf32);
						}
						ifs.close();
						return load(file, nana::unicode::utf16);
					}
				}
				else if(0xFE == ch)
				{
					if(ifs.get() == 0xFF)
					{
						//UTF16(big-endian)
						ifs.close();
						return load(file, nana::unicode::utf16);
					}
				}
				else if(0 == ch)
				{
					if(bytes >= 4 && ifs.get() == 0)
					{
						ch = ifs.get();
						if(0xFE == ch && ifs.get() == 0xFF)
						{
							//UTF32(big_endian)
							ifs.close();
							return load(file, nana::unicode::utf32);
						}
					}
				}
			}

			ifs.clear();
			ifs.seekg(0, std::ios::beg);

			text_cont_.clear();		//Clear only if the file can be opened.
			attr_max_.reset();

			std::string str_mbs;
			while(ifs.good())
			{
				std::getline(ifs, str_mbs);
				text_cont_.emplace_back(static_cast<string_type&&>(nana::charset{ str_mbs }));
				if (text_cont_.back().size() > attr_max_.size)
				{
					attr_max_.size = text_cont_.back().size();
					attr_max_.line = text_cont_.size() - 1;
				}
			}

			_m_saved(file);
			return true;
		}

		static void byte_order_translate_2bytes(std::string& str)
		{
			char * s = const_cast<char*>(str.c_str());
			char * end = s + str.size();
			for(; s < end; s += 2)
			{
				char c = *s;
				*s = *(s + 1);
				*(s + 1) = c;
			}
		}

		static void byte_order_translate_4bytes(std::string& str)
		{
			char * s = const_cast<char*>(str.c_str());
			char * end = s + str.size();
			for(; s < end; s += 4)
			{
				char c = *s;
				*s = *(s + 3);
				*(s + 3) = c;

				c = *(s + 1);
				*(s + 1) = *(s + 2);
				*(s + 2) = c;
			}
		}

		bool load(const path_type& file, nana::unicode encoding)
		{
			std::ifstream ifs{ file.string() };

			if (!ifs)
				return false;

			std::string str;
			bool big_endian = true;

			if(ifs.good())
			{
				text_cont_.clear();		//Clear only if the file can be opened.
				attr_max_.reset();

				std::getline(ifs, str);

				std::size_t len_of_BOM = 0;
				switch(encoding)
				{
				case nana::unicode::utf8:
					len_of_BOM = 3;	break;
				case nana::unicode::utf16:
					len_of_BOM = 2;	break;
				case nana::unicode::utf32:
					len_of_BOM = 4;	break;
				default:
					throw std::runtime_error("Specified a wrong UTF");
				}

				big_endian = (str[0] == 0x00 || str[0] == char(0xFE));
				str.erase(0, len_of_BOM);
				if(big_endian)
				{
					if(nana::unicode::utf16 == encoding)
						byte_order_translate_2bytes(str);
					else
						byte_order_translate_4bytes(str);
				}

				text_cont_.emplace_back(static_cast<string_type&&>(nana::charset{ str, encoding }));
				attr_max_.size = text_cont_.back().size();
				attr_max_.line = 0;
			}

			while(ifs.good())
			{
				std::getline(ifs, str);

				if(big_endian)
				{
					if(nana::unicode::utf16 == encoding)
						byte_order_translate_2bytes(str);
					else
						byte_order_translate_4bytes(str);
				}

				text_cont_.emplace_back(static_cast<string_type&&>(nana::charset{ str, encoding }));
				if (text_cont_.back().size() > attr_max_.size)
				{
					attr_max_.size = text_cont_.back().size();
					attr_max_.line = text_cont_.size() - 1;
				}
			}

			_m_saved(file);
			return true;
		}

		void store(const path_type& filename, bool is_unicode, ::nana::unicode encoding) const
		{
			std::ofstream ofs(filename.string(), std::ios::binary);
			if(ofs && text_cont_.size())
			{
				auto i = text_cont_.cbegin();
				auto const count = text_cont_.size() - 1;

				std::string last_mbs;

				if (is_unicode)
				{
					const char * le_boms[] = { "\xEF\xBB\xBF", "\xFF\xFE", "\xFF\xFE\x0\x0" };	//BOM for little-endian
					int bytes = 0;
					switch (encoding)
					{
					case nana::unicode::utf8:
						bytes = 3;	break;
					case nana::unicode::utf16:
						bytes = 2;	break;
					case nana::unicode::utf32:
						bytes = 4;	break;
					}

					if (bytes)
						ofs.write(le_boms[static_cast<int>(encoding)], bytes);

					for (std::size_t pos = 0; pos < count; ++pos)
					{
						auto mbs = nana::charset(*(i++)).to_bytes(encoding);
						ofs.write(mbs.c_str(), static_cast<std::streamsize>(mbs.size()));
						ofs.write("\r\n", 2);
					}

					last_mbs = nana::charset(text_cont_.back()).to_bytes(encoding);
				}
				else
				{
					for (std::size_t pos = 0; pos < count; ++pos)
					{
						std::string mbs = nana::charset(*(i++));
						ofs.write(mbs.c_str(), mbs.size());
						ofs.write("\r\n", 2);
					}
					last_mbs = nana::charset(text_cont_.back());
				}

				ofs.write(last_mbs.c_str(), static_cast<std::streamsize>(last_mbs.size()));
				_m_saved(filename);
			}
		}

		//Triggers the text_changed event.
		//It is exposed for outter classes. For a outter class(eg. text_editor), a changing text content operation
		//may contains multiple textbase operations, therefore, the outter class determines when an event should be triggered.
		//
		//Addtional, using text_changed() method, it is possible to allow a outter class performing some updating operations
		//before triggering text_changed event.
		void text_changed()
		{
			if (!changed_)
			{
				_m_emit_first_change();
				changed_ = true;
			}

			if (edited_)
			{
				if (evt_agent_)
					evt_agent_->text_changed();

				edited_ = false;
			}
		}

		size_type lines() const
		{
			return text_cont_.size();
		}

		const string_type& getline(size_type pos) const
		{
			if (pos < text_cont_.size())
				return text_cont_[pos];
			return nullstr_;
		}

		std::pair<size_t, size_t> max_line() const
		{
			return std::make_pair(attr_max_.line, attr_max_.size);
		}
	public:
		void replace(size_type pos, string_type && text)
		{
			if (text_cont_.size() <= pos)
			{
				text_cont_.emplace_back(std::move(text));
				pos = text_cont_.size() - 1;
			}
			else
				_m_at(pos).swap(text);

			_m_make_max(pos);
			edited_ = true;
		}

		void insert(upoint pos, string_type && str)
		{
			if(pos.y < text_cont_.size())
			{
				string_type& lnstr = _m_at(pos.y);

				if(pos.x < lnstr.size())
					lnstr.insert(pos.x, str);
				else
					lnstr += str;
			}
			else
			{
				text_cont_.emplace_back(std::move(str));
				pos.y = static_cast<unsigned>(text_cont_.size() - 1);
			}

			_m_make_max(pos.y);
			edited_ = true;
		}

		void insertln(size_type pos, string_type&& str)
		{
			if (pos < text_cont_.size())
				text_cont_.emplace(_m_iat(pos), std::move(str));
			else
				text_cont_.emplace_back(std::move(str));

			_m_make_max(pos);
			edited_ = true;
		}

		void erase(size_type line, size_type pos, size_type count)
		{
			if (line < text_cont_.size())
			{
				string_type& lnstr = _m_at(line);
				if ((pos == 0) && (count >= lnstr.size()))
					lnstr.clear();
				else
					lnstr.erase(pos, count);

				if (attr_max_.line == line)
					_m_scan_for_max();

				edited_ = true;
			}
		}

		bool erase(size_type pos, std::size_t n)
		{
			//Bounds checking
			if ((pos >= text_cont_.size()) || (0 == n))
				return false;
			
			if (pos + n > text_cont_.size())
				n = text_cont_.size() - pos;

			text_cont_.erase(_m_iat(pos), _m_iat(pos + n));

			if (pos <= attr_max_.line && attr_max_.line < pos + n)
				_m_scan_for_max();
			else if (pos < attr_max_.line)
				attr_max_.line -= n;

			edited_ = true;
			return true;
		}

		void erase_all()
		{
			text_cont_.clear();
			attr_max_.reset();
			text_cont_.emplace_back(); //text_cont_ must not be empty

			_m_saved({});
		}

		void merge(size_type pos)
		{
			if(pos + 1 < text_cont_.size())
			{
				auto i = _m_iat(pos + 1);
				_m_at(pos) += *i;

				text_cont_.erase(i);
				_m_make_max(pos);

				//If the maxline is behind the pos line,
				//decrease the maxline. Because a line between maxline and pos line
				//has been deleted.
				if(pos < attr_max_.line)
					--attr_max_.line;

				edited_ = true;
			}
		}

		const path_type& filename() const
		{
			return filename_;
		}

		bool edited() const
		{
			return changed_;
		}

		void reset_status(bool remain_saved_filename)
		{
			if(!remain_saved_filename)
				filename_.clear();

			changed_ = false;
		}

		bool saved() const
		{
			return !(changed_ || filename_.empty());
		}
	private:
		string_type& _m_at(size_type pos)
		{
			return *_m_iat(pos);
		}

		typename std::deque<string_type>::iterator _m_iat(size_type pos)
		{
			return text_cont_.begin() + pos;
		}

		void _m_make_max(std::size_t pos)
		{
			const string_type& str = _m_at(pos);
			if(str.size() > attr_max_.size)
			{
				attr_max_.size = str.size();
				attr_max_.line = pos;
			}
		}

		void _m_scan_for_max()
		{
			attr_max_.reset();
			for (std::size_t i = 0; i < text_cont_.size(); ++i)
				_m_make_max(i);
		}

		void _m_emit_first_change() const
		{
			if (evt_agent_)
				evt_agent_->first_change();
		}

		void _m_saved(const path_type& filename) const
		{
			if((filename_ != filename) || changed_)
			{
				filename_ = filename;
				_m_emit_first_change();
			}

			changed_ = false;
		}
	private:
		std::deque<string_type> text_cont_;
		textbase_event_agent_interface* evt_agent_{ nullptr };

		mutable bool		changed_{ false };
		mutable bool		edited_{ false };
		mutable path_type filename_;	///< The saved filename
		const string_type nullstr_;

		struct attr_max
		{
			std::size_t line; ///< The line number of max line
			std::size_t size; ///< The number of characters in max line

			void reset()
			{
				line = 0;
				size = 0;
			}
		}attr_max_;
	};

}//end namespace detail
}//end namespace widgets
}//end namespace nana
#include <nana/pop_ignore_diagnostic>

#endif
