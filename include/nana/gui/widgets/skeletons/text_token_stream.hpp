/*
 *	Text Token Stream
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2018 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/skeletons/text_token_stream.hpp
 */

#ifndef NANA_GUI_WIDGETS_SKELETONS_TEXT_TOKEN_STREAM
#define NANA_GUI_WIDGETS_SKELETONS_TEXT_TOKEN_STREAM

#include <nana/gui/layout_utility.hpp>

#include <deque>
#include <vector>
#include <list>
#include <stack>
#include <stdexcept>

#include <nana/push_ignore_diagnostic>
#include <nana/unicode_bidi.hpp>

namespace nana{ namespace widgets{	namespace skeletons
{
	//The tokens are defined for representing a text, the tokens are divided
	//into two parts.
	//Formatted tokens: The tokens present in a format block, they form the format-syntax.
	//Data tokens: The tokens present a text displaying on the screen.
	enum class token
	{
		tag_begin, tag_end, format_end,
		font, bold, size, color, url, target, image, top, center, bottom, baseline,
		number, string, _true, _false, red, green, blue, white, black, binary, min_limited, max_limited,

		equal, comma, backslash,
		data, endl,
		eof
	};

	class tokenizer
	{
	public:
		tokenizer(const std::wstring& s, bool format_enabled)
			:	iptr_(s.data()),
				endptr_(s.data() + s.size()),
				format_enabled_(format_enabled)
		{
		}

		void push(token tk)
		{
			revert_token_ = tk;
		}

		//Read the token.
		token read()
		{
			if(revert_token_ != token::eof)
			{
				token tk = revert_token_;
				revert_token_ = token::eof;
				return tk;
			}

			if(iptr_ == endptr_)
				return token::eof;

			//Check whether it is a format token.
			if(format_enabled_ && format_state_)
				return _m_format_token();

			return _m_token();
		}

		const ::std::wstring& idstr() const
		{
			return idstr_;
		}

		const std::pair<std::wstring, std::wstring>& binary() const
		{
			return binary_;
		}

		std::pair<unsigned, unsigned> binary_number() const
		{
			return{ std::stoul(binary_.first), std::stoul(binary_.second) };
		}

		int number() const
		{
			return std::stoi(idstr_, nullptr, 0);
		}
	private:
		static bool _m_unicode_word_breakable(const wchar_t* ch) noexcept
		{
			if (*ch)
				return unicode_wordbreak(*ch, ch[1]);
			return true;
		}

		//Read the data token
		token _m_token()
		{
			wchar_t ch = *iptr_;

			if(ch > 0xFF)
			{
				//This is the Unicode.

				idstr_.clear();
				idstr_.append(1, ch);

				if (_m_unicode_word_breakable(iptr_))
				{
					++iptr_;
					return token::data;
				}

				ch = *++iptr_;
				while((iptr_ != endptr_) && (ch > 0xFF) && (false == _m_unicode_word_breakable(iptr_)))
				{
					idstr_.append(1, ch);

					ch = *++iptr_;
				}

				return token::data;
			}

			if('\n' == ch)
			{
				++iptr_;
				return token::endl;
			}

			if(('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z'))
			{
				auto idstr = iptr_;
				do
				{
					ch = *(++iptr_);
				}
				while(('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z'));

				idstr_.assign(idstr, iptr_);

				return token::data;
			}

			if('0' <= ch && ch <= '9')
			{
				_m_read_number();
				return token::data;
			}

			if(('<' == ch) && format_enabled_)
			{
				//pos keeps the current position, and it used for restring
				//iptr_ when the search is failed.
				auto pos = ++iptr_;
				_m_eat_whitespace();
				if(*iptr_ == '/')
				{
					++iptr_;
					_m_eat_whitespace();
					if(*iptr_ == '>')
					{
						++iptr_;
						return token::format_end;
					}
				}

				//Restore the iptr_;
				iptr_ = pos;

				format_state_ = true;
				return token::tag_begin;
			}


			//Escape
			if(this->format_enabled_ && (ch == '\\'))
			{
				if(iptr_ + 1 < endptr_)
				{
					ch = *(iptr_ + 1);

					if ('<' == ch || '>' == ch)	//two characters need to be escaped.
					{
						iptr_ += 2;
					}
					else
					{
						//ignore escape
						ch = '\\';
						iptr_++;
					}
				}
				else
				{
					iptr_ = endptr_;
					return token::eof;
				}
			}
			else
				++iptr_;

			idstr_.clear();
			idstr_.append(1, ch);
			return token::data;
		}

