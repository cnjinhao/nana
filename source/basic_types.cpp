/*
 *	Basic Types definition
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2017 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/basic_types.cpp
 *	@contributos: Jan
 */

#include <nana/basic_types.hpp>
#if defined(USE_STD_REGEX)
#include <regex>
#else
#include <vector>
#endif
#include <algorithm>

#include <stdexcept>

namespace nana
{
	//class color
	color::color(colors clr)
		: color((static_cast<unsigned>(clr)& 0xFF0000) >> 16, (static_cast<unsigned>(clr)& 0xFF00) >> 8, static_cast<unsigned>(clr)& 0xFF)
	{}

	color::color(colors clr, double alpha)
		: color((static_cast<unsigned>(clr)& 0xFF0000) >> 16, (static_cast<unsigned>(clr)& 0xFF00) >> 8, static_cast<unsigned>(clr)& 0xFF, alpha)
	{}

	color::color(color_rgb rgb)
		:	r_((static_cast<int>(rgb) >> 16) & 0xFF),
			g_((static_cast<int>(rgb) >> 8) & 0xFF),
			b_(static_cast<int>(rgb) & 0xFF),
			a_(1.0)
	{}

	color::color(color_argb argb)
		:	r_((static_cast<int>(argb) >> 16) & 0xFF),
			g_((static_cast<int>(argb) >> 8) & 0xFF),
			b_(static_cast<int>(argb) & 0xFF),
			a_(((static_cast<int>(argb) >> 24) & 0xFF) / 255.0)
	{}

	color::color(color_rgba rgba)
		:	r_((static_cast<int>(rgba) >> 24) & 0xFF),
			g_((static_cast<int>(rgba) >> 16) & 0xFF),
			b_((static_cast<int>(rgba) >> 8) & 0xFF),
			a_((static_cast<int>(rgba) & 0xFF) / 255.0)
	{}

	color::color(unsigned red, unsigned green, unsigned blue, double alpha)
		: r_(red), g_(green), b_(blue), a_(alpha)
	{
		if (alpha < 0.0)
			a_ = 0.0;
		else if (alpha > 1.0)
			a_ = 1.0;
	}

#if !defined(USE_STD_REGEX)
	std::string read_number(std::string& str, std::size_t& pos)
	{
		pos = str.find_first_of("0123456789", pos);
		if (pos == str.npos)
			return{};

		auto end = str.find_first_not_of("0123456789", pos + 1);
		//integer part
		if (end == str.npos)
		{
			pos = end;
			return str.substr(pos);
		}

		if (str[end] == '.')
		{
			auto decimal_end = str.find_first_not_of("0123456789", end + 1);
			if ((decimal_end == str.npos) || (decimal_end == end + 1)) //Because of missing %
				return{};

			end = decimal_end;
		}

		auto ch = str[end];
		if (ch == '%' || ch == ' ' || ch == ',' || ch == ')')
		{
			auto start = pos;
			pos = end + (ch == '%' ? 1 : 0);
			return str.substr(start, pos - start);
		}
		return{};
	}
#endif

	//Initializes the color with a CSS-like string
	//contributor: BigDave(mortis2007 at hotmail co uk)
	//date: February 3, 2015
	//maintainer: Jinhao, extended the support of CSS-spec

