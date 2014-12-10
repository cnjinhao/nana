/*
 *	Pixel Buffer Implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/paint/pixel_buffer.cpp
 *	@note: The format of Xorg 16bits depth is 565
 */

#include <nana/config.hpp>
#include PLATFORM_SPEC_HPP
#include <nana/paint/pixel_buffer.hpp>
#include <nana/gui/layout_utility.hpp>
#include <nana/paint/detail/native_paint_interface.hpp>
#include <nana/paint/detail/image_process_provider.hpp>

#include <stdexcept>

namespace nana{	namespace paint
{

	nana::rectangle valid_rectangle(const nana::size& s, const nana::rectangle& r)
	{
		nana::rectangle good_r;
		nana::overlap(s, r, good_r);
		return good_r;
	}

	struct pixel_buffer::pixel_buffer_storage
		: private nana::noncopyable
	{
	public:
		const drawable_type drawable; //Attached handle
		const nana::rectangle valid_r;
		const nana::size pixel_size;
		pixel_rgb_t * raw_pixel_buffer;
		const std::size_t bytes_per_line;
		bool	alpha_channel;
#if defined(NANA_X11)
		struct x11_members
		{
			bool attached;
			XImage * image;
		}x11;
#endif

		struct image_processor_tag
		{
			paint::image_process::stretch_interface * stretch_receptacle;
			paint::image_process::stretch_interface * const * stretch;
			paint::image_process::alpha_blend_interface * alpha_blend_receptacle;
			paint::image_process::alpha_blend_interface * const * alpha_blend;
			paint::image_process::blend_interface * blend_receptacle;
			paint::image_process::blend_interface * const * blend;
			paint::image_process::line_interface * line_receptacle;
			paint::image_process::line_interface * const * line;
			paint::image_process::blur_interface * blur_receptacle;
			paint::image_process::blur_interface * const * blur;

			image_processor_tag()
				:	stretch_receptacle(nullptr),
					alpha_blend_receptacle(nullptr),
					blend_receptacle(nullptr),
					line_receptacle(nullptr),
					blur_receptacle(nullptr)
			{
				detail::image_process_provider & provider = detail::image_process_provider::instance();
				stretch = provider.stretch();
				alpha_blend = provider.alpha_blend();
				blend = provider.blend();
				line = provider.line();
				blur = provider.blur();
			}
		}img_pro;

		pixel_buffer_storage(std::size_t width, std::size_t height)
			:	drawable(nullptr),
				valid_r(0, 0, static_cast<unsigned>(width), static_cast<unsigned>(height)),
				pixel_size(static_cast<unsigned>(width), static_cast<unsigned>(height)),
				raw_pixel_buffer(new pixel_rgb_t[width * height]),
				bytes_per_line(width * sizeof(pixel_rgb_t)),
				alpha_channel(false)
		{
#if defined(NANA_X11)
			nana::detail::platform_spec & spec = nana::detail::platform_spec::instance();
			x11.image = ::XCreateImage(spec.open_display(), spec.screen_visual(), 32, ZPixmap, 0, reinterpret_cast<char*>(raw_pixel_buffer), width, height, 32, 0);
			x11.attached = false;
			if(nullptr == x11.image)
			{
				delete [] raw_pixel_buffer;
				throw std::runtime_error("Nana.pixel_buffer: XCreateImage failed");
			}
			
			if(static_cast<int>(bytes_per_line) != x11.image->bytes_per_line)
			{
				x11.image->data = nullptr;
				XDestroyImage(x11.image);
				delete [] raw_pixel_buffer;
				throw std::runtime_error("Nana.pixel_buffer: Invalid pixel buffer context.");
			}
#endif
		}

		pixel_buffer_storage(drawable_type drawable, const nana::rectangle& want_r)
			:	drawable(drawable),
				valid_r(valid_rectangle(paint::detail::drawable_size(drawable), want_r)),
				pixel_size(valid_r),
#if defined(NANA_WINDOWS)
				raw_pixel_buffer(reinterpret_cast<pixel_rgb_t*>(reinterpret_cast<char*>(drawable->pixbuf_ptr + valid_r.x) + drawable->bytes_per_line * valid_r.y)),
				bytes_per_line(drawable->bytes_per_line),
#else
				raw_pixel_buffer(nullptr),
				bytes_per_line(sizeof(pixel_rgb_t) * valid_r.width),
#endif
				alpha_channel(false)
		{
#if defined(NANA_X11)
			nana::detail::platform_spec & spec = nana::detail::platform_spec::instance();
			
			//Ensure that the pixmap is updated before we copy its content.
			::XFlush(spec.open_display());
			x11.image = ::XGetImage(spec.open_display(), drawable->pixmap, valid_r.x, valid_r.y, valid_r.width, valid_r.height, AllPlanes, ZPixmap);
			x11.attached = true;
			if(nullptr == x11.image)
				throw std::runtime_error("Nana.pixel_buffer: XGetImage failed");

			if(32 == x11.image->depth || (24 == x11.image->depth && 32 == x11.image->bitmap_pad))
			{
				if(static_cast<int>(bytes_per_line) != x11.image->bytes_per_line)
				{
					XDestroyImage(x11.image);
					throw std::runtime_error("Nana.pixel_buffer: Invalid pixel buffer context.");
				}
				raw_pixel_buffer = reinterpret_cast<pixel_rgb_t*>(x11.image->data);
			}
			else if(16 == x11.image->depth)
			{
				//565 to 32
				raw_pixel_buffer = new pixel_rgb_t[valid_r.width * valid_r.height];
				assign(reinterpret_cast<unsigned char*>(x11.image->data), valid_r.width, valid_r.height, 16, x11.image->bytes_per_line, false);
			}
			else
			{
				XDestroyImage(x11.image);
				throw std::runtime_error("Nana.pixel_buffer: The color depth is not supported");
			}
#endif
		}

		~pixel_buffer_storage()
		{
#if defined(NANA_X11)
			if(nullptr == drawable) //not attached
				x11.image->data = nullptr;	//the image data is allocated by pixel_buffer when it is not attached with a drawable
			else if(x11.attached)	//the image should be uploaded when it is attached.
				put(drawable->pixmap, drawable->context, 0, 0, valid_r.x, valid_r.y, valid_r.width, valid_r.height);

			if(x11.image->data != reinterpret_cast<char*>(raw_pixel_buffer))
				delete [] raw_pixel_buffer;
			
			XDestroyImage(x11.image);
#else
			if(nullptr == drawable)	//not attached
				delete [] raw_pixel_buffer;
#endif
		}

		void assign(const unsigned char* rawbits, std::size_t width, std::size_t height, std::size_t bits_per_pixel, std::size_t bytes_per_line, bool is_negative)
		{
			pixel_rgb_t * rawptr = raw_pixel_buffer;
			if(rawptr)
			{
				if(32 == bits_per_pixel)
				{
					if((pixel_size.width == width) && (pixel_size.height == height) && is_negative)
					{
						memcpy(rawptr, rawbits, (pixel_size.width * pixel_size.height) * 4);
					}
					else
					{
						std::size_t line_bytes = (pixel_size.width < width ? pixel_size.width : width) * sizeof(pixel_rgb_t);

						if(pixel_size.height < height)
							height = pixel_size.height;

						pixel_rgb_t * d = rawptr;
						if(is_negative)
						{
							const unsigned char* s = rawbits;
							for(std::size_t i = 0; i < height; ++i)
							{
								memcpy(d, s, line_bytes);
								d += pixel_size.width;
								s += bytes_per_line;
							}
						}
						else
						{
							const unsigned char* s = rawbits + bytes_per_line * (height - 1);
							for(std::size_t i = 0; i < height; ++i)
							{
								memcpy(d, s, line_bytes);
								d += pixel_size.width;
								s -= bytes_per_line;
							}
						}
					}
				}
				else if(24 == bits_per_pixel)
				{
					if(pixel_size.width < width)
						width = pixel_size.width;

					if(pixel_size.height < height)
						height = pixel_size.height;

					pixel_rgb_t * d = rawptr;
					if(is_negative)
					{
						const unsigned char* s = rawbits;
						for(std::size_t i = 0; i < height; ++i)
						{
							pixel_rgb_t * p = d;
							pixel_rgb_t * end = p + width;
							std::size_t s_index = 0;
							for(; p < end; ++p)
							{
								const unsigned char * s_p = s + s_index;
								p->u.element.blue = s_p[0];
								p->u.element.green = s_p[1];
								p->u.element.red = s_p[2];
								s_index += 3;
							}
							d += pixel_size.width;
							s += bytes_per_line;
						}
					}
					else
					{
						const unsigned char* s = rawbits + bytes_per_line * (height - 1);
						for(std::size_t i = 0; i < height; ++i)
						{
							pixel_rgb_t * p = d;
							pixel_rgb_t * end = p + width;
							const unsigned char* s_p = s;
							for(; p < end; ++p)
							{
								p->u.element.blue = s_p[0];
								p->u.element.green = s_p[1];
								p->u.element.red = s_p[2];
								s_p += 3;
							}
							d += pixel_size.width;
							s -= bytes_per_line;
						}
					}
				}
				else if(16 == bits_per_pixel)
				{
					if(pixel_size.width < width)
						width = pixel_size.width;

					if(pixel_size.height < height)
						height = pixel_size.height;

					pixel_rgb_t * d = rawptr;

					unsigned char * rgb_table = new unsigned char[32];

					for(std::size_t i =0; i < 32; ++i)
						rgb_table[i] = static_cast<unsigned char>(i * 255 / 31);

					if(is_negative)
					{
						//const unsigned short* s = reinterpret_cast<const unsigned short*>(rawbits);
						for(std::size_t i = 0; i < height; ++i)
						{
							pixel_rgb_t * p = d;
							const pixel_rgb_t * const end = p + width;
							const unsigned short* s_p = reinterpret_cast<const unsigned short*>(rawbits);
							for(; p < end; ++p)
							{
								p->u.element.red = rgb_table[(*s_p >> 11) & 0x1F];
#if defined(NANA_X11)
								p->u.element.green = (*s_p >> 5) & 0x3F;
								p->u.element.blue = rgb_table[*s_p & 0x1F];
#else
								p->u.element.green = rgb_table[(*s_p>> 6) & 0x1F];
								p->u.element.blue = rgb_table[(*s_p >> 1) & 0x1F];
#endif
								++s_p;
							}
							d += pixel_size.width;
							rawbits += bytes_per_line;
						}
					}
					else
					{
						//	const unsigned short* s = reinterpret_cast<const unsigned short*>(rawbits + bytes_per_line * (height - 1));
						rawbits += bytes_per_line * (height - 1);
						for(std::size_t i = 0; i < height; ++i)
						{
							pixel_rgb_t * p = d;
							const pixel_rgb_t * const end = p + width;
							const unsigned short* s_p = reinterpret_cast<const unsigned short*>(rawbits);
							for(; p < end; ++p)
							{
								p->u.element.red = rgb_table[(*s_p >> 11) & 0x1F];
#if defined(NANA_X11)
								p->u.element.green = ((*s_p >> 5) & 0x3F);
								p->u.element.blue = rgb_table[*s_p & 0x1F];
#else
								p->u.element.green = rgb_table[(*s_p & 0x7C0) >> 6];
								p->u.element.blue = rgb_table[(*s_p >> 1) & 0x1F];
#endif
								++s_p;
							}
							d += pixel_size.width;
							//s -= bytes_per_line;
							rawbits -= bytes_per_line;
						}
					}
					delete [] rgb_table;
				}
			}
		}

#if defined(NANA_X11)
		//The implementation of attach in X11 is same with non-attach's, because the image buffer of drawable can't be refered indirectly
		//so the pixel_buffer::open() method may call the attached version of pixel_buffer_storage construction.
		void detach()
		{
			x11.attached = false;
		}

		void put(Drawable dw, GC gc, int src_x, int src_y, int x, int y, unsigned width, unsigned height)
		{		
			auto & spec = nana::detail::platform_spec::instance();
			Display * disp = spec.open_display();
			const int depth = spec.screen_depth();

			XImage* img = ::XCreateImage(disp, spec.screen_visual(), depth, ZPixmap, 0, 0, pixel_size.width, pixel_size.height, (16 == depth ? 16 : 32), 0);
			if(sizeof(pixel_rgb_t) * 8 == depth || 24 == depth)
			{	
				img->data = reinterpret_cast<char*>(raw_pixel_buffer);
				::XPutImage(disp, dw, gc,
							img, src_x, src_y, x, y, width, height);
			}
			else if(16 == depth)
			{
				//The format of Xorg 16bits depth is 565
				std::unique_ptr<unsigned short[]> table_holder(new unsigned short[256]);
				unsigned short * const fast_table = table_holder.get();
				for(int i = 0; i < 256; ++i)
					fast_table[i] = i * 31 / 255;
				
				std::size_t length = width * height;

				std::unique_ptr<unsigned short[]> px_holder(new unsigned short[length]);
				unsigned short * pixbuf_16bits = px_holder.get();

				if(length == pixel_size.width * pixel_size.height)
				{
					for(auto i = raw_pixel_buffer, end = raw_pixel_buffer + length; i != end; ++i)
					{
						*(pixbuf_16bits++) = (fast_table[i->u.element.red] << 11) | ( (i->u.element.green * 63 / 255) << 6) | fast_table[i->u.element.blue];
					}
				}
				else if(height)
				{
					unsigned sp_line_len = pixel_size.width;
					auto sp = raw_pixel_buffer + (src_x + sp_line_len * src_y);
					
					unsigned top = 0;
					while(true)
					{
						for(auto i = sp, end = sp + width; i != end; ++i)
						{
							*(pixbuf_16bits++) = (fast_table[i->u.element.red] << 11) | ((i->u.element.green * 63 / 255) << 6) | fast_table[i->u.element.blue];
						}

						if(++top < height)
								sp += sp_line_len;
					}
				}

				img->data = reinterpret_cast<char*>(px_holder.get());
				::XPutImage(disp, dw, gc,
					img, src_x, src_y, x, y, width, height);				
			}
			img->data = nullptr;	//Set null pointer to avoid XDestroyImage destroyes the buffer.
			XDestroyImage(img);
		}
#endif
	};

