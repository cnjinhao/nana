
namespace nana
{
	namespace reshaping
	{
		namespace arabic
		{
			const unsigned short TATWEEL = 0x0640;
			const unsigned short ZWJ = 0x200D;

			const int unshaped = 255;
			const int isolated = 0;
			const int initial = 1;
			const int medial = 2;
			const int final = 3;

			unsigned short letters[][4] = {
	    		{0xFE80, 0, 0, 0},			//ARABIC LETTER HAMZA
	    		{0xFE81, 0, 0, 0xFE82},		//ARABIC LETTER ALEF WITH MADDA ABOVE
	    		{0xFE83, 0, 0, 0xFE84},		//ARABIC LETTER ALEF WITH HAMZA ABOVE
    			{0xFE85, 0, 0, 0xFE86},		//ARABIC LETTER WAW WITH HAMZA ABOVE
    			{0xFE87, 0, 0, 0xFE88},		//ARABIC LETTER ALEF WITH HAMZA BELOW
    			{0xFE89, 0xFE8B, 0xFE8C, 0xFE8A},	//ARABIC LETTER YEH WITH HAMZA ABOVE
    			{0xFE8D, 0, 0, 0xFE8E},				//ARABIC LETTER ALEF
    			{0xFE8F, 0xFE91, 0xFE92, 0xFE90},	//ARABIC LETTER BEH
    			{0xFE93, 0, 0, 0xFE94},	//ARABIC LETTER TEH MARBUTA
    			{0xFE95, 0xFE97, 0xFE98, 0xFE96},	//ARABIC LETTER TEH
    			{0xFE99, 0xFE9B, 0xFE9C, 0xFE9A},	//ARABIC LETTER THEH
    			{0xFE9D, 0xFE9F, 0xFEA0, 0xFE9E},	//ARABIC LETTER JEEM
    			{0xFEA1, 0xFEA3, 0xFEA4, 0xFEA2},	//ARABIC LETTER HAH
    			{0xFEA5, 0xFEA7, 0xFEA8, 0xFEA6},	//ARABIC LETTER KHAH
    			{0xFEA9, 0, 0, 0xFEAA},	//ARABIC LETTER DAL
    			{0xFEAB, 0, 0, 0xFEAC},	//ARABIC LETTER THAL
    			{0xFEAD, 0, 0, 0xFEAE},	//ARABIC LETTER REH
    			{0xFEAF, 0, 0, 0xFEB0},	//ARABIC LETTER ZAIN
    			{0xFEB1, 0xFEB3, 0xFEB4, 0xFEB2},	//ARABIC LETTER SEEN
    			{0xFEB5, 0xFEB7, 0xFEB8, 0xFEB6},	//ARABIC LETTER SHEEN
    			{0xFEB9, 0xFEBB, 0xFEBC, 0xFEBA},	//ARABIC LETTER SAD
    			{0xFEBD, 0xFEBF, 0xFEC0, 0xFEBE},	//ARABIC LETTER DAD
    			{0xFEC1, 0xFEC3, 0xFEC4, 0xFEC2},	//ARABIC LETTER TAH
    			{0xFEC5, 0xFEC7, 0xFEC8, 0xFEC6},	//ARABIC LETTER ZAH
    			{0xFEC9, 0xFECB, 0xFECC, 0xFECA},	//ARABIC LETTER AIN
    			{0xFECD, 0xFECF, 0xFED0, 0xFECE},	//ARABIC LETTER GHAIN
    			{TATWEEL,   TATWEEL,  TATWEEL,  TATWEEL},	//ARABIC TATWEEL
    			{0xFED1, 0xFED3, 0xFED4, 0xFED2},	//ARABIC LETTER FEH
    			{0xFED5, 0xFED7, 0xFED8, 0xFED6},	//ARABIC LETTER QAF
    			{0xFED9, 0xFEDB, 0xFEDC, 0xFEDA},	//ARABIC LETTER KAF
    			{0xFEDD, 0xFEDF, 0xFEE0, 0xFEDE},	//ARABIC LETTER LAM
    			{0xFEE1, 0xFEE3, 0xFEE4, 0xFEE2},	//ARABIC LETTER MEEM
    			{0xFEE5, 0xFEE7, 0xFEE8, 0xFEE6},	//ARABIC LETTER NOON
    			{0xFEE9, 0xFEEB, 0xFEEC, 0xFEEA},	//ARABIC LETTER HEH
    			{0xFEED, 0, 0, 0xFEEE},			//ARABIC LETTER WAW
    			{0xFEEF, 0xFBE8, 0xFBE9, 0xFEF0},	//ARABIC LETTER (UIGHUR KAZAKH KIRGHIZ)? ALEF MAKSURA
    			{0xFEF1, 0xFEF3, 0xFEF4, 0xFEF2},	//ARABIC LETTER YEH
    			{0xFB50, 0, 0, 0xFB51},			//ARABIC LETTER ALEF WASLA
    			{0xFBDD, 0, 0, 0},					//ARABIC LETTER U WITH HAMZA ABOVE
    			{0xFB66, 0xFB68, 0xFB69, 0xFB67},	//ARABIC LETTER TTEH
    			{0xFB5E, 0xFB60, 0xFB61, 0xFB5F},	//ARABIC LETTER TTEHEH
    			{0xFB52, 0xFB54, 0xFB55, 0xFB53},	//ARABIC LETTER BEEH
    			{0xFB56, 0xFB58, 0xFB59, 0xFB57},	//ARABIC LETTER PEH
    			{0xFB62, 0xFB64, 0xFB65, 0xFB63},	//ARABIC LETTER TEHEH
    			{0xFB5A, 0xFB5C, 0xFB5D, 0xFB5B},	//ARABIC LETTER BEHEH
    			{0xFB76, 0xFB78, 0xFB79, 0xFB77},	//ARABIC LETTER NYEH
    			{0xFB72, 0xFB74, 0xFB75, 0xFB73},	//ARABIC LETTER DYEH
    			{0xFB7A, 0xFB7C, 0xFB7D, 0xFB7B},	//ARABIC LETTER TCHEH
    			{0xFB7E, 0xFB80, 0xFB81, 0xFB7F},	//ARABIC LETTER TCHEHEH
    			{0xFB88, 0, 0, 0xFB89},	//ARABIC LETTER DDAL
    			{0xFB84, 0, 0, 0xFB85},	//ARABIC LETTER DAHAL
    			{0xFB82, 0, 0, 0xFB83},	//ARABIC LETTER DDAHAL
    			{0xFB86, 0, 0, 0xFB87},	//ARABIC LETTER DUL
    			{0xFB8C, 0, 0, 0xFB8D},	//ARABIC LETTER RREH
    			{0xFB8A, 0, 0, 0xFB8B},	//ARABIC LETTER JEH
    			{0xFB6A, 0xFB6C, 0xFB6D, 0xFB6B},	//ARABIC LETTER VEH
    			{0xFB6E, 0xFB70, 0xFB71, 0xFB6F},	//ARABIC LETTER PEHEH
    			{0xFB8E, 0xFB90, 0xFB91, 0xFB8F},	//ARABIC LETTER KEHEH
    			{0xFBD3, 0xFBD5, 0xFBD6, 0xFBD4},	//ARABIC LETTER NG
    			{0xFB92, 0xFB94, 0xFB95, 0xFB93},	//ARABIC LETTER GAF
    			{0xFB9A, 0xFB9C, 0xFB9D, 0xFB9B},	//ARABIC LETTER NGOEH
    			{0xFB96, 0xFB98, 0xFB99, 0xFB97},	//ARABIC LETTER GUEH
    			{0xFB9E, 0, 0, 0xFB9F},	//ARABIC LETTER NOON GHUNNA
    			{0xFBA0, 0xFBA2, 0xFBA3, 0xFBA1},	//ARABIC LETTER RNOON
    			{0xFBAA, 0xFBAC, 0xFBAD, 0xFBAB},	//ARABIC LETTER HEH DOACHASHMEE
    			{0xFBA4, 0, 0, 0xFBA5},	//ARABIC LETTER HEH WITH YEH ABOVE
    			{0xFBA6, 0xFBA8, 0xFBA9, 0xFBA7},	//ARABIC LETTER HEH GOAL
    			{0xFBE0, 0, 0, 0xFBE1},	//ARABIC LETTER KIRGHIZ OE
    			{0xFBD9, 0, 0, 0xFBDA},	//ARABIC LETTER OE
    			{0xFBD7, 0, 0, 0xFBD8},	//ARABIC LETTER U
    			{0xFBDB, 0, 0, 0xFBDC},	//ARABIC LETTER YU
    			{0xFBE2, 0, 0, 0xFBE3},	//ARABIC LETTER KIRGHIZ YU
    			{0xFBDE, 0, 0, 0xFBDF},	//ARABIC LETTER VE
    			{0xFBFC, 0xFBFE, 0xFBFF, 0xFBFD},	//ARABIC LETTER FARSI YEH
    			{0xFBE4, 0xFBE6, 0xFBE7, 0xFBE5},	//ARABIC LETTER E
    			{0xFBAE, 0, 0, 0xFBAF},	//ARABIC LETTER YEH BARREE
    			{0xFBB0, 0, 0, 0xFBB1},	//ARABIC LETTER YEH BARREE WITH HAMZA ABOVE
				{ZWJ, ZWJ, ZWJ, ZWJ}
			};