		//Read the format token
		token _m_format_token()
		{
			_m_eat_whitespace();

			auto ch = *iptr_++;

			switch(ch)
			{
			case ',':	return token::comma;
			case '/':	return token::backslash;
			case '=':	return token::equal;
			case '>':
				format_state_ = false;
				return token::tag_end;
			case '"':
				//Here is a string and all the meta characters will be ignored except "
				{
					auto str = iptr_;

					while((iptr_ != endptr_) && (*iptr_ != '"'))
						++iptr_;

					idstr_.assign(str, iptr_++);
				}
				return token::string;
			case '(':
				_m_eat_whitespace();
				if((iptr_ < endptr_) && _m_is_idstr_element(*iptr_))
				{
					auto pbegin = iptr_;
					while((iptr_ < endptr_) && _m_is_idstr_element(*iptr_))
						++iptr_;

					binary_.first.assign(pbegin, iptr_);

					_m_eat_whitespace();
					if((iptr_ < endptr_) && (',' == *iptr_))
					{
						++iptr_;
						_m_eat_whitespace();
						if((iptr_ < endptr_) && _m_is_idstr_element(*iptr_))
						{
							pbegin = iptr_;
							while((iptr_ < endptr_) && _m_is_idstr_element(*iptr_))
								++iptr_;

							binary_.second.assign(pbegin, iptr_);

							_m_eat_whitespace();
							if((iptr_ < endptr_) && (')' == *iptr_))
							{
								++iptr_;
								return token::binary;
							}
						}
					}
				}
				return token::eof;
			}

			

			if(('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') || '_' == ch)
			{
				--iptr_;
				//Here is a identifier
				_m_read_idstr();

				if(L"font" == idstr_)
					return token::font;
				else if(L"bold" == idstr_)
					return token::bold;
				else if(L"size" == idstr_)
					return token::size;
				else if(L"baseline" == idstr_)
					return token::baseline;
				else if(L"top" == idstr_)
					return token::top;
				else if(L"center" == idstr_)
					return token::center;
				else if(L"bottom" == idstr_)
					return token::bottom;
				else if(L"color" == idstr_)
					return token::color;
				else if(L"image" == idstr_)
					return token::image;
				else if(L"true" == idstr_)
					return token::_true;
				else if(L"url" == idstr_)
					return token::url;
				else if(L"target" == idstr_)
					return token::target;
				else if(L"false" == idstr_)
					return token::_false;
				else if(L"red" == idstr_)
					return token::red;
				else if(L"green" == idstr_)
					return token::green;
				else if(L"blue" == idstr_)
					return token::blue;
				else if(L"white" == idstr_)
					return token::white;
				else if(L"black" == idstr_)
					return token::black;
				else if(L"min_limited" == idstr_)
					return token::min_limited;
				else if(L"max_limited" == idstr_)
					return token::max_limited;

				return token::string;
			}

			if('0' <= ch && ch <= '9')
			{
				--iptr_;
				_m_read_number();
				return token::number;
			}

			return token::eof;
		}

		static bool _m_is_idstr_element(wchar_t ch)
		{
			return (('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') || ('_' == ch) || ('0' <= ch && ch <= '9'));
		}

		//Read the identifier.
		void _m_read_idstr()
		{
			auto idstr = iptr_;

			wchar_t ch;
			do
			{
				ch = *(++iptr_);
			}
			while(('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z') || ('_' == ch) || ('0' <= ch && ch <= '9'));

			idstr_.assign(idstr, iptr_);
		}