	pixel_buffer::pixel_buffer()
	{}

	pixel_buffer::pixel_buffer(drawable_type drawable, const nana::rectangle& want_rectangle)
	{
		open(drawable, want_rectangle);
	}

	pixel_buffer::pixel_buffer(drawable_type drawable, std::size_t top, std::size_t lines)
	{
		open(drawable, nana::rectangle(0, static_cast<int>(top), 0, static_cast<unsigned>(lines)));
	}

	pixel_buffer::pixel_buffer(std::size_t width, std::size_t height)
	{
		open(width, height);
	}

	pixel_buffer::~pixel_buffer()
	{
		close();
	}

	void pixel_buffer::attach(drawable_type drawable, const nana::rectangle& want_r)
	{
		storage_.reset();
		if(drawable)
		{
			nana::rectangle r;
			if(nana::overlap(nana::paint::detail::drawable_size(drawable), want_r, r))
				storage_ = std::make_shared<pixel_buffer_storage>(drawable, r);
		}
	}

	bool pixel_buffer::open(drawable_type drawable)
	{
		nana::size sz = nana::paint::detail::drawable_size(drawable);
		if(sz.empty()) return false;

#if defined(NANA_WINDOWS)
		auto * sp = storage_.get();
		if((nullptr == sp) || (sp->pixel_size != sz) || sp->drawable/*attached*/)
		{
			storage_ = std::make_shared<pixel_buffer_storage>(sz.width, sz.height);
			sp = storage_.get();
		}

		BITMAPINFO bmpinfo;
		bmpinfo.bmiHeader.biSize = sizeof(bmpinfo.bmiHeader);
		bmpinfo.bmiHeader.biWidth = sz.width;
		bmpinfo.bmiHeader.biHeight = -static_cast<int>(sz.height);
		bmpinfo.bmiHeader.biPlanes = 1;
		bmpinfo.bmiHeader.biBitCount = 32;
		bmpinfo.bmiHeader.biCompression = BI_RGB;
		bmpinfo.bmiHeader.biSizeImage = static_cast<DWORD>(sz.width * sz.height * sizeof(pixel_rgb_t));
		bmpinfo.bmiHeader.biClrUsed = 0;
		bmpinfo.bmiHeader.biClrImportant = 0;

		std::size_t read_lines = ::GetDIBits(drawable->context, drawable->pixmap, 0, static_cast<UINT>(sz.height), sp->raw_pixel_buffer, &bmpinfo, DIB_RGB_COLORS);

		return (sz.height == read_lines);
#elif defined(NANA_X11)
		try
		{
			storage_ = std::make_shared<pixel_buffer_storage>(drawable, sz);
			storage_->detach();
			return true;
		}
		catch(...)
		{}
#endif
		return false;
	}

