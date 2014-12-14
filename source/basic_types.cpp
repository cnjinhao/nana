/*
 *	Basic Types definition
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/basic_types.cpp
 */

#include <nana/basic_types.hpp>

namespace nana
{
	//class color

	expr_color::expr_color(colors col)
		: expr_color((static_cast<unsigned>(col)& 0xFF0000) >> 16, (static_cast<unsigned>(col)& 0xFF00) >> 8, static_cast<unsigned>(col)& 0xFF)
	{
	}

	expr_color::expr_color(colors col, double alpha)
		: expr_color((static_cast<unsigned>(col)& 0xFF0000) >> 16, (static_cast<unsigned>(col)& 0xFF00) >> 8, static_cast<unsigned>(col)& 0xFF, alpha)
	{
	}

	expr_color::expr_color(unsigned red, unsigned green, unsigned blue)
		: a_(1.0), r_(red), g_(green), b_(blue)
	{
	}
	
	expr_color::expr_color(unsigned red, unsigned green, unsigned blue, double alpha)
		: a_(alpha), r_(red), g_(green), b_(blue)
	{
		if (alpha < 0.0)
			a_ = 0.0;
		else if (alpha > 1.0)
			a_ = 1.0;
	}

	void expr_color::blend(const expr_color& bgcolor, bool ignore_bgcolor_alpha)
	{
		if (a_ < 1.0)
		{
			if (0.0 < a_)
			{
				if (ignore_bgcolor_alpha || (1.0 == bgcolor.b_))
				{
					r_ = r_ * a_ + bgcolor.r_ * (1.0 - a_);
					g_ = g_ * a_ + bgcolor.g_ * (1.0 - a_);
					b_ = b_ * a_ + bgcolor.b_ * (1.0 - a_);
					a_ = 1.0;
				}
				else
				{
					r_ = r_ * a_ + bgcolor.r_ * bgcolor.a_ * (1.0 - a_);
					g_ = g_ * a_ + bgcolor.g_ * bgcolor.a_ * (1.0 - a_);
					b_ = b_ * a_ + bgcolor.b_ * bgcolor.a_ * (1.0 - a_);
					a_ = a_ + (bgcolor.a_ * (1.0 - a_));
				}
			}
			else
			{
				r_ = bgcolor.r_;
				g_ = bgcolor.g_;
				b_ = bgcolor.b_;
				a_ = (ignore_bgcolor_alpha ? 1.0 : bgcolor.a_);
			}
		}
	}

	void expr_color::blend(const expr_color& bgcolor, double alpha)
	{
		r_ = r_ * alpha + bgcolor.r_ * (1.0 - alpha);
		g_ = g_ * alpha + bgcolor.g_ * (1.0 - alpha);
		b_ = b_ * alpha + bgcolor.b_ * (1.0 - alpha);
		a_ = 1.0;
	}

	bool expr_color::invisible() const
	{
		return (a_ == 0.0);
	}
	
	pixel_argb_t expr_color::argb() const
	{
		pixel_argb_t argb;
		argb.element.red = static_cast<unsigned>(r_);
		argb.element.green = static_cast<unsigned>(g_);
		argb.element.blue = static_cast<unsigned>(b_);
		argb.element.alpha_channel = static_cast<unsigned>(a_ * 255);
		return argb;
	}

	pixel_rgba_t expr_color::rgba() const
	{
		pixel_rgba_t rgba;
		rgba.element.red = static_cast<unsigned>(r_);
		rgba.element.green = static_cast<unsigned>(g_);
		rgba.element.blue = static_cast<unsigned>(b_);
		rgba.element.alpha_channel = static_cast<unsigned>(a_ * 255);
		return rgba;
	}

	const double& expr_color::r() const
	{
		return r_;
	}
	const double& expr_color::g() const
	{
		return g_;
	}
	const double& expr_color::b() const
	{
		return b_;
	}

