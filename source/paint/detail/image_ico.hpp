#ifndef NANA_PAINT_DETAIL_IMAGE_ICO_HPP
#define NANA_PAINT_DETAIL_IMAGE_ICO_HPP

#include "image_pixbuf.hpp"
#include <fstream>

#if defined(NANA_WINDOWS)
#	include <windows.h>
#endif

namespace nana {
namespace paint {
namespace detail {
// These next two structs represent how the icon information is stored
// in an ICO file.
typedef struct
{
	std::uint8_t   bWidth;                // Width of the image
	std::uint8_t   bHeight;               // Height of the image (times 2)
	std::uint8_t   bColorCount;           // Number of colors in image (0 if >=8bpp)
	std::uint8_t   bReserved;             // Reserved
	std::uint16_t  wPlanes;               // Color Planes
	std::uint16_t  wBitCount;             // Bits per pixel
	std::uint32_t  dwBytesInRes;          // how many bytes in this resource?
	std::uint32_t  dwImageOffset;         // where in the file is this image
} ICONDIRENTRY, *LPICONDIRENTRY;

typedef struct
{
	std::uint16_t  idReserved;            // Reserved
	std::uint16_t  idType;                // resource type (1 for icons)
	std::uint16_t  idCount;               // how many images?
	//ICONDIRENTRY  idEntries[1];    // the entries for each image
} ICONDIR, *LPICONDIR;

// size - 40 bytes
typedef struct
{
	std::uint32_t  biSize;
	std::uint32_t  biWidth;
	std::uint32_t  biHeight;   // Icon Height (added height of XOR-Bitmap and AND-Bitmap)
	std::uint16_t  biPlanes;
	std::uint16_t  biBitCount;
	std::uint32_t  biCompression;
	std::int32_t   biSizeImage;
	std::uint32_t  biXPelsPerMeter;
	std::uint32_t  biYPelsPerMeter;
	std::uint32_t  biClrUsed;
	std::uint32_t  biClrImportant;
} s_BITMAPINFOHEADER, *s_PBITMAPINFOHEADER;

// 46 bytes
typedef struct
{
	s_BITMAPINFOHEADER    icHeader;       // DIB header
	std::uint32_t              icColors[1];    // Color table (short 4 bytes) //RGBQUAD
	std::uint8_t               icXOR[1];       // DIB bits for XOR mask
	std::uint8_t               icAND[1];       // DIB bits for AND mask
} ICONIMAGE, *LPICONIMAGE;


class image_ico
	: public basic_image_pixbuf
{
	bool _m_read_ico(const void* data, std::size_t /*size*/)
	{
		auto width = 0;
		auto height = 0;
		auto buffer = (std::uint8_t *)data;
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
	~image_ico()
	{
#if defined(NANA_WINDOWS)
		if (native_handle_)
			::DestroyIcon(reinterpret_cast<HICON>(native_handle_));
#endif
	}

	bool open(const std::filesystem::path& ico_file) override
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

		if (okret)
			path_ = ico_file;

		return okret;
	}

	bool open(const void* data, std::size_t bytes) override
	{
		if (_m_read_ico(data, bytes))
		{
#if defined(NANA_WINDOWS)
			native_handle_ = ::CreateIconFromResourceEx(reinterpret_cast<PBYTE>(const_cast<void*>(data)), static_cast<DWORD>(bytes), TRUE, 0x00030000, 0, 0, LR_DEFAULTCOLOR);
#endif
			return true;
		}
		return false;
	}

	void* native_handle()
	{
#if defined(NANA_WINDOWS)
		if (native_handle_)
			return native_handle_;

		native_handle_ = ::LoadImage(nullptr, path_.c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
		return native_handle_;
#else
		return nullptr;
#endif
	}
private:
	std::filesystem::path path_;
#if defined(NANA_WINDOWS)
	void* native_handle_{nullptr};
#endif
};
}//end namespace detail
}//end namespace paint
}//end namespace nana

#endif
