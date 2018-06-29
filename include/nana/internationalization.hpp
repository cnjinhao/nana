/*
 *	An Implementation of i18n
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2018 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/internationalization.hpp
 */

#ifndef NANA_INTERNATIONALIZATION_HPP
#define NANA_INTERNATIONALIZATION_HPP
#include <vector>
#include <sstream>
#include <functional>
#include <memory>
#include <nana/deploy.hpp>

namespace nana
{
	class internationalization
	{
		friend class i18n_eval;
	public:
		/// Sets a handler to handle a msgid which hasn't been translated.
		static void set_missing(std::function<void(const std::string& msgid_utf8)> handler);

		void load(const std::string& file);
		void load_utf8(const std::string& file);

		template<typename ...Args>
		::std::string get(std::string msgid_utf8, Args&&... args) const
		{
			std::vector<std::string> arg_strs;

#ifdef __cpp_fold_expressions
			(_m_fetch_args(arg_strs, std::forward<Args>(args)),...);
#else
			_m_fetch_args(arg_strs, std::forward<Args>(args)...);
#endif
			
			auto msgstr = _m_get(std::move(msgid_utf8));
			_m_replace_args(msgstr, &arg_strs);
			return msgstr;
		}

		::std::string get(std::string msgid_utf8) const;
		void set(std::string msgid_utf8, ::std::string msgstr);

		template<typename ...Args>
		::std::string operator()(std::string msgid_utf8, Args&&... args) const
		{
			return get(msgid_utf8, std::forward<Args>(args)...);
		}
	private:
		std::string _m_get(std::string&& msgid) const;
		void _m_replace_args(::std::string& str, std::vector<::std::string> * arg_strs) const;

#ifndef __cpp_fold_expressions
		static void _m_fetch_args(std::vector<std::string>&); //Termination of _m_fetch_args
#endif

		static void _m_fetch_args(std::vector<std::string>& v, const char* arg);
		static void _m_fetch_args(std::vector<std::string>& v, const std::string& arg);
		static void _m_fetch_args(std::vector<std::string>& v, std::string& arg);
		static void _m_fetch_args(std::vector<std::string>& v, std::string&& arg);
		static void _m_fetch_args(std::vector<std::string>& v, const wchar_t* arg);
		static void _m_fetch_args(std::vector<std::string>& v, const std::wstring& arg);
		static void _m_fetch_args(std::vector<std::string>& v, std::wstring& arg);
		static void _m_fetch_args(std::vector<std::string>& v, std::wstring&& arg);

		template<typename Arg>
		static void _m_fetch_args(std::vector<std::string>& v, Arg&& arg)
		{
			std::stringstream ss;
			ss << arg;
			v.emplace_back(ss.str());
		}

#ifndef __cpp_fold_expressions
		template<typename ...Args>
		void _m_fetch_args(std::vector<std::string>& v, const char* arg, Args&&... args) const
		{
			v.emplace_back(arg);
			_m_fetch_args(v, std::forward<Args>(args)...);
		}

		template<typename ...Args>
		void _m_fetch_args(std::vector<std::string>& v, const std::string& arg, Args&&... args) const
		{
			v.emplace_back(arg);
			_m_fetch_args(v, std::forward<Args>(args)...);
		}

		template<typename ...Args>
		void _m_fetch_args(std::vector<std::string>& v, std::string& arg, Args&&... args) const
		{
			v.emplace_back(arg);
			_m_fetch_args(v, std::forward<Args>(args)...);
		}

		template<typename ...Args>
		void _m_fetch_args(std::vector<std::string>& v, std::string&& arg, Args&&... args) const
		{
			v.emplace_back(std::move(arg));
			_m_fetch_args(v, std::forward<Args>(args)...);
		}

		template<typename ...Args>
		void _m_fetch_args(std::vector<std::string>& v, const wchar_t* arg, Args&&... args) const
		{
			v.emplace_back(to_utf8(arg));
			_m_fetch_args(v, std::forward<Args>(args)...);
		}