	const double& expr_color::a() const
	{
		return a_;
	}

	bool expr_color::operator==(const expr_color& other) const
	{
		return (r_ == other.r_ && g_ == other.g_ && b_ == other.b_ && a_ == other.a_);
	}
	bool expr_color::operator!=(const expr_color& other) const
	{
		return (r_ != other.r_ || g_ == other.g_ || b_ == other.b_ || a_ == other.a_);
	}

	//end class color
	//struct point
		point::point():x(0), y(0){}
		point::point(int x, int y):x(x), y(y){}
		point::point(const rectangle& r)
			: x(r.x), y(r.y)
		{}

		point& point::operator=(const rectangle& r)
		{
			x = r.x;
			y = r.y;
			return *this;
		}

		bool point::operator==(const point& rhs) const
		{
			return ((x == rhs.x) && (y == rhs.y));
		}

		bool point::operator!=(const point& rhs) const
		{
			return ((x != rhs.x) || (y != rhs.y));
		}

		bool point::operator<(const point& rhs) const
		{
			return ((y < rhs.y) || (y == rhs.y && x < rhs.x));
		}

		bool point::operator<=(const point& rhs) const
		{
			return ((y < rhs.y) || (y == rhs.y && x <= rhs.x));
		}

		bool point::operator>(const point& rhs) const
		{
			return ((y > rhs.y) || (y == rhs.y && x > rhs.x));
		}

		bool point::operator>=(const point& rhs) const
		{
			return ((y > rhs.y) || (y == rhs.y && x >= rhs.x));
		}

		point point::operator-(const point& rhs) const
		{
			return{x - rhs.x, y - rhs.y};
		}

		point point::operator+(const point& rhs) const
		{
			return{ x + rhs.x, y + rhs.y };
		}

		point& point::operator-=(const point& rhs)
		{
			x -= rhs.x;
			y -= rhs.y;
			return *this;
		}

		point& point::operator+=(const point& rhs)
		{
			x += rhs.x;
			y += rhs.y;
			return *this;
		}
	//end struct point

	//struct upoint
		upoint::upoint():x(0), y(0){}
		upoint::upoint(unsigned x, unsigned y):x(x), y(y){}

		bool upoint::operator==(const upoint& rhs) const
		{
			return ((x == rhs.x) && (y == rhs.y));
		}

		bool upoint::operator!=(const upoint& rhs) const
		{
			return ((x != rhs.x) || (y != rhs.y));
		}

		bool upoint::operator<(const upoint& rhs) const
		{
			return ((y < rhs.y) || (y == rhs.y && x < rhs.x));
		}

		bool upoint::operator<=(const upoint& rhs) const
		{
			return ((y < rhs.y) || (y == rhs.y && x <= rhs.x));
		}

		bool upoint::operator>(const upoint& rhs) const
		{
			return ((y > rhs.y) || (y == rhs.y && x > rhs.x));
		}

		bool upoint::operator>=(const upoint& rhs) const
		{
			return ((y > rhs.y) || (y == rhs.y && x >= rhs.x));
		}
	//end struct upoint

	//struct size
		size::size():width(0), height(0){}
		size::size(unsigned width, unsigned height):width(width), height(height){}
		size::size(const rectangle& r)
			: width(r.width), height(r.height)
		{}

		size& size::operator=(const rectangle& r)
		{
			width = r.width;
			height = r.height;
			return *this;
		}

		bool size::empty() const
		{
			return (0 == width || 0 == height);
		}

		bool size::is_hit(const point& pos) const
		{
			return (0 <= pos.x && pos.x < static_cast<int>(width) && 0 <= pos.y && pos.y < static_cast<int>(height));
		}

		bool size::operator==(const size& rhs) const
		{
			return (width == rhs.width) && (height == rhs.height);
		}

		bool size::operator!=(const size& rhs) const
		{
			return (width != rhs.width) || (height != rhs.height);
		}

