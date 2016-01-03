/*
*	Elements of GUI Gadgets
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file: nana/gui/element.cpp
*/

#include <nana/gui/element.hpp>
#include <nana/gui/detail/bedrock.hpp>
#include <nana/gui/detail/element_store.hpp>
#include <nana/paint/image.hpp>
#include <map>
#include <string>

#if defined(STD_THREAD_NOT_SUPPORTED)
	#include <nana/std_mutex.hpp>
#else
	#include <mutex>
#endif

namespace nana
{
	//Element definitions
	namespace element
	{
		namespace detail
		{
			void factory_abstract::destroy(element_abstract* ptr)
			{
				delete ptr;
			}
		}

		class crook
			: public crook_interface
		{
			bool draw(graph_reference graph, const nana::color& bgcolor, const nana::color& fgcolor, const nana::rectangle& r, element_state es, const data& crook_data) override
			{
				if(crook_data.radio)
				{
					unsigned bmp_unchecked[12][12] = {
						{0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xCFD0D0, 0xAAABAB, 0x919292, 0x919292, 0xAAABAB, 0xCFD0D0, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF},
						{0xFFFFFF, 0xFFFFFF, 0xA3A4A4, 0xB9BABA, 0xDBDBDB, 0xF2F2F2, 0xF2F2F2, 0xDBDBDB, 0xB9BABA, 0xA3A4A4, 0xFFFFFF, 0xFFFFFF},
						{0xFFFFFF, 0xA2A3A3, 0xC3C3C3, 0xEDEDEE, 0xC6C9CD, 0xB5BABF, 0xB5BABF, 0xC8CBCE, 0xEDEEEE, 0xC3C3C3, 0xA2A3A3, 0xFFFFFF},
						{0xCFD0D0, 0xB9BABA, 0xE9EAEB, 0xB3B8BD, 0xBDC2C7, 0xC8CDD2, 0xC9CED3, 0xC5C8CC, 0xBEC1C5, 0xEBECEC, 0xB9BABA, 0xCFD0D0},
						{0xA9A9A9, 0xDCDCDC, 0xC5C8CC, 0xBEC3C9, 0xCBCFD5, 0xCED2D7, 0xD5D8DC, 0xDCDEE0, 0xD3D4D7, 0xD4D5D5, 0xDCDCDC, 0xA9A9A9},
						{0x919292, 0xF2F2F2, 0xB4B9BD, 0xCDD1D6, 0xD3D6DA, 0xDBDDDF, 0xE4E4E5, 0xE9E9E9, 0xE8E8E9, 0xD0D1D2, 0xF2F2F2, 0x919292},
						{0x919292, 0xF2F2F2, 0xBBBEC2, 0xD7DADD, 0xE0E1E3, 0xE9E9E9, 0xEFEFEF, 0xF0F0F0, 0xEFEFF0, 0xDBDCDC, 0xEFEFEF, 0x939494},
						{0xA7A8A8, 0xDDDDDD, 0xCFD1D3, 0xD5D6D8, 0xE9E9E9, 0xEFEFEF, 0xF4F4F4, 0xF5F5F5, 0xEEEEEE, 0xE8E8E8, 0xDDDDDD, 0xA7A8A8},
						{0xCECECE, 0xBABBBB, 0xECECED, 0xCDCECF, 0xE1E2E2, 0xF0F0F0, 0xF4F4F4, 0xF1F1F1, 0xEBEBEB, 0xF2F2F2, 0xBABBBB, 0xCECECE},
						{0xFFFFFF, 0xA2A3A3, 0xC3C3C3, 0xF0F0F1, 0xE2E3E3, 0xE4E4E5, 0xE9EAEA, 0xEEEEEF, 0xF3F3F3, 0xC3C3C3, 0xA2A3A3, 0xFFFFFF},
						{0xFFFFFF, 0xFFFFFF, 0xA2A3A3, 0xBABBBB, 0xDBDBDB, 0xF4F4F4, 0xF4F4F4, 0xDCDCDC, 0xBABBBB, 0xA2A3A3, 0xFFFFFF, 0xFFFFFF},
						{0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xCECECE, 0xAAABAB, 0x8E8F8F, 0x8E8F8F, 0xA9A9A9, 0xCECECE, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF}
									};
					unsigned bmp_unchecked_highlight[12][12] = {
						{0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xB7CCD8, 0x7FA4BA, 0x5989A5, 0x5989A5, 0x7FA4BA, 0xB7CCD8, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF},
						{0xFFFFFF, 0xFFFFFF, 0x759DB4, 0x8FB7C8, 0xBCDDE5, 0xDBF6F8, 0xDBF6F8, 0xBCDDE5, 0x8FB7C8, 0x759DB4, 0xFFFFFF, 0xFFFFFF},
						{0xFFFFFF, 0x739BB3, 0x9CC2D0, 0xD3F4FA, 0x9BD7F9, 0x84CBF9, 0x84CBF9, 0x9CD8F9, 0xD4F4FA, 0x9CC2D0, 0x739BB3, 0xFFFFFF},
						{0xB7CCD8, 0x8FB7C8, 0xCFF1FA, 0x80CAF9, 0x96D3FB, 0xAADDFD, 0xABDDFD, 0x9AD5FB, 0x86CEF9, 0xCFF2FA, 0x8FB7C8, 0xB7CCD8},
						{0x7DA2B9, 0xBEDEE6, 0x9AD7F9, 0x98D5FB, 0xB1DFFD, 0xB2E0FD, 0xB7E3FD, 0xBCE5FD, 0xA6DCFB, 0xA1DCF9, 0xBEDEE6, 0x7DA2B9},
						{0x5989A5, 0xDBF6F8, 0x80CAF9, 0xAFDEFD, 0xB6E2FD, 0xBBE5FD, 0xC1E8FD, 0xC5EAFD, 0xC7EBFD, 0x99D8FA, 0xDBF6F8, 0x5989A5},
						{0x5989A5, 0xDBF6F8, 0x84CDF9, 0xB6E2FD, 0xBFE7FD, 0xC7EBFD, 0xD5F0FE, 0xDAF2FE, 0xD8F1FE, 0xB1E1FB, 0xD8F4F6, 0x5D8CA7},
						{0x7BA1B8, 0xBFDFE7, 0x9FDBF9, 0xA7DDFB, 0xC8EBFD, 0xD6F1FE, 0xE2F5FE, 0xE5F6FE, 0xD8F0FD, 0xCAEDFB, 0xBFDFE7, 0x7BA1B8},
						{0xB5CAD7, 0x91B8C9, 0xCFF2FA, 0x92D5F9, 0xBAE5FC, 0xDAF2FE, 0xE4F5FE, 0xDFF3FE, 0xD2EEFD, 0xDBF7FA, 0x91B8C9, 0xB5CAD7},
						{0xFFFFFF, 0x739BB3, 0x9CC2D0, 0xD7F6FA, 0xBDE8FB, 0xC2E8FC, 0xD0EDFD, 0xD7F2FC, 0xDDF8FA, 0x9CC2D0, 0x739BB3, 0xFFFFFF},
						{0xFFFFFF, 0xFFFFFF, 0x739BB3, 0x91B8C9, 0xBCDDE5, 0xDEF9FA, 0xDEF9FA, 0xBEDEE6, 0x91B8C9, 0x739BB3, 0xFFFFFF, 0xFFFFFF},
						{0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xB5CAD7, 0x7FA4BA, 0x5586A3, 0x5586A3, 0x7DA2B9, 0xB5CAD7, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF}
									};
					unsigned bmp_checked[12][12] = {
						{0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xCFD0D0, 0xAAABAB, 0x919292, 0x919292, 0xAAABAB, 0xCFD0D0, 0xFCFCFC, 0xFFFFFF, 0xFFFFFF},
						{0xFFFFFF, 0xFFFFFF, 0xA3A4A4, 0xB9BABA, 0xDBDBDB, 0xF2F2F2, 0xF2F2F2, 0xDBDBDB, 0xB9BABA, 0xA3A4A4, 0xF3F3F3, 0xFFFFFF},
						{0xFFFFFF, 0xA2A3A3, 0xC3C3C3, 0xEDEDEE, 0xBABFC5, 0x85939F, 0x85939F, 0xBCC1C5, 0xEDEEEE, 0xC3C3C3, 0xA2A3A3, 0xFCFCFC},
						{0xCFD0D0, 0xB9BABA, 0xE9EAEB, 0x8997A2, 0x274760, 0x486378, 0x365166, 0x204058, 0x8E9AA4, 0xEBECEC, 0xB9BABA, 0xCFD0D0},
						{0xA9A9A9, 0xDCDCDC, 0xB9BEC4, 0x24445D, 0x91B2C6, 0xC7EBFD, 0x69C2D4, 0x14405C, 0x1E3F57, 0xC9CCCD, 0xDCDCDC, 0xA9A9A9},
						{0x919292, 0xF2F2F2, 0x7D8D98, 0x304B5F, 0x90D5E5, 0x5DCEDD, 0x28A2D1, 0x178AC7, 0x183348, 0x8F9CA6, 0xF2F2F2, 0x919292},
						{0x919292, 0xF2F2F2, 0x82909C, 0x183347, 0x228FC6, 0x209DD1, 0x1898D1, 0x0E84C6, 0x183348, 0x97A3AC, 0xEFEFEF, 0x939494},
						{0xA7A8A8, 0xDDDDDD, 0xC0C5C9, 0x1E3F57, 0x0F3F5D, 0x0F83C7, 0x0B82C7, 0x0C3D5D, 0x1F3F58, 0xD9DCDE, 0xDDDDDD, 0xA7A8A8},
						{0xCECECE, 0xBABBBB, 0xECECED, 0x99A3AB, 0x1D3E57, 0x18354A, 0x19344A, 0x1E3E57, 0xAEB8BF, 0xF2F2F2, 0xBABBBB, 0xCECECE},
						{0xFFFFFF, 0xA2A3A3, 0xC3C3C3, 0xF0F0F1, 0xD1D5D7, 0xA6B0B9, 0xA9B4BC, 0xDCDFE2, 0xF3F3F3, 0xC3C3C3, 0xA2A3A3, 0xFFFFFF},
						{0xFFFFFF, 0xFFFFFF, 0xA2A3A3, 0xBABBBB, 0xDBDBDB, 0xF4F4F4, 0xF4F4F4, 0xDCDCDC, 0xBABBBB, 0xA2A3A3, 0xFFFFFF, 0xFFFFFF},
						{0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xCECECE, 0xAAABAB, 0x8E8F8F, 0x8E8F8F, 0xA9A9A9, 0xCECECE, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF}
									};

					unsigned bmp_checked_highlight[12][12] = {
						{0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xB7CCD8, 0x7FA4BA, 0x5989A5, 0x5989A5, 0x7FA4BA, 0xB7CCD8, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF},
						{0xFFFFFF, 0xFFFFFF, 0x759DB4, 0x8FB7C8, 0xBCDDE5, 0xDBF6F8, 0xDBF6F8, 0xBCDDE5, 0x8FB7C8, 0x759DB4, 0xFFFFFF, 0xFFFFFF},
						{0xFFFFFF, 0x739BB3, 0x9CC2D0, 0xD3F4FA, 0x92CCED, 0x639FC7, 0x639FC7, 0x93CDED, 0xD4F4FA, 0x9CC2D0, 0x739BB3, 0xFFFFFF},
						{0xB7CCD8, 0x8FB7C8, 0xCFF1FA, 0x66A3CC, 0x264862, 0x47647A, 0x355268, 0x1E405A, 0x66A3C9, 0xCFF2FA, 0x8FB7C8, 0xB7CCD8},
						{0x7DA2B9, 0xBEDEE6, 0x91CCED, 0x22445E, 0x9DBBCD, 0xE9F7FE, 0x7FE6EE, 0x154664, 0x1D3F58, 0x99D3EF, 0xBEDEE6, 0x7DA2B9},
						{0x5989A5, 0xDBF6F8, 0x5C98BF, 0x2F4B60, 0xB1F6FA, 0x74FFFF, 0x32CAFF, 0x1DAAF3, 0x173348, 0x6CA1C0, 0xDBF6F8, 0x5989A5},
						{0x5989A5, 0xDBF6F8, 0x5E99BF, 0x173348, 0x2AB0F2, 0x28C4FF, 0x1EBEFF, 0x11A3F2, 0x173348, 0x7BA6C0, 0xD8F4F6, 0x5D8CA7},
						{0x7BA1B8, 0xBFDFE7, 0x94CEEB, 0x1D3F58, 0x114567, 0x13A2F3, 0x0DA0F3, 0x0D4367, 0x1E3F58, 0xBEE0EF, 0xBFDFE7, 0x7BA1B8},
						{0xB5CAD7, 0x91B8C9, 0xCFF2FA, 0x6FA8C9, 0x1C3E58, 0x18354B, 0x18354B, 0x1D3E58, 0x9CBACC, 0xDBF7FA, 0x91B8C9, 0xB5CAD7},
						{0xFFFFFF, 0x739BB3, 0x9CC2D0, 0xD7F6FA, 0xAFDAED, 0x8EB3C9, 0x98B7CA, 0xC7E3EE, 0xDDF8FA, 0x9CC2D0, 0x739BB3, 0xFFFFFF},
						{0xFFFFFF, 0xFFFFFF, 0x739BB3, 0x91B8C9, 0xBCDDE5, 0xDEF9FA, 0xDEF9FA, 0xBEDEE6, 0x91B8C9, 0x739BB3, 0xFFFFFF, 0xFFFFFF},
						{0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xB5CAD7, 0x7FA4BA, 0x5586A3, 0x5586A3, 0x7DA2B9, 0xB5CAD7, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF}
									};

					unsigned bmp_checked_press[12][12] = {
						{0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xA6BDCE, 0x6089A8, 0x31668E, 0x31668E, 0x6089A8, 0xA6BDCE, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF},
						{0xFFFFFF, 0xFFFFFF, 0x5480A1, 0x6C99B8, 0x9DC4DC, 0xBEE1F3, 0xBEE1F3, 0x9DC4DC, 0x6C99B8, 0x5480A1, 0xFFFFFF, 0xFFFFFF},
						{0xFFFFFF, 0x517E9F, 0x7AA5C2, 0xB6DDF3, 0x73B2D6, 0x4A8AB0, 0x4A8AB0, 0x74B3D8, 0xB7DEF3, 0x7AA5C2, 0x517E9F, 0xFFFFFF},
						{0xA6BDCE, 0x6C99B8, 0xB1DBF1, 0x4B8DB4, 0x244660, 0x456279, 0x335167, 0x1D3F59, 0x4C90B7, 0xB1DCF3, 0x6C99B8, 0xA6BDCE},
						{0x5E87A6, 0x9FC5DD, 0x71B1D6, 0x21435D, 0x7EA5BC, 0x95D9FC, 0x478BAE, 0x113858, 0x1B3E58, 0x78BDE2, 0x9FC5DD, 0x5E87A6},
						{0x31668E, 0xBEE1F3, 0x4484AA, 0x2E4A60, 0x5DA2C6, 0x3A84AA, 0x19658D, 0x0F5984, 0x153248, 0x5794B7, 0xBEE1F3, 0x31668E},
						{0x31668E, 0xBEE1F3, 0x4687AE, 0x153247, 0x165D84, 0x14628D, 0x0F5F8D, 0x095684, 0x163248, 0x6B9DB9, 0xBBDEF1, 0x366990},
						{0x5B85A5, 0xA0C7DE, 0x74B7DC, 0x1B3E58, 0x0D3659, 0x0A5583, 0x075483, 0x0A3459, 0x1E3F58, 0xA9D2E9, 0xA0C7DE, 0x5B85A5},
						{0xA3BBCD, 0x6D9BBA, 0xB2DDF3, 0x5599BE, 0x1C3E57, 0x17344A, 0x17344B, 0x1D3E57, 0x91B3C7, 0xC1E4F6, 0x6D9BBA, 0xA3BBCD},
						{0xFFFFFF, 0x517E9F, 0x7AA5C2, 0xBBE1F5, 0x98CAE4, 0x80AAC3, 0x8CAFC5, 0xB7D7EA, 0xC2E4F6, 0x7AA5C2, 0x517E9F, 0xFFFFFF},
						{0xFFFFFF, 0xFFFFFF, 0x517E9F, 0x6D9BBA, 0x9DC4DC, 0xC2E4F6, 0xC2E4F6, 0x9FC5DD, 0x6D9BBA, 0x517E9F, 0xFFFFFF, 0xFFFFFF},
						{0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xA3BBCD, 0x6089A8, 0x2C628B, 0x2C628B, 0x5E87A6, 0xA3BBCD, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF}
									};

					unsigned (*colormap)[12][12] = &bmp_unchecked;

					switch(es)
					{
					case element_state::normal:
					case element_state::focus_normal:
						colormap = (crook_data.check_state != state::unchecked ? &bmp_checked : &bmp_unchecked);
						break;
					case element_state::hovered:
					case element_state::focus_hovered:
						colormap = (crook_data.check_state != state::unchecked ? &bmp_checked_highlight : &bmp_unchecked_highlight);
						break;
					case element_state::pressed:
						colormap = &bmp_checked_press;
						break;
					default:
						break;
					}

					const int x = r.x + 2;
					const int y = r.y + 2;

					for(int top = 0; top < 12; ++top)
					{
						for(int left = 0; left < 12; ++left)
						{
							if((*colormap)[top][left] != 0xFFFFFF)
								graph.set_pixel(left + x, top + y, static_cast<colors>((*colormap)[top][left]));
						}
					}
				}
				else
				{
					::nana::color highlighted(0x5e, 0xb6, 0xf7);
					auto bld_bgcolor = bgcolor;
					auto bld_fgcolor = fgcolor;
					switch(es)
					{
					case element_state::hovered:
					case element_state::focus_hovered:
						bld_bgcolor = bgcolor.blend(highlighted, 0.8);
						bld_fgcolor = fgcolor.blend(highlighted, 0.8);
						break;
					case element_state::pressed:
						bld_bgcolor = bgcolor.blend(highlighted, 0.4);
						bld_fgcolor = fgcolor.blend(highlighted, 0.4);
						break;
					case element_state::disabled:
						bld_bgcolor = bld_fgcolor = nana::color(0xb2, 0xb7, 0xbc);
						break;
					default:
						//Leave things as they are
						break;
					}
					const int x = r.x + 1;
					const int y = r.y + 1;

					graph.rectangle(rectangle{ x + 1, y + 1, 11, 11 }, true, bld_bgcolor);
					graph.rectangle(rectangle{ x, y, 13, 13 }, false, bld_fgcolor);

					switch(crook_data.check_state)
					{
					case state::checked:
						{
							int sx = x + 2;
							int sy = y + 4;

							for(int i = 0; i < 3; i++)
							{
								sx++;
								sy++;
								graph.line(point{ sx, sy }, point{ sx, sy + 3 });
							}

							for(int i = 0; i < 4; i++)
							{
								sx++;
								sy--;
								graph.line(point{ sx, sy }, point{ sx, sy + 3 });
							}
						}
						break;
					case state::partial:
						graph.rectangle(rectangle{ x + 2, y + 2, 9, 9 }, true);
						break;
					default:
						break;
					}
				}
				return true;
			}
		};	//end class crook

