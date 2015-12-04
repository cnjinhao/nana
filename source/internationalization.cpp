/*
*	An Implementation of i18n
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2014 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file: nana/internationalization.cpp
*/

#include <nana/internationalization.hpp>
#include <nana/gui/widgets/widget.hpp>
#include <unordered_map>
#include <fstream>

#if defined(STD_THREAD_NOT_SUPPORTED)
#include <nana/std_mutex.hpp>
#else
#include <mutex>
#endif


#include <map>

namespace nana
{
	namespace internationalization_parts
	{
		enum class token
		{
			msgid, msgstr, string, eof
		};

		//Forward declaration
		void use_eval();

		class tokenizer
		{
		public:
			tokenizer(const std::string& file)
			{
				std::ifstream ifs(file.data(), std::ios::binary);
				if (ifs)
				{
					ifs.seekg(0, std::ios::end);
					auto len = static_cast<unsigned>(ifs.tellg());
					ifs.seekg(0, std::ios::beg);
					if (len > 0)
					{
						data_.reset(new char[len]);
						ifs.read(data_.get(), len);
						read_ptr_ = data_.get();
						end_ptr_ = read_ptr_ + len;
					}
				}
			}

			token read()
			{
				if (read_ptr_ == end_ptr_)
					return token::eof;
				str_.clear();
				_m_eat_ws();

				if (*read_ptr_ == '"')
				{
					bool reach_right_quota;
					while (true)
					{
						reach_right_quota = false;
						bool escape = false;
						for (auto i = read_ptr_ + 1; i != end_ptr_; ++i)
						{
							if (escape)
							{
								escape = false;
								str_ += *i;
								continue;
							}

							if ('"' == *i)
							{
								read_ptr_ = i + 1;
								reach_right_quota = true;
								break;
							}
							else if ('\\' != *i)
								str_ += *i;
							else
								escape = true;
						}
						_m_eat_ws();
						if (read_ptr_ == end_ptr_ || '"' != *read_ptr_)
							break;
					}

					if (reach_right_quota)
						return token::string;
				}
				else if ('a' <= *read_ptr_ && *read_ptr_ <= 'z')
				{
					for (auto i = read_ptr_; i != end_ptr_; ++i)
					{
						if (*i < 'a' || 'z' < *i)
						{
							std::string id(read_ptr_, i);
							read_ptr_ = i;
							if (id == "msgid")
								return token::msgid;
							else if (id == "msgstr")
								return token::msgstr;
							break;
						}
					}
					read_ptr_ = end_ptr_;
				}
				return token::eof;
			}

			std::string& get_str()
			{
				return str_;
			}
		private:
			void _m_eat_ws()
			{
				for (auto i = read_ptr_; i != end_ptr_; ++i)
				{
					switch (*i)
					{
					case ' ': case '\t': case '\r': case '\n':
						break;
					default:
						read_ptr_ = i;
						return;
					}
				}
				read_ptr_ = end_ptr_;
			}

		private:
			std::unique_ptr<char[]> data_;
			const char * read_ptr_{ nullptr };
			const char * end_ptr_{ nullptr };
			std::string str_;
		};//end class tokenizer

		struct data
		{
			std::unordered_map<std::string, std::string> table;
		};

		static std::shared_ptr<data>& get_data_ptr()
		{
			static std::shared_ptr<data> data_ptr;
			if (!data_ptr)
				data_ptr = std::make_shared<data>();

			return data_ptr;
		}

