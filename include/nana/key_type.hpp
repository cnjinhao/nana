/*
*	A Key Implementation
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2017 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file: nana/key_type.hpp
*/
#ifndef NANA_KEY_TYPE_HPP
#define NANA_KEY_TYPE_HPP

namespace nana
{
	namespace detail
	{
		class key_interface
		{
		public:
			virtual ~key_interface(){}

			virtual bool same_type(const key_interface*) const noexcept = 0;
			virtual bool compare(const key_interface*) const noexcept = 0; ///< is this key less than right key? [call it less(rk), less_than(rk) or compare_less(rk)?: if (lk.less_than(rk )) ]
		};	//end class key_interface

		//Use less compare for equal compare [call it equal_by_less()?]
		inline bool pred_equal(const key_interface * left, const key_interface* right) noexcept
		{
			return (left->same_type(right) && (left->compare(right) == false) && (right->compare(left) == false));
		}

		template<typename T>
		struct type_escape
		{
			using type = T;
		};

		template<>
		struct type_escape<char*>
		{
			using type = ::std::string;
		};

		template<>
		struct type_escape<const char*>
		{
			using type = ::std::string;
		};

		template<int Size>
		struct type_escape<char[Size]>
		{
			using type = ::std::string;
		};

		template<int Size>
		struct type_escape<const char[Size]>
		{
			using type = ::std::string;
		};

		template<>
		struct type_escape<wchar_t*>
		{
			using type = ::std::wstring;
		};

		template<>
		struct type_escape<const wchar_t*>
		{
			using type = ::std::wstring;
		};

		template<int Size>
		struct type_escape<wchar_t[Size]>
		{
			using type = ::std::wstring;
		};

		template<int Size>
		struct type_escape<const wchar_t[Size]>
		{
			using type = ::std::wstring;
		};
	}

	template<typename T, typename Compare>
	class key
		: public detail::key_interface
	{
	public:
		typedef detail::key_interface key_interface;
		typedef T key_type;

		key(const key_type& k)
			: key_object_(k)
		{}

		key(key_type&& k)
			: key_object_(std::move(k))
		{
		}
	public:
		//implement key_interface methods
		bool same_type(const key_interface * p) const noexcept override
		{
			return (nullptr != dynamic_cast<const key*>(p));
		}

		bool compare(const key_interface* p) const noexcept override
		{
			auto rhs = dynamic_cast<const key*>(p);
			return rhs && compare_(key_object_, rhs->key_object_);
		}
	private:
		Compare		compare_;
		key_type	key_object_;
	};
}

#endif