		class menu_crook
			: public crook_interface
		{
			bool draw(graph_reference graph, const ::nana::color&, const ::nana::color& fgcolor, const nana::rectangle& r, element_state es, const data& crook_data) override
			{
				if(crook_data.check_state == state::unchecked)
					return true;

				if(crook_data.radio)
				{
					unsigned colormap[8][8] = {
						{0xFF000000,0xdee7ef,0x737baa,0x232674,0x3c3f84,0x8d96ba,0xe0e9ef,0xFF000000},
						{0xdce4ed,0x242875,0x6f71b3,0x9fa0d6,0xc3c4e9,0xb1b2da,0x5c6098,0xdbe4ed},
						{0x7b81ad,0x4f5199,0x8182c1,0xa1a2d4,0xccccea,0xcccced,0x9c9dcf,0x7981ae},
						{0x2b2d77,0x4f509a,0x696baf,0x7879ba,0xa4a6d4,0xa9aad9,0x9193ce,0x1e2271},
						{0x36387f,0x383a87,0x52549c,0x6162a8,0x6f71b3,0x7e7fbf,0x7879ba,0x282c78},
						{0x9094ba,0x1b1c71,0x3c3e8b,0x4a4b96,0x585aa1,0x6768ac,0x464893,0x828bb6},
						{0xe2eaf1,0x4b4d8d,0x16186d,0x292b7c,0x333584,0x2c2e7f,0x454b8c,0xdfe9f0},
						{0xFF000000,0xe4ecf2,0x9599bd,0x454688,0x414386,0x9095bb,0xe3ebf2,0xFF000000}
					};

					int x = r.x + (static_cast<int>(r.width) - 8) / 2;
					int y = r.y + (static_cast<int>(r.height) - 8) / 2;

					for(int u = 0; u < 8; ++u)
					{
						for(int v = 0; v < 8; ++v)
						{
							if(colormap[u][v] & 0xFF000000)
								continue;
							graph.set_pixel(x + v, y, static_cast<color_rgb>(colormap[u][v]));
						}
						++y;
					}
				}
				else
				{
					int x = r.x + (static_cast<int>(r.width) - 16) / 2;
					int y = r.y + (static_cast<int>(r.height) - 16) / 2;

					graph.palette(false, fgcolor);
					graph.line(point{ x + 3, y + 7 }, point{ x + 6, y + 10 });
					graph.line(point{ x + 7, y + 9 }, point{ x + 12, y + 4 });

					graph.palette(false, fgcolor.blend(colors::white, 0.5));
					graph.line(point{ x + 3, y + 8 }, point{ x + 6, y + 11 });
					graph.line(point{ x + 7, y + 10 }, point{ x + 12, y + 5 });
					graph.line(point{ x + 4, y + 7 }, point{ x + 6, y + 9 });
					graph.line(point{ x + 7, y + 8 }, point{ x + 11, y + 4 });
				}
				return true;
			}
		};

