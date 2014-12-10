/*
*	A Generic Cloneable Pattern Implementation
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2014 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file: nana/pat/cloneable.hpp
*	@description: A generic easy-to-use cloneable pattern implementation
*/
#ifndef NANA_PAT_CLONEABLE_HPP
#define NANA_PAT_CLONEABLE_HPP

#include <cstddef>
#include <type_traits>
#include <memory>

namespace nana{ namespace pat{

	namespace detail
	{
		template<typename T>
		class cloneable_interface
		{
		public:
			typedef T interface_t;
			typedef cloneable_interface cloneable_t;
			virtual ~cloneable_interface() = default;
			virtual interface_t& refer() = 0;
			virtual const interface_t& refer() const = 0;
			virtual cloneable_t* clone() const = 0;
			virtual void self_delete() const = 0;
		};


		template<typename T, typename SuperOfT>
		class cloneable_wrapper
			: public cloneable_interface<SuperOfT>
		{
		public:
			typedef T value_t;
			typedef typename cloneable_interface<SuperOfT>::interface_t interface_t;

			cloneable_wrapper()
			{}

			cloneable_wrapper(const value_t& obj)
				:object_(obj)
			{}

			cloneable_wrapper(value_t&& rv)
				:object_(std::move(rv))
			{}

			template<typename U>
			cloneable_wrapper(const U& u)
				: object_(u)
			{}

			template<typename U>
			cloneable_wrapper(U& u)
				:object_(u)
			{}

			virtual interface_t& refer() override
			{
				return object_;
			}

			virtual const interface_t& refer() const override
			{
				return object_;
			}

			virtual cloneable_interface<interface_t>* clone() const override
			{
				return (new cloneable_wrapper(object_));
			}

			virtual void self_delete() const override
			{
				(delete this);
			}
		private:
			value_t object_;
		};

	}//end namespace detail

	template<typename Base, bool Mutable = false>
	class cloneable
	{
		using base_t = Base;
		using interface_t = detail::cloneable_interface < base_t > ;

		using const_base_ptr = typename std::conditional<Mutable, base_t*, const base_t*>::type;
		using const_base_ref = typename std::conditional<Mutable, base_t&, const base_t&>::type;

		struct deleter
		{
			void operator()(interface_t * p)
			{
				if(p)
					p->self_delete();
			}
		};

		struct inner_bool
		{
			int true_stand;
		};

		typedef int inner_bool::* operator_bool_t;

		template<typename U>
		struct member_enabled
			: public std::enable_if<(!std::is_base_of<cloneable, typename std::remove_reference<U>::type>::value) && std::is_base_of<base_t, typename std::remove_reference<U>::type>::value, int>
		{};
	public:
		cloneable() = default;

		cloneable(std::nullptr_t){}

		template<typename T, typename member_enabled<T>::type* = nullptr>
		cloneable(T&& t)
			:	cwrapper_(new detail::cloneable_wrapper<typename std::remove_cv<typename std::remove_reference<T>::type>::type, base_t>(std::forward<T>(t)), deleter()),
				fast_ptr_(&(cwrapper_->refer()))
		{}

		cloneable(const cloneable& r)
		{
			if(r.cwrapper_)
			{
				cwrapper_ = std::shared_ptr<interface_t>(r.cwrapper_->clone(), deleter());
				fast_ptr_ = &(cwrapper_->refer());
			}
		}

		cloneable(cloneable && r)
			:	cwrapper_(std::move(r.cwrapper_)),
				fast_ptr_(r.fast_ptr_)
		{
			r.fast_ptr_ = nullptr;
		}

		cloneable & operator=(const cloneable& r)
		{
			if((this != &r) && r.cwrapper_)
			{
				cwrapper_ = std::shared_ptr<interface_t>(r.cwrapper_->clone(), deleter());
				fast_ptr_ = &(cwrapper_->refer());
			}
			return *this;
		}

		cloneable & operator=(cloneable&& r)
		{
			if(this != &r)
			{
				cwrapper_ = r.cwrapper_;
				fast_ptr_ = r.fast_ptr_;

				r.cwrapper_.reset();
				r.fast_ptr_ = nullptr;
			}
			return *this;
		}

		base_t& operator*()
		{
			return *fast_ptr_;
		}

		const_base_ref operator*() const
		{
			return *fast_ptr_;
		}

		base_t * operator->()
		{
			return fast_ptr_;
		}

		const_base_ptr operator->() const
		{
			return fast_ptr_;
		}

		base_t * get() const
		{
			return fast_ptr_;
		}

		void reset()
		{
			fast_ptr_ = nullptr;
			cwrapper_.reset();
		}

		operator operator_bool_t() const volatile
		{
			return (fast_ptr_ ? &inner_bool::true_stand : nullptr);
		}
	private:
		std::shared_ptr<interface_t> cwrapper_;
		base_t * fast_ptr_{nullptr};
	};

	template<typename T>
	using mutable_cloneable = cloneable < T, true > ;
}//end namespace pat
}//end namespace nana

#endif