	bool pixel_buffer::open(drawable_type drawable, const nana::rectangle & want_rectangle)
	{
		nana::size sz = nana::paint::detail::drawable_size(drawable);
		if(want_rectangle.x >= static_cast<int>(sz.width) || want_rectangle.y >= static_cast<int>(sz.height))
			return false;

		nana::rectangle want_r = want_rectangle;
		if(want_r.width == 0) want_r.width = sz.width - want_r.x;
		if(want_r.height == 0) want_r.height = sz.height - want_r.y;

		nana::rectangle r;
		if(false == overlap(sz, want_r, r))
			return false;
#if defined(NANA_WINDOWS)
		BITMAPINFO bmpinfo;
		bmpinfo.bmiHeader.biSize = sizeof(bmpinfo.bmiHeader);
		bmpinfo.bmiHeader.biWidth = want_r.width;
		bmpinfo.bmiHeader.biHeight = -static_cast<int>(want_r.height);
		bmpinfo.bmiHeader.biPlanes = 1;
		bmpinfo.bmiHeader.biBitCount = 32;
		bmpinfo.bmiHeader.biCompression = BI_RGB;
		bmpinfo.bmiHeader.biSizeImage = static_cast<DWORD>(want_r.width * want_r.height * sizeof(pixel_rgb_t));
		bmpinfo.bmiHeader.biClrUsed = 0;
		bmpinfo.bmiHeader.biClrImportant = 0;

		bool need_dup = (r.width != sz.width || r.height != sz.height);

		HDC context = drawable->context;
		HBITMAP pixmap = drawable->pixmap;
		HBITMAP orig_bmp;
		if(need_dup)
		{
			context = ::CreateCompatibleDC(drawable->context);
			pixmap = ::CreateCompatibleBitmap(drawable->context, static_cast<int>(want_r.width), static_cast<int>(want_r.height));
			orig_bmp = reinterpret_cast<HBITMAP>(::SelectObject(context, pixmap));
			::BitBlt(context, r.x - want_r.x, r.y - want_r.y, r.width, r.height, drawable->context, r.x, r.y, SRCCOPY);
		}

		storage_ = std::make_shared<pixel_buffer_storage>(want_r.width, want_r.height);
		std::size_t read_lines = ::GetDIBits(context, pixmap, 0, static_cast<UINT>(want_r.height), storage_->raw_pixel_buffer, &bmpinfo, DIB_RGB_COLORS);

		if(need_dup)
		{
			::SelectObject(context, orig_bmp);
			::DeleteObject(pixmap);
			::DeleteDC(context);
		}

		return (want_r.height == read_lines);
#elif defined(NANA_X11)
		nana::detail::platform_spec & spec = nana::detail::platform_spec::instance();
		Window root;
		int x, y;
		unsigned width, height;
		unsigned border, depth;
		nana::detail::platform_scope_guard psg;
		::XFlush(spec.open_display());
		::XGetGeometry(spec.open_display(), drawable->pixmap, &root, &x, &y, &width, &height, &border, &depth);
		XImage * image = ::XGetImage(spec.open_display(), drawable->pixmap, r.x, r.y, r.width, r.height, AllPlanes, ZPixmap);

		storage_ = std::make_shared<pixel_buffer_storage>(want_r.width, want_r.height);
		pixel_rgb_t * pixbuf = storage_->raw_pixel_buffer;
		if(image->depth == 32 || (image->depth == 24 && image->bitmap_pad == 32))
		{
			if(want_r.width != static_cast<unsigned>(image->width) || want_r.height != static_cast<unsigned>(image->height))
			{
				pixbuf += (r.x - want_r.x);
				pixbuf += (r.y - want_r.y) * want_r.width;
				const char* img_data = image->data;
				for(int i = 0; i < image->height; ++i)
				{
					memcpy(pixbuf, img_data, image->bytes_per_line);
					img_data += image->bytes_per_line;
					pixbuf += want_r.width;
				}
			}
			else
				memcpy(pixbuf, image->data, image->bytes_per_line * image->height);
		}
		else if(16 == image->depth)
		{
			//The format of Xorg 16bits depth is 565
			std::unique_ptr<unsigned[]> table_holder(new unsigned[32]);
			unsigned * const fast_table = table_holder.get();
			for(unsigned i = 0; i < 32; ++i)
				fast_table[i] = (i * 255 / 31);

			pixbuf += (r.x - want_r.x);
			pixbuf += (r.y - want_r.y) * want_r.width;
			const char* img_data = image->data;
			for(int i = 0; i < image->height; ++i)
			{
				const unsigned short * const px_data = reinterpret_cast<const unsigned short*>(img_data);

				for(int x = 0; x < image->width; ++x)
				{
					pixbuf[x].u.element.red		= fast_table[(px_data[x] >> 11) & 0x1F];
					pixbuf[x].u.element.green	= (px_data[x] >> 5) & 0x3F;
					pixbuf[x].u.element.blue	= fast_table[px_data[x] & 0x1F];
					pixbuf[x].u.element.alpha_channel = 0;
				}
				img_data += image->bytes_per_line;
				pixbuf += want_r.width;
			}			
		}
		else
		{
			XDestroyImage(image);
			return false;
		}

		XDestroyImage(image);
#endif
        return true;
	}