		class border_depressed
			: public border_interface
		{
			bool draw(graph_reference graph, const ::nana::color& bgcolor, const ::nana::color& fgcolor, const ::nana::rectangle& r, element_state estate, unsigned weight)
			{
				graph.rectangle(r, false, static_cast<color_rgb>((element_state::focus_hovered == estate || element_state::focus_normal == estate) ? 0x0595E2 : 0x999A9E));
				graph.rectangle(::nana::rectangle(r).pare_off(1), false, bgcolor);
				return true;
			}
		};

		class arrow_solid_triangle
			: public arrow_interface
		{
			bool draw(graph_reference graph, const ::nana::color& bgcolor, const ::nana::color& fgcolor, const ::nana::rectangle& r, element_state estate, direction dir) override
			{
				::nana::point pos{ r.x + 3, r.y + 3 };
				switch (dir)
				{
				case ::nana::direction::east:
					pos.x += 3;
					pos.y += 1;
					for (int i = 0; i < 5; ++i)
						graph.line(point{ pos.x + i, pos.y + i }, point{ pos.x + i, pos.y + 8 - i });
					break;
				case ::nana::direction::south:
					pos.y += 3;
					for (int i = 0; i < 5; ++i)
						graph.line(point{ pos.x + i, pos.y + i }, point{ pos.x + 8 - i, pos.y + i });
					break;
				case ::nana::direction::west:
					pos.x += 5;
					pos.y += 1;
					for (int i = 0; i < 5; ++i)
						graph.line(point{ pos.x - i, pos.y + i }, point{ pos.x - i, pos.y + 8 - i });
					break;
				case ::nana::direction::north:
					pos.y += 7;
					for (int i = 0; i < 5; ++i)
						graph.line(point{ pos.x + i, pos.y - i }, point{ pos.x + 8 - i, pos.y - i });
					break;
				case direction::southeast:
					pos.x += 2;
					pos.y += 7;
					for (int i = 0; i < 6; ++i)
						graph.line(point{ pos.x + i, pos.y - i }, point{ pos.x + 5, pos.y - i });
					break;
				}
				return true;
			}
		};//end class arrow_solid_triangle

