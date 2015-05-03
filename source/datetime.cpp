/*
 *	A Date Time Implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/datetime.cpp
 */

#include <nana/config.hpp>
#include <nana/datetime.hpp>
#if defined(NANA_WINDOWS)
	#include <windows.h>
#endif
#include <cassert>

namespace {
	void localtime(struct tm& tm)
	{
#if defined(NANA_WINDOWS) && !defined(NANA_MINGW)
		time_t t;
		::time(&t);
		if(localtime_s(&tm, &t) != 0)
		{
			assert(false);
		}
#else
		time_t t = std::time(nullptr);
		struct tm * tm_addr = std::localtime(&t);
		assert(tm_addr);
		tm = *tm_addr;
#endif
	}
} // namespace anonymous

namespace nana
{
	//class date
		void date::set(const std::tm& t)
		{
			value_.year = t.tm_year + 1900;
			value_.month = t.tm_mon + 1;
			value_.day = t.tm_mday;
		}

		date::date()
		{
			struct tm t;
			localtime(t);
			set(t);
		}

		date::date(const std::tm& t)
		{
			set(t);
		}

		date::date(int year, int month, int day)
		{
			if(1601 <= year && year < 30827 && 0 < month && month < 13 && day > 0)
			{
				if(day <= static_cast<int>(date::month_days(year, month)))
				{
					value_.year = year;
					value_.month = month;
					value_.day = day;
					return;
				}
			}

			struct tm t;
			localtime(t);
			set(t);
		}

		date date::operator - (int off) const
		{
			if(off < 0)
				return _m_add(static_cast<unsigned>(-off));
			return _m_sub(static_cast<unsigned>(off));
		}

		date date::operator + (int off) const
		{
			if(off < 0)
				return _m_sub(static_cast<unsigned>(-off));
			return _m_add(static_cast<unsigned>(off));
		}

		bool date::operator==(const date& r) const
		{
			return (r.value_.year == value_.year && r.value_.month == value_.month && r.value_.day == value_.day);
		}

		bool date::operator!=(const date& r) const
		{
			return ! (this->operator==(r));
		}

		bool date::operator <(const date & r) const
		{
			if(value_.year < r.value_.year)
			{
				return true;
			}
			else if(value_.year == r.value_.year)
			{
				if(value_.month < r.value_.month)
					return true;
				else if(value_.month == r.value_.month)
					return (value_.day < r.value_.day);
			}
			return false;
		}

		bool date::operator>(const date & r) const
		{
			if(value_.year > r.value_.year)
			{
				return true;
			}
			else if(value_.year == r.value_.year)
			{
				if(value_.month > r.value_.month)
					return true;
				else if(value_.month == r.value_.month)
					return (value_.day > r.value_.day);
			}
			return false;
		}

		bool date::operator<=(const date& r) const
		{
			return !(this->operator>(r));
		}

		bool date::operator>=(const date& r) const
		{
			return !(this->operator<(r));
		}

		int date::day_of_week() const
		{
			return day_of_week(static_cast<int>(value_.year), static_cast<int>(value_.month), static_cast<int>(value_.day));
		}

		int date::day_of_week(int year, int month, int day)
		{
			//w=y+[y/4]+[c/4]-2c+[26(m+1)/10]+d-1;
			//the Jan and Feb of Every year are treated as the 13th/14th month of last year.

			if(month < 3)
			{
				month += 12;
				--year;
			}

			int century = year / 100;
			year = year % 100;

			int w = year + (year / 4) + (century / 4) - (2 * century) + (26 * ( month + 1) / 10) + day - 1;

			if(w >= 0)
				return (w % 7);
			return ((1 - (w / 7)) * 7 + w);
		}

		const date::value & date::read() const
		{
			return this->value_;
		}

		date date::_m_add(unsigned x) const
		{
			date d(*this);
			while(x)
			{
				unsigned off = month_days(d.value_.year, d.value_.month) - d.value_.day;
				if(off < x)
				{
					d.value_.day = 1;
					if(d.value_.month == 12)
					{
						d.value_.month = 1;
						++ d.value_.year;
					}
					else
						++ d.value_.month;

					x -= (off + 1);
				}
				else if(off >= x)
				{
					d.value_.day += x;
					break;
				}
			}
			return d;
		}

		date date::_m_sub(unsigned x) const
		{
			date d(*this);
			while(x)
			{
				if(d.value_.day <= x)
				{
					if(d.value_.month == 1)
					{
						d.value_.month = 12;
						-- d.value_.year;
					}
					else
						-- d.value_.month;

					d.value_.day = month_days(d.value_.year, d.value_.month);

					x -= d.value_.day;
				}
				else
				{
					d.value_.day -= x;
					break;
				}
			}
			return d;
		}

		unsigned date::day_in_year(unsigned y, unsigned m, unsigned d)
		{
			unsigned days = 0;
			for(unsigned i = 1; i < m; ++i)
				days += month_days(y, i);
			return days + d;
		}

		unsigned date::month_days(unsigned year, unsigned month)
		{
			unsigned num[] = {31, 0, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
			if(month != 2)
				return num[month - 1];

			if(((year % 4 == 0) && (year % 100)) || (year % 400 == 0))
				return 29;
			return 28;
		}

		unsigned date::year_days(unsigned year)
		{
			if(((year % 4 == 0) && (year % 100)) || (year % 400 == 0))
				return 366;
			return 365;
		}
	//end class date

	//class time
		void time::set(const std::tm& t)
		{
			value_.hour = t.tm_hour;
			value_.minute = t.tm_min;
			value_.second = t.tm_sec;
		}

		time::time()
		{
			struct tm t;
			localtime(t);
			set(t);
		}

		time::time(const std::tm& t)
		{
			value_.hour = t.tm_hour;
			value_.minute = t.tm_min;
			value_.second = t.tm_sec;
		}

		time::time(unsigned hour, unsigned minute, unsigned second)
		{
			if(hour < 24 && minute < 60 && second < 62)
			{
				value_.hour = hour;
				value_.minute = minute;
				value_.second = second;
				return;
			}
			struct tm t;
			localtime(t);
			set(t);
		}

		const time::value& time::read() const
		{
			return value_;
		}
	//end class time
}//end namespace nana
