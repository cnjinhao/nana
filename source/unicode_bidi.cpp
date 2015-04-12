#include <nana/unicode_bidi.hpp>

namespace nana
{
	namespace bidi_charmap
	{
		enum t{	L, LRE, LRO, R, AL, RLE, RLO,
				PDF, EN, ES, ET, AN, CS, NSM, BN,
				B, S, WS, ON};

		static unsigned char charmap_0x0000_0x00C0[192] = {
			BN, BN, BN, BN, BN, BN, BN, BN, BN, S,  B,  S,  WS, B, BN, BN,
			BN, BN, BN, BN, BN, BN, BN, BN, BN, BN, BN, BN, B,  B,  B,  S, 
			WS, ON, ON, ET, ET, ET, ON, ON, ON, ON, ON, ES, CS, ES, CS, CS,
			EN, EN, EN, EN, EN, EN, EN, EN, EN, EN, CS, ON, ON, ON, ON, ON,
			ON, L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,
			L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  ON, ON, ON, ON, ON,
			ON, L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,
			L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  L,  ON, ON, ON, ON, BN,
			BN, BN, BN, BN, BN, B,  BN, BN, BN, BN, BN, BN, BN, BN, BN, BN,
			BN, BN, BN, BN, BN, BN, BN, BN, BN, BN, BN, BN, BN, BN, BN, BN,
			CS, ON, ET, ET, ET, ET, ON, ON, ON, ON, L,  ON, ON, BN, ON, ON,
			ET, ET, EN, EN, ON, L,  ON, ON, ON, EN, L,  ON, ON, ON, ON, ON
		};

