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
		class crook
			: public crook_interface
		{
			bool draw(graph_reference graph, nana::color_t bgcolor, nana::color_t fgcolor, const nana::rectangle& r, element_state es, const data& crook_data) override
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
								graph.set_pixel(left + x, top + y, (*colormap)[top][left]);
						}
					}
				}
				else
				{
					const nana::color_t highlighted = 0x5EB6F7;

					switch(es)
					{
					case element_state::hovered:
					case element_state::focus_hovered:
						bgcolor = graph.mix(bgcolor, highlighted, 0.8);
						fgcolor = graph.mix(fgcolor, highlighted, 0.8);
						break;
					case element_state::pressed:
						bgcolor = graph.mix(bgcolor, highlighted, 0.4);
						fgcolor = graph.mix(fgcolor, highlighted, 0.4);
						break;
					case element_state::disabled:
						bgcolor = fgcolor = 0xB2B7BC;
						break;
					default:
						//Leave things as they are
						break;
					}
					const int x = r.x + 1;
					const int y = r.y + 1;

					graph.rectangle(x, y, 13, 13, fgcolor, false);
					graph.rectangle(x + 1, y + 1, 11, 11, bgcolor, true);

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
								graph.line(sx, sy, sx, sy + 3, fgcolor);
							}

							for(int i = 0; i < 4; i++)
							{
								sx++;
								sy--;
								graph.line(sx, sy, sx, sy + 3, fgcolor);
							}
						}
						break;
					case state::partial:
						graph.rectangle(x + 2, y + 2, 9, 9, fgcolor, true);
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
			bool draw(graph_reference graph, nana::color_t, nana::color_t fgcolor, const nana::rectangle& r, element_state es, const data& crook_data) override
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
							graph.set_pixel(x + v, y, colormap[u][v]);
						}
						++y;
					}
				}
				else
				{
					int x = r.x + (static_cast<int>(r.width) - 16) / 2;
					int y = r.y + (static_cast<int>(r.height) - 16) / 2;

					nana::color_t light = graph.mix(fgcolor, 0xFFFFFF, 0.5);

					graph.line(x + 3, y + 7, x + 6, y + 10, fgcolor);
					graph.line(x + 7, y + 9, x + 12, y + 4, fgcolor);
					graph.line(x + 3, y + 8, x + 6, y + 11, light);
					graph.line(x + 7, y + 10, x + 12, y + 5, light);
					graph.line(x + 4, y + 7, x + 6, y + 9, light);
					graph.line(x + 7, y + 8, x + 11, y + 4, light);
				}
				return true;
			}
		};
	}

	template<typename ElementInterface>
	class element_object
		: nana::noncopyable, nana::nonmovable
	{
		typedef ElementInterface element_t;
		typedef pat::cloneable<element::provider::factory_interface<element_t>> factory_interface;

	public:
		element_object()
			:	element_ptr_(nullptr)
		{
		}

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
			element_ptr_ = factory_->create();

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

		element_t * const * keeper() const
		{
			return &element_ptr_;
		}
	private:
		factory_interface factory_;	//Keep the factory for destroying the element
		element_t * element_ptr_;
		std::vector<std::pair<element_t*, factory_interface>> spare_;
	};

	class element_manager
		: nana::noncopyable, nana::nonmovable
	{
		//VC2012 does not support alias declaration.
		//template<typename E> using factory_interface = element::provider::factory_interface<E>;

		template<typename ElementInterface>
		struct item
		{
			element_object<ElementInterface> * employee;
			std::map<std::string, std::shared_ptr<element_object<ElementInterface>>> table;
		};

		element_manager()
		{
			crook_.employee = nullptr;
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
			}
			return obj;
		}

		void crook(const std::string& name, const pat::cloneable<element::provider::factory_interface<element::crook_interface>>& factory)
		{
			_m_add(name, crook_, factory);
		}

		element::crook_interface * const * crook(const std::string& name) const
		{
			return _m_get(name, crook_).keeper();
		}
	private:
		typedef std::lock_guard<std::recursive_mutex> lock_guard;

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
		item<element::crook_interface> crook_;
	};

	namespace element
	{
		//class provider
		void provider::add_crook(const std::string& name, const pat::cloneable<factory_interface<crook_interface>>& factory)
		{
			element_manager::instance().crook(name, factory);
		}

		crook_interface* const * provider::keeper_crook(const std::string& name)
		{
			return element_manager::instance().crook(name);
		}
	}//end namespace element

	//facades
	//template<> class facade<element::crook>
		facade<element::crook>::facade()
			:	keeper_(element::provider().keeper_crook(""))
		{
			data_.check_state = state::unchecked;
			data_.radio = false;
		}

		facade<element::crook>::facade(const char* name)
			:	keeper_(element::provider().keeper_crook(name ? name : ""))
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
			keeper_ = element::provider().keeper_crook(name);
		}

		bool facade<element::crook>::draw(graph_reference graph, nana::color_t bgcol, nana::color_t fgcol, const nana::rectangle& r, element_state es)
		{
			return (*keeper_)->draw(graph, bgcol, fgcol, r, es, data_);
		}
	//end class facade<element::crook>

	namespace element
	{
		void set_bground(const char* name, const pat::cloneable<element_interface>& obj)
		{
			detail::bedrock::instance().get_element_store().bground(name, obj);
		}

		void set_bground(const char* name, pat::cloneable<element_interface> && obj)
		{
			detail::bedrock::instance().get_element_store().bground(name, std::move(obj));
		}

		//class cite
		cite_bground::cite_bground(const char* name)
			: ref_ptr_(detail::bedrock::instance().get_element_store().bground(name))
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
			ref_ptr_ = detail::bedrock::instance().get_element_store().bground(name);
		}

		bool cite_bground::draw(graph_reference dst, nana::color_t bgcolor, nana::color_t fgcolor, const nana::rectangle& r, element_state state)
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
				p->graph.make(graph.width(), graph.height());
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
		bool bground::draw(graph_reference dst, nana::color_t bgcolor, nana::color_t fgcolor, const nana::rectangle& to_r, element_state state)
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