		class arrow_hollow_triangle
			: public arrow_interface
		{
			bool draw(graph_reference graph, const ::nana::color& bgcolor, const ::nana::color& fgcolor, const ::nana::rectangle& r, element_state estate, ::nana::direction dir) override
			{
				int x = r.x + 3;
				int y = r.y + 3;
				switch (dir)
				{
				case ::nana::direction::east:
					x += 3;
					graph.line(point{ x, y + 1 }, point{ x, y + 9 });
					graph.line(point{ x + 1, y + 2 }, point{ x + 4, y + 5 });
					graph.line(point{ x + 3, y + 6 }, point{ x + 1, y + 8 });
					break;
				case direction::southeast:
					x += 7;
					y += 6;
					graph.line(point{ x - 5 , y + 1 }, point{ x, y + 1 });
					graph.line(point{ x, y - 4 }, point{ x, y });
					graph.line(point{ x - 4, y }, point{ x - 1, y - 3 });
					break;
				case direction::south:
					y += 3;
					graph.line(point{ x, y }, point{ x + 8, y });
					graph.line(point{ x + 1, y + 1 }, point{ x + 4, y + 4 });
					graph.line(point{ x + 7, y + 1 }, point{ x + 5, y + 3 });
					break;
				case direction::west:
					x += 5;
					y += 1;
					graph.line(point{ x, y }, point{ x, y + 8 });
					graph.line(point{ x - 4, y + 4 }, point{ x - 1, y + 1 });
					graph.line(point{ x - 3, y + 5 }, point{ x - 1, y + 7 });
					break;
				case direction::north:
					y += 7;
					graph.line(point{ x, y }, point{ x + 8, y });
					graph.line(point{ x + 1, y - 1 }, point{ x + 4, y - 4 });
					graph.line(point{ x + 5, y - 3 }, point{ x + 7, y - 1 });
					break;
				}
				return true;
			}
		};//end class arrow_hollow_triangle

		class arrowhead
			: public arrow_interface
		{
			bool draw(graph_reference graph, const ::nana::color& bgcolor, const ::nana::color& fgcolor, const ::nana::rectangle& r, element_state estate, ::nana::direction dir) override
			{
				int x = r.x;
				int y = r.y + 5;
				switch (dir)
				{
				case direction::north:
				{
					x += 3;
					int pixels = 1;
					for (int l = 0; l < 4; ++l)
					{
						for (int i = 0; i < pixels; ++i)
						{
							if (l == 3 && i == 3)
								continue;
							graph.set_pixel(x + i, y);
						}

						x--;
						y++;
						pixels += 2;
					}

					graph.set_pixel(x + 1, y);
					graph.set_pixel(x + 2, y);
					graph.set_pixel(x + 6, y);
					graph.set_pixel(x + 7, y);
				}
				break;
				case direction::south:
				{
					graph.set_pixel(x, y);
					graph.set_pixel(x + 1, y);
					graph.set_pixel(x + 5, y);
					graph.set_pixel(x + 6, y);

					++y;
					int pixels = 7;
					for (int l = 0; l < 4; ++l)
					{
						for (int i = 0; i < pixels; ++i)
						{
							if (l != 0 || i != 3)
								graph.set_pixel(x + i, y);
						}

						x++;
						y++;
						pixels -= 2;
					}
				}
				default:break;
				}
				return true;
			}
		};//end class arrowhead