		template<typename ...Args>
		void _m_fetch_args(std::vector<std::string>& v, const std::wstring& arg, Args&&... args) const
		{
			v.emplace_back(to_utf8(arg));
			_m_fetch_args(v, std::forward<Args>(args)...);
		}

		template<typename ...Args>
		void _m_fetch_args(std::vector<std::string>& v, std::wstring& arg, Args&&... args) const
		{
			v.emplace_back(to_utf8(arg));
			_m_fetch_args(v, std::forward<Args>(args)...);
		}

		template<typename ...Args>
		void _m_fetch_args(std::vector<std::string>& v, std::wstring&& arg, Args&&... args) const
		{
			v.emplace_back(to_utf8(arg));
			_m_fetch_args(v, std::forward<Args>(args)...);
		}

		template<typename Arg, typename ...Args>
		void _m_fetch_args(std::vector<std::string>& v, Arg&& arg, Args&&... args) const
		{
			std::stringstream ss;
			ss << arg;
			v.emplace_back(ss.str());
			_m_fetch_args(v, std::forward<Args>(args)...);
		}
#endif
	};//end class internationalization

	class i18n_eval
	{
		class eval_arg
		{
		public:
			virtual ~eval_arg() = default;
			virtual std::string eval() const = 0;
			virtual std::unique_ptr<eval_arg> clone() const = 0;
		};

		class arg_string;
		class arg_eval;

		template<typename Return>
		class arg_function: public eval_arg
		{
		public:
			arg_function(std::function<Return()> fn)
				: fn_(fn)
			{}

			std::string eval() const override
			{
				std::stringstream ss;
				ss << fn_();
				return ss.str();
			}

			std::unique_ptr<eval_arg> clone() const override
			{
				return std::unique_ptr<eval_arg>(new arg_function(*this));
			}
		private:
			std::function<Return()> fn_;
		};
	public:
		i18n_eval() = default;

		template<typename ...Args>
		i18n_eval(std::string msgid_utf8, Args&&... args)
			: msgid_(std::move(msgid_utf8))
		{
#ifdef __cpp_fold_expressions
			(_m_fetch_args(std::forward<Args>(args)), ...);
#else
			_m_fetch_args(std::forward<Args>(args)...);
#endif
		}

		i18n_eval(const i18n_eval&);

		//Workaround for VC2013, becuase it can't specified a default explicit move-constructor
		i18n_eval(i18n_eval&&); //= default

		i18n_eval& operator=(const i18n_eval&);
		i18n_eval& operator=(i18n_eval&& rhs);

		std::string operator()() const;
	private:
#ifndef __cpp_fold_expressions
		void _m_fetch_args(){}	//Termination of _m_fetch_args

		template<typename Arg, typename ...Args>
		void _m_fetch_args(Arg&& arg, Args&&... args)
		{
			_m_add_args(std::forward<Arg>(arg));
			_m_fetch_args(std::forward<Args>(args)...);
		}
#endif

		template<typename Arg>
		void _m_add_args(Arg&& arg)
		{
			std::stringstream ss;
			ss << arg;
			_m_add_args(ss.str());
		}

		template<typename Return>
		void _m_add_args(std::function<Return()> fn)
		{
			args_.emplace_back(new arg_function<Return>(fn));
		}

		void _m_add_args(i18n_eval&);
		void _m_add_args(const i18n_eval&);
		void _m_add_args(i18n_eval&&);

		void _m_add_args(std::string&);
		void _m_add_args(const std::string&);
		void _m_add_args(std::string&&);

		void _m_add_args(const std::wstring&);
	private:
		std::string msgid_;
		std::vector<std::unique_ptr<eval_arg>> args_;
	};//end class i18n_eval;
}

#endif//NANA_I18N_HPP