		//Read the number
		void _m_read_number()
		{
			idstr_.clear();

			wchar_t ch = *iptr_;

			idstr_ += ch;

			//First check the number whether will be a hex number.
			if('0' == ch)
			{
				ch = *++iptr_;
				if((!('0' <= ch && ch <= '9')) && (ch != 'x' && ch != 'X'))
					return;

				if(ch == 'x' || ch == 'X')
				{
					//Here is a hex number
					idstr_ += 'x';
					ch = *++iptr_;
					while(('0' <= ch && ch <= '9') || ('a' <= ch && ch <= 'f') || ('A' <= ch && ch <= 'F'))
					{
						idstr_ += ch;
						ch = *++iptr_;
					}
					return;
				}

				//Here is not a hex number
				idstr_ += ch;
			}

			ch = *++iptr_;
			while('0' <= ch && ch <= '9')
			{
				idstr_ += ch;
				ch = *++iptr_;
			}
		}

		void _m_eat_whitespace()
		{
			while(true)
			{
				switch(*iptr_)
				{
				case ' ':
				case '\t':
					++iptr_;
					break;
				default:
					return;
				}
			}
		}
	private:
		const wchar_t * iptr_;
		const wchar_t * endptr_;
		const bool	format_enabled_;
		bool	format_state_{false};

		std::wstring idstr_;
		std::pair<std::wstring, std::wstring> binary_;
		token revert_token_{token::eof};
	};

	//The fblock states a format, and a format from which it is inherted
	struct fblock
	{
		struct aligns
		{
			enum t
			{
				top, center, bottom,
				baseline
			};
		};

		::std::string	font;
		double	font_size;
		bool	bold;
		bool	bold_empty;	//bold should be ignored if bold_empty is true
		aligns::t	text_align;
		::nana::color	bgcolor;	//If the color is not specified, it will be ignored, and the system will search for its parent.
		::nana::color	fgcolor;	//ditto

		::std::wstring	target;
		::std::wstring	url;

		fblock * parent;
	};

	//The abstruct data class states a data.
	class data
	{
	public:
		typedef nana::paint::graphics& graph_reference;

		virtual ~data(){}

		virtual bool	is_text() const = 0;
		virtual bool	is_whitespace() const = 0;
		virtual const std::wstring& text() const = 0;
		virtual void measure(graph_reference) = 0;
		virtual void nontext_render(graph_reference, int x, int y) = 0;
		virtual const nana::size & size() const = 0;
		virtual std::size_t ascent() const = 0;
	};


	class data_text
		: public data
	{
	public:
		data_text(const std::wstring& s)
			: str_(s)
		{}
	private:
		virtual bool is_text() const override
		{
			return true;
		}

		virtual bool is_whitespace() const override
		{
			return false;
		}

		virtual const std::wstring& text() const override
		{
			return str_;
		}

		virtual void measure(graph_reference graph) override
		{
			size_ = graph.text_extent_size(str_);
			unsigned ascent;
			unsigned descent;
			unsigned internal_leading;
			graph.text_metrics(ascent, descent, internal_leading);
			ascent_ = ascent;
		}

		virtual void nontext_render(graph_reference, int, int) override
		{
		}

		virtual const nana::size & size() const override
		{
			return size_;
		}

		virtual std::size_t ascent() const override
		{
			return ascent_;
		}
	private:
		std::wstring str_;
		nana::size	size_;
		std::size_t ascent_;
	};

	class data_image
		: public data
	{
	public:
		data_image(const std::wstring& imgpath, const nana::size & sz, std::size_t limited)
			: image_(imgpath)//, limited_(limited)
		{
			size_ = image_.size();

			if(sz.width != 0 && sz.height != 0)
			{
				bool make_fit = false;
				switch(limited)
				{
				case 1:
					make_fit = (size_.width < sz.width || size_.height < sz.height);
					break;
				case 2:
					make_fit = (size_.width > sz.width || size_.height > sz.height);
					break;
				}

				if(make_fit)
				{
					nana::size  res;
					nana::fit_zoom(size_, sz, res);
					size_ = res;
				}
				else
					size_ = sz;
			}
		}
	private:
		//implement data interface
		virtual bool is_text() const override
		{
			return false;
		}

		virtual bool is_whitespace() const override
		{
			return false;
		}

		virtual const std::wstring& text() const override
		{
			return str_;
		}

		virtual void measure(graph_reference) override
		{
		}

		virtual void nontext_render(graph_reference graph, int x, int y) override
		{
			if(size_ != image_.size())
				image_.stretch(::nana::rectangle{ image_.size() }, graph, nana::rectangle(x, y, size_.width, size_.height));
			else
				image_.paste(graph, point{ x, y });
		}

		virtual const nana::size & size() const override
		{
			return size_;
		}

		virtual std::size_t ascent() const override
		{
			return size_.height;
		}
	private:
		std::wstring str_;
		nana::paint::image image_;
		nana::size size_;
	};