		size size::operator+(const size& sz) const
		{
			return{width + sz.width, height + sz.height};
		}
	//end struct size

	//struct rectangle
		rectangle::rectangle()
			:x(0), y(0), width(0), height(0)
		{}

		rectangle::rectangle(int x, int y, unsigned width, unsigned height)
			:x(x), y(y), width(width), height(height)
		{}

		rectangle::rectangle(const size & sz)
			:x(0), y(0), width(sz.width), height(sz.height)
		{}

		rectangle::rectangle(const point & pos, const size& sz)
			: x(pos.x), y(pos.y), width(sz.width), height(sz.height)
		{}

		bool rectangle::operator==(const rectangle& rhs) const
		{
			return (width == rhs.width) && (height == rhs.height) && (x == rhs.x) && (y == rhs.y);
		}

		bool rectangle::operator!=(const rectangle& rhs) const
		{
			return (width != rhs.width) || (height != rhs.height) || (x != rhs.x) || (y != rhs.y);
		}

		rectangle & rectangle::operator=(const point& pos)
		{
			x = pos.x;
			y = pos.y;
			return *this;
		}

		rectangle & rectangle::operator=(const size & sz)
		{
			width = sz.width;
			height = sz.height;
			return *this;
		}

		rectangle& rectangle::set_pos(const point& pos)
		{
			x = pos.x;
			y = pos.y;
			return *this;
		}

		rectangle& rectangle::set_size(const size& sz)
		{
			width = sz.width;
			height = sz.height;
			return *this;
		}

		rectangle& rectangle::pare_off(int pixels)
		{
			x += pixels;
			y += pixels;
			width -= (pixels << 1);
			height -= (pixels << 1);
			return *this;
		}

		int rectangle::right() const
		{
			return static_cast<int>(x + width);
		}

		int rectangle::bottom() const
		{
			return static_cast<int>(y + height);
		}

		bool rectangle::is_hit(int pos_x, int pos_y) const
		{
			return	(x <= pos_x && pos_x < x + static_cast<int>(width)) &&
					(y <= pos_y && pos_y < y + static_cast<int>(height));
		}

		bool rectangle::is_hit(const point& pos) const
		{
			return	(x <= pos.x && pos.x < x + static_cast<int>(width)) &&
				(y <= pos.y && pos.y < y + static_cast<int>(height));
		}

		bool rectangle::empty() const
		{
			return (0 == width) || (0 == height);
		}
	//end struct rectangle

	//class area_rotator
		area_rotator::area_rotator(bool rotated, const ::nana::rectangle& area)
			: rotated_(rotated),
			area_(area)
		{}

		int area_rotator::x() const
		{
			return (rotated_ ? area_.y : area_.x);
		}

		int & area_rotator::x_ref()
		{
			return (rotated_ ? area_.y : area_.x);
		}

		int area_rotator::y() const
		{
			return (rotated_ ? area_.x : area_.y);
		}

		int & area_rotator::y_ref()
		{
			return (rotated_ ? area_.x : area_.y);
		}

		unsigned area_rotator::w() const
		{
			return (rotated_ ? area_.height : area_.width);
		}

		unsigned & area_rotator::w_ref()
		{
			return (rotated_ ? area_.height : area_.width);
		}

		unsigned area_rotator::h() const
		{
			return (rotated_ ? area_.width : area_.height);
		}

		unsigned & area_rotator::h_ref()
		{
			return (rotated_ ? area_.width : area_.height);
		}

		int area_rotator::right() const
		{
			return (rotated_ ? area_.y + static_cast<int>(area_.height) : area_.x + static_cast<int>(area_.width));
		}

		int area_rotator::bottom() const
		{
			return (rotated_ ? area_.x + static_cast<int>(area_.width) : area_.y + static_cast<int>(area_.height));
		}

		const ::nana::rectangle& area_rotator::result() const
		{
			return area_;
		}
	//end class area_rotator
}