		class arrow_double
			: public arrow_interface
		{
			bool draw(graph_reference graph, const ::nana::color& bgcolor, const ::nana::color& fgcolor, const ::nana::rectangle& r, element_state estate, ::nana::direction dir) override
			{
				int x = r.x;
				int y = r.y;
				switch (dir)
				{
				case direction::east:
					_m_line(graph, x + 4, y + 6, true);
					_m_line(graph, x + 5, y + 7, true);
					_m_line(graph, x + 6, y + 8, true);
					_m_line(graph, x + 5, y + 9, true);
					_m_line(graph, x + 4, y + 10, true);
					break;
				case direction::west:
					_m_line(graph, x + 5, y + 6, true);
					_m_line(graph, x + 4, y + 7, true);
					_m_line(graph, x + 3, y + 8, true);
					_m_line(graph, x + 4, y + 9, true);
					_m_line(graph, x + 5, y + 10, true);
					break;
				case direction::south:
					_m_line(graph, x + 5, y + 4, false);
					_m_line(graph, x + 6, y + 5, false);
					_m_line(graph, x + 7, y + 6, false);
					_m_line(graph, x + 8, y + 5, false);
					_m_line(graph, x + 9, y + 4, false);
					break;
				case direction::north:
					_m_line(graph, x + 5, y + 6, false);
					_m_line(graph, x + 6, y + 5, false);
					_m_line(graph, x + 7, y + 4, false);
					_m_line(graph, x + 8, y + 5, false);
					_m_line(graph, x + 9, y + 6, false);
					break;
				default:
					break;
				}
				return true;
			}

			static void _m_line(nana::paint::graphics & graph, int x, int y, bool horizontal)
			{
				graph.set_pixel(x, y);
				if (horizontal)
				{
					graph.set_pixel(x + 1, y);
					graph.set_pixel(x + 4, y);
					graph.set_pixel(x + 5, y);
				}
				else
				{
					graph.set_pixel(x, y + 1);
					graph.set_pixel(x, y + 4);
					graph.set_pixel(x, y + 5);
				}
			}
		};//end class arrow_double

		class annex_button
			: public element_interface
		{
			bool draw(graph_reference graph, const ::nana::color& arg_bgcolor, const ::nana::color& fgcolor, const rectangle& r, element_state estate) override
			{
				auto bgcolor = arg_bgcolor;

				switch (estate)
				{
				case element_state::hovered:
				case element_state::focus_hovered:
					bgcolor = arg_bgcolor.blend(colors::white, 0.8);
					break;
				case element_state::pressed:
					bgcolor = arg_bgcolor.blend(colors::black, 0.8);
					break;
				case element_state::disabled:
					bgcolor = colors::dark_gray;
				default:
					break;
				}

				auto part_px = (r.height - 3) * 5 / 13;
				graph.rectangle(r, false, bgcolor.blend(colors::black, 0.6));
				
				::nana::point left_top{ r.x + 1, r.y + 1 }, right_top{r.right() - 2, r.y + 1};
				::nana::point left_mid{ r.x + 1, r.y + 1 + static_cast<int>(part_px) }, right_mid{ right_top.x, left_mid.y };
				::nana::point left_bottom{ r.x + 1, r.bottom() - 2 }, right_bottom{ r.right() - 2, r.bottom() - 2 };

				graph.palette(false, bgcolor.blend(colors::white, 0.9));
				graph.line(left_top, left_mid);
				graph.line(right_top, right_mid);

				graph.palette(false, bgcolor.blend(colors::white, 0.5));
				graph.line(left_top, right_top);

				left_mid.y++;
				right_mid.y++;
				graph.palette(false, bgcolor.blend(colors::black, 0.8));
				graph.line(left_mid, left_bottom);
				graph.line(right_mid, right_bottom);

				::nana::rectangle part_r{ r.x + 2, r.y + 2, r.width - 4, part_px };
				graph.rectangle(part_r, true, bgcolor.blend(colors::white, 0.8));

				part_r.y += static_cast<int>(part_r.height);
				part_r.height = (r.height - 3 - part_r.height);
				graph.rectangle(part_r, true, bgcolor);
				return true;
			}
		};//end class annex_button

		class x_icon
			: public element_interface
		{
			bool draw(graph_reference graph, const ::nana::color&, const ::nana::color& fgcolor, const rectangle& r, element_state estate) override
			{
				auto clr = fgcolor;

				switch (estate)
				{
				case element_state::hovered:
				case element_state::pressed:
					clr = clr.blend(colors::black, 0.8);
					break;
				case element_state::disabled:
					clr = colors::dark_gray;
				default:
					break;
				}

				graph.palette(false, clr);

				const int x = r.x + 4;
				const int y = r.y + 4;

				point p1{ x, y }, p2{ x + 7, y + 7 };

				graph.line(p1, p2);

				++p1.x;
				--p2.y;
				graph.line(p1, p2);

				p1.x = x;
				++p1.y;
				p2.x = x + 6;
				p2.y = y + 7;
				graph.line(p1, p2);

				p1.x += 7;
				p1.y = y;
				p2.x = x;
				graph.line(p1, p2);

				p1.x = x + 6;
				p2.y = y + 6;
				graph.line(p1, p2);

				++p1.x;
				++p1.y;
				++p2.x;
				++p2.y;
				graph.line(p1, p2);

				return true;
			}
		};
	}//end namespace element

	template<typename ElementInterface>
	class element_object
		: nana::noncopyable, nana::nonmovable
	{
		using element_type		= ElementInterface;
		using factory_interface = pat::cloneable<element::detail::factory_abstract>;

	public:
		~element_object()
		{
			if(factory_)
				factory_->destroy(element_ptr_);
		}

		void push(const factory_interface& rhs)
		{
			auto keep_f = factory_;
			auto keep_e = element_ptr_;

			factory_ = rhs;
			element_ptr_ = static_cast<element_type*>(static_cast<element::provider::factory_interface<element_type>&>(*factory_).create());

			if(nullptr == factory_ || nullptr == element_ptr_)
			{
				if(element_ptr_)
					factory_->destroy(element_ptr_);

				factory_.reset();

				factory_ = keep_f;
				element_ptr_ = keep_e;
			}
			else
				spare_.emplace_back(keep_e, keep_f);
		}

		element_type * const * cite() const
		{
			return &element_ptr_;
		}
	private:
		factory_interface factory_;	//Keep the factory for destroying the element
		element_type * element_ptr_{nullptr};
		std::vector<std::pair<element_type*, factory_interface>> spare_;
	};