	bool pixel_buffer::open(std::size_t width, std::size_t height)
	{
		if(width && height)
		{
			storage_ = std::make_shared<pixel_buffer_storage>(width, height);
			return true;
		}
		return false;
	}

	void pixel_buffer::alpha_channel(bool enabled)
	{
		if(storage_)
			storage_->alpha_channel = enabled;
	}

	bool pixel_buffer::alpha_channel() const
	{
		return (storage_ ? storage_->alpha_channel : false);
	}

	void pixel_buffer::close()
	{
		storage_ = nullptr;
	}

	bool pixel_buffer::empty() const
	{
		return (nullptr == storage_);
	}

	pixel_buffer::operator unspecified_bool_t() const
	{
		return (storage_ ? &pixel_buffer::empty : nullptr);
	}

	std::size_t pixel_buffer::bytes() const
	{
		auto sp = storage_.get();
		if(sp)
			return sizeof(pixel_rgb_t) * (static_cast<std::size_t>(sp->pixel_size.width) * static_cast<std::size_t>(sp->pixel_size.height));
		return 0;
	}

	std::size_t pixel_buffer::bytes_per_line() const
	{
		return (storage_ ? storage_->bytes_per_line : 0);
	}

	nana::size pixel_buffer::size() const
	{
		return (storage_ ? storage_->pixel_size : nana::size());
	}