		void load(const std::string& file, bool utf8)
		{
			auto impl = std::make_shared<data>();

			tokenizer tknizer(file);
			while (true)
			{
				if (token::msgid != tknizer.read())
					break;
				if (token::string != tknizer.read())
					return;

				std::string msgid = std::move(tknizer.get_str());

				if (!utf8)
					msgid = nana::charset(std::move(msgid)).to_bytes(nana::unicode::utf8);

				if (token::msgstr != tknizer.read())
					return;
				if (token::string != tknizer.read())
					return;

				std::string str;

				if (utf8)
					str = nana::charset(std::move(tknizer.get_str()), nana::unicode::utf8);
				else
					str = nana::charset(std::move(tknizer.get_str()));

				std::string::size_type pos = 0;
				while (true)
				{
					pos = str.find('\\', pos);
					if (pos == str.npos)
						break;

					if (pos + 1 < str.size())
					{
						auto ch = str[pos + 1];
						switch (ch)
						{
						case '"': case '\\':
							str.erase(pos, 1);
							break;
						case 'n':
							str.erase(pos, 1);
							str[pos] = '\n';
							break;
						case 't':
							str.erase(pos, 1);
							str[pos] = '\t';
							break;
						}
						++pos;
					}
					else
						break;
				}
				impl->table[std::move(msgid)].swap(str);
			}

			//Assign all language texts to the new table.
			auto & cur_table = get_data_ptr()->table;
			auto & new_table = impl->table;
			for (auto & m : cur_table)
			{
				auto & value = new_table[m.first];
				if (value.empty())
					value = m.second;
			}

			get_data_ptr().swap(impl);
			use_eval();
		}


		struct eval_window
		{
			nana::event_handle destroy{nullptr};
			i18n_eval eval;

			eval_window() = default;

			eval_window(i18n_eval&& arg)
				: eval(std::move(arg))
			{}
		};

		struct eval_manager
		{
			std::recursive_mutex mutex;
			std::map<window, eval_window> table;
		};

		eval_manager& get_eval_manager()
		{
			static eval_manager evals;
			return evals;
		}

		void set_eval(nana::window wd, i18n_eval&& eval)
		{
			auto & mgr = get_eval_manager();
			std::lock_guard<std::recursive_mutex> lock(mgr.mutex);
			auto i = mgr.table.find(wd);
			if (i == mgr.table.end())
			{
				auto result = mgr.table.emplace(wd, std::move(eval));
				result.first->second.destroy = nana::API::events(wd).destroy([wd]{
					auto & eval_mgr = get_eval_manager();
					std::lock_guard<std::recursive_mutex> lockgd(eval_mgr.mutex);

					eval_mgr.table.erase(wd);
				});
			}
			else
				i->second.eval = std::move(eval);
		}

		void use_eval()
		{
			auto & mgr = get_eval_manager();
			std::lock_guard<std::recursive_mutex> lock(mgr.mutex);
			for (auto & eval : mgr.table)
			{
				nana::API::window_caption(eval.first, eval.second.eval());
			}
		}
	}//end namespace internationalization_parts


	void internationalization::load(const std::string& file)
	{
		internationalization_parts::load(file, false);
	}

	void internationalization::load_utf8(const std::string& file)
	{
		internationalization_parts::load(file, true);
	}

	std::string internationalization::get(std::string msgid) const
	{
		std::string str;
		if(_m_get(msgid, str))
			_m_replace_args(str, nullptr);
		return str;
	}

	void internationalization::set(std::string msgid, std::string msgstr)
	{
		auto & ptr = internationalization_parts::get_data_ptr();
		ptr->table[msgid].swap(msgstr);
	}

	bool internationalization::_m_get(std::string& msgid, std::string& msgstr) const
	{
		auto & impl = internationalization_parts::get_data_ptr();
		auto i = impl->table.find(msgid);
		if (i != impl->table.end())
		{
			msgstr = i->second;
			return true;
		}

		msgstr = nana::charset(std::move(msgid), nana::unicode::utf8);
		return false;
	}

	void internationalization::_m_replace_args(std::string& str, std::vector<std::string> * arg_strs) const
	{
		std::string::size_type offset = 0;
		while (true)
		{
			auto pos = str.find("%arg", offset);
			if (pos == str.npos)
				break;

			offset = pos;
			pos = str.find_first_not_of("0123456789", offset + 4);

			if ((pos == str.npos) || (pos != offset + 4))
			{
				std::string::size_type erase_n = 0;
				std::string::size_type arg_n = str.npos;
				if (pos != str.npos)
				{
					erase_n = pos - offset;
					arg_n = pos - offset - 4;
				}
				else
					erase_n = str.size() - offset;

				//If there is not a parameter for %argNNN, the %argNNN will be erased.
				std::size_t arg = static_cast<std::size_t>(std::stoi(str.substr(offset + 4, arg_n)));

				if (arg_strs && arg < arg_strs->size())
					str.replace(offset, erase_n, (*arg_strs)[arg]);
				else
					str.erase(offset, erase_n);
			}
			else
				offset += 4;
		}
	}
	//end class internationalization