		t bidi_char_type(wchar_t ch)
		{
			if(ch <= 0x0FC6)
			{
				if(ch < 0x00C0)	return static_cast<bidi_charmap::t>(charmap_0x0000_0x00C0[ch]);
				if(ch == 0x00D7 || ch == 0x00F7) return ON;
				if(ch <= 0x02B8) return L;	//N = 449
				if(ch <= 0x02BA) return ON;	//N = 2
				if(ch <= 0x02C1) return L;	//N = 7
				if(ch <= 0x02CF) return ON;	//N = 14
				if(ch <= 0x02D1) return L;	//N = 2
				if(ch <= 0x02DF) return ON;	//N = 14
				if(ch <= 0x02E4) return L;	//N = 5
				if(0x02EE == ch) return L;
				if(ch <= 0x02FF) return ON;	//N = 17
				if(ch <= 0x036F) return NSM;	//N = 112
				if(ch <= 0x0373) return L;	//N = 4
				if(ch <= 0x0375) return ON;	//N = 2
				if(ch <= 0x037D) return L;	//N = 8
				if(ch <= 0x0385) return ON;	//N = 8
				if(0x0387 == ch || 0x03F6 == ch) return ON;
				if(ch <= 0x0482) return L;	//N = 140
				if(ch <= 0x0489) return NSM;	//N = 7
				if(ch <= 0x0589) return L;	//N = 256
				if(0x058A == ch) return ON;
				if(0x058F == ch) return ET;
				if(0x05BE == ch || 0x05C0 == ch || 0x05C3 == ch || 0x05C6 == ch) return R;
				if(ch <= 0x05C7) return NSM;
				if(ch <= 0x05F4) return R;	//N = 37
				if(ch <= 0x0604) return AN;	//N = 5
				if(ch <= 0x0607) return ON;	//N = 2
				if(0x0608 == ch) return AL;
				if(ch <= 0x060A) return ET;	//N = 2
				if(0x060B == ch) return AL;
				if(0x060C == ch) return CS;
				if(0x060D == ch) return AL;
				if(ch <= 0x060F) return ON;		//N = 2
				if(ch <= 0x061A) return NSM;	//N = 11
				if(ch <= 0x064A) return AL;		//N = 48
				if(ch <= 0x065F) return NSM;	//N = 21
				if (0x066A == ch) return ET;
				if(ch <= 0x066C) return AN;	//N = 2
				if (0x0670 == ch) return NSM;
				if(ch <= 0x06D5) return AL;	//N = 101
				if(0x06DD == ch) return AN;
				if(0x06DE == ch) return ON;
				if(ch <= 0x06E4) return NSM;	//N = 6
				if(ch <= 0x06E6) return AL;	//N = 2
				if(0x06E9 == ch) return ON;
				if(ch <= 0x06ED) return NSM;	//N = 4
				if(ch <= 0x06EF) return AL;	//N = 2
				if(ch <= 0x06F9) return EN;	//N = 10
				if(0x0711 == ch) return NSM;
				if(ch <= 0x072F) return AL;	//N = 30
				if(ch <= 0x074A) return NSM;	//N = 27
				if(ch <= 0x07A5) return AL;	//N = 89
				if(ch <= 0x07B0) return NSM;	//N = 11
				if(0x07B1 == ch) return AL;
				if(ch <= 0x07EA) return R;	//N = 43
				if(ch <= 0x07F3) return NSM;	//N = 9
				if(ch <= 0x07F5) return R;	//N = 2
				if(ch <= 0x07F9) return ON;	//N = 4
				if(ch <= 0x0815) return R;	//N = 28
				if(0x081A == ch || 0x0824 == ch || 0x0828 == ch) return R;
				if(ch <= 0x082D) return NSM;	//N = 5
				if(ch <= 0x0858) return R;	//N = 41
				if(ch <= 0x085B) return NSM;	//N = 3
				if(0x085E == ch) return R;
				if(ch <= 0x08AC) return AL;	//N = 13
				if(ch <= 0x0902) return NSM;	//N = 31
				if(0x093A == ch || 0x093C == ch) return NSM;
				if(ch <= 0x0940) return L;	//N = 4
				if(ch <= 0x0948) return NSM;	//N = 8
				if(0x094D == ch) return NSM;
				if(ch <= 0x0950) return L;	//N = 3
				if(ch <= 0x0957) return NSM;	//N = 7
				if(ch <= 0x0961) return L;	//N = 10
				if(ch <= 0x0963) return NSM;	//N = 2
				if(0x0981 == ch || 0x09BC == ch) return NSM;
				if(ch <= 0x09C0) return L;	//N = 4
				if(ch <= 0x09C4) return NSM;	//N = 4
				if(0x09CD == ch) return NSM;
				if(ch <= 0x09E1) return L;	//N = 20
				if(ch <= 0x09E3) return NSM;	//N = 2
				if(ch <= 0x09F1) return L;	//N = 12
				if(ch <= 0x09F3) return ET;	//N = 2
				if(ch <= 0x09FA) return L;	//N = 7
				if(0x09FB == ch) return ET;
				if(ch <= 0x0A02) return NSM;	//N = 2
				if(0x0A3C == ch) return NSM;
				if(ch <= 0x0A40) return L;	//N = 3
				if(ch <= 0x0A51) return NSM;	//N = 17
				if(ch <= 0x0A6F) return L;	//N = 23
				if(ch <= 0x0A71) return NSM;	//N = 2
				if(ch <= 0x0A74) return L;	//N = 3
				if(ch <= 0x0A82) return NSM;	//N = 14
				if(0x0ABC == ch) return NSM;
				if(ch <= 0x0AC0) return L;	//N = 4
				if(ch <= 0x0AC8) return NSM;	//N = 8
				if(0x0ACD == ch) return NSM;
				if(ch <= 0x0AE1) return L;	//N = 18
				if(ch <= 0x0AE3) return NSM;	//N = 2
				if(0x0AF1 == ch) return ET;
				if(0x0B01 == ch || 0x0B3C == ch || 0x0B3F == ch) return NSM;
				if(ch <= 0x0B40) return L;
				if(ch <= 0x0B44) return NSM;	//N = 4
				if(ch <= 0x0B4C) return L;	//N = 6
				if(ch <= 0x0B56) return NSM;	//N = 10
				if(ch <= 0x0B61) return L;	//N = 11
				if(ch <= 0x0B63) return NSM;	//N = 2
				if(0x0B82 == ch || 0x0BC0 == ch || 0x0BCD == ch) return NSM;
				if(ch <= 0x0BF2) return L;	//N = 35
				if(0x0BF9 == ch) return ET;
				if(ch <= 0x0BFA) return ON;
				if(ch <= 0x0C3D) return L;	//N = 61
				if(ch <= 0x0C40) return NSM;	//N = 3
				if(ch <= 0x0C44) return L;	//N = 4
				if(ch <= 0x0C56) return NSM;	//N = 17
				if(ch <= 0x0C61) return L;	//N = 10
				if(ch <= 0x0C63) return NSM;	//N = 2
				if(ch <= 0x0C6F) return L;	//N = 10
				if(ch <= 0x0C7E) return ON;	//N = 7
				if(0x0CBC == ch) return NSM;
				if(ch <= 0x0CCB) return L;	//N = 15
				if(ch <= 0x0CCD) return NSM;	//N = 2
				if(ch <= 0x0CE1) return L;	//N = 13
				if(ch <= 0x0CE3) return NSM;	//N = 2
				if(ch <= 0x0D40) return L;	//N = 91
				if(ch <= 0x0D44) return NSM;	//N = 4
				if(0x0D4D == ch) return NSM;
				if(ch <= 0x0D61) return L;	//N = 20
				if(ch <= 0x0D63) return NSM;	//N = 2
				if(0x0DCA == ch) return NSM;
				if(ch <= 0x0DD1) return L;	//N = 3
				if(ch <= 0x0DD6) return NSM;	//N = 5
				if(0x0E31 == ch) return NSM;
				if(ch <= 0x0E33) return L;	//N = 2
				if(ch <= 0x0E3A) return NSM;	//N = 7
				if(0x0E3F == ch) return ET;
				if(ch <= 0x0E46) return L;	//N = 7
				if(ch <= 0x0E4E) return NSM;	//N = 8
				if(0x0EB1 == ch) return NSM;
				if(ch <= 0x0EB3) return L;	//N = 2
				if(ch <= 0x0EBC) return NSM;	//N = 9
				if(ch <= 0x0EC6) return L;	//N = 10
				if(ch <= 0x0ECD) return NSM;	//N = 6
				if(ch <= 0x0F17) return L;	//N = 72
				if(ch <= 0x0F19) return NSM;	//N = 2
				if(ch <= 0x0F34) return L;	//N = 27
				if(ch <= 0x0F39) return (ch & 1 ? NSM : L);
				if(ch <= 0x0F3D) return ON;	//N = 4
				if(ch <= 0x0F6C) return L;	//N = 47
				if(0x0F7F == ch || 0x0F85 == ch) return L;
				if(ch <= 0x0F87) return NSM;	//N = 2
				if(ch <= 0x0F8C) return L;	//N = 5
				if(ch <= 0x0FBC) return NSM;	//N = 48
				if(ch <= 0x0FC5) return L;	//N = 8
				if(0x0FC6 == ch) return NSM;
			}

			if(ch <= 0x1FFE)
			{
				if(ch <= 0x102C) return L;	//N = 102
				if(0x1031 == ch || 0x1038 == ch) return L;
				if(ch <= 0x103A) return NSM;	//N = 2
				if(ch <= 0x103C) return L;	//N = 2
				if(ch <= 0x103E) return NSM;	//N = 2
				if(ch <= 0x1057) return L;	//N = 25
				if(ch <= 0x1059) return NSM;	//N = 2
				if(ch <= 0x105D) return L;	//N = 4
				if(ch <= 0x1060) return NSM;	//N = 3
				if(ch <= 0x1070) return L;	//N = 16
				if(ch <= 0x1074) return NSM;	//N = 4
				if(0x1082 == ch) return NSM;
				if(ch <= 0x1084) return L;	//N = 2
				if(ch <= 0x1086) return NSM;	//N = 2
				if(0x108D == ch || 0x109D == ch) return NSM;
				if(ch <= 0x135A) return L;	//N = 701
				if(ch <= 0x135F) return NSM;	//N = 3
				if(ch <= 0x138F) return L;	//N = 48
				if(ch <= 0x1399) return ON;	//N = 10
				if(0x1400 == ch) return ON;
				if(0x1680 == ch) return WS;
				if(ch <= 0x169A) return L;	//N = 26
				if(ch <= 0x169C) return ON;	//N = 2
				if(ch <= 0x1711) return L;	//N = 114
				if(ch <= 0x1714) return NSM;	//N = 3
				if(ch <= 0x1731) return L;	//N = 18
				if(ch <= 0x1734) return NSM;	//N = 3
				if(ch <= 0x1751) return L;	//N = 29
				if(ch <= 0x1753) return NSM;	//N = 2
				if(ch <= 0x1770) return L;	//N = 17
				if(ch <= 0x1773) return NSM;	//N = 2
				if(ch <= 0x17B3) return L;	//N = 52
				if(0x17B6 == ch) return L;
				if(ch <= 0x17BD) return NSM;	//N = 7
				if(0x17C6 == ch) return NSM;
				if(ch <= 0x17C8) return L;	//N = 2
				if(ch <= 0x17D3) return NSM;	//N = 11
				if(0x17DB == ch) return ET;
				if(0x17DD == ch) return NSM;
				if(ch <= 0x17E9) return L;	//N = 10
				if(ch <= 0x180A) return ON;	//N = 27
				if(ch <= 0x180D) return NSM;	//N = 3
				if(0x180E == ch) return WS;
				if(0x18A9 == ch) return NSM;
				if(ch <= 0x191C) return L;	//N = 115
				if(ch <= 0x1922) return NSM;	//N = 3
				if(ch <= 0x1926) return L;	//N = 4
				if(ch <= 0x1928) return NSM;	//N = 2
				if(0x1932 == ch) return NSM;
				if(ch <= 0x1938) return L;	//N = 6
				if(ch <= 0x193B) return NSM;	//N = 3
				if(ch <= 0x1945) return ON;	//N = 6
				if(ch <= 0x19DA) return L;	//N = 149
				if(ch <= 0x19FF) return ON;	//N = 34
				if(ch <= 0x1A16) return L;	//N = 23
				if(ch <= 0x1A18) return NSM;	//N = 2
				if(ch <= 0x1A55) return L;	//N = 61
				if(0x1A57 == ch) return L;	
				if(ch <= 0x1A60) return NSM;	//N = 9
				if(0x1A62 == ch) return NSM;
				if(ch <= 0x1A64) return L;	//N = 2
				if(ch <= 0x1A6C) return NSM;	//N = 8
				if(ch <= 0x1A72) return L;	//N = 6
				if(ch <= 0x1A7F) return NSM;	//N = 13
				if(ch <= 0x1AAD) return L;	//N = 46
				if(ch <= 0x1B03) return NSM;	//N = 4
				if(ch <= 0x1B33) return L;	//N = 48
				if(0x1B35 == ch || 0x1B3B == ch) return L;
				if(ch <= 0x1B3C) return NSM;
				if(0x1B42 == ch) return NSM;
				if(ch <= 0x1B6A) return L;	//N = 40
				if(ch <= 0x1B73) return NSM;	//N = 9
				if(ch <= 0x1B7C) return L;	//N = 9
				if(ch <= 0x1B81) return NSM;	//N = 2
				if(ch <= 0x1BA1) return L;	//N = 32
				if(ch <= 0x1BA5) return NSM;	//N = 4
				if(ch <= 0x1BA7) return L;	//N = 2
				if(ch <= 0x1BA9) return NSM;	//N = 2
				if(0x1BAB == ch || 0x1BE6 == ch) return NSM;
				if(ch <= 0x1BE7) return L;
				if(ch <= 0x1BE9) return NSM;	//N = 2
				if(ch <= 0x1BEC) return L;	//N = 3
				if(0x1BEE == ch) return L;
				if(ch <= 0x1BF1) return NSM;	//N = 3
				if(ch <= 0x1C2B) return L;	//N = 58
				if(ch <= 0x1C33) return NSM;	//N = 8
				if(ch <= 0x1C35) return L;	//N = 2
				if(ch <= 0x1C37) return NSM;	//N = 2
				if(ch <= 0x1CC7) return L;	//N = 141
				if(0x1CD3 == ch || 0x1CE1 == ch) return L;
				if(ch <= 0x1CE8) return NSM;	//N = 7
				if(0x1CED == ch) return NSM;
				if(0x1CF4 == ch) return NSM;
				if(ch <= 0x1DBF) return L;	//N = 203
				if(ch <= 0x1DFF) return NSM;	//N = 64
				if(ch <= 0x1FBC) return L;	//N = 445
				if(0x1FBE == ch) return L;
				if(ch <= 0x1FC1) return ON;	//N = 3
				if(ch <= 0x1FCC) return L;	//N = 11
				if(ch <= 0x1FCF) return ON;	//N = 3
				if(ch <= 0x1FDB) return L;	//N = 12
				if(ch <= 0x1FDF) return ON;	//N = 3
				if(ch <= 0x1FEC) return L;	//N = 13
				if(ch <= 0x1FEF) return ON;	//N = 3
				if(ch <= 0x1FFC) return L;	//N = 11
				if(ch <= 0x1FFE) return ON;	//N = 2
			}
			if(ch <= 0x200A) return WS;	//N = 11
			if(ch <= 0x200D) return BN;	//N = 3
			if(0x200E == ch) return L;
			if(0x200F == ch) return R;
			if(ch <= 0x2027) return ON;	//N = 24
			if(0x2028 == ch) return WS;
			if(0x2029 == ch) return B;
			if(0x202A == ch) return LRE;
			if(0x202B == ch) return RLE;
			if(0x202C == ch) return PDF;
			if(0x202D == ch) return LRO;
			if(0x202E == ch) return RLO;
			if(0x202F == ch) return CS;
			if(ch <= 0x2034) return ET;	//N = 5
			if(ch <= 0x2043) return ON;	//N = 15
			if(0x2044 == ch) return CS;
			if(ch <= 0x205E) return ON;	//N = 26
			if(0x205F == ch) return WS;
			if(ch <= 0x206F) return BN;	//N = 16
			if(0x2070 == ch) return EN;
			if(0x2071 == ch) return L;
			if(ch <= 0x2079) return EN;	//N = 6
			if(ch <= 0x207B) return ES;	//N = 2
			if(ch <= 0x207E) return ON;	//N = 3
			if(0x207F == ch) return L;
			if(ch <= 0x2089) return EN;	//N = 10
			if(ch <= 0x208B) return ES;	//N = 2
			if(ch <= 0x208E) return ON;	//N = 3
			if(ch <= 0x209C) return L;	//N = 13
			if(ch <= 0x20B9) return ET;	//N = 26
			if(ch <= 0x20F0) return NSM;	//N = 33
			if(0x2102 == ch) return L;
			if(0x2107 == ch) return L;
			if(ch <= 0x2109) return ON;	//N = 2
			if(ch <= 0x2113) return L;	//N = 10
			if(0x2115 == ch) return L;
			if(ch <= 0x2118) return ON;	//N = 3
			if(ch <= 0x211D) return L;	//N = 5
			if(ch <= 0x2123) return ON;	//N = 6
			if(ch <= 0x2129) return (ch & 1 ? ON : L);
			if(0x212E == ch) return ET;
			if(ch <= 0x2139) return L;	//N = 11
			if(ch <= 0x213B) return ON;	//N = 2
			if(ch <= 0x213F) return L;	//N = 4
			if(ch <= 0x2144) return ON;	//N = 5
			if(ch <= 0x2149) return L;	//N = 5
			if(ch <= 0x214D) return ON;	//N = 4
			if(ch <= 0x214F) return L;	//N = 2
			if(ch <= 0x215F) return ON;	//N = 16
			if(ch <= 0x2188) return L;	//N = 41
			if(ch <= 0x2211) return ON;	//N = 137
			if(0x2212 == ch) return ES;
			if(0x2213 == ch) return ET;
			if(ch <= 0x2335) return ON;	//N = 290
			if(ch <= 0x237A) return L;	//N = 69
			if(0x2395 == ch) return L;
			if(ch <= 0x2487) return ON;	//N = 242
			if(ch <= 0x249B) return EN;	//N = 20
			if(ch <= 0x24E9) return L;	//N = 78
			if(0x26AC == ch) return L;
			if(ch <= 0x27FF) return ON;	//N = 339
			if(ch <= 0x28FF) return L;	//N = 256
			if(ch <= 0x2B59) return ON;	//N = 602
			if(ch <= 0x2CE4) return L;	//N = 229
			if(ch <= 0x2CEA) return ON;	//N = 6
			if(ch <= 0x2CEE) return L;	//N = 4
			if(ch <= 0x2CF1) return NSM;	//N = 3
			if(ch <= 0x2CF3) return L;	//N = 2
			if(ch <= 0x2CFF) return ON;	//N = 7
			if(0x2D7F == ch) return NSM;
			if(ch <= 0x2DDE) return L;	//N = 95
			if(ch <= 0x2DFF) return NSM;	//N = 32
			if(0x3000 == ch) return WS;
			if(ch <= 0x3004) return ON;	//N = 4
			if(ch <= 0x3007) return L;	//N = 3
			if(ch <= 0x3020) return ON;	//N = 25
			if(ch <= 0x3029) return L;	//N = 9
			if(ch <= 0x302D) return NSM;	//N = 4
			if(0x3030 == ch) return ON;
			if(ch <= 0x3035) return L;	//N = 5
			if(ch <= 0x3037) return ON;	//N = 2
			if(ch <= 0x303C) return L;	//N = 5
			if(ch <= 0x303F) return ON;	//N = 3
			if(ch <= 0x3096) return L;	//N = 86
			if(ch <= 0x309A) return NSM;	//N = 2
			if(ch <= 0x309C) return ON;	//N = 2
			if(0x30A0 == ch) return ON;
			if(0x30FB == ch) return ON;
			if(ch <= 0x31BA) return L;	//N = 191
			if(ch <= 0x31E3) return ON;	//N = 36
			if(ch <= 0x321C) return L;	//N = 45
			if(ch <= 0x321E) return ON;	//N = 2
			if(ch <= 0x324F) return L;	//N = 48
			if(ch <= 0x325F) return ON;	//N = 16
			if(ch <= 0x327B) return L;	//N = 28
			if(ch <= 0x327E) return ON;	//N = 3
			if(ch <= 0x32B0) return L;	//N = 50
			if(ch <= 0x32BF) return ON;	//N = 15
			if(ch <= 0x32CB) return L;	//N = 12
			if(ch <= 0x32CF) return ON;	//N = 4
			if(ch <= 0x3376) return L;	//N = 167
			if(ch <= 0x337A) return ON;	//N = 4
			if(ch <= 0x33DD) return L;	//N = 99
			if(ch <= 0x33DF) return ON;	//N = 2
			if(0x33FF == ch) return ON;
			if(ch <= 0x4DB5) return L;	//N = 6582
			if(ch <= 0x4DFF) return ON;	//N = 64
			if(ch <= 0xA48C) return L;	//N = 22157
			if(ch <= 0xA4C6) return ON;	//N = 55
			if(ch <= 0xA60C) return L;	//N = 317
			if(ch <= 0xA60F) return ON;	//N = 3
			if(ch <= 0xA66E) return L;	//N = 95
			if(0xA673 == ch) return ON;
			if(ch <= 0xA67D) return NSM;	//N = 10
			if(ch <= 0xA67F) return ON;	//N = 2
			if(0xA69F == ch) return NSM;
			if(ch <= 0xA6EF) return L;	//N = 80
			if(ch <= 0xA6F1) return NSM;	//N = 2
			if(ch <= 0xA6F7) return L;	//N = 6
			if(ch <= 0xA721) return ON;	//N = 34
			if(0xA788 == ch) return ON;
			if(0xA802 == ch) return NSM;
			if(0xA806 == ch) return NSM;
			if(0xA80B == ch) return NSM;
			if(ch <= 0xA824) return L;	//N = 25
			if(ch <= 0xA826) return NSM;	//N = 2
			if(0xA827 == ch) return L;
			if(ch <= 0xA82B) return ON;	//N = 4
			if(ch <= 0xA837) return L;	//N = 8
			if(ch <= 0xA839) return ET;	//N = 2
			if(ch <= 0xA873) return L;	//N = 52
			if(ch <= 0xA877) return ON;	//N = 4
			if(0xA8C4 == ch) return NSM;
			if(ch <= 0xA8D9) return L;	//N = 12
			if(ch <= 0xA8F1) return NSM;	//N = 18
			if(ch <= 0xA925) return L;	//N = 52
			if(ch <= 0xA92D) return NSM;	//N = 8
			if(ch <= 0xA946) return L;	//N = 25
			if(ch <= 0xA951) return NSM;	//N = 11
			if(ch <= 0xA97C) return L;	//N = 43
			if(ch <= 0xA982) return NSM;	//N = 3
			if(0xA9B3 == ch) return NSM;
			if(ch <= 0xA9B5) return L;	//N = 2
			if(ch <= 0xA9B9) return NSM;	//N = 4
			if(0xA9BC == ch) return NSM;
			if(ch <= 0xAA28) return L;	//N = 108
			if(ch <= 0xAA2E) return NSM;	//N = 6
			if(ch <= 0xAA30) return L;	//N = 2
			if(ch <= 0xAA32) return NSM;	//N = 2
			if(ch <= 0xAA34) return L;	//N = 2
			if(ch <= 0xAA36) return NSM;	//N = 2
			if(0xAA43 == ch) return NSM;
			if(0xAA4C == ch) return NSM;
			if(0xAAB0 == ch) return NSM;
			if(ch <= 0xAAB1) return L;
			if(ch <= 0xAAB4) return NSM;	//N = 3
			if(ch <= 0xAAB6) return L;	//N = 2
			if(ch <= 0xAAB8) return NSM;	//N = 2
			if(ch <= 0xAABD) return L;	//N = 5
			if(ch <= 0xAABF) return NSM;	//N = 2
			if(0xAAC1 == ch) return NSM;
			if(ch <= 0xAAEB) return L;	//N = 42
			if(ch <= 0xAAED) return NSM;	//N = 2
			if(0xAAF6 == ch) return NSM;
			if(0xABE5 == ch) return NSM;
			if(0xABE8 == ch) return NSM;
			if(0xABED == ch) return NSM;
			if(ch <= 0xFB17) return L;	//N = 20264
			if(0xFB1E == ch) return NSM;
			if(0xFB29 == ch) return ES;
			if(ch <= 0xFB4F) return R;	//N = 38
			if(ch <= 0xFD3D) return AL;	//N = 494
			if(ch <= 0xFD3F) return ON;	//N = 2
			if(ch <= 0xFDFC) return AL;	//N = 173
			if(0xFDFD == ch) return ON;
			if(ch <= 0xFE0F) return NSM;	//N = 16
			if(ch <= 0xFE19) return ON;	//N = 10
			if(ch <= 0xFE26) return NSM;	//N = 7
			if(ch <= 0xFE4F) return ON;	//N = 32
			if(ch <= 0xFE52) return (ch & 1 ? ON : CS);
			if(0xFE55 == ch) return CS;
			if(0xFE5F == ch) return ET;
			if(ch <= 0xFE61) return ON;	//N = 2
			if(ch <= 0xFE63) return ES;	//N = 2
			if(ch <= 0xFE68) return ON;	//N = 5
			if(ch <= 0xFE6A) return ET;	//N = 2
			if(0xFE6B == ch) return ON;
			if(ch <= 0xFEFC) return AL;	//N = 141
			if(0xFEFF == ch) return BN;
			if(ch <= 0xFF02) return ON;	//N = 2
			if(ch <= 0xFF05) return ET;	//N = 3
			if(ch <= 0xFF0A) return ON;	//N = 5
			if(ch <= 0xFF0D) return (ch & 1 ? ES : CS);
			if(ch <= 0xFF0F) return CS;	//N = 2
			if(ch <= 0xFF19) return EN;	//N = 10
			if(0xFF1A == ch) return CS;
			if(ch <= 0xFF20) return ON;	//N = 6
			if(ch <= 0xFF3A) return L;	//N = 26
			if(ch <= 0xFF40) return ON;	//N = 6
			if(ch <= 0xFF5A) return L;	//N = 26
			if(ch <= 0xFF65) return ON;	//N = 11
			if(ch <= 0xFFDC) return L;	//N = 119
			if(ch <= 0xFFE1) return ET;	//N = 2
			if(ch <= 0xFFE4) return ON;	//N = 3
			if(ch <= 0xFFE6) return ET;	//N = 2
			if(ch <= 0xFFFD) return ON;	//N = 22

			return ON;
		}
	}