	color::color(std::string css_color)
		: a_(1.0)
	{
		const char * excpt_what = "color: invalid rgb format";

		auto pos = css_color.find_first_not_of(' ');
		if (pos == css_color.npos)
			throw std::invalid_argument(excpt_what);

		if ('#' == css_color[pos])
		{
			if (css_color.size() < pos + 4)
				throw std::invalid_argument(excpt_what);

			auto endpos = css_color.find_first_not_of("0123456789abcdefABCDEF", pos + 1);
			if (endpos == css_color.npos)
				endpos = static_cast<decltype(endpos)>(css_color.size());
			
			if ((endpos - pos != 4) && (endpos - pos != 7))
				throw std::invalid_argument(excpt_what);

			auto n = std::stoi(css_color.substr(pos + 1, endpos - pos - 1), nullptr, 16);

			if (endpos - pos == 4)
			{
				r_ = ((0xF00 & n) >> 4) | ((0xF00 & n) >> 8);
				g_ = (0xF0 & n) | ((0xF0 & n) >> 4);
				b_ = (0xF & n) | ((0xF & n) << 4);
			}
			else
			{
				r_ = (0xFF0000 & n) >> 16;
				g_ = (0xFF00 & n) >> 8;
				b_ = (0xFF & n);
			}

			return;
		}

		//std::tolower is not allowed because of concept requirements
		std::transform(css_color.begin(), css_color.end(), css_color.begin(), [](char ch){
			if('A' <= ch && ch <= 'Z')
				return static_cast<char>(ch - ('A' - 'a'));
			return ch;
		});
		auto endpos = css_color.find(' ', pos + 1);
		if (endpos == css_color.npos)
			endpos = css_color.size();

		if ((endpos - pos == 11) && (css_color.substr(pos, 11) == "transparent"))
		{
			r_ = 0;
			g_ = 0;
			b_ = 0;
			a_ = 0;
			return;
		}

		auto type_end = css_color.find_first_of(" (", pos + 1);

		if (type_end == css_color.npos || ((type_end - pos != 3) && (type_end - pos != 4)))	//rgb/hsl = 3, rgba/hsla = 4
			throw std::invalid_argument(excpt_what);

		bool has_alpha = false;
		if (type_end - pos == 4) //maybe rgba/hsla
		{
			if (css_color[pos + 3] != 'a')
				throw std::invalid_argument(excpt_what);
			has_alpha = true;
		}

#if defined(USE_STD_REGEX)
		std::regex pat;
		std::regex_iterator<std::string::iterator> i, end;
		auto type_name = css_color.substr(pos, 3);
		if ("rgb" == type_name)
		{
			pat.assign("(\\d*\\.)?\\d+\\%?");
			i = std::regex_iterator<std::string::iterator>(css_color.begin() + pos, css_color.end(), pat);

			if (i == end)
				throw std::invalid_argument(excpt_what);
			
			std::vector<std::string> rgb;
#ifdef _nana_std_has_emplace_return_type
			auto const is_real = (rgb.emplace_back(i->str()).back() == '%');
#else
			rgb.emplace_back(i->str());
			const bool is_real = (rgb.back().back() == '%');
#endif
			pat.assign(is_real ? "(\\d*\\.)?\\d+\\%" : "\\d+");

			for (++i; i != end; ++i)
			{
				rgb.emplace_back(i->str());
				if (rgb.size() == 3)
					break;
			}

			if (rgb.size() != 3)
				throw std::invalid_argument(excpt_what);

			if (is_real)
			{
				auto pr = ::nana::stod(rgb[0].substr(0, rgb[0].size() - 1));
				r_ = (pr > 100 ? 255.0 : 2.55 * pr);

				pr = ::nana::stod(rgb[1].substr(0, rgb[1].size() - 1));
				g_ = (pr > 100 ? 255.0 : 2.55 * pr);

				pr = ::nana::stod(rgb[2].substr(0, rgb[2].size() - 1));
				b_ = (pr > 100 ? 255.0 : 2.55 * pr);
			}
			else
			{
				r_ = ::nana::stod(rgb[0]);
				if (r_ > 255.0)	r_ = 255;

				g_ = ::nana::stod(rgb[1]);
				if (g_ > 255.0)	g_ = 255;

				b_ = ::nana::stod(rgb[2]);
				if (b_ > 255.0)	b_ = 255;
			}
		}
		else if ("hsl" == type_name)
		{
			pat.assign("(\\d*\\.)?\\d+");
			i = std::regex_iterator<std::string::iterator>(css_color.begin() + pos, css_color.end(), pat);

			if (i == end)
				throw std::invalid_argument(excpt_what);

			auto h = ::nana::stod(i->str());

			pat.assign("(\\d*\\.)?\\d+\\%");

			if (++i == end)
				throw std::invalid_argument(excpt_what);

			auto str = i->str();
			auto s = ::nana::stod(str.substr(0, str.size() - 1));

			if (++i == end)
				throw std::invalid_argument(excpt_what);

			str = i->str();
			auto l = ::nana::stod(str.substr(0, str.size() - 1));

			from_hsl(h, s / 100, l / 100);
		}
		else
			throw std::invalid_argument(excpt_what);	//invalid color type

		if (has_alpha)
		{
			pat.assign("(\\d*\\.)?\\d+");
			if (++i == end)
				throw std::invalid_argument(excpt_what);	//invalid alpha value
			a_ = ::nana::stod(i->str());
		}
#else
		auto type_name = css_color.substr(pos, 3);
		pos = css_color.find_first_not_of(' ', type_end);
		if (pos == css_color.npos || css_color[pos] != '(')
			throw std::invalid_argument(excpt_what);

		auto str = read_number(css_color, ++pos);
		if (str.empty())
			throw std::invalid_argument(excpt_what);

		if ("rgb" == type_name)
		{
			std::vector<std::string> rgb;

#ifdef _nana_std_has_emplace_return_type
			auto const is_real = (rgb.emplace_back(std::move(str)).back() == '%');
#else
			rgb.emplace_back(std::move(str));

			const bool is_real = (rgb.back().back() == '%');
#endif

			for (int i = 0; i < 2; ++i)
			{
				pos = css_color.find_first_not_of(' ', pos);
				if (pos == css_color.npos || css_color[pos] != ',')
					throw std::invalid_argument(excpt_what);

				str = read_number(css_color, ++pos);
				if (str.empty())
					throw std::invalid_argument(excpt_what);

				rgb.emplace_back(std::move(str));
				if (rgb.size() == 3)
					break;
			}

			if (rgb.size() != 3)
				throw std::invalid_argument(excpt_what);

			if (is_real)
			{
				auto pr = std::stod(rgb[0].substr(0, rgb[0].size() - 1));
				r_ = (pr > 100 ? 255.0 : 2.55 * pr);

				pr = std::stod(rgb[1].substr(0, rgb[1].size() - 1));
				g_ = (pr > 100 ? 255.0 : 2.55 * pr);

				pr = std::stod(rgb[2].substr(0, rgb[2].size() - 1));
				b_ = (pr > 100 ? 255.0 : 2.55 * pr);
			}
			else
			{
				r_ = std::stod(rgb[0]);
				if (r_ > 255.0)	r_ = 255;

				g_ = std::stod(rgb[1]);
				if (g_ > 255.0)	g_ = 255;

				b_ = std::stod(rgb[2]);
				if (b_ > 255.0)	b_ = 255;
			}
		}
		else if ("hsl" == type_name)
		{
			if (str.back() == '%')
				throw std::invalid_argument(excpt_what);

			auto h = std::stod(str);

			pos = css_color.find_first_not_of(' ', pos);
			if (pos == css_color.npos || css_color[pos] != ',')
				throw std::invalid_argument(excpt_what);

			str = read_number(css_color, ++pos);
			if (str.empty() || str.back() != '%')
				throw std::invalid_argument(excpt_what);

			auto s = std::stod(str.substr(0, str.size() - 1));

			pos = css_color.find_first_not_of(' ', pos);
			if (pos == css_color.npos || css_color[pos] != ',')
				throw std::invalid_argument(excpt_what);

			str = read_number(css_color, ++pos);
			if (str.empty() || str.back() != '%')
				throw std::invalid_argument(excpt_what);

			auto l = std::stod(str.substr(0, str.size() - 1));

			from_hsl(h, s / 100, l / 100);
		}
		else
			throw std::invalid_argument(excpt_what);	//invalid color type

		if (has_alpha)
		{
			str = read_number(css_color, ++pos);
			if (str.empty() || str.back() == '%')
				throw std::invalid_argument(excpt_what); //invalid alpha value

			a_ = std::stod(str);
		}
#endif
	}

