/*
 *	Operation System Shared Linkage Library Wrapper Implementation
 *	Copyright(C) 2003-2016 Jinhao
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/system/shared_wrapper.hpp
 *	@description:
 */
#ifndef NANA_SYSTEM_SHARED_WRAPPER_HPP
#define NANA_SYSTEM_SHARED_WRAPPER_HPP

#include <nana/deploy.hpp>
#include <type_traits>
#include <stdexcept>


namespace nana
{
namespace system
{

	namespace detail
	{
		namespace shared_helper
		{

			typedef void*	module_t;
			void* symbols(module_t handle, const char* symbol);

		}; //end struct shared_helper
	}//end namespace detail

	class shared_wrapper
	{
		typedef detail::shared_helper::module_t module_t;

	
		template<typename Function>
		struct function_ptr
		{
			typedef typename std::conditional<std::is_function<Function>::value,
										Function*,
										typename std::conditional<std::is_function<Function>::value && std::is_pointer<Function>::value, Function, int>::type
			>::type type;
		};

		struct impl_type
		{
			module_t	handle;
			std::string	symbol;
			void*		proc_address;

			impl_type();
		};
	public:
		shared_wrapper();
		shared_wrapper(const char* filename);
		~shared_wrapper();

		bool open(const char* filename);
		void close();
		bool empty() const;

		template<typename Function>
		typename function_ptr<Function>::type symbols(const char* symbol)
		{
			typedef typename function_ptr<Function>::type fptr_type;

			//if(nana::traits::is_function_pointer<fptr_type>::value == false)
			if (::std::is_function<typename std::remove_pointer<fptr_type>::type>::value == false)
				throw std::invalid_argument("shared_wrapper::symbols, template<_Function> is not a function type or a function pointer type");

			if(symbol == 0)
				throw std::invalid_argument("shared_wrapper::symbols, symbol is null string");

			if(impl_.handle == 0)
				throw std::logic_error("shared_wrapper::symbols, empty handle");

			if(impl_.symbol != symbol)
			{
				void *result = detail::shared_helper::symbols(impl_.handle, symbol);
				if(result == 0)
					throw std::logic_error("shared_wrapper::symbols, empty proc address");
			
				impl_.proc_address = result;
				impl_.symbol = symbol;
			}
			return (fptr_type)(this->impl_.proc_address);
		}

	private:
		impl_type impl_;
	};
}//end namespace system
}//end namespace nana

#endif