	class i18n_eval::arg_string
		: public eval_arg
	{
	public:
		arg_string(std::string str)
			: str_(std::move(str))
		{}

		std::string eval() const override
		{
			return str_;
		}

		std::unique_ptr<eval_arg> clone() const override
		{
			return std::unique_ptr<eval_arg>(new arg_string(str_));
		}
	private:
		std::string str_;
	};

	class i18n_eval::arg_eval
		: public eval_arg
	{
	public:
		arg_eval(const i18n_eval& eval)
			: eval_{ eval }
		{}

		arg_eval(i18n_eval&& eval)
			: eval_{ std::move(eval) }
		{}

		std::string eval() const override
		{
			return eval_();
		}

		std::unique_ptr<eval_arg> clone() const override
		{
			return std::unique_ptr<eval_arg>(new arg_eval(eval_));
		}
	private:
		i18n_eval eval_;
	};

	//class i18n_eval
	i18n_eval::i18n_eval(const i18n_eval& rhs)
		: msgid_(rhs.msgid_)
	{
		for (auto & arg : rhs.args_)
			args_.emplace_back(arg->clone());
	}

	//Workaround for VC2013, becuase it can't specified a default explicit move-constructor
	i18n_eval::i18n_eval(i18n_eval&& other)
		: msgid_(std::move(other.msgid_)), args_(std::move(other.args_))
	{
	}

	i18n_eval& i18n_eval::operator=(const i18n_eval& rhs)
	{
		if (this != &rhs)
		{
			msgid_ = rhs.msgid_;
			for (auto & arg : rhs.args_)
				args_.emplace_back(arg->clone());
		}
		return *this;
	}

	i18n_eval& i18n_eval::operator=(i18n_eval&& rhs)
	{
		if (this != &rhs)
		{
			msgid_ = std::move(rhs.msgid_);
			args_ = std::move(rhs.args_);
		}
		return *this;
	}

	std::string i18n_eval::operator()() const
	{
		if (msgid_.empty())
			return{};

		std::vector<std::string> arg_strs;
		for (auto & arg : args_)
			arg_strs.emplace_back(arg->eval());

		internationalization i18n;
		std::string msgid = msgid_;	//msgid is required to be movable by i18n._m_get
		std::string msgstr;
		if (i18n._m_get(msgid, msgstr))
			i18n._m_replace_args(msgstr, &arg_strs);
		return msgstr;
	}

	void i18n_eval::_m_add_args(i18n_eval& eval)
	{
		args_.emplace_back(new arg_eval(eval));
	}

	void i18n_eval::_m_add_args(const i18n_eval& eval)
	{
		args_.emplace_back(new arg_eval(eval));
	}

	void i18n_eval::_m_add_args(i18n_eval&& eval)
	{
		args_.emplace_back(new arg_eval(std::move(eval)));
	}

	void i18n_eval::_m_add_args(std::string& str)
	{
		args_.emplace_back(new arg_string(nana::charset(str)));
	}

	void i18n_eval::_m_add_args(const std::string& str)
	{
		args_.emplace_back(new arg_string(nana::charset(str)));
	}

	void i18n_eval::_m_add_args(std::string&& str)
	{
		args_.emplace_back(new arg_string(nana::charset(std::move(str))));
	}

	void i18n_eval::_m_add_args(std::wstring& str)
	{
		args_.emplace_back(new arg_string(nana::charset(str)));
	}

	void i18n_eval::_m_add_args(const std::wstring& str)
	{
		args_.emplace_back(new arg_string(nana::charset(str)));
	}

	void i18n_eval::_m_add_args(std::wstring&& str)
	{
		args_.emplace_back(new arg_string(nana::charset(std::move(str))));
	}
	//end class i18n_eval
}
