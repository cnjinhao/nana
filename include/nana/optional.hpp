/**
 *	Optional
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2017 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file  nana/optional.hpp
 *
 *	@brief An implementation of experimental library optional of C++ standard(http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3793.html)
 */

#ifndef NANA_STD_OPTIONAL_HEADER_INCLUDED
#define NANA_STD_OPTIONAL_HEADER_INCLUDED

#include <nana/c++defines.hpp>

#ifndef _nana_std_optional
#include <optional>
#else
#include <stdexcept>

namespace nana
{
	namespace detail
	{
		template<typename T>
		class storage
		{
		public:
			using value_type = T;

			storage() = default;

			template<typename U>
			storage(U&& value)
				: initialized_{ true }
			{
				::new (data_) value_type(std::forward<U>(value));
			}

			template<typename U>
			storage(const U& value)
				: initialized_{ true }
			{
				::new (data_) value_type(value);
			}

			storage(const storage& other)
				: initialized_{ other.initialized_ }
			{
				if (other.initialized_)
					::new (data_) value_type(*other.ptr());
			}

			storage(storage&& other)
				: initialized_{ other.initialized_ }
			{
				if (other.initialized_)
					::new (data_) value_type(std::move(*other.ptr()));
			}

			template<typename U>
			storage(const storage<U>& other)
				: initialized_{ other.initialized_ }
			{
				if (other.initialized_)
					::new (data_) value_type(*other.ptr());
			}

			~storage()
			{
				destroy();
			}

			bool initialized() const noexcept
			{
				return initialized_;
			}

			void set_initialized()
			{
				initialized_ = true;
			}

			void destroy()
			{
				if (initialized_)
				{
					ptr()->~T();
					initialized_ = false;
				}
			}

			template<typename U>
			void assign(U&& value)
			{
				if (initialized_)
					*ptr() = std::forward<U>(value);
				else
				{
					::new (data_) value_type(std::forward<U>(value));
					initialized_ = true;
				}
			}

			void assign(const storage& other)
			{
				if (!other.initialized_)
				{
					destroy();
					return;
				}

				if (initialized_)
					*ptr() = *other.ptr();
				else
				{
					::new (data_) value_type(*other.ptr());
					initialized_ = true;
				}
			}

			void assign(storage&& other)
			{
				if (!other.initialized_)
				{
					destroy();
					return;
				}

				if (initialized_)
					*ptr() = std::move(*other.ptr());
				else
				{
					::new (data_) value_type(std::move(*other.ptr()));
					initialized_ = true;
				}
			}

			const T* ptr() const
			{
				return reinterpret_cast<const T*>(data_);
			}

			T* ptr()
			{
				return reinterpret_cast<T*>(data_);
			}
		private:
			bool initialized_{ false };
			char data_[sizeof(value_type)];
		};
	}//end namespace detail

	class bad_optional_access
		: public std::logic_error
	{
	public:
		bad_optional_access()
			: std::logic_error("Attempted to access the value of an uninitialized optional object.")
		{}
	};

	template<typename T>
	class optional
	{
	public:
		using value_type = T;

		constexpr optional() = default;
		constexpr optional(std::nullptr_t) {}

		optional(const optional& other)
			: storage_(other.storage_)
		{}

		optional(optional&& other)
			: storage_(std::move(other.storage_))
		{}

		constexpr optional(const value_type& value)
			: storage_(value)
		{}

		constexpr optional(value_type&& value)
			: storage_(std::move(value))
		{}

		optional& operator=(std::nullptr_t)
		{
			storage_.destroy();
			return *this;
		}

		optional& operator=(const optional& other)
		{
			if (this != &other)
			{
				storage_.assign(other.storage_);
			}
			return *this;
		}

		optional& operator=(optional&& other)
		{
			if (this != &other)
			{
				storage_.assign(std::move(other.storage_));
			}
			return *this;
		}

		template<typename U>
		optional& operator=(U&& value)
		{
			storage_.assign(std::forward<U>(value));

			return *this;
		}

		//Value access
		//constexpr
		value_type* operator->()
		{
			return storage_.ptr();
		}
		constexpr const value_type* operator->() const
		{
			return storage_.ptr();
		}

		//constexpr
		value_type& operator*()
		{
			return *storage_.ptr();
		}

		constexpr const value_type& operator*() const
		{
			return *storage_.ptr();
		}

		/*
		//constexpr
		value_type&& operator*() &&
		{
			return std::move(*storage_.ptr());
		}

		//constexpr
		const value_type&& operator*() const &&
		{
			return std::move(*storage_.ptr());
		}
		*/

		//Condition
		constexpr explicit operator bool() const
		{
			return storage_.initialized();
		}

		constexpr bool has_value() const
		{
			return storage_.initialized();
		}

		//constexpr
		value_type& value()
		{
			if (!storage_.initialized())
				throw bad_optional_access{};
				
			return *storage_.ptr();
		}

		constexpr const value_type& value() const
		{
			if (!storage_.initialized())
				throw bad_optional_access{};

			return *storage_.ptr();
		}
		/*
		//constexpr
		value_type&& value()
		{
			if (!storage_.initialized())
				throw bad_optional_access{};

			return std::move(*storage_.ptr());
		}

		constexpr const value_type&& value() const
		{
			if (!storage_.initialized())
				throw bad_optional_access{};

			return std::move(*storage_.ptr());
		}
		*/

		template<typename U>
		constexpr T value_or(U&& default_value) const
		{
			return (has_value() ? **this : static_cast<T>(std::forward<U>(default_value)));
		}

		template<typename U>
		//constexpr
		T value_or(U&& default_value)
		{
			return (has_value() ? std::move(**this) : static_cast<T>(std::forward<U>(default_value)));
		}

		//Modifiers
		void swap(optional& other)
		{
			if (has_value() && other.has_value())
			{
				std::swap(**this, *other);
				return;
			}
			else if (has_value())
			{
				other.emplace(std::move(***this));
				storage_.destroy();
			}
			else if (other.has_value())
			{
				this->emplace(std::move(*other));
				other.storage_.destroy();
			}
		}

		void reset()
		{
			storage_.destroy();
		}

		template<typename... Args>
		void emplace(Args&&... args)
		{
			storage_.destroy();
			::new (storage_.ptr()) T(std::forward<Args>(args)...);

			storage_.set_initialized();
		}

		template<typename U, typename... Args>
		void emplace(std::initializer_list<U> il, Args&& ... args)
		{
			storage_.destroy();
			::new (storage_.ptr()) T(il, std::forward<Args>(args)...);

			storage_.set_initialized();
		}


	private:
		detail::storage<T> storage_;

	};
}

namespace std
{
	using nana::optional;
}

#endif	//_nana_std_optional
#endif