	//class unicode_bidi
		void unicode_bidi::linestr(const char_type* str, std::size_t len, std::vector<entity> & reordered)
		{
			levels_.clear();
			const char_type * const end = str + len;

			std::vector<remember> stack;
			
			remember cur = {0, directional_override_status::neutral};
			cur.level = _m_paragraph_level(str, end);

			//First character type
			bidi_char begin_char_type;
			const char_type * begin_character = nullptr;

			for(const char_type * c = str; c < end; ++c)
			{
				if (PDF == *c)
				{
					if (cur.level)
					{
						if (begin_character)
						{
							_m_push_entity(begin_character, c, cur.level, begin_char_type);
							begin_character = nullptr;
						}
						cur = stack.back();
						stack.pop_back();
					}
					continue;
				}
				else if (LRE <= *c && *c <= RLO)
				{
					stack.push_back(cur);
					if (begin_character)
					{
						_m_push_entity(begin_character, c, cur.level, begin_char_type);
						begin_character = nullptr;
					}

					switch (*c)
					{
					case LRE:
						cur.directional_override = directional_override_status::neutral;
						cur.level += 2 - (cur.level & 1);
						break;
					case RLE:
						cur.directional_override = directional_override_status::neutral;
						cur.level += (cur.level & 1) + 1;
						break;
					case LRO:
						cur.directional_override = directional_override_status::left_to_right;
						cur.level += 2 - (cur.level & 1);
						break;
					case RLO:
						cur.directional_override = directional_override_status::right_to_left;
						cur.level = (cur.level & 1) + 1;
						break;
					}
					continue;
				}

				bidi_char type = _m_char_dir(*c);
				if (nullptr == begin_character)
				{
					begin_character = c;
					begin_char_type = type;
				}
				else if (begin_char_type != type)
				{
					_m_push_entity(begin_character, c, cur.level, begin_char_type);
					begin_char_type = type;
					begin_character = c;
				}
			}
			if(begin_character)
				_m_push_entity(begin_character, end, cur.level, begin_char_type);

			_m_resolve_weak_types();
			_m_resolve_neutral_types();
			_m_resolve_implicit_levels();
			_m_reordering_resolved_levels(str, reordered);
		}

		unsigned unicode_bidi::_m_paragraph_level(const char_type * begin, const char_type * end)
		{
			for(const char_type* i = begin; i != end; ++i)
			{
				switch(_m_char_dir(*i))
				{
				case bidi_char::AL:
				case bidi_char::R:
					return 1;
				case bidi_char::L:
					return 0;
				default:
					break;
				}
			}
			return 0;
		}

		void unicode_bidi::_m_push_entity(const char_type * begin, const char_type *end, unsigned level, bidi_char bidi_char_type)
		{
			entity e;
			e.begin = begin;
			e.end = end;
			e.level = level;
			e.bidi_char_type = bidi_char_type;
			levels_.push_back(e);
		}

		std::vector<unicode_bidi::entity>::iterator unicode_bidi::_m_search_first_character()
		{
			return levels_.begin();
		}

		auto unicode_bidi::_m_eor(std::vector<entity>::iterator i) ->bidi_char
		{
			const auto end = levels_.end();
			unsigned level = i->level;
			for (; i != end; ++i)
			{
				if (i->level != level)
					return i->bidi_char_type;
			}

			i = _m_search_first_character();
			if(i != end)
				return (i->level & 1 ? bidi_char::R : bidi_char::L);

			return bidi_char::L;
		}

		void unicode_bidi::_m_resolve_weak_types()
		{
			const auto end = levels_.end();
			auto begin_character = _m_search_first_character();
			if(begin_character == end)
				return;

			unsigned level_of_run = begin_character->level;
			const auto last = end - 1;

			//W1. Examine each nonspacing mark, and change the type of the NSM to the type of the previous
			//character.
			//W2. Search backward from each instance of a European number until the first strong type(R, L, AL, or sor) is found
			//If an AL is found, change the type of the European Number to arbic number.
			//W3. Change all ALs to R.

			//The three phases could be combined as one process. Because these phases are standalone.
			//Generally, the directional char type of sor is taken on current level.
			bidi_char prev = (level_of_run & 1 ? bidi_char::R : bidi_char::L); //sor is either L or R
			bool change_european_number = false;

			for(auto i = begin_character; i != end; ++i)
			{
				if(level_of_run != i->level)
				{
					level_of_run = i->level;
					prev = (level_of_run & 1 ? bidi_char::R : bidi_char::L);	//sor is either L or R
				}

				switch(i->bidi_char_type)
				{
				case bidi_char::NSM:
					i->bidi_char_type = prev;
					break;
				case bidi_char::EN:
					if(change_european_number)
						i->bidi_char_type = bidi_char::AN;
					break;
				case bidi_char::AL:
					change_european_number = true;
					i->bidi_char_type = bidi_char::R;
					break;
				case bidi_char::L:
				case bidi_char::R:
					if(change_european_number)
						change_european_number = false;
					break;
				default:	break;
				}

				prev = i->bidi_char_type;
			}


			//W4. A single european separator between two european numbers changes to a european number.
			//A single common separator between two numbers of the same type changes to that type.
			//
			//W5. A sequence of European terminators adjacent to European numbers changes to all European numbers.
			//W6. Otherwise, separators and terminators change to Other Neutral.
			auto etpos = end; //Indicates the head of the sequence of European Terminators
			bool head = true;
			for(auto i = begin_character; i != end; ++i)
			{
				switch(i->bidi_char_type)
				{
				case bidi_char::ES:
					i->bidi_char_type = bidi_char::ON;	//W6
					if((false == head) && (i != last))
					{
						if((bidi_char::EN == (i - 1)->bidi_char_type) && (bidi_char::EN == (i + 1)->bidi_char_type))
							i->bidi_char_type = bidi_char::EN;
					}
					else
						head = false;
					break;
				case bidi_char::CS:
					i->bidi_char_type = bidi_char::ON;	//W6
					if((false == head) && (i != last))
					{
						auto right_one = (i + 1)->bidi_char_type;
						if(((i-1)->bidi_char_type == right_one) && (bidi_char::AN == right_one || bidi_char::EN == right_one))
							i->bidi_char_type = right_one;
					}
					else
						head = false;
					break;
				case bidi_char::ET:
					if((false == head) && (bidi_char::EN == (i-1)->bidi_char_type))
					{
						i->bidi_char_type = bidi_char::EN;
					}
					else
					{
						if(etpos == end)
							etpos = i;
					}
					break;
				case bidi_char::EN:
					if(end != etpos)
					{
						for(; etpos != i; ++etpos)
							etpos->bidi_char_type = bidi_char::EN;

						etpos = end;
					}
					break;
				default:
					break;
				}

				//W6.
				if((end != etpos) && (bidi_char::ET != i->bidi_char_type))
				{
					for(; etpos != i; ++etpos)
						etpos->bidi_char_type = bidi_char::ON;
					etpos = end;
				}
			}

			//The final check etpos out
			for(; etpos != end; ++etpos)
				etpos->bidi_char_type = bidi_char::ON;
			
			//W7. Search backward from each instance of a European number until the first strong type (R, L, or sor) is found.
			//If an L is found, then change the type of the European number to L.

			level_of_run = begin_character->level;
			bidi_char sor = (level_of_run & 1 ? bidi_char::R : bidi_char::L);
			change_european_number = (sor == bidi_char::L);
			for(auto i = begin_character; i != end; ++i)
			{
				if(level_of_run != i->level)
				{
					level_of_run = i->level;
					sor = (level_of_run & 1 ? bidi_char::R : bidi_char::L);
					change_european_number = (sor == bidi_char::L);
				}

				switch(i->bidi_char_type)
				{
				case bidi_char::EN:
					if(change_european_number)
						i->bidi_char_type = bidi_char::L;
					break;
				case bidi_char::L:
					change_european_number = true;
					break;
				case bidi_char::R:
					change_european_number = false;
					break;
				default:
					break;
				}
			}
		}