	color& color::from_rgb(unsigned red, unsigned green, unsigned blue)
	{
		r_ = red;
		g_ = green;
		b_ = blue;
		return *this;
	}

	double rgb_from_hue(double v1, double v2, double h)
	{
		if (h < 0.0)
			h += 1.0;
		else if (h > 1.0)
			h -= 1.0;

		if (h < 0.1666666) return v1 + (v2 - v1) * (6.0 * h);
		if (h < 0.5) return v2;
		if (h < 0.6666666) return v1 + (v2 - v1) * (4.0 - h * 6.0);
		return v1;
	}

	color& color::from_hsl(double hue, double saturation, double lightness)
	{
		if (0.0 == saturation)
		{
			r_ = lightness * 255.0;
			g_ = r_;
			b_ = r_;
		}
		else
		{
			double var2;
			if (lightness < 0.5)
				var2 = lightness * (1.0 + saturation);
			else
				var2 = (lightness + saturation) - (saturation * lightness);

			double var1 = 2.0 * lightness - var2;

			hue /= 360;
			r_ = 255.0 * rgb_from_hue(var1, var2, hue + 0.33333);
			g_ = 255.0 * rgb_from_hue(var1, var2, hue);
			b_ = 255.0 * rgb_from_hue(var1, var2, hue - 0.33333);
		}
		return *this;
	}

	color& color::alpha(double al)
	{
		if (al < 0.0)
			a_ = 0.0;
		else if (al > 1.0)
			a_ = 1.0;
		else
			a_ = al;
		return *this;
	}

	color color::blend(const color& bgcolor, double alpha) const
	{
		color result;
		result.r_ = r_ * (1.0 - alpha) + bgcolor.r_ * alpha;
		result.g_ = g_ * (1.0 - alpha) + bgcolor.g_ * alpha;
		result.b_ = b_ * (1.0 - alpha) + bgcolor.b_ * alpha;
		result.a_ = 1.0;
		return result;
	}

	bool color::invisible() const
	{
		return (a_ == 0.0);
	}

	pixel_color_t color::px_color() const
	{
		return argb();
	}

