/**
*	Standard Library for C++11/14/17
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2017 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file  nana/stdc++.cpp
*/

#include <nana/stdc++.hpp>

//Implement workarounds for GCC/MinGW which version is below 4.8.2
#if defined(STD_NUMERIC_CONVERSIONS_NOT_SUPPORTED)
#include <sstream>
#include <cstdlib>
#include <stdexcept>
namespace std
{
	int stoi(const std::string& str, std::size_t * pos, int base)
	{
		auto sptr = str.c_str();
		char *end;
		errno = 0;
		auto result = std::strtol(sptr, &end, base);

		if (sptr == end)
			throw std::invalid_argument("invalid stoi argument");
		if (errno == ERANGE)
			throw std::out_of_range("stoi argument out of range");

		if (pos)
			*pos = (std::size_t)(end - sptr);
		return ((int)result);
	}

	int stoi(const std::wstring& str, std::size_t* pos, int base)
	{
		auto sptr = str.data();
		wchar_t *end;
		errno = 0;
		auto result = std::wcstol(sptr, &end, base);

		if (sptr == end)
			throw std::invalid_argument("invalid stoi argument");
		if (errno == ERANGE)
			throw std::out_of_range("stoi argument out of range");

		if (pos)
			*pos = (std::size_t)(end - sptr);
		return ((int)result);
	}
	using ::strtof;
	using ::strtold;
	using ::wcstold;
	using ::strtoll;
	using ::wcstoll;
	using ::strtoull;
	using ::wcstoull;

	float stof(const std::string& str, std::size_t * pos)
	{
		auto *ptr = str.data();
		errno = 0;
		char *end;
		auto result = std::strtof(ptr, &end);

		if (ptr == end)
			throw std::invalid_argument("invalid stod argument");
		if (errno == ERANGE)
			throw std::out_of_range("stof argument out of range");
		if (pos)
			*pos = (std::size_t)(end - ptr);
		return result;
	}

	float stof(const std::wstring& str, std::size_t* pos)
	{
		auto *ptr = str.data();
		errno = 0;
		wchar_t *end;
		auto result = std::wcstof(ptr, &end);

		if (ptr == end)
			throw std::invalid_argument("invalid stod argument");
		if (errno == ERANGE)
			throw std::out_of_range("stof argument out of range");
		if (pos)
			*pos = (std::size_t)(end - ptr);
		return result;
	}

	double stod(const std::string& str, std::size_t * pos)
	{
		auto *ptr = str.data();
		errno = 0;
		char *end;
		auto result = std::strtod(ptr, &end);

		if (ptr == end)
			throw std::invalid_argument("invalid stod argument");
		if (errno == ERANGE)
			throw std::out_of_range("stod argument out of range");
		if (pos)
			*pos = (std::size_t)(end - ptr);
		return result;
	}

	double stod(const std::wstring& str, std::size_t* pos)
	{
		auto *ptr = str.data();
		errno = 0;
		wchar_t *end;
		auto result = std::wcstod(ptr, &end);

		if (ptr == end)
			throw std::invalid_argument("invalid stod argument");
		if (errno == ERANGE)
			throw std::out_of_range("stod argument out of range");
		if (pos)
			*pos = (std::size_t)(end - ptr);
		return result;
	}

	long double stold(const std::string& str, std::size_t * pos)
	{
		auto *ptr = str.data();
		errno = 0;
		char *end;
		auto result = std::strtold(ptr, &end);

		if (ptr == end)
			throw std::invalid_argument("invalid stod argument");
		if (errno == ERANGE)
			throw std::out_of_range("stold argument out of range");
		if (pos)
			*pos = (std::size_t)(end - ptr);
		return result;
	}

	long double stold(const std::wstring& str, std::size_t* pos)
	{
		auto *ptr = str.data();
		errno = 0;
		wchar_t *end;
		auto result = std::wcstold(ptr, &end);

		if (ptr == end)
			throw std::invalid_argument("invalid stod argument");
		if (errno == ERANGE)
			throw std::out_of_range("stold argument out of range");
		if (pos)
			*pos = (std::size_t)(end - ptr);
		return result;
	}