			bool harakat(wchar_t letter)
			{
				return
					(0x0610 <= letter && letter <= 0x061A)	|| 
					(0x064B <= letter && letter <= 0x065F)	||
					(0x0670 == letter) ||
					(0x06D6 <= letter && letter <= 0x06DC)	||
					(0x06DF <= letter && letter <= 0x06E8)	||
					(0x06EA <= letter && letter <= 0x06ED)	||
					(0x08D4 <= letter && letter <= 0x08E1)	||
					(0x08D4 <= letter && letter <= 0x08ED)	||
					(0x08E3 <= letter && letter <= 0x08FF);
			}

			int form_index(wchar_t letter)
			{
				static unsigned short ranges[][2]={
					{0x0621, 0x063A},
					{0x0640, 0x064A},
					{0x0671, 0x0671},
					{0x0677, 0x0677},
					{0x0679, 0x067B},
					{0x067E, 0x0680},
					{0x0683, 0x0684},
					{0x0686, 0x0688},
					{0x068C, 0x068E},
					{0x0691, 0x0691},
					{0x0698, 0x0698},
					{0x06A4, 0x06A4},
					{0x06A6, 0x06A6},
					{0x06A9, 0x06A9},
					{0x06AD, 0x06AD},
					{0x06AF, 0x06AF},
					{0x06B1, 0x06B1},
					{0x06B3, 0x06B3},
					{0x06BA, 0x06BB},
					{0x06BE, 0x06BE},
					{0x06C0, 0x06C1},
					{0x06C5, 0x06C9},
					{0x06CB, 0x06CC},
					{0x06D0, 0x06D0},
					{0x06D2, 0x06D3},
					{ZWJ, ZWJ}
				};

				if((letter < 0x0621) || (0x06D3 < letter && letter != ZWJ))
					return -1;

				int base = 0;
				for(std::size_t i = 0; i < sizeof(ranges) / sizeof(unsigned short) / 2; ++i)
				{
					if(ranges[i][0] <= letter && letter <= ranges[i][1])
						return static_cast<int>(letter - ranges[i][0]) + base;

					base += static_cast<int>(ranges[i][1] - ranges[i][0]) + 1;
				}
				return base;
			}