	class dstream
	{
		struct value
		{
			fblock * fblock_ptr;
			data * data_ptr;
		};
	public:
		typedef std::list<std::deque<value> >::iterator iterator;
		typedef std::deque<value> linecontainer;

		~dstream()
		{
			close();
		}

		void close()
		{
			for(auto & values: lines_)
			{
				for(std::deque<value>::iterator u = values.begin(); u != values.end(); ++u)
					delete u->data_ptr;
			}

			lines_.clear();

			for(auto p : fblocks_)
				delete p;

			fblocks_.clear();
		}

		void parse(const std::wstring& s, bool format_enabled)
		{
			close();

			tokenizer tknizer(s, format_enabled);
			std::stack<fblock*> fstack;

			fstack.push(_m_create_default_fblock());

			while(true)
			{
				token tk = tknizer.read();

				switch(tk)
				{
				case token::data:
					_m_data_factory(tk, tknizer.idstr(), fstack.top(), lines_.back());
					break;
				case token::endl:
					lines_.emplace_back();
					break;
				case token::tag_begin:
					_m_parse_format(tknizer, fstack);

					if(attr_image_.path.size())
					{
						_m_data_image(fstack.top(), lines_.back());
						//This fblock just serves the image. So we should restore the pervious fblock
						fstack.pop();
					}

					break;
				case token::format_end:
					if(fstack.size() > 1)
						fstack.pop();
					break;
				case token::eof:
					return;
				default:
					throw std::runtime_error("invalid token");
				}
			}
		}

		iterator begin()
		{
			return lines_.begin();
		}

		iterator end()
		{
			return lines_.end();
		}
	private:
		void _m_parse_format(tokenizer & tknizer, std::stack<fblock*> & fbstack)
		{
			fblock * fp = _m_inhert_from(fbstack.top());

			attr_image_.reset();

			while(true)
			{
				switch(tknizer.read())
				{
				case token::comma:	//Eat the comma, now the comma can be omitted.
					break;
				case token::eof:
				case token::tag_end:
					fblocks_.push_back(fp);
					fbstack.push(fp);
					return;
				case token::font:
					if(token::equal != tknizer.read())
						throw std::runtime_error("");

					if(token::string != tknizer.read())
						throw std::runtime_error("");

					fp->font = to_utf8(tknizer.idstr());
					break;
				case token::size:
					if(token::equal != tknizer.read())
						throw std::runtime_error("");

					switch(tknizer.read())
					{
					case token::number:
						fp->font_size = tknizer.number();
						break;
					case token::binary:
						{
							auto value = tknizer.binary_number();
							attr_image_.size.width = value.first;
							attr_image_.size.height = value.second;
						}
						break;
					default:
						throw std::runtime_error("");
					}
					break;
				case token::color:
					if(token::equal != tknizer.read())
						throw std::runtime_error("");

					switch(tknizer.read())
					{
					case token::number:
					{
						pixel_color_t px;
						px.value = static_cast<unsigned>(tknizer.number());
						fp->fgcolor = {px.element.red, px.element.green, px.element.blue};
					}
						break;
					case token::red:
						fp->fgcolor = colors::red;
						break;
					case token::green:
						fp->fgcolor = colors::green;
						break;
					case token::blue:
						fp->fgcolor = colors::blue;
						break;
					case token::white:
						fp->fgcolor = colors::white;
						break;
					case token::black:
						fp->fgcolor = colors::black;
						break;
					default:
						throw std::runtime_error("");
					}
					break;
				case token::red:	//support the omitting of color.
					fp->fgcolor = colors::red;
					break;
				case token::green:	//support the omitting of color.
					fp->fgcolor = colors::green;
					break;
				case token::blue:	//support the omitting of color.
					fp->fgcolor = colors::blue;
					break;
				case token::white:	//support the omitting of color.
					fp->fgcolor = colors::white;
					break;
				case token::black:	//support the omitting of color.
					fp->fgcolor = colors::black;
					break;
				case token::baseline:
					fp->text_align = fblock::aligns::baseline;
					break;
				case token::top:
					fp->text_align = fblock::aligns::top;
					break;
				case token::center:
					fp->text_align = fblock::aligns::center;
					break;
				case token::bottom:
					fp->text_align = fblock::aligns::bottom;
					break;
				case token::image:
					if(token::equal != tknizer.read())
						throw std::runtime_error("");

					if(token::string != tknizer.read())
						throw std::runtime_error("");

					attr_image_.path = tknizer.idstr();
					break;
				case token::min_limited:
					attr_image_.limited = 1;
					break;
				case token::max_limited:
					attr_image_.limited = 2;
					break;
				case token::target:
					if(token::equal != tknizer.read())
						throw std::runtime_error("error: a '=' is required behind 'target'");

					if(token::string != tknizer.read())
						throw std::runtime_error("error: the value of 'target' should be a string");

					fp->target = tknizer.idstr();
					break;
				case token::url:
					if(token::equal != tknizer.read())
						throw std::runtime_error("error: a '=' is required behind 'url'");

					if(token::string != tknizer.read())
						throw std::runtime_error("error: the value of 'url' should be a string");

					fp->url = tknizer.idstr();
					break;
				case token::bold:
					{
						token tk = tknizer.read();
						if(token::equal == tk)
						{
							switch(tknizer.read())
							{
							case token::_true:
								fp->bold = true;
								break;
							case token::_false:
								fp->bold = false;
								break;
							default:
								throw std::runtime_error("");
							}
						}
						else
						{
							tknizer.push(tk);
							fp->bold = true;
						}
						fp->bold_empty = false;
					}
					break;
                default:
                    throw std::runtime_error("");
				}
			}
		}

