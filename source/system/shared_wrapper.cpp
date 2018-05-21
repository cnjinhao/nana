/*
 *	Operation System Shared Linkage Library Wrapper Implementation
 *	Copyright(C) 2003-2018 Jinhao
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/system/shared_wrapper.cpp
 */

#include <nana/system/shared_wrapper.hpp>
#include <algorithm>
#include <iterator>
#if defined(NANA_POSIX)
#include <dlfcn.h>
#else
#include <windows.h>
#endif

namespace nana
{
namespace system
{

	namespace detail
	{
		namespace shared_helper
		{

			module_t open(const char* filename)
			{
#if defined(NANA_POSIX)
				return ::dlopen(filename, RTLD_LAZY);
#else
				return ::LoadLibraryA(filename);
#endif
			}

			void* symbols(module_t handle, const char* symbol)
			{
#if defined(NANA_POSIX)
				return ::dlsym(handle, const_cast<char*>(symbol));
#else
				return (void*)(::GetProcAddress(reinterpret_cast<HMODULE>(handle), symbol));
#endif
			}

			void close(module_t handle)
			{
#if defined(NANA_POSIX)
				::dlclose(handle);
#else
				::FreeLibrary(reinterpret_cast<HMODULE>(handle));
#endif
			}
		} //end namespace shared_helper
	}//end namespace detail


	shared_wrapper::impl_type::impl_type()
	{}

	//class shared_wrapper
	shared_wrapper::shared_wrapper()
	{}

	shared_wrapper::shared_wrapper(const char* filename)
		{
			this->open(filename);
		}

	shared_wrapper::~shared_wrapper()
		{
			this->close();
		}

	bool shared_wrapper::open(const char* filename)
		{
			this->close();

			if(filename)
			{
				std::string file;
				std::string ofn = filename;
				std::string::size_type length = ofn.length();

				if(length > 13)
				{
					std::transform(filename + length - 13, filename + length , std::back_inserter(file), tolower);
					if(file == ".nana_shared")
					{
#if defined(NANA_POSIX)
						ofn.replace(length - 13, 13, ".so");
#else
						ofn.replace(length - 13, 13, ".DLL");
#endif
						filename = ofn.c_str();
					}
				}

				impl_.handle = detail::shared_helper::open(filename);
			}

			return (impl_.handle != 0);
		}

	void shared_wrapper::close()
		{
			if(impl_.handle)
			{
				detail::shared_helper::close(impl_.handle);
				impl_.symbol = "";
				impl_.proc_address = 0;
				impl_.handle = 0;
			}
		}

	bool shared_wrapper::empty() const
		{
			return (impl_.handle == 0);
		}
	//end class shared_wrapper
}//end namespace system
}//end namespace nana
