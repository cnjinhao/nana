/*
 *	Paint Image Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/paint/image.cpp
 *	@contributors:
 *		nabijaczleweli(pr#106)
 */

#include <nana/push_ignore_diagnostic>
#include "../detail/platform_spec_selector.hpp"
#include <nana/paint/image.hpp>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <stdexcept>

#include <nana/paint/detail/image_impl_interface.hpp>
#include <nana/paint/pixel_buffer.hpp>
#include <nana/filesystem/filesystem_ext.hpp>

#if defined(NANA_ENABLE_JPEG)
#include "detail/image_jpeg.hpp"
#endif

#if defined(NANA_ENABLE_PNG)
	#include "detail/image_png.hpp"
#endif

#include "detail/image_bmp.hpp"

#include "image_accessor.hpp"
#include "detail/image_ico_resource.hpp"
#include "detail/image_ico.hpp"

namespace fs = std::filesystem;

namespace nana
{
namespace paint
{
#if defined(NANA_WINDOWS)
	HICON image_accessor::icon(const nana::paint::image& img)
	{
		auto ico_res = dynamic_cast<paint::detail::image_ico_resource*>(img.image_ptr_.get());
		if (ico_res)
			return reinterpret_cast<HICON>(ico_res->native_handle());

		auto ico = dynamic_cast<paint::detail::image_ico*>(img.image_ptr_.get());
		if (ico)
			return reinterpret_cast<HICON>(ico->native_handle());

		return nullptr;
	}
#else
	int image_accessor::icon(const image&)
	{
		return 0;
	}
#endif

	image::image_impl_interface::~image_impl_interface()
	{}

	//class image
		image::image() noexcept
		{}

		image::image(const image& rhs)
			:	image_ptr_(rhs.image_ptr_)
		{}

		image::image(image&& r)
			:	image_ptr_(std::move(r.image_ptr_))
		{}

		image::image(const std::string& file)
		{
			open(file);
		}

		image::image(const std::wstring& file)
		{
			open(file);
		}

		image::~image()
		{
			close();
		}

		image& image::operator=(const image& r)
		{
			if(this != &r)
				image_ptr_ = r.image_ptr_;

			return * this;
		}

		image& image::operator=(image&& r)
		{
			if(this != &r)
				image_ptr_ = std::move(r.image_ptr_);
			return *this;
		}

		std::shared_ptr<image::image_impl_interface> create_image(const fs::path & p)
		{
			std::shared_ptr<image::image_impl_interface> ptr;

			auto ext = p.extension().native();
			if (ext.empty())
				return ptr;

			std::transform(ext.begin(), ext.end(), ext.begin(), [](int ch)
			{
				if ('A' <= ch && ch <= 'Z')
					ch -= ('A' - 'a');
				return ch;
			});

#if defined(NANA_WINDOWS)
			const wchar_t* ext_ico = L".ico";
			const wchar_t* ext_png = L".png";
			const wchar_t* ext_jpg = L".jpg";
			const wchar_t* ext_jpeg = L".jpeg";
#else
			const char* ext_ico = ".ico";
			const char* ext_png = ".png";
			const char* ext_jpg = ".jpg";
			const char* ext_jpeg = ".jpeg";
#endif
			do
			{
				if (ext_ico == ext)
				{
					ptr = std::make_shared<detail::image_ico>();
					break;
				}

				if (ext_png == ext)
				{
#if defined(NANA_ENABLE_PNG)
					ptr = std::make_shared<detail::image_png>();
#else
					return ptr;
#endif
					break;
				}

				if (ext_jpg == ext || ext_jpeg == ext)
				{
#if defined(NANA_ENABLE_JPEG)
					ptr = std::make_shared<detail::image_jpeg>();
#else
					return ptr;
#endif
					break;
				}
			} while (false);

			//Check for BMP
			if (!ptr)
			{
#ifndef NANA_MINGW
				std::ifstream ifs(p.c_str(), std::ios::binary);
#else
				std::ifstream ifs(to_osmbstr(to_utf8(p.native())).c_str(), std::ios::binary);
#endif
				if (ifs)
				{
					unsigned short meta = 0;
					ifs.read(reinterpret_cast<char*>(&meta), 2);
					if (*reinterpret_cast<const short*>("BM") == meta)
						ptr = std::make_shared<detail::image_bmp>();
					else if (*reinterpret_cast<const short*>("MZ") == meta)
						ptr = std::make_shared<detail::image_ico_resource>();
				}
			}

			return ptr;
		}