	pixel_argb_t color::argb() const
	{
		pixel_argb_t argb;
		argb.element.red = static_cast<unsigned>(r_);
		argb.element.green = static_cast<unsigned>(g_);
		argb.element.blue = static_cast<unsigned>(b_);
		argb.element.alpha_channel = static_cast<unsigned>(a_ * 255);
		return argb;
	}

	pixel_rgba_t color::rgba() const
	{
		pixel_rgba_t rgba;
		rgba.element.red = static_cast<unsigned>(r_);
		rgba.element.green = static_cast<unsigned>(g_);
		rgba.element.blue = static_cast<unsigned>(b_);
		rgba.element.alpha_channel = static_cast<unsigned>(a_ * 255);
		return rgba;
	}

	const double& color::r() const
	{
		return r_;
	}
	const double& color::g() const
	{
		return g_;
	}
	const double& color::b() const
	{
		return b_;
	}

	const double& color::a() const
	{
		return a_;
	}

	bool color::operator==(const color& other) const
	{
		return (px_color().value == other.px_color().value);
	}
	bool color::operator!=(const color& other) const
	{
		return (px_color().value != other.px_color().value);
	}

	color operator+(const color& x, const color& y)
	{
		double a = x.a_ + y.a_;
		auto r = static_cast<unsigned>(x.r_ + y.r_);
		auto g = static_cast<unsigned>(x.g_ + y.g_);
		auto b = static_cast<unsigned>(x.b_ + y.b_);

		return color{
			r > 255 ? 255 : r,
			g > 255 ? 255 : g,
			b > 255 ? 255 : b,
			a > 1.0 ? 1.0 : a };
	}
	//end class color


	//struct size
		size::size():width(0), height(0){}
		size::size(value_type width, value_type height) : width(width), height(height){}

		bool size::empty() const
		{
			return (0 == width || 0 == height);
		}

		bool size::is_hit(const point& pos) const
		{
			return (0 <= pos.x && pos.x < static_cast<int>(width) && 0 <= pos.y && pos.y < static_cast<int>(height));
		}

		size& size::shift()
		{
			std::swap(width, height);
			return *this;
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

		point rectangle::position() const noexcept
		{
			return{ x, y };
		}

		rectangle& rectangle::position(const point& p) noexcept
		{
			x = p.x;
			y = p.y;
			return *this;
		}

		size rectangle::dimension() const noexcept
		{
			return{width, height};
		}

		rectangle& rectangle::dimension(const size& sz) noexcept
		{
			width = sz.width;
			height = sz.height;
			return *this;
		}

		rectangle& rectangle::pare_off(int pixels)
		{
			x += pixels;
			y += pixels;
			auto const px_twice = (pixels << 1);
			if (px_twice > static_cast<int>(width))
				width = 0;
			else
				width -= px_twice;

			if (px_twice > static_cast<int>(height))
				height = 0;
			else
				height -= px_twice;

			return *this;
		}

		int rectangle::right() const noexcept
		{
			return x + static_cast<int>(width);
		}

		int rectangle::bottom() const noexcept
		{
			return y + static_cast<int>(height);
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

		rectangle& rectangle::shift()
		{
			std::swap(x, y);
			std::swap(width, height);
			return *this;
		}
	//end struct rectangle

	//class rectangle_rotator
		rectangle_rotator::rectangle_rotator(bool rotated, const ::nana::rectangle& area)
			: rotated_(rotated),
			area_(area)
		{}

		int rectangle_rotator::x() const
		{
			return (rotated_ ? area_.y : area_.x);
		}

		int & rectangle_rotator::x_ref()
		{
			return (rotated_ ? area_.y : area_.x);
		}

		int rectangle_rotator::y() const
		{
			return (rotated_ ? area_.x : area_.y);
		}

		int & rectangle_rotator::y_ref()
		{
			return (rotated_ ? area_.x : area_.y);
		}

		unsigned rectangle_rotator::w() const
		{
			return (rotated_ ? area_.height : area_.width);
		}

		unsigned & rectangle_rotator::w_ref()
		{
			return (rotated_ ? area_.height : area_.width);
		}

		unsigned rectangle_rotator::h() const
		{
			return (rotated_ ? area_.width : area_.height);
		}

		unsigned & rectangle_rotator::h_ref()
		{
			return (rotated_ ? area_.width : area_.height);
		}

		int rectangle_rotator::right() const
		{
			return (rotated_ ? area_.y + static_cast<int>(area_.height) : area_.x + static_cast<int>(area_.width));
		}

		int rectangle_rotator::bottom() const
		{
			return (rotated_ ? area_.x + static_cast<int>(area_.width) : area_.y + static_cast<int>(area_.height));
		}

		const ::nana::rectangle& rectangle_rotator::result() const
		{
			return area_;
		}
	//end class rectangle_rotator
}