			wchar_t connect_before(wchar_t letter)
			{
				auto idx = form_index(letter);
				if(idx < 0)
					return 0;

				return letters[idx][final] || letters[idx][medial];
			}

			wchar_t connect_after(wchar_t letter)
			{
				auto idx = form_index(letter);
				if(idx < 0)
					return 0;

				return letters[idx][initial] || letters[idx][medial];
			}

			wchar_t connect_before_after(wchar_t letter)
			{
				auto idx = form_index(letter);
				if(idx < 0)
					return 0;

				return letters[idx][medial];
			}

			std::wstring reshape(const std::wstring& text)
			{
				bool const use_unshaped_instead_of_isolated = false;
				bool const delete_harakat = true;
				bool const shift_harakat_position = false;
				bool const delete_tatweel = false;
				bool const support_zwj = true;

				const int no_form = -1;
				const int isolated_form = use_unshaped_instead_of_isolated ? unshaped : isolated;

				std::wstring output;
				std::vector<int> forms;

				std::map<int, std::wstring> positions_harakat;

				for(auto letter: text)
				{
					if(harakat(letter))
					{
						if(!delete_harakat)
						{
							int position = static_cast<int>(output.size()) - 1;

		                    if (shift_harakat_position)
		                        --position;
		                    if (positions_harakat.count(position) == 0)
		                        positions_harakat[position];

		                    if (shift_harakat_position)
		                    {
		                    	auto & ph = positions_harakat[position];
		                    	ph.insert(ph.cbegin(), letter);
		                    }
		                    else
		                        positions_harakat[position] += letter;
						}
						continue;
					}
					else if(((TATWEEL == letter) && delete_tatweel) || (ZWJ == letter && !support_zwj))
					{
						continue;
					}

					auto idx = form_index(letter);
					if(idx < 0)
					{
						output += letter;
						forms.push_back(no_form);
						continue;
					}

					if(forms.empty())
					{
						output += letter;
						forms.push_back(isolated_form);
						continue;
					}

					if((forms.back() == no_form) || (!connect_before(letter)) || (!connect_after(output.back())) ||
						((forms.back() == final) && !connect_before_after(output.back())))
					{
						output += letter;
						forms.push_back(isolated_form);
					}
					else if(forms.back() == isolated_form)
					{
						forms.back() = initial;

						output += letter;
						forms.push_back(final);
					}
					else
					{
						forms.back() = medial;
						output += letter;
						forms.push_back(final);
					}

					//Remove ZWJ if it's the second to last item as it won't be useful
					if(support_zwj && (output.size() > 1) && (output[output.size() - 2] == ZWJ))
						output.erase(output.size() - 2, 1);
				}

				//Remove ZWJ if it's the second to last item as it won't be useful
				if(support_zwj && (output.size() > 0) && (output.back() == ZWJ))
					output.pop_back();


				std::wstring result;
				if((!delete_harakat) && positions_harakat.count(-1))
					result += positions_harakat[-1];

				for(std::size_t i = 0; i < output.size(); ++i)
				{
					if(output[i])
					{
						if(forms[i] == no_form || forms[i] == unshaped)
							result += output[i];
						else
							result += letters[form_index(output[i])][forms[i]];
					}

					if(!delete_harakat)
						if(positions_harakat.count(i))
							result += positions_harakat[i];
				}

				return result;
			}

		}//end namespace arabic
	}//end namespace reshaping
}