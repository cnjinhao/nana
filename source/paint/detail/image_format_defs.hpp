#ifndef NANA_PAINT_DETAIL_IMAGE_FORMAE_DEFS_INCLUDED
#define NANA_PAINT_DETAIL_IMAGE_FORMAE_DEFS_INCLUDED

namespace nana
{
	namespace paint
	{
		namespace detail
		{
#ifndef NANA_WINDOWS
			struct bitmap_file_header
			{
				unsigned short bfType;
				unsigned bfSize;
				unsigned short bfReserved1;
				unsigned short bfReserved2;
				unsigned bfOffBits;
			} __attribute__((packed));

			struct bitmap_core_header
			{
				unsigned biSize;
				unsigned short  biWidth;
				unsigned short  biHeight;
				unsigned short  biPlanes;
				unsigned short  biBitCount;
			} __attribute__((packed));

			struct bitmap_info_header {
				unsigned biSize;
				int  biWidth;
				int  biHeight;
				unsigned short  biPlanes;
				unsigned short  biBitCount;
				unsigned		biCompression;
				unsigned		biSizeImage;
				int  biXPelsPerMeter;
				int  biYPelsPerMeter;
				unsigned	biClrUsed;
				unsigned	biClrImportant;
			}__attribute__((packed));

			struct rgb_quad
			{
				unsigned char rgbBlue;
				unsigned char rgbGreen;
				unsigned char rgbRed;
				unsigned char rgbReserved;
			};
#else
			typedef BITMAPFILEHEADER	bitmap_file_header;
			typedef BITMAPCOREHEADER	bitmap_core_header;
			typedef BITMAPINFOHEADER	bitmap_info_header;
			typedef RGBQUAD		rgb_quad;
#endif

#pragma pack(push, 1)
			struct bitmap_info_v3_header
			{
				bitmap_info_header head;
				unsigned long mask_red;
				unsigned long mask_green;
				unsigned long mask_blue;
				unsigned long mask_alpha;
			};
#pragma pack(pop)
		}
	}
}
#endif