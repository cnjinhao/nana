/*
*	An Implementation of i18n
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2018 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file: nana/internationalization.cpp
*/

#include <nana/push_ignore_diagnostic>

#include <nana/internationalization.hpp>
#include <nana/gui/programming_interface.hpp>
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
			tokenizer(const std::string& file, bool utf8)
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
						if (utf8 && len > 3)
						{
							if (static_cast<unsigned char>(read_ptr_[0]) == 0xEF && static_cast<unsigned char>(read_ptr_[1]) == 0xBB && static_cast<unsigned char>(read_ptr_[2]) == 0xBF)
								read_ptr_ += 3;
						}
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
								str_ += '\\';
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
			std::function<void(const std::string&)> on_missing;
			std::unordered_map<std::string, std::string> table;

			data()
			{
				//initializes nana's translation
				table["NANA_BUTTON_OK"] = "OK";
				table["NANA_BUTTON_OK_SHORTKEY"] = "&OK";
				table["NANA_BUTTON_YES"] = "Yes";
				table["NANA_BUTTON_NO"] = "No";
				table["NANA_BUTTON_BROWSE"] = "Browse";
				table["NANA_BUTTON_CANCEL"] = "Cancel";
				table["NANA_BUTTON_CANCEL_SHORTKEY"] = "&Cancel";
				table["NANA_BUTTON_CREATE"] = "Create";
		
				table["NANA_FILEBOX_BYTES"] = "Bytes";
				table["NANA_FILEBOX_FILESYSTEM"] = "FILESYSTEM";
				table["NANA_FILEBOX_FILTER"] = "Filter";
				table["NANA_FILEBOX_NEW_FOLDER"] = "New Folder";
				table["NANA_FILEBOX_NEW_FOLDER_SHORTKEY"] = "&New Folder";
				table["NANA_FILEBOX_HEADER_NAME"] = "Name";
				table["NANA_FILEBOX_HEADER_MODIFIED"] = "Modified";
				table["NANA_FILEBOX_HEADER_TYPE"] = "Type";
				table["NANA_FILEBOX_HEADER_SIZE"] = "Size";
				table["NANA_FILEBOX_NEW_FOLDER_CAPTION"] = "Name the new folder";

				table["NANA_FILEBOX_SAVE_AS"] = "Save As";
				table["NANA_FILEBOX_OPEN"] = "Open";
				table["NANA_FILEBOX_OPEN_DIRECTORY"] = "Select A Directory";
				table["NANA_FILEBOX_FILE"] = "File";
				table["NANA_FILEBOX_FILE_COLON"] = "File:";
				table["NANA_FILEBOX_DIRECTORY"] = "Directory";
				table["NANA_FILEBOX_DIRECTORY_COLON"] = "Directory:";
				table["NANA_FILEBOX_ERROR_INVALID_FOLDER_NAME"] = "Please input a valid name for the new folder.";
				table["NANA_FILEBOX_ERROR_RENAME_FOLDER_BECAUSE_OF_EXISTING"] = "The folder is existing, please rename it.";
				table["NANA_FILEBOX_ERROR_RENAME_FOLDER_BECAUSE_OF_FAILED_CREATION"] = "Failed to create the folder, please rename it.";
				table["NANA_FILEBOX_ERROR_INVALID_FILENAME"] = "\"%arg0\"\nThe filename is invalid.";
				table["NANA_FILEBOX_ERROR_NOT_EXISTING_AND_RETRY"] = "The file \"%arg0\"\n is not existing. Please check and retry.";
				table["NANA_FILEBOX_ERROR_DIRECTORY_NOT_EXISTING_AND_RETRY"] = "The directory \"%arg0\"\n is not existing. Please check and retry.";
				table["NANA_FILEBOX_ERROR_DIRECTORY_INVALID"] = "The directory \"%arg0\"\n is invalid. Please check and retry.";
				table["NANA_FILEBOX_ERROR_QUERY_REWRITE_BECAUSE_OF_EXISTING"] = "The input file is existing, do you want to overwrite it?";
			}
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

			tokenizer tknizer(file, utf8);
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
					str = tknizer.get_str();
				else
					str = nana::charset(std::move(tknizer.get_str())).to_bytes(nana::unicode::utf8);

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
				result.first->second.destroy = nana::API::events(wd).destroy.connect([wd](const arg_destroy&){
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

	void internationalization::set_missing(std::function<void(const std::string& msgid_utf8)> handler)
	{
		internationalization_parts::get_data_ptr()->on_missing = std::move(handler);
	}

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
		std::string str = _m_get(std::move(msgid));
		_m_replace_args(str, nullptr);
		return str;
	}

	void internationalization::set(std::string msgid, std::string msgstr)
	{
		auto & ptr = internationalization_parts::get_data_ptr();
		ptr->table[msgid].swap(msgstr);
	}

	std::string internationalization::_m_get(std::string&& msgid) const
	{
		auto & impl = internationalization_parts::get_data_ptr();
		auto i = impl->table.find(msgid);
		if (i != impl->table.end())
			return i->second;

		if (impl->on_missing)
			impl->on_missing(msgid);

		return std::move(msgid);
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

#ifndef __cpp_fold_expressions
	void internationalization::_m_fetch_args(std::vector<std::string>&)
	{}
#endif
	
	void internationalization::_m_fetch_args(std::vector<std::string>& v, const char* arg)
	{
		v.emplace_back(arg);
	}

	void internationalization::_m_fetch_args(std::vector<std::string>& v, const std::string& arg)
	{
		v.emplace_back(arg);
	}

	void internationalization::_m_fetch_args(std::vector<std::string>& v, std::string& arg)
	{
		v.emplace_back(arg);
	}

	void internationalization::_m_fetch_args(std::vector<std::string>& v, std::string&& arg)
	{
		v.emplace_back(std::move(arg));
	}

	void internationalization::_m_fetch_args(std::vector<std::string>& v, const wchar_t* arg)
	{
		v.emplace_back(to_utf8(arg));
	}

	void internationalization::_m_fetch_args(std::vector<std::string>& v, const std::wstring& arg)
	{
		v.emplace_back(to_utf8(arg));
	}

	void internationalization::_m_fetch_args(std::vector<std::string>& v, std::wstring& arg)
	{
		v.emplace_back(to_utf8(arg));
	}

	void internationalization::_m_fetch_args(std::vector<std::string>& v, std::wstring&& arg)
	{
		v.emplace_back(to_utf8(arg));
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

	//Workaround for VC2013, because it can't specify a default explicit move-constructor
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

		std::string msgstr = i18n._m_get(std::string{msgid_});		
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
		args_.emplace_back(new arg_string(str));
	}

	void i18n_eval::_m_add_args(const std::string& str)
	{
		args_.emplace_back(new arg_string(str));
	}

	void i18n_eval::_m_add_args(std::string&& str)
	{
		args_.emplace_back(new arg_string(std::move(str)));
	}

	void i18n_eval::_m_add_args(const std::wstring& str)
	{
		args_.emplace_back(new arg_string(to_utf8(str)));
	}
	//end class i18n_eval
}

#include <nana/pop_ignore_diagnostic>
