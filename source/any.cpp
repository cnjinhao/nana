/**
*	Any
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file  nana/any.cpp
*
*	@brief An implementation of experimental library any of C++ standard(http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/n4480.html#any)
*/

#include <nana/any.hpp>
#include <utility>

namespace nana
{
	//constructors and destructor
	any::any() noexcept
		: content_(nullptr)
	{
	}

	any::any(const any& other)
		: content_(other.content_ ? other.content_->clone() : nullptr)
	{}

	any::any(any && other) noexcept
		: content_(other.content_)
	{
		other.content_ = nullptr;
	}

	any::~any()
	{
		delete content_;
	}

	//assignments
	any& any::operator=(const any& other)
	{
		if (this != &other)
			any(other).swap(*this);
		
		return *this;
	}

	any& any::operator=(any&& other) noexcept
	{
		if (this != &other)
		{
			other.swap(*this);
			other.clear();
		}
		return *this;
	}

	//modifiers
	void any::clear() noexcept
	{
		if (content_)
		{
			auto cnt = content_;
			content_ = nullptr;
			delete cnt;
		}
	}

	void any::swap(any& other) noexcept
	{
		std::swap(content_, other.content_);
	}

	//observers
	bool any::empty() const noexcept
	{
		return (nullptr == content_);
	}

	const std::type_info& any::type() const noexcept
	{
		return (content_ ? content_->type() : typeid(void));
	}
}//end namespace nana