	long stol(const std::string& str, std::size_t* pos, int base)
	{
		auto *ptr = str.data();
		errno = 0;
		char *end;
		auto result = std::strtol(ptr, &end, base);

		if (ptr == end)
			throw std::invalid_argument("invalid stod argument");
		if (errno == ERANGE)
			throw std::out_of_range("stol argument out of range");
		if (pos)
			*pos = (std::size_t)(end - ptr);
		return result;
	}

	long stol(const std::wstring& str, std::size_t* pos, int base)
	{
		auto *ptr = str.data();
		errno = 0;
		wchar_t *end;
		auto result = std::wcstol(ptr, &end, base);

		if (ptr == end)
			throw std::invalid_argument("invalid stod argument");
		if (errno == ERANGE)
			throw std::out_of_range("stol argument out of range");
		if (pos)
			*pos = (std::size_t)(end - ptr);
		return result;
	}

	//Workaround for no implemenation of std::stoll in MinGW.
	long long stoll(const std::string& str, std::size_t* pos, int base)
	{
		auto *ptr = str.data();
		errno = 0;
		char* end;
		auto result = std::strtoll(ptr, &end, base);

		if (ptr == end)
			throw std::invalid_argument("invalid stod argument");
		if (errno == ERANGE)
			throw std::out_of_range("stoll argument out of range");
		if (pos)
			*pos = (std::size_t)(end - ptr);
		return result;
	}

	long long stoll(const std::wstring& str, std::size_t* pos, int base)
	{
		auto *ptr = str.data();
		errno = 0;
		wchar_t* end;
		auto result = std::wcstoll(ptr, &end, base);

		if (ptr == end)
			throw std::invalid_argument("invalid stod argument");
		if (errno == ERANGE)
			throw std::out_of_range("stoll argument out of range");
		if (pos)
			*pos = (std::size_t)(end - ptr);
		return result;
	}

	unsigned long long stoull(const std::string& str, std::size_t* pos, int base)
	{
		auto *ptr = str.data();
		errno = 0;
		char* end;
		auto result = std::strtoull(ptr, &end, base);

		if (ptr == end)
			throw std::invalid_argument("invalid stod argument");
		if (errno == ERANGE)
			throw std::out_of_range("stoull argument out of range");
		if (pos)
			*pos = (std::size_t)(end - ptr);
		return result;
	}

	unsigned long long stoull(const std::wstring& str, std::size_t* pos, int base)
	{
		auto *ptr = str.data();
		errno = 0;
		wchar_t* end;
		auto result = std::wcstoull(ptr, &end, base);

		if (ptr == end)
			throw std::invalid_argument("invalid stod argument");
		if (errno == ERANGE)
			throw std::out_of_range("stoull argument out of range");
		if (pos)
			*pos = (std::size_t)(end - ptr);
		return result;
	}

	//Workaround for no implemenation of std::stoul in MinGW.
	unsigned long stoul(const std::string& str, std::size_t* pos, int base)
	{
		auto *ptr = str.data();
		errno = 0;
		char* end;
		auto result = std::strtoul(ptr, &end, base);

		if (ptr == end)
			throw std::invalid_argument("invalid stod argument");
		if (errno == ERANGE)
			throw std::out_of_range("stoul argument out of range");
		if (pos)
			*pos = (std::size_t)(end - ptr);
		return result;
	}

	unsigned long stoul(const std::wstring& str, std::size_t* pos, int base)
	{
		auto *ptr = str.data();
		errno = 0;
		wchar_t* end;
		auto result = std::wcstoul(ptr, &end, base);

		if (ptr == end)
			throw std::invalid_argument("invalid stod argument");
		if (errno == ERANGE)
			throw std::out_of_range("stoul argument out of range");
		if (pos)
			*pos = (std::size_t)(end - ptr);
		return result;
	}
}//end namespace std
#endif //STD_NUMERIC_CONVERSIONS_NOT_SUPPORTED

#ifdef STD_TO_STRING_NOT_SUPPORTED
#include <sstream>
namespace std
{
	std::string to_string(double v)
	{
		std::stringstream ss;
		ss << v;
		return ss.str();
	}