		fblock* _m_create_default_fblock()
		{
			//Make sure that there is not a fblock is created.
			if(fblocks_.size())
				return fblocks_.front();

			//Create a default fblock.
			fblock * fbp = new fblock;

			fbp->font_size = -1;
			fbp->bold = false;
			fbp->bold_empty = true;
			fbp->text_align = fblock::aligns::baseline;

			fbp->parent = nullptr;

			fblocks_.push_back(fbp);
			lines_.emplace_back();

			return fbp;
		}

		fblock * _m_inhert_from(fblock* fp)
		{
			fblock * fbp = new fblock;

			fbp->font = fp->font;
			fbp->font_size = fp->font_size;
			fbp->bold = fp->bold;
			fbp->bold_empty = fp->bold_empty;
			fbp->text_align = fp->text_align;

			fbp->bgcolor = fp->bgcolor;
			fbp->fgcolor = fp->fgcolor;

			fbp->target = fp->target;

			fbp->parent = fp;

			return fbp;
		}

		void _m_data_factory(token tk, const std::wstring& idstr, fblock* fp, std::deque<value>& line)
		{
			value v;
			v.fblock_ptr = fp;

			switch(tk)
			{
			case token::data:
				v.data_ptr = new data_text(idstr);
				break;
			default:
				break;
			}

			line.push_back(v);
		}

		void _m_data_image(fblock* fp, std::deque<value>& line)
		{
			value v;
			v.fblock_ptr = fp;

			v.data_ptr = new data_image(attr_image_.path, attr_image_.size, attr_image_.limited);

			line.push_back(v);
		}

	private:
		std::vector<fblock*> fblocks_;
		std::list<std::deque<value> > lines_;

		struct attr_image_tag
		{
			std::wstring	path;
			nana::size		size;
			std::size_t		limited;

			void reset()
			{
				path.clear();
				size.width = size.height = 0;
				limited = 0;
			}
		}attr_image_;
	};
}//end namespace skeletons
}//end namespace widgets
}//end namepsace nana
#include <nana/pop_ignore_diagnostic>
#endif	//NANA_GUI_WIDGETS_SKELETONS_TEXT_TOKEN_STREAM