		void unicode_bidi::_m_resolve_neutral_types()
		{
			const auto end = levels_.end();
			auto begin_character = _m_search_first_character();
			if(begin_character == end)
				return;

			unsigned level_of_run = begin_character->level;
			bool head_of_run = true;
			auto begin_neutral = end;


			//N1. A sequence of neutrals takes the direction of the surrounding strong text if the text on both sides has the same direction.
			//European and Arabic numbers act as if they were R in terms of their influence on neutrals.
			//Start-of-level-run (sor) and end-of-level-run (eor) are used at level run boundaries.
			bidi_char left;
			for(auto i = begin_character; i != end; ++i)
			{
				if(level_of_run != i->level)
				{
					level_of_run = i->level;
					head_of_run = true;
				}

				if(_m_bidi_category(i->bidi_char_type) == bidi_category::neutral)
				{
					if(begin_neutral == end)
					{
						begin_neutral = i;
						left = (head_of_run ? (level_of_run & 1 ? bidi_char::R : bidi_char::L) : (i-1)->bidi_char_type);	//Head ? sor : prev_char
						if(bidi_char::EN == left || bidi_char::AN == left)
							left = bidi_char::R;
					}
				}
				else if(begin_neutral != end)	//This element is not a neutral
				{
					bidi_char right;
					//Check if i is the end of the level run.

					if(i->level != begin_neutral->level)
						right = _m_eor(begin_neutral);
					else
						right = i->bidi_char_type;

					if(bidi_char::EN == right || bidi_char::AN == right)
						right = bidi_char::R;

					bidi_char target = (left == right ? left : (begin_neutral->level & 1 ? bidi_char::R : bidi_char::L));

					for(auto n = begin_neutral; n != i; ++n)
						n->bidi_char_type = target;

					begin_neutral = end;
				}

				head_of_run = false;
			}

			if(begin_neutral != end)
			{
				bidi_char eor = begin_character->level & 1 ? bidi_char::R : bidi_char::L;
				bidi_char target = (left == eor ? left : (begin_neutral->level & 1 ? bidi_char::R : bidi_char::L));
				for(auto i = begin_neutral; i != end; ++i)
					i->bidi_char_type = target;
			}
		}

