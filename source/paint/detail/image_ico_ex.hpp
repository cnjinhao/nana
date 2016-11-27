#ifndef NANA_PAINT_DETAIL_IMAGE_ICO_EX_HPP
#define NANA_PAINT_DETAIL_IMAGE_ICO_EX_HPP

#include "image_pixbuf.hpp"

namespace nana {
namespace paint {
namespace detail {
// These next two structs represent how the icon information is stored
// in an ICO file.
typedef struct
{
	uint8_t   bWidth;                // Width of the image
	uint8_t   bHeight;               // Height of the image (times 2)
	uint8_t   bColorCount;           // Number of colors in image (0 if >=8bpp)
	uint8_t   bReserved;             // Reserved
	uint16_t  wPlanes;               // Color Planes
	uint16_t  wBitCount;             // Bits per pixel
	uint32_t  dwBytesInRes;          // how many bytes in this resource?
	uint32_t  dwImageOffset;         // where in the file is this image
} ICONDIRENTRY, *LPICONDIRENTRY;

typedef struct
{
	uint16_t  idReserved;            // Reserved
	uint16_t  idType;                // resource type (1 for icons)
	uint16_t  idCount;               // how many images?
	//ICONDIRENTRY  idEntries[1];    // the entries for each image
} ICONDIR, *LPICONDIR;

// size - 40 bytes
typedef struct
{
	uint32_t  biSize;
	uint32_t  biWidth;
	uint32_t  biHeight;   // Icon Height (added height of XOR-Bitmap and AND-Bitmap)
	uint16_t  biPlanes;
	uint16_t  biBitCount;
	uint32_t  biCompression;
	int32_t   biSizeImage;
	uint32_t  biXPelsPerMeter;
	uint32_t  biYPelsPerMeter;
	uint32_t  biClrUsed;
	uint32_t  biClrImportant;
} s_BITMAPINFOHEADER, *s_PBITMAPINFOHEADER;

// 46 bytes
typedef struct
{
	s_BITMAPINFOHEADER    icHeader;       // DIB header
	uint32_t              icColors[1];    // Color table (short 4 bytes) //RGBQUAD
	uint8_t               icXOR[1];       // DIB bits for XOR mask
	uint8_t               icAND[1];       // DIB bits for AND mask
} ICONIMAGE, *LPICONIMAGE;


class image_ico_ex
	: public basic_image_pixbuf
{
	bool _m_read_ico(const void* data, std::size_t /*size*/)
	{
		auto width = 0;
		auto height = 0;
		auto buffer = (unsigned char *)data;
		auto icoDir = reinterpret_cast<LPICONDIR>(buffer);
		int iconsCount = icoDir->idCount;
		if (icoDir->idReserved != 0 || icoDir->idType != 1 || iconsCount == 0 || iconsCount > 20)
			return false;

		auto cursor = buffer;
		cursor += 6;
		auto dirEntry = reinterpret_cast<ICONDIRENTRY*>(cursor);
		auto maxSize = 0;
		auto offset = 0;
		auto maxBitCount = 0;
		for (auto i = 0; i < iconsCount; i++, ++dirEntry)
		{
			int w = dirEntry->bWidth;
			int h = dirEntry->bHeight;
			int bitCount = dirEntry->wBitCount;
			if (w * h > maxSize || bitCount > maxBitCount) // we choose icon with max resolution
			{
				width = w;
				height = h;
				offset = dirEntry->dwImageOffset;
				maxSize = w * h;
				maxBitCount = bitCount;
			}
		}

		if (offset == 0) return false;

		cursor = buffer;
		cursor += offset;
		auto icon = reinterpret_cast<ICONIMAGE*>(cursor);
		auto realBitsCount = static_cast<int>(icon->icHeader.biBitCount);
		auto hasAndMask = (realBitsCount < 32) && (height != static_cast<int>(icon->icHeader.biHeight));
		cursor += 40;
		pixbuf_.open(width, height);

		// rgba + vertical swap
		if (realBitsCount >= 32)
		{
			for (auto x = 0; x < width; ++x)
				for (auto y = 0; y < height; ++y)
				{
					pixbuf_.alpha_channel(true);
					auto shift2 = 4 * (x + (height - y - 1) * width);
					pixel_color_t image;
					image.element.red = cursor[shift2 + 2];
					image.element.green = cursor[shift2 + 1];
					image.element.blue = cursor[shift2];
					image.element.alpha_channel = cursor[shift2 + 3];
					pixbuf_.pixel(x, y, image);
				}
		}

		else if (realBitsCount == 24)
		{
			for (auto x = 0; x < width; x++)
				for (auto y = 0; y < height; y++)
				{
					pixbuf_.alpha_channel(true);
					auto shift2 = 3 * (x + (height - y - 1) * width);
					pixel_color_t image;
					image.element.red = cursor[shift2 + 2];
					image.element.green = cursor[shift2 + 1];
					image.element.blue = cursor[shift2];
					image.element.alpha_channel = 255;
					pixbuf_.pixel(x, y, image);
				}
		}

		else if (realBitsCount == 8)  /// 256 colors
		{
			// 256 color table
			auto colors = reinterpret_cast<unsigned char *>(cursor);
			cursor += 256 * 4;
			for (auto x = 0; x < width; x++)
				for (auto y = 0; y < height; y++)
				{
					pixbuf_.alpha_channel(true);

					auto shift2 = (x + (height - y - 1) * width);
					auto index = 4 * cursor[shift2];
					pixel_color_t image;
					image.element.red = colors[index + 2];
					image.element.green = colors[index + 1];
					image.element.blue = colors[index];
					image.element.alpha_channel = 255;
					pixbuf_.pixel(x, y, image);
				}
		}

		else if (realBitsCount == 4)  /// 16 colors
		{
			// 16 color table
			auto colors = reinterpret_cast<unsigned char *>(cursor);
			cursor += 16 * 4;
			for (auto x = 0; x < width; x++)
				for (auto y = 0; y < height; y++)
				{
					auto shift2 = (x + (height - y - 1) * width);
					auto index = cursor[shift2 / 2];
					if (shift2 % 2 == 0)
						index = (index >> 4) & 0xF;
					else
						index = index & 0xF;
					index *= 4;
					pixbuf_.alpha_channel(true);
					pixel_color_t image;
					image.element.red = colors[index + 2];
					image.element.green = colors[index + 1];
					image.element.blue = colors[index];
					image.element.alpha_channel = 255;
					pixbuf_.pixel(x, y, image);
				}
		}

		else if (realBitsCount == 1)  /// 2 colors
		{
			// 2 color table
			auto colors = reinterpret_cast<unsigned char *>(cursor);
			cursor += 2 * 4;
			auto boundary = width; //!!! 32 bit boundary (http://www.daubnet.com/en/file-format-ico)
			while (boundary % 32 != 0) boundary++;
			for (auto x = 0; x < width; x++)
				for (auto y = 0; y < height; y++)
				{
					auto shift2 = (x + (height - y - 1) * boundary);
					auto index = cursor[shift2 / 8];

					// select 1 bit only
					unsigned char bit = 7 - (x % 8);
					index = (index >> bit) & 0x01;
					index *= 4;
					pixbuf_.alpha_channel(true);
					pixel_color_t image;
					image.element.red = colors[index + 2];
					image.element.green = colors[index + 1];
					image.element.blue = colors[index];
					image.element.alpha_channel = 255;
					pixbuf_.pixel(x, y, image);
				}
		}

		// Read AND mask after base color data - 1 BIT MASK
		if (hasAndMask)
		{
			auto boundary = width * realBitsCount; //!!! 32 bit boundary (http://www.daubnet.com/en/file-format-ico)
			while (boundary % 32 != 0) boundary++;
			cursor += boundary * height / 8;
			boundary = width;
			while (boundary % 32 != 0) boundary++;
			for (auto y = 0; y < height; y++)
				for (auto x = 0; x < width; x++)
				{
					unsigned char bit = 7 - (x % 8);
					auto shift2 = (x + (height - y - 1) * boundary) / 8;
					auto mask = (0x01 & (static_cast<unsigned char>(cursor[shift2]) >> bit));
					auto pc = pixbuf_.pixel(x, y);
					auto alpha = pc.element.alpha_channel;
					alpha *= 1 - mask;
					pc.element.alpha_channel = alpha;
					pixbuf_.pixel(x, y, pc);
				}
		}
		return true;
	}
public:

	bool open(const std::experimental::filesystem::path& ico_file) override
	{
		std::ifstream file(ico_file.string(), std::ios::binary);
		if (!file.is_open()) return false;

		// allocates a buffer for the image
		file.seekg(0, std::ios::end);
		const auto bytes = static_cast<std::size_t>(file.tellg());
		file.seekg(0, std::ios::beg);
		auto buffer = new char[bytes];

		// read data from the file and set them in the buffer
		file.read(buffer, bytes);
		auto okret = _m_read_ico(buffer, bytes);

		// delete buffer and return
		delete[] buffer;
		return okret;
	}

	bool open(const void* data, std::size_t bytes) override
	{
		return _m_read_ico(data, bytes);
	}

};
}//end namespace detail
}//end namespace paint
}//end namespace nana

#endif
