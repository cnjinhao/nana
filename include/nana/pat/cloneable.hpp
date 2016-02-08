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

#include <nana/c++defines.hpp>
#include <cstddef>
#include <type_traits>
#include <memory>


namespace nana{ namespace pat{

	namespace detail
	{
		class cloneable_interface
		{
		public:
			virtual ~cloneable_interface() = default;
			virtual void * get() = 0;
			virtual cloneable_interface* clone() const = 0;
			virtual void self_delete() const = 0;
		};

		struct cloneable_interface_deleter
		{
			void operator()(cloneable_interface * p)
			{
				if (p)
					p->self_delete();
			}
		};

		template<typename T>
		class cloneable_wrapper
			: public cloneable_interface
		{
		public:
			using value_type = T;

			cloneable_wrapper() = default;

			cloneable_wrapper(const value_type& obj)
				:value_obj_(obj)
			{}

			cloneable_wrapper(value_type&& rv)
				:value_obj_(std::move(rv))
			{}
		private:
			//Implement cloneable_interface
			virtual void* get() override
			{
				return &value_obj_;
			}

			virtual cloneable_interface* clone() const override
			{
				return (new cloneable_wrapper{ value_obj_ });
			}

			virtual void self_delete() const override
			{
				(delete this);
			}
		private:
			value_type value_obj_;
		};
	}//end namespace detail

	template<typename Base, bool Mutable = false>
	class cloneable
	{
		using base_t = Base;
		using cloneable_interface = detail::cloneable_interface;

		using const_base_ptr = typename std::conditional<Mutable, base_t*, const base_t*>::type;
		using const_base_ref = typename std::conditional<Mutable, base_t&, const base_t&>::type;

		template<typename OtherBase, bool OtherMutable> friend class cloneable;

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
		cloneable() noexcept = default;

		cloneable(std::nullptr_t) noexcept{}

		template<typename T, typename member_enabled<T>::type* = nullptr>
		cloneable(T&& t)
			: cwrapper_(new detail::cloneable_wrapper<typename std::remove_cv<typename std::remove_reference<T>::type>::type>(std::forward<T>(t)), detail::cloneable_interface_deleter()),
				fast_ptr_(reinterpret_cast<typename std::remove_cv<typename std::remove_reference<T>::type>::type*>(cwrapper_->get()))
		{}

		cloneable(const cloneable& r)
		{
			if(r.cwrapper_)
			{
				cwrapper_ = std::move(std::shared_ptr<cloneable_interface>(r.cwrapper_->clone(), detail::cloneable_interface_deleter{}));
				fast_ptr_ = reinterpret_cast<base_t*>(cwrapper_->get());
			}
		}

		cloneable(cloneable && r)
			:	cwrapper_(std::move(r.cwrapper_)),
				fast_ptr_(r.fast_ptr_)
		{
			r.fast_ptr_ = nullptr;
		}

		template<typename OtherBase, typename std::enable_if<std::is_base_of<base_t, OtherBase>::value>::type* = nullptr>
		cloneable(const cloneable<OtherBase, Mutable>& other)
		{
			if (other)
			{
				char* value_ptr = reinterpret_cast<char*>(other.cwrapper_->get());
				char* base_ptr = reinterpret_cast<char*>(other.fast_ptr_);

				auto ptr_diff = std::distance(base_ptr, value_ptr);

				cwrapper_.reset(other.cwrapper_->clone(), detail::cloneable_interface_deleter{});
				fast_ptr_ = reinterpret_cast<OtherBase*>(reinterpret_cast<char*>(cwrapper_->get()) - ptr_diff);
			}
		}

		cloneable & operator=(const cloneable& r)
		{
			if((this != &r) && r.cwrapper_)
			{
				cwrapper_ = std::shared_ptr<cloneable_interface>(r.cwrapper_->clone(), detail::cloneable_interface_deleter());
				fast_ptr_ = reinterpret_cast<base_t*>(cwrapper_->get());
			}
			return *this;
		}

		cloneable & operator=(cloneable&& r)
		{
			if(this != &r)
			{
				cwrapper_ = std::move(r.cwrapper_);
				fast_ptr_ = r.fast_ptr_;
				r.fast_ptr_ = nullptr;
			}
			return *this;
		}

		base_t& operator*()
		{
			return *fast_ptr_;
		}

		const_base_ref operator*() const noexcept
		{
			return *fast_ptr_;
		}

		base_t * operator->() noexcept
		{
			return fast_ptr_;
		}

		const_base_ptr operator->() const noexcept
		{
			return fast_ptr_;
		}

		base_t * get() const noexcept
		{
			return fast_ptr_;
		}

		void reset()
		{
			fast_ptr_ = nullptr;
			cwrapper_.reset();
		}

		operator operator_bool_t() const volatile noexcept
		{
			return (fast_ptr_ ? &inner_bool::true_stand : nullptr);
		}
	private:
		std::shared_ptr<cloneable_interface> cwrapper_;
		base_t * fast_ptr_{nullptr};
	};

	template<typename T>
	using mutable_cloneable = cloneable<T, true>;
}//end namespace pat
}//end namespace nana

#endif
