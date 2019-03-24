/*
 *	Utility Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/layout_utility.hpp
 */

#ifndef NANA_GUI_LAYOUT_UTILITY_HPP
#define NANA_GUI_LAYOUT_UTILITY_HPP

#include "basis.hpp"

namespace nana
{
	//overlap test if overlapped between r1 and r2
	bool overlapped(const rectangle& r1, const rectangle& r2);

	// overlap, compute the overlapping area between r1 and r2. the r is for root
	bool overlap(const rectangle& r1, const rectangle& r2, rectangle& r);

	bool overlap(const rectangle& ir, const size& valid_input_area, const rectangle & dr, const size& valid_dst_area, rectangle& output_src_r, rectangle& output_dst_r);

	bool intersection(const rectangle & r, point pos_beg, point pos_end, point& good_pos_beg, point& good_pos_end);

	/// Zoom the input_s to fit for ref_s
	void fit_zoom(const size& input_s, const size& ref_s, size& result_s);
	size fit_zoom(const size& input_s, size ref_s);

	//zoom
	//@brief:	Calculate the scaled rectangle by refer dst rectangle, that scale factor is same as that between scaled and refer.
	void zoom(const rectangle& refer, const rectangle& scaled, const rectangle& refer_dst, rectangle& r);

	//covered
	//@brief:	Tests a rectangle whether it is wholly covered by another.
	bool covered(const rectangle& underlying, // 1st rectangle must be placed under 2nd rectangle
						const rectangle& cover);
}//end namespace nana
#endif