	class element_manager
		: nana::noncopyable, nana::nonmovable
	{
		template<typename ElementInterface>
		struct item
		{
			element_object<ElementInterface> * employee;
			std::map<std::string, std::shared_ptr<element_object<ElementInterface>>> table;
		};

		element_manager()
		{
			crook_.employee = nullptr;
			border_.employee = nullptr;
		}

	public:
		static element_manager& instance()
		{
			static bool initial = true;
			static element_manager obj;
			if(initial)
			{
				initial = false;

				element::add_crook<element::crook>("");
				element::add_crook<element::menu_crook>("menu_crook");

				element::add_border<element::border_depressed>("");
				
				element::add_arrow<element::arrowhead>("");				//"arrowhead" in default
				element::add_arrow<element::arrow_double>("double");
				element::add_arrow<element::arrow_solid_triangle>("solid_triangle");
				element::add_arrow<element::arrow_hollow_triangle>("hollow_triangle");

				element::add_button<element::annex_button>("");	//"annex" in default

				element::add_x_icon<element::x_icon>("");
			}
			return obj;
		}

		void crook(const std::string& name, const pat::cloneable<element::provider::factory_interface<element::crook_interface>>& factory)
		{
			_m_add(name, crook_, factory);
		}

		element::crook_interface * const * crook(const std::string& name) const
		{
			return _m_get(name, crook_).cite();
		}

		void cross(const std::string& name, const pat::cloneable<element::provider::factory_interface<element::element_interface>>& factory)
		{
			_m_add(name, cross_, factory);
		}

		element::element_interface* const * cross(const std::string& name) const
		{
			return _m_get(name, cross_).cite();
		}

		void border(const std::string& name, const pat::cloneable<element::provider::factory_interface<element::border_interface>>& factory)
		{
			_m_add(name, border_, factory);
		}

		element::border_interface * const * border(const std::string& name) const
		{
			return _m_get(name, border_).cite();
		}

		void arrow(const std::string& name, const pat::cloneable<element::provider::factory_interface<element::arrow_interface>>& factory)
		{
			_m_add((name.empty() ? "arrowhead" : name), arrow_, factory);
		}

		element::arrow_interface * const * arrow(const std::string& name) const
		{
			return _m_get((name.empty() ? "arrowhead" : name), arrow_).cite();
		}

		void button(const std::string& name, const pat::cloneable<element::provider::factory_interface<element::element_interface>>& factory)
		{
			_m_add((name.empty() ? "annex" : name), button_, factory);
		}

		element::element_interface * const * button(const std::string& name) const
		{
			return _m_get((name.empty() ? "annex" : name), button_).cite();
		}

		void x_icon(const std::string& name, const pat::cloneable<element::provider::factory_interface<element::element_interface>>& factory)
		{
			_m_add(name, x_icon_, factory);
		}

		element::element_interface * const * x_icon(const std::string& name) const
		{
			return _m_get(name, x_icon_).cite();
		}
	private:
		using lock_guard = std::lock_guard<std::recursive_mutex>;

		template<typename ElementInterface>
		void _m_add(const std::string& name, item<ElementInterface>& m, const pat::cloneable<element::provider::factory_interface<ElementInterface>>& factory)
		{
			typedef element_object<ElementInterface> element_object_t;
			lock_guard lock(mutex_);

			auto & eop = m.table[name];
			if(nullptr == eop)
				eop = std::make_shared<element_object_t>();

			eop->push(factory);
			if(nullptr == m.employee)
				m.employee = eop.get();
		}

		template<typename ElementInterface>
		const element_object<ElementInterface>& _m_get(const std::string& name, const item<ElementInterface>& m) const
		{
			lock_guard lock(mutex_);

			auto i = m.table.find(name);
			if(i != m.table.end())
				return *(i->second);

			return *m.employee;
		}

	private:
		mutable std::recursive_mutex mutex_;
		item<element::crook_interface>	crook_;
		item<element::element_interface> cross_;
		item<element::border_interface>	border_;
		item<element::arrow_interface>	arrow_;
		item<element::element_interface>	button_;
		item<element::element_interface>	x_icon_;
	};

	namespace element
	{
		//class provider
		void provider::add_crook(const std::string& name, const pat::cloneable<factory_interface<crook_interface>>& factory)
		{
			element_manager::instance().crook(name, factory);
		}

		crook_interface* const * provider::cite_crook(const std::string& name)
		{
			return element_manager::instance().crook(name);
		}

		void provider::add_cross(const std::string& name, const pat::cloneable<factory_interface<element_interface>>& factory)
		{
			element_manager::instance().cross(name, factory);
		}

		element_interface* const* provider::cite_cross(const std::string& name)
		{
			return element_manager::instance().cross(name);
		}

		void provider::add_border(const std::string& name, const pat::cloneable<factory_interface<border_interface>>& factory)
		{
			element_manager::instance().border(name, factory);
		}

		border_interface* const * provider::cite_border(const std::string& name)
		{
			return element_manager::instance().border(name);
		}

		void provider::add_arrow(const std::string& name, const pat::cloneable<factory_interface<arrow_interface>>& factory)
		{
			element_manager::instance().arrow(name, factory);
		}

		arrow_interface* const * provider::cite_arrow(const std::string& name)
		{
			return element_manager::instance().arrow(name);
		}

		void provider::add_button(const std::string& name, const pat::cloneable<factory_interface<element_interface>>& factory)
		{
			element_manager::instance().button(name, factory);
		}

		element_interface* const* provider::cite_button(const std::string& name)
		{
			return element_manager::instance().button(name);
		}

		void provider::add_x_icon(const std::string& name, const pat::cloneable<factory_interface<element_interface>>& factory)
		{
			element_manager::instance().x_icon(name, factory);
		}

		element_interface* const* provider::cite_x_icon(const std::string& name)
		{
			return element_manager::instance().x_icon(name);
		}
	}//end namespace element

	//facades
	//template<> class facade<element::crook>
		facade<element::crook>::facade(const char* name)
			:	cite_(element::provider().cite_crook(name ? name : ""))
		{
			data_.check_state = state::unchecked;
			data_.radio = false;
		}

		facade<element::crook> & facade<element::crook>::reverse()
		{
			data_.check_state = (data_.check_state == facade<element::crook>::state::unchecked ? facade<element::crook>::state::checked : facade<element::crook>::state::unchecked);
			return *this;
		}

		facade<element::crook> & facade<element::crook>::check(state s)
		{
			data_.check_state = s;
			return *this;
		}

		facade<element::crook>::state facade<element::crook>::checked() const
		{
			return data_.check_state;
		}

		facade<element::crook> & facade<element::crook>::radio(bool r)
		{
			data_.radio = r;
			return *this;
		}

		bool facade<element::crook>::radio() const
		{
			return data_.radio;
		}

		void facade<element::crook>::switch_to(const char* name)
		{
			cite_ = element::provider().cite_crook(name ? name : "");
		}

		bool facade<element::crook>::draw(graph_reference graph, const ::nana::color& bgcol, const ::nana::color& fgcol, const nana::rectangle& r, element_state es)
		{
			return (*cite_)->draw(graph, bgcol, fgcol, r, es, data_);
		}
	//end class facade<element::crook>

	//class facade<element::cross>
		facade<element::cross>::facade(const char* name)
			: cite_(element::provider().cite_cross(name ? name : ""))
		{
		
		}

		void facade<element::cross>::switch_to(const char* name)
		{
			cite_ = element::provider().cite_cross(name ? name : "");
		}

		void facade<element::cross>::thickness(unsigned thk)
		{
			thickness_ = thk;
		}

		void facade<element::cross>::size(unsigned size_px)
		{
			size_ = size_px;
		}