	pixel_rgb_t * pixel_buffer::at(const point& pos) const
	{
		pixel_buffer_storage * sp = storage_.get();
		if (sp && (pos.y < static_cast<int>(sp->pixel_size.height) + sp->valid_r.y))
			return reinterpret_cast<pixel_rgb_t*>(reinterpret_cast<char*>(sp->raw_pixel_buffer) + sp->bytes_per_line * (pos.y - sp->valid_r.y)) + (pos.x - sp->valid_r.x);
		return nullptr;
	}

	pixel_rgb_t * pixel_buffer::raw_ptr(std::size_t row) const
	{
		pixel_buffer_storage * sp = storage_.get();
		if(sp && (row < sp->pixel_size.height))
			return reinterpret_cast<pixel_rgb_t*>(reinterpret_cast<char*>(sp->raw_pixel_buffer) + sp->bytes_per_line * row);
		return nullptr;
	}

	pixel_rgb_t * pixel_buffer::operator[](std::size_t row) const
	{
		pixel_buffer_storage * sp = storage_.get();
		return reinterpret_cast<pixel_rgb_t*>(reinterpret_cast<char*>(sp->raw_pixel_buffer) + sp->bytes_per_line * row);
	}

	void pixel_buffer::put(const unsigned char* rawbits, std::size_t width, std::size_t height, std::size_t bits_per_pixel, std::size_t bytes_per_line, bool is_negative)
	{
		if(storage_)
			storage_->assign(rawbits, width, height, bits_per_pixel, bytes_per_line, is_negative);
	}

	pixel_rgb_t pixel_buffer::pixel(int x, int y) const
	{
		pixel_buffer_storage * sp = storage_.get();
		if(sp && 0 <= x && x < static_cast<int>(sp->pixel_size.width) && 0 <= y && y < static_cast<int>(sp->pixel_size.height))
			return *reinterpret_cast<const pixel_rgb_t*>(reinterpret_cast<const char*>(sp->raw_pixel_buffer + x) + y * sp->bytes_per_line);

		return pixel_rgb_t();
	}

	void pixel_buffer::pixel(int x, int y, pixel_rgb_t px)
	{
		pixel_buffer_storage * sp = storage_.get();
		if(sp && 0 <= x && x < static_cast<int>(sp->pixel_size.width) && 0 <= y && y < static_cast<int>(sp->pixel_size.height))
			*reinterpret_cast<pixel_rgb_t*>(reinterpret_cast<char*>(sp->raw_pixel_buffer + x) + y * sp->bytes_per_line) = px;
	}

	void pixel_buffer::paste(drawable_type drawable, int x, int y) const
	{
		if(storage_)
			paste(nana::rectangle(storage_->pixel_size), drawable, x, y);
	}