	std::string to_string(long double v)
	{
		std::stringstream ss;
		ss << v;
		return ss.str();
	}

	std::string to_string(unsigned v)
	{
		std::stringstream ss;
		ss << v;
		return ss.str();
	}

	std::string to_string(int v)
	{
		std::stringstream ss;
		ss << v;
		return ss.str();
	}

	std::string to_string(long v)
	{
		std::stringstream ss;
		ss << v;
		return ss.str();
	}

	std::string to_string(unsigned long v)
	{
		std::stringstream ss;
		ss << v;
		return ss.str();
	}

	std::string to_string(long long v)
	{
		std::stringstream ss;
		ss << v;
		return ss.str();
	}

	std::string to_string(unsigned long long v)
	{
		std::stringstream ss;
		ss << v;
		return ss.str();
	}

	std::string to_string(float v)
	{
		std::stringstream ss;
		ss << v;
		return ss.str();
	}
}
#endif // STD_TO_STRING_NOT_SUPPORTED

#ifdef STD_TO_WSTRING_NOT_SUPPORTED
#include <sstream>
namespace std
{
	std::wstring to_wstring(double v)
	{
		std::wstringstream ss;
		ss << v;
		return ss.str();
	}

	std::wstring to_wstring(long double v)
	{
		std::wstringstream ss;
		ss << v;
		return ss.str();
	}

	std::wstring to_wstring(unsigned v)
	{
		std::wstringstream ss;
		ss << v;
		return ss.str();
	}

	std::wstring to_wstring(int v)
	{
		std::wstringstream ss;
		ss << v;
		return ss.str();
	}

	std::wstring to_wstring(long v)
	{
		std::wstringstream ss;
		ss << v;
		return ss.str();
	}

	std::wstring to_wstring(unsigned long v)
	{
		std::wstringstream ss;
		ss << v;
		return ss.str();
	}

	std::wstring to_wstring(long long v)
	{
		std::wstringstream ss;
		ss << v;
		return ss.str();
	}

	std::wstring to_wstring(unsigned long long v)
	{
		std::wstringstream ss;
		ss << v;
		return ss.str();
	}

	std::wstring to_wstring(float v)
	{
		std::wstringstream ss;
		ss << v;
		return ss.str();
	}
}
#endif

#ifdef _nana_std_put_time
#include <cwchar>
namespace std
{
	//Workaround for no implemenation of std::put_time in gcc < 5.
	/* std unspecified return type */
	//template< class CharT, class RTSTR >// let fail for CharT != char / wchar_t
	//RTSTR put_time(const std::tm* tmb, const CharT* fmt);

	//template<   >
	std::string put_time/*<char, std::string>*/(const std::tm* tmb, const char* fmt)
	{
		std::size_t sz = 200;
		std::string str(sz, '\0');
		sz = std::strftime(&str[0], str.size() - 1, fmt, tmb);
		str.resize(sz);
		return str;
	}
	//Defined in header <ctime>
	//	std::size_t strftime(char* str, std::size_t count, const char* format, const std::tm* time);
	//template<>
	//std::wstring put_time<wchar_t, std::wstring>(const std::tm* tmb, const wchar_t* fmt)
	//{
	//	unsigned sz = 200;
	//	std::wstring str(sz, L'\0');
	//	sz = std::wcsftime(&str[0], str.size() - 1, fmt, tmb);
	//	str.resize(sz);
	//	return str;
	//}
	// http://en.cppreference.com/w/cpp/chrono/c/wcsftime
	// Defined in header <cwchar>
	//	std::size_t wcsftime(wchar_t* str, std::size_t count, const wchar_t* format, const std::tm* time);
	// Converts the date and time information from a given calendar time time to a null - terminated
	// wide character string str according to format string format.Up to count bytes are written.
	//	Parameters
	//	str - pointer to the first element of the wchar_t array for output
	//	count - maximum number of wide characters to write
	//	format - pointer to a null - terminated wide character string specifying the format of conversion.

}
#endif  // _enable_std_put_time