		//Implement element_interface
		bool facade<element::cross>::draw(graph_reference graph, const ::nana::color&, const ::nana::color& fgcolor, const ::nana::rectangle& r, element_state)
		{
			if (thickness_ + 2 <= size_)
			{
				int gap = (static_cast<int>(size_) - static_cast<int>(thickness_)) / 2;

				nana::point ps[12];
				ps[0].x = r.x + gap;
				ps[1].x = ps[0].x + static_cast<int>(thickness_) - 1;
				ps[1].y = ps[0].y = r.y;

				ps[2].x = ps[1].x;
				ps[2].y = r.y + gap;

				ps[3].x = ps[2].x + gap;
				ps[3].y = ps[2].y;

				ps[4].x = ps[3].x;
				ps[4].y = ps[3].y + static_cast<int>(thickness_)-1;

				ps[5].x = ps[1].x;
				ps[5].y = ps[4].y;

				ps[6].x = ps[5].x;
				ps[6].y = ps[5].y + gap;

				ps[7].x = r.x + gap;
				ps[7].y = ps[6].y;

				ps[8].x = ps[7].x;
				ps[8].y = ps[4].y;

				ps[9].x = r.x;
				ps[9].y = ps[4].y;

				ps[10].x = r.x;
				ps[10].y = r.y + gap;

				ps[11].x = r.x + gap;
				ps[11].y = r.y + gap;

				graph.palette(false, fgcolor.blend(colors::black, true));

				for (int i = 0; i < 11; ++i)
					graph.line(ps[i], ps[i + 1]);
				graph.line(ps[11], ps[0]);

				graph.palette(false, fgcolor);

				unsigned thk_minus_2 = thickness_ - 2;
				graph.rectangle(rectangle{ ps[10].x + 1, ps[10].y + 1, (gap << 1) + thk_minus_2, thk_minus_2 }, true);
				graph.rectangle(rectangle{ ps[0].x + 1, ps[0].y + 1, thk_minus_2, (gap << 1) + thk_minus_2 }, true);
			}
			return true;
		}
	//end class facade<element::cross>

	//class facade<element::border>
		facade<element::border>::facade(const char* name)
			: cite_(element::provider().cite_border(name ? name : ""))
		{}

		void facade<element::border>::switch_to(const char* name)
		{
			cite_ = element::provider().cite_border(name ? name : "");
		}

		bool facade<element::border>::draw(graph_reference graph, const nana::color& bgcolor, const nana::color& fgcolor, const nana::rectangle& r, element_state es)
		{
			return (*cite_)->draw(graph, bgcolor, fgcolor, r, es, 2);
		}
	//end class facade<element::border>

	//class facade<element::arrow>
		facade<element::arrow>::facade(const char* name)
			: cite_(element::provider().cite_arrow(name ? name : ""))
		{
		}

		void facade<element::arrow>::switch_to(const char* name)
		{
			cite_ = element::provider().cite_arrow(name ? name : "");
		}

		void facade<element::arrow>::direction(::nana::direction dir)
		{
			dir_ = dir;
		}

		//Implement element_interface
		bool facade<element::arrow>::draw(graph_reference graph, const nana::color& bgcolor, const nana::color& fgcolor, const nana::rectangle& r, element_state estate)
		{
			graph.palette(false, fgcolor);
			return (*cite_)->draw(graph, bgcolor, fgcolor, r, estate, dir_);
		}
	//end class facade<element::arrow>

	//class facade<element::button>::
		facade<element::button>::facade(const char* name)
			: cite_(element::provider().cite_button(name ? name : ""))
		{}

		void facade<element::button>::switch_to(const char* name)
		{
			cite_ = element::provider().cite_button(name ? name : "");
		}

		//Implement element_interface
		bool facade<element::button>::draw(graph_reference graph, const ::nana::color& bgcolor, const ::nana::color& fgcolor, const ::nana::rectangle& r, element_state estate)
		{
			return (*cite_)->draw(graph, bgcolor, fgcolor, r, estate);
		}
	//end class facade<element::button>


	//class facade<element::x_icon>
		facade<element::x_icon>::facade(const char* name)
			: cite_(element::provider().cite_x_icon(name ? name : ""))
		{}

		void facade<element::x_icon>::switch_to(const char* name)
		{
			cite_ = element::provider().cite_x_icon(name ? name : "");
		}

		//Implement element_interface
		bool facade<element::x_icon>::draw(graph_reference graph, const ::nana::color& bgcolor, const ::nana::color& fgcolor, const ::nana::rectangle& r, element_state estate)
		{
			return (*cite_)->draw(graph, bgcolor, fgcolor, r, estate);
		}
	//end class facade<element::x_icon>

	namespace element
	{
		using brock = ::nana::detail::bedrock;

		void set_bground(const char* name, const pat::cloneable<element_interface>& obj)
		{
			brock::instance().get_element_store().bground(name, obj);
		}

		void set_bground(const char* name, pat::cloneable<element_interface> && obj)
		{
			brock::instance().get_element_store().bground(name, std::move(obj));
		}

		//class cite
		cite_bground::cite_bground(const char* name)
			: ref_ptr_(brock::instance().get_element_store().bground(name))
		{
		}

		void cite_bground::set(const cloneable_element& rhs)
		{
			holder_ = rhs;
			place_ptr_ = holder_.get();
			ref_ptr_ = &place_ptr_;
		}

		void cite_bground::set(const char* name)
		{
			holder_.reset();
			ref_ptr_ = brock::instance().get_element_store().bground(name);
		}

		bool cite_bground::draw(graph_reference dst, const ::nana::color& bgcolor, const ::nana::color& fgcolor, const nana::rectangle& r, element_state state)
		{
			if (ref_ptr_ && *ref_ptr_)
				return (*ref_ptr_)->draw(dst, bgcolor, fgcolor, r, state);

			return false;
		}
		//end class cite

		//class bground
		struct bground::draw_method
		{
			virtual ~draw_method(){}

			virtual draw_method * clone() const = 0;

			virtual void paste(const nana::rectangle& from_r, graph_reference, const nana::point& dst_pos) = 0;
			virtual void stretch(const nana::rectangle& from_r, graph_reference dst, const nana::rectangle & to_r) = 0;
		};

		struct bground::draw_image
			: public draw_method
		{
			nana::paint::image image;

			draw_image(const nana::paint::image& img)
				: image(img)
			{}

			draw_method * clone() const override
			{
				return new draw_image(image);
			}

			void paste(const nana::rectangle& from_r, graph_reference dst, const nana::point& dst_pos) override
			{
				image.paste(from_r, dst, dst_pos);
			}

			void stretch(const nana::rectangle& from_r, graph_reference dst, const nana::rectangle& to_r) override
			{
				image.stretch(from_r, dst, to_r);
			}
		};

		struct bground::draw_graph
			: public draw_method
		{
			nana::paint::graphics graph;

			draw_graph()
			{}

			draw_graph(const nana::paint::graphics& g)
				: graph(g)
			{}

			draw_method * clone() const override
			{
				auto p = new draw_graph;
				p->graph.make(graph.size());
				graph.paste(p->graph, 0, 0);
				return p;
			}

			void paste(const nana::rectangle& from_r, graph_reference dst, const nana::point& dst_pos) override
			{
				graph.paste(from_r, dst, dst_pos.x, dst_pos.y);
			}

			void stretch(const nana::rectangle& from_r, graph_reference dst, const nana::rectangle& to_r) override
			{
				graph.stretch(from_r, dst, to_r);
			}
		};

		bground::bground()
			:	method_(nullptr),
				vertical_(false),
				stretch_all_(true),
				left_(0), top_(0), right_(0), bottom_(0)
		{
			reset_states();
		}