		void unicode_bidi::_m_resolve_implicit_levels()
		{
			//I1. For all characters with an even (left-to-right) embedding direction, those of type R go up one level and those of type AN or EN go up two levels.
			//I2. For all characters with an odd (right-to-left) embedding direction, those of type L, EN or AN go up one level.
			for(auto & i : levels_)
			{
				switch(i.bidi_char_type)
				{
				case bidi_char::L:
					i.level += (i.level & 1);
					break;
				case bidi_char::R:
					if(0 == (i.level & 1))
						++(i.level);
					break;
				case bidi_char::EN:
				case bidi_char::AN:
					i.level += 2 - (i.level & 1);
					break;
				default:
					break;
				}
			}
		}

		void unicode_bidi::_m_reordering_resolved_levels(const char_type * str, std::vector<entity> & reordered)
		{
			reordered = levels_;

			//First find the highest_level for resolution, because the resolution is from highest level.
			unsigned highest_level = 0;
			for(auto & i : levels_)
			{
				if(i.level > highest_level)
					highest_level = i.level;
			}

			for(unsigned level = highest_level; level >= 1; --level)
			{
				for(auto i = levels_.begin(); i != levels_.end(); ++i)
				{
					if(i->level >= level)
					{
						auto beg = i, end = i + 1;
						while(end != levels_.end() && (end->level >= level))
							++end;

						//Reverse this run.
						std::size_t p = beg - levels_.begin();
						std::size_t plast = (end - levels_.begin() - 1);
						for(; p < plast; ++p, --plast)
							std::swap(reordered[p], reordered[plast]);

						i = end - 1;
					}
				}
			}
		}

		unicode_bidi::bidi_category unicode_bidi::_m_bidi_category(bidi_char bidi_char_type)
		{
			return static_cast<unicode_bidi::bidi_category>(static_cast<int>(bidi_char_type) & 0xF000);
		}
		
		unicode_bidi::bidi_char unicode_bidi::_m_char_dir(char_type ch)
		{
			auto type = bidi_charmap::bidi_char_type(ch);
			if (type < bidi_charmap::PDF)
				return static_cast<bidi_char>(type);
			if (type < bidi_charmap::B)
				return static_cast<bidi_char>(static_cast<int>(type - bidi_charmap::PDF) + 0x1000);

			return static_cast<bidi_char>(static_cast<int>(type - bidi_charmap::B) + 0x2000);
		}
	//end class unicode_bidi
}//end namespace nana
