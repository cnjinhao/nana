/**
 *	Standard Library for C++11/14/17
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2017-2018 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file  nana/stdc++.hpp
 *
 *	@brief Implement the lack support of standard library.
 */

#ifndef NANA_STDCXX_INCLUDED
#define NANA_STDCXX_INCLUDED

#include "c++defines.hpp"

//Implement workarounds for GCC/MinGW which version is below 4.8.2
#if defined(STD_NUMERIC_CONVERSIONS_NOT_SUPPORTED)
namespace std
{
	//Workaround for no implementation of std::stoi in MinGW.
	int stoi(const std::string&, std::size_t * pos = nullptr, int base = 10);
	int stoi(const std::wstring&, std::size_t* pos = nullptr, int base = 10);

	//Workaround for no implementation of std::stof in MinGW.
	float stof(const std::string&, std::size_t * pos = nullptr);
	float stof(const std::wstring&, std::size_t* pos = nullptr);

	//Workaround for no implementation of std::stod in MinGW.
	double stod(const std::string&, std::size_t * pos = nullptr);
	double stod(const std::wstring&, std::size_t* pos = nullptr);

	//Workaround for no implementation of std::stold in MinGW.
	long double stold(const std::string&, std::size_t * pos = nullptr);
	long double stold(const std::wstring&, std::size_t* pos = nullptr);

	//Workaround for no implementation of std::stol in MinGW.
	long stol(const std::string&, std::size_t* pos = nullptr, int base = 10);
	long stol(const std::wstring&, std::size_t* pos = nullptr, int base = 10);

	//Workaround for no implementation of std::stoll in MinGW.
	long long stoll(const std::string&, std::size_t* pos = nullptr, int base = 10);
	long long stoll(const std::wstring&, std::size_t* pos = nullptr, int base = 10);

	//Workaround for no implementation of std::stoul in MinGW.
	unsigned long stoul(const std::string&, std::size_t* pos = nullptr, int base = 10);
	unsigned long stoul(const std::wstring&, std::size_t* pos = nullptr, int base = 10);

	//Workaround for no implementation of std::stoull in MinGW.
	unsigned long long stoull(const std::string&, std::size_t* pos = nullptr, int base = 10);
	unsigned long long stoull(const std::wstring&, std::size_t* pos = nullptr, int base = 10);
}
#endif //STD_NUMERIC_CONVERSIONS_NOT_SUPPORTED

#ifdef STD_TO_STRING_NOT_SUPPORTED
namespace std
{
	//Workaround for no implementation of std::to_string/std::to_wstring in MinGW.
	std::string to_string(long double);
	std::string to_string(double);
	std::string to_string(unsigned);
	std::string to_string(int);
	std::string to_string(long);
	std::string to_string(unsigned long);
	std::string to_string(long long);
	std::string to_string(unsigned long long);
	std::string to_string(float);
}
#endif

#ifdef STD_TO_WSTRING_NOT_SUPPORTED
namespace std
{
	std::wstring to_wstring(long double);
	std::wstring to_wstring(double);
	std::wstring to_wstring(unsigned);
	std::wstring to_wstring(int);
	std::wstring to_wstring(long);
	std::wstring to_wstring(unsigned long);
	std::wstring to_wstring(long long);
	std::wstring to_wstring(unsigned long long);
	std::wstring to_wstring(float);
}
#endif

#ifdef _nana_std_make_unique
	// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3656.htm

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

namespace std {
	template<class T> struct _Unique_if {
		typedef unique_ptr<T> _Single_object;
	};

	template<class T> struct _Unique_if<T[]> {
		typedef unique_ptr<T[]> _Unknown_bound;
	};

	template<class T, size_t N> struct _Unique_if<T[N]> {
		typedef void _Known_bound;
	};

	template<class T, class... Args>
	typename _Unique_if<T>::_Single_object
		make_unique(Args&&... args) {
		return unique_ptr<T>(new T(std::forward<Args>(args)...));
	}

	template<class T>
	typename _Unique_if<T>::_Unknown_bound
		make_unique(size_t n) {
		typedef typename remove_extent<T>::type U;
		return unique_ptr<T>(new U[n]());
	}

	template<class T, class... Args>
	typename _Unique_if<T>::_Known_bound
		make_unique(Args&&...) = delete;
}
#endif //_nana_std_make_unique

#ifdef _nana_std_put_time
#include <ctime>
#include <string>
namespace std
{
	//Workaround for no implementation of std::put_time in gcc < 5.
	/* std unspecified return type */
	//template< class CharT, class RTSTR >// let fail for CharT != char / wchar_t
	//RTSTR put_time(const std::tm* tmb, const CharT* fmt);

	//template<   >
	std::string put_time/*<char, std::string>*/(const std::tm* tmb, const char* fmt);

	//Defined in header <ctime>
	//	std::size_t strftime(char* str, std::size_t count, const char* format, const std::tm* time);
	//template<>
	//std::wstring put_time<wchar_t, std::wstring>(const std::tm* tmb, const wchar_t* fmt);
}
#endif  // _nana_std_put_time

#if defined(_nana_std_clamp)
namespace std
{
	//<algorithm> since C++17
	template<typename T>
	constexpr const T& clamp(const T& v, const T& lo, const T& hi)
	{
		return (v < lo ? lo : (hi < v ? hi : v));
	}
}
#endif

#endif // NANA_STDCXX_INCLUDED