		bground::bground(const bground& rhs)
			: method_(rhs.method_ ? rhs.method_->clone() : nullptr),
			vertical_(rhs.vertical_),
			valid_area_(rhs.valid_area_),
			states_(rhs.states_),
			join_(rhs.join_),
			stretch_all_(rhs.stretch_all_),
			left_(rhs.left_), top_(rhs.top_), right_(rhs.right_), bottom_(rhs.bottom_)
		{
		}

		bground::~bground()
		{
			delete method_;
		}

		bground& bground::operator=(const bground& rhs)
		{
			if (this != &rhs)
			{
				delete method_;
				method_		= (rhs.method_ ? rhs.method_->clone() : nullptr);
				vertical_	= rhs.vertical_;
				valid_area_ = rhs.valid_area_;
				states_ = rhs.states_;
				join_ = rhs.join_;
				stretch_all_ = rhs.stretch_all_;
				left_	= rhs.left_;
				top_	= rhs.top_;
				right_	= rhs.right_;
				bottom_	= rhs.bottom_;
			}
			return *this;
		}

		//Set a picture for the background
		bground& bground::image(const paint::image& img, bool vertical, const nana::rectangle& valid_area)
		{
			delete method_;
			method_ = new draw_image(img);
			vertical_ = vertical;

			if (valid_area.width && valid_area.height)
				valid_area_ = valid_area;
			else
				valid_area_ = nana::rectangle(img.size());
			return *this;
		}

		bground& bground::image(const paint::graphics& graph, bool vertical, const nana::rectangle& valid_area)
		{
			delete method_;
			method_ = new draw_graph(graph);
			vertical_ = vertical;

			if (valid_area.width && valid_area.height)
				valid_area_ = valid_area;
			else
				valid_area_ = nana::rectangle(graph.size());
			return *this;
		}

		//Set the state sequence of the background picture.
		void bground::states(const std::vector<element_state> & s)
		{
			states_ = s;
		}

		void bground::states(std::vector<element_state> && s)
		{
			states_ = std::move(s);
		}

		void bground::reset_states()
		{
			states_.clear();
			states_.push_back(element_state::normal);
			states_.push_back(element_state::hovered);
			states_.push_back(element_state::focus_normal);
			states_.push_back(element_state::focus_hovered);
			states_.push_back(element_state::pressed);
			states_.push_back(element_state::disabled);
			join_.clear();
		}

		void bground::join(element_state target, element_state joiner)
		{
			join_[joiner] = target;
		}

		void bground::stretch_parts(unsigned left, unsigned top, unsigned right, unsigned bottom)
		{
			left_ = left;
			top_ = top;
			right_ = right;
			bottom_ = bottom;

			stretch_all_ = !(left || right || top || bottom);
		}

		//Implement the methods of bground_interface.
		bool bground::draw(graph_reference dst, const ::nana::color&, const ::nana::color&, const nana::rectangle& to_r, element_state state)
		{
			if (nullptr == method_)
				return false;

			auto mi = join_.find(state);
			if (mi != join_.end())
				state = mi->second;

			std::size_t pos = 0;
			for (; pos < states_.size(); ++pos)
			{
				if (states_[pos] == state)
					break;
			}

			if (pos == states_.size())
				return false;

			nana::rectangle from_r = valid_area_;
			if (vertical_)
			{
				from_r.height /= static_cast<unsigned>(states_.size());
				from_r.y += static_cast<int>(from_r.height * pos);
			}
			else
			{
				from_r.width /= static_cast<unsigned>(states_.size());
				from_r.x += static_cast<int>(from_r.width * pos);
			}

			if (stretch_all_)
			{
				if (from_r.width == to_r.width && from_r.height == to_r.height)
					method_->paste(from_r, dst, to_r);
				else
					method_->stretch(from_r, dst, to_r);

				return true;
			}


			auto perf_from_r = from_r;
			auto perf_to_r = to_r;

			if (left_ + right_ < to_r.width)
			{
				nana::rectangle src_r = from_r;
				src_r.y += static_cast<int>(top_);
				src_r.height -= top_ + bottom_;

				nana::rectangle dst_r = to_r;
				dst_r.y += static_cast<int>(top_);
				dst_r.height -= top_ + bottom_;

				if (left_)
				{
					src_r.width = left_;
					dst_r.width = left_;

					method_->stretch(src_r, dst, dst_r);

					perf_from_r.x += static_cast<int>(left_);
					perf_from_r.width -= left_;
					perf_to_r.x += static_cast<int>(left_);
					perf_to_r.width -= left_;
				}

				if (right_)
				{
					src_r.x += (static_cast<int>(from_r.width) - static_cast<int>(right_));
					src_r.width = right_;

					dst_r.x += (static_cast<int>(to_r.width) - static_cast<int>(right_));
					dst_r.width = right_;

					method_->stretch(src_r, dst, dst_r);

					perf_from_r.width -= right_;
					perf_to_r.width -= right_;
				}
			}

			if (top_ + bottom_ < to_r.height)
			{
				nana::rectangle src_r = from_r;
				src_r.x += static_cast<int>(left_);
				src_r.width -= left_ + right_;

				nana::rectangle dst_r = to_r;
				dst_r.x += static_cast<int>(left_);
				dst_r.width -= left_ + right_;

				if (top_)
				{
					src_r.height = top_;
					dst_r.height = top_;

					method_->stretch(src_r, dst, dst_r);

					perf_from_r.y += static_cast<int>(top_);
					perf_to_r.y += static_cast<int>(top_);
				}

				if (bottom_)
				{
					src_r.y += static_cast<int>(from_r.height - bottom_);
					src_r.height = bottom_;

					dst_r.y += static_cast<int>(to_r.height - bottom_);
					dst_r.height = bottom_;

					method_->stretch(src_r, dst, dst_r);
				}

				perf_from_r.height -= (top_ + bottom_);
				perf_to_r.height -= (top_ + bottom_);
			}

			if (left_)
			{
				nana::rectangle src_r = from_r;
				src_r.width = left_;
				if (top_)
				{
					src_r.height = top_;
					method_->paste(src_r, dst, to_r);
				}
				if (bottom_)
				{
					src_r.y += static_cast<int>(from_r.height) - static_cast<int>(bottom_);
					src_r.height = bottom_;
					method_->paste(src_r, dst, nana::point(to_r.x, to_r.y + static_cast<int>(to_r.height - bottom_)));
				}
			}

			if (right_)
			{
				const int to_x = to_r.x + int(to_r.width - right_);

				nana::rectangle src_r = from_r;
				src_r.x += static_cast<int>(src_r.width) - static_cast<int>(right_);
				src_r.width = right_;
				if (top_)
				{
					src_r.height = top_;
					method_->paste(src_r, dst, nana::point(to_x, to_r.y));
				}
				if (bottom_)
				{
					src_r.y += (static_cast<int>(from_r.height) - static_cast<int>(bottom_));
					src_r.height = bottom_;
					method_->paste(src_r, dst, nana::point(to_x, to_r.y + int(to_r.height - bottom_)));
				}
			}

			method_->stretch(perf_from_r, dst, perf_to_r);
			return true;
		}
		//end class bground
	}//end namespace element
}//end namespace nana