	void pixel_buffer::paste(const nana::rectangle& src_r, drawable_type drawable, int x, int y) const
	{
		pixel_buffer_storage * sp = storage_.get();
		if(drawable && sp)
		{
			if(sp->alpha_channel)
			{
				nana::rectangle s_good_r, d_good_r;
				if(overlap(src_r, sp->pixel_size, nana::rectangle(x, y, src_r.width, src_r.height), paint::detail::drawable_size(drawable), s_good_r, d_good_r))
				{
					pixel_buffer d_pixbuf;
					d_pixbuf.attach(drawable, d_good_r);
					(*(sp->img_pro.alpha_blend))->process(*this, s_good_r, d_pixbuf, nana::point(d_good_r.x, d_good_r.y));
				}
				return;
			}
#if defined(NANA_WINDOWS)
			BITMAPINFO bi;
			bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
			bi.bmiHeader.biWidth = sp->pixel_size.width;
			bi.bmiHeader.biHeight = -static_cast<int>(sp->pixel_size.height);
			bi.bmiHeader.biPlanes = 1;
			bi.bmiHeader.biBitCount = sizeof(pixel_rgb_t) * 8;
			bi.bmiHeader.biCompression = BI_RGB;
			bi.bmiHeader.biSizeImage = static_cast<DWORD>(sizeof(pixel_rgb_t) * sp->pixel_size.width * sp->pixel_size.height);
			bi.bmiHeader.biClrUsed = 0;
			bi.bmiHeader.biClrImportant = 0;

			::SetDIBitsToDevice(drawable->context,
				x, y, src_r.width, src_r.height,
				src_r.x, static_cast<int>(sp->pixel_size.height) - src_r.y - src_r.height, 0, sp->pixel_size.height,
				sp->raw_pixel_buffer, &bi, DIB_RGB_COLORS);
#elif defined(NANA_X11)
			sp->put(drawable->pixmap, drawable->context, src_r.x, src_r.y, x, y, src_r.width, src_r.height);
#endif
		}
	}

	void pixel_buffer::paste(native_window_type wd, int x, int y) const
	{
		pixel_buffer_storage * sp = storage_.get();
		if(nullptr == wd || nullptr == sp)	return;
#if defined(NANA_WINDOWS)
		HDC	handle = ::GetDC(reinterpret_cast<HWND>(wd));
		if(handle)
		{
			BITMAPINFO bi;
			bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
			bi.bmiHeader.biWidth = sp->pixel_size.width;
			bi.bmiHeader.biHeight = -static_cast<int>(sp->pixel_size.height);
			bi.bmiHeader.biPlanes = 1;
			bi.bmiHeader.biBitCount = sizeof(pixel_rgb_t) * 8;
			bi.bmiHeader.biCompression = BI_RGB;
			bi.bmiHeader.biSizeImage = static_cast<DWORD>(sizeof(pixel_rgb_t) * sp->pixel_size.width * sp->pixel_size.height);
			bi.bmiHeader.biClrUsed = 0;
			bi.bmiHeader.biClrImportant = 0;

			::SetDIBitsToDevice(handle,
				x, y, sp->pixel_size.width, sp->pixel_size.height,
				0, 0, 0, sp->pixel_size.height,
				sp->raw_pixel_buffer, &bi, DIB_RGB_COLORS);

			::ReleaseDC(reinterpret_cast<HWND>(wd), handle);
		}
#elif defined(NANA_X11)
		auto & spec = nana::detail::platform_spec::instance();
		Display * disp = spec.open_display();
		sp->put(reinterpret_cast<Window>(wd), XDefaultGC(disp, XDefaultScreen(disp)), 0, 0, x, y, sp->pixel_size.width, sp->pixel_size.height);
#endif

	}

	void pixel_buffer::line(const std::string& name)
	{
		pixel_buffer_storage * sp = storage_.get();
		if(sp && name.size())
		{
			sp->img_pro.line_receptacle = detail::image_process_provider::instance().ref_line(name);
			if(sp->img_pro.line_receptacle == *detail::image_process_provider::instance().line())
				sp->img_pro.line = detail::image_process_provider::instance().line();
			else
				sp->img_pro.line = & sp->img_pro.line_receptacle;
		}
	}

	void pixel_buffer::line(const nana::point &pos_beg, const nana::point &pos_end, nana::color_t color, double fade_rate)
	{
		pixel_buffer_storage * sp = storage_.get();
		if(nullptr == sp) return;

		//Test if the line intersects the rectangle, and retrive the two points that
		//are always in the area of rectangle, good_pos_beg is left point, good_pos_end is right.
		nana::point good_pos_beg, good_pos_end;
		if(intersection(nana::rectangle(sp->pixel_size), pos_beg, pos_end, good_pos_beg, good_pos_end))
			(*(sp->img_pro.line))->process(*this, good_pos_beg, good_pos_end, color, fade_rate);
	}