		bool image::open(const ::std::string& file)
		{
			fs::path path(file);
			image_ptr_ = create_image(path);
			return (image_ptr_ ? image_ptr_->open(path) : false);
		}

		bool image::open(const std::wstring& file)
		{
			fs::path path(file);
			image_ptr_ = create_image(path);
			return (image_ptr_ ? image_ptr_->open(path) : false);
		}

		bool image::open(const void* data, std::size_t bytes)
		{
			close();

			if (bytes > 2)
			{
				std::shared_ptr<image::image_impl_interface> ptr;

				auto meta = *reinterpret_cast<const unsigned short*>(data);

				if (*reinterpret_cast<const short*>("BM") == meta)
					ptr = std::make_shared<detail::image_bmp>();
				else if (*reinterpret_cast<const short*>("MZ") == meta)
					ptr = std::make_shared<detail::image_ico_resource>();
				else
				{
					if (bytes > 8 && (0x474e5089 == *reinterpret_cast<const unsigned*>(data)))
					{
#if defined(NANA_ENABLE_PNG)
						ptr = std::make_shared<detail::image_png>();
#endif
					}
					else
					{
#if defined(NANA_ENABLE_JPEG)
						if ((bytes > 11) && (0xd8ff == *reinterpret_cast<const unsigned short*>(data)))
						{
							switch(*reinterpret_cast<const unsigned*>(reinterpret_cast<const char*>(data)+6))
							{
							case 0x4649464A:	//JFIF
							case 0x66697845:	//Exif
								ptr = std::make_shared<detail::image_jpeg>();
							}
						}
						else
#endif
						if ((!ptr) && (bytes > 40))
						{
							switch (*reinterpret_cast<const unsigned*>(data))
							{
							case 40:
							case 0x00010000:
								if (!ptr && bytes > 40)
									ptr = std::make_shared<detail::image_ico>();
							}
						}
					}
				}


				if (ptr)
				{
					image_ptr_.swap(ptr);
					return image_ptr_->open(data, bytes);
				}
			}

			return false;
		}


		bool image::empty() const noexcept
		{
			return ((nullptr == image_ptr_) || image_ptr_->empty());
		}

		image::operator unspecified_bool_t() const
		{
			return (image_ptr_ ? &image::empty : nullptr);
		}

		//Fixed missing noexcept specifier by nabijaczleweli(pr#106)
		void image::close() noexcept
		{
			image_ptr_.reset();
		}

		//Fixed missing noexcept specifier by nabijaczleweli(pr#106)
		bool image::alpha() const noexcept
		{
			return (image_ptr_ ? image_ptr_->alpha_channel() : false);
		}

		//Fixed missing noexcept specifier by nabijaczleweli(pr#106)
		nana::size image::size() const noexcept
		{
			return (image_ptr_ ? image_ptr_->size() : nana::size());
		}

		void image::paste(graphics& dst, const point& p_dst) const
		{
			this->paste(rectangle{ this->size() }, dst, p_dst);
		}

		void image::paste(const rectangle& r_src, graphics & dst, const point& p_dst) const
		{
			if (image_ptr_)
			{
				if (dst.empty())
					dst.make({ r_src.width, r_src.height });	//throws if failed to create

				image_ptr_->paste(r_src, dst, p_dst);
			}
			else
				throw std::runtime_error("image is empty");
		}

		void image::stretch(const nana::rectangle& r_src, graphics& dst, const nana::rectangle & r_dst) const
		{
			if (image_ptr_)
			{
				if (dst.empty())
					dst.make({ r_src.width, r_src.height });	//throws if failed to create

				image_ptr_->stretch(r_src, dst, r_dst);
			}
			else
				throw std::runtime_error("image is empty");
		}
	//end class image

}//end namespace paint
}//end namespace nana