	void pixel_buffer::rectangle(const nana::rectangle &r, nana::color_t col, double fade_rate, bool solid)
	{
		pixel_buffer_storage * sp = storage_.get();
		if((nullptr == sp) || (fade_rate == 1.0)) return;

		bool fade = (fade_rate != 0.0);
		unsigned char * fade_table = nullptr;
		nana::pixel_rgb_t rgb_imd;
		if(fade)
		{
			fade_table = detail::alloc_fade_table(1 - fade_rate);
			rgb_imd.u.color = col;
			rgb_imd = detail::fade_color_intermedia(rgb_imd, fade_table);
		}

		int xbeg = (0 <= r.x ? r.x : 0);
		int xend = static_cast<int>(r.x + r.width < sp->pixel_size.width ? r.x + r.width : sp->pixel_size.width);
		int ybeg = (0 <= r.y ? r.y : 0);
		int yend = static_cast<int>(r.y + r.height < sp->pixel_size.height ? r.y + r.height : sp->pixel_size.height);

		if (solid)
		{
			nana::pixel_rgb_t * p_rgb = sp->raw_pixel_buffer + ybeg * sp->pixel_size.width;
			auto lineptr = p_rgb + xbeg;
			auto end = p_rgb + xend;
			if (fade)
			{
				for (int top = ybeg; top < yend; ++top)
				{
					for (auto i = lineptr; i != end; ++i)
					{
						*i = detail::fade_color_by_intermedia(*i, rgb_imd, fade_table);
					}
					lineptr += sp->pixel_size.width;
					end = lineptr + (xend - xbeg);
				}
			}
			else
			{
				for (int top = ybeg; top < yend; ++top)
				{
					for (auto i = lineptr; i != end; ++i)
					{
						i->u.color = col;
					}
					lineptr += sp->pixel_size.width;
					end = lineptr + (xend - xbeg);
				}
			}
			return;
		}

		if((ybeg == r.y) && (r.y + static_cast<int>(r.height) == yend))
		{
			nana::pixel_rgb_t * p_rgb = sp->raw_pixel_buffer + ybeg * sp->pixel_size.width;
			nana::pixel_rgb_t * i = p_rgb + xbeg;
			nana::pixel_rgb_t * end = p_rgb + xend;
			nana::pixel_rgb_t * i_other = sp->raw_pixel_buffer + (yend - 1) * sp->pixel_size.width + xbeg;
			if(fade)
			{
				for(;i != end; ++i, ++i_other)
				{
					*i = detail::fade_color_by_intermedia(*i, rgb_imd, fade_table);
					*i_other = detail::fade_color_by_intermedia(*i_other, rgb_imd, fade_table);
				}
			}
			else
			{
				for(;i != end; ++i, ++i_other)
				{
					i->u.color = col;
					i_other->u.color = col;
				}
			}
		}
		else
		{
			if(ybeg == r.y)
			{
				nana::pixel_rgb_t * p_rgb = sp->raw_pixel_buffer + ybeg * sp->pixel_size.width;
				nana::pixel_rgb_t * i = p_rgb;
				nana::pixel_rgb_t * end = p_rgb + xend;
				if(fade)
				{
					for(; i != end; ++i)
						*i = detail::fade_color_by_intermedia(*i, rgb_imd, fade_table);
				}
				else
				{
					for(;i != end; ++i)
						i->u.color = col;
				}
			}

			if(r.y + static_cast<int>(r.height) == yend)
			{
				nana::pixel_rgb_t * p_rgb = sp->raw_pixel_buffer + (yend - 1) * sp->pixel_size.width;
				nana::pixel_rgb_t * i = p_rgb;
				nana::pixel_rgb_t * end = p_rgb + xend;

				if(fade)
				{
					for(; i != end; ++i)
						*i = detail::fade_color_by_intermedia(*i, rgb_imd, fade_table);
				}
				else
				{
					for(;i != end; ++i)
						i->u.color = col;
				}
			}
		}

		if((xbeg == r.x) && (r.x + static_cast<int>(r.width) == xend))
		{
			nana::pixel_rgb_t * p_rgb = sp->raw_pixel_buffer + ybeg * sp->pixel_size.width;
			nana::pixel_rgb_t * i = p_rgb + xbeg;
			nana::pixel_rgb_t * end = sp->raw_pixel_buffer + (yend - 1) * sp->pixel_size.width + xbeg;

			nana::pixel_rgb_t * i_other = p_rgb + (xend - 1);

			if(fade)
			{
				while(true)
				{
					*i = detail::fade_color_by_intermedia(*i, rgb_imd, fade_table);
					*i_other = detail::fade_color_by_intermedia(*i_other, rgb_imd, fade_table);
					if(i == end)
						break;

					i += sp->pixel_size.width;
					i_other += sp->pixel_size.width;
				}
			}
			else
			{
				while(true)
				{
					i->u.color = col;
					i_other->u.color = col;
					if(i == end)
						break;

					i += sp->pixel_size.width;
					i_other += sp->pixel_size.width;
				}
			}
		}
		else
		{
			if(xbeg == r.x)
			{
				nana::pixel_rgb_t * p_rgb = sp->raw_pixel_buffer + ybeg * sp->pixel_size.width;
				nana::pixel_rgb_t * i = p_rgb + xbeg;
				nana::pixel_rgb_t * end = sp->raw_pixel_buffer + (yend - 1) * sp->pixel_size.width + xbeg;
				if(fade)
				{
					while(true)
					{
						*i = detail::fade_color_by_intermedia(*i, rgb_imd, fade_table);
						if(i == end)	break;

						i += sp->pixel_size.width;
					}
				}
				else
				{
					while(true)
					{
						i->u.color = col;
						if(i == end)	break;

						i += sp->pixel_size.width;
					}
				}
			}

			if(r.x + static_cast<int>(r.width) == xend)
			{
				nana::pixel_rgb_t * p_rgb = sp->raw_pixel_buffer + ybeg * sp->pixel_size.width;
				nana::pixel_rgb_t * i = p_rgb + (xend - 1);
				nana::pixel_rgb_t * end = sp->raw_pixel_buffer + (yend - 1) * sp->pixel_size.width + (xend - 1);
				if(fade)
				{
					while(true)
					{
						*i = detail::fade_color_by_intermedia(*i, rgb_imd, fade_table);
						if(i == end)	break;
						i += sp->pixel_size.width;
					}
				}
				else
				{
					while(true)
					{
						i->u.color = col;
						if(i == end)	break;
						i += sp->pixel_size.width;
					}
				}
			}
		}

		detail::free_fade_table(fade_table);
	}

	void pixel_buffer::shadow_rectangle(const nana::rectangle& draw_rct, nana::color_t beg, nana::color_t end, double fade_rate, bool vertical)
	{
		pixel_buffer_storage * sp = storage_.get();
		if(nullptr == sp) return;

		nana::rectangle rct;
		if(false == overlap(nana::rectangle(sp->pixel_size), draw_rct, rct))
			return;

		int deltapx = int(vertical ? rct.height : rct.width);
		if(sp && deltapx)
		{
			nana::color_t r, g, b;
			const int delta_r = (int(end & 0xFF0000) - int(r = (beg & 0xFF0000))) / deltapx;
			const int delta_g = (int((end & 0xFF00) << 8) - int(g = ((beg & 0xFF00) << 8))) / deltapx;
			const int delta_b = (int((end & 0xFF) << 16 ) - int(b = ((beg & 0xFF)<< 16))) / deltapx;

			nana::pixel_rgb_t * pxbuf = sp->raw_pixel_buffer + rct.x + rct.y * sp->pixel_size.width;
			if(vertical)
			{
				if(deltapx + rct.y > 0)
				{
					unsigned align_4 = (rct.width & ~3);
					unsigned align_reset = rct.width & 3;
					while(deltapx--)
					{
						nana::pixel_rgb_t px;

						px.u.color = ((r += delta_r) & 0xFF0000) | (((g += delta_g) & 0xFF0000) >> 8) | (((b += delta_b) & 0xFF0000) >> 16);
						
						nana::pixel_rgb_t * dpx = pxbuf;
						for(nana::pixel_rgb_t *dpx_end = pxbuf + align_4; dpx != dpx_end; dpx += 4)
						{
							*dpx = px;
							dpx[1] = px;
							dpx[2] = px;
							dpx[3] = px;
						}

						for(nana::pixel_rgb_t * dpx_end = dpx + align_reset; dpx != dpx_end; ++dpx)
							*dpx = px;

						pxbuf += sp->pixel_size.width;
					}
				}
			}
			else
			{
				if(deltapx + rct.x > 0)
				{
					nana::pixel_rgb_t * pxbuf_end = pxbuf + rct.width;

					for(; pxbuf != pxbuf_end; ++pxbuf)
					{
						nana::pixel_rgb_t px;
						px.u.color = ((r += delta_r) & 0xFF0000) | (((g += delta_g) & 0xFF0000) >> 8) | (((b += delta_b) & 0xFF0000) >> 16);
						nana::pixel_rgb_t * dpx_end = pxbuf + rct.height * sp->pixel_size.width;
						for(nana::pixel_rgb_t * dpx = pxbuf; dpx != dpx_end; dpx += sp->pixel_size.width)
							*dpx = px;
					}
				}			
			}
		}
	}

	//stretch
	void pixel_buffer::stretch(const std::string& name)
	{
		pixel_buffer_storage * sp = storage_.get();
		if(sp && name.size())
		{
			auto op_default = detail::image_process_provider::instance().stretch();
			sp->img_pro.stretch_receptacle = detail::image_process_provider::instance().ref_stretch(name);
			if(sp->img_pro.stretch_receptacle == *op_default)
				sp->img_pro.stretch = op_default;
			else
				sp->img_pro.stretch = & sp->img_pro.stretch_receptacle;
		}
	}

	void pixel_buffer::stretch(const nana::rectangle& src_r, drawable_type drawable, const nana::rectangle& r) const
	{
		pixel_buffer_storage * sp = storage_.get();
		if(nullptr == sp) return;

		nana::rectangle good_src_r, good_dst_r;
		if(overlap(src_r, sp->pixel_size, r, paint::detail::drawable_size(drawable), good_src_r, good_dst_r))
		{
			pixel_buffer dst;
			dst.attach(drawable, good_dst_r);
			(*(sp->img_pro.stretch))->process(*this, good_src_r, dst, nana::rectangle(0, 0, good_dst_r.width, good_dst_r.height));
		}
	}

	//blend
	void pixel_buffer::blend(const std::string& name)
	{
		pixel_buffer_storage * sp = storage_.get();
		if(sp && name.size())
		{
			auto op_default = detail::image_process_provider::instance().blend();
			sp->img_pro.blend_receptacle = detail::image_process_provider::instance().ref_blend(name);
			if(sp->img_pro.blend_receptacle == *op_default)
				sp->img_pro.blend = op_default;
			else
				sp->img_pro.blend = & sp->img_pro.blend_receptacle;
		}
	}

	void pixel_buffer::blend(const nana::rectangle& s_r, drawable_type dw_dst, const nana::point& d_pos, double fade_rate) const
	{
		pixel_buffer_storage * sp = storage_.get();
		if(nullptr == sp) return;

		nana::rectangle s_good_r, d_good_r;
		if(overlap(s_r, sp->pixel_size, nana::rectangle(d_pos.x, d_pos.y, s_r.width, s_r.height), paint::detail::drawable_size(dw_dst), s_good_r, d_good_r))
		{
			pixel_buffer d_pixbuf;
			d_pixbuf.attach(dw_dst, d_good_r);
			(*(sp->img_pro.blend))->process(*this, s_good_r, d_pixbuf, nana::point(), fade_rate);
		}
	}

	void pixel_buffer::blur(const nana::rectangle& r, std::size_t radius)
	{
		auto sp = storage_.get();
		if(nullptr == sp || radius < 1)	return;

		nana::rectangle good_r;
		if(overlap(r, this->size(), good_r))
			(*(sp->img_pro.blur))->process(*this, good_r, radius);
	}
}//end namespace paint
}//end namespace nana
