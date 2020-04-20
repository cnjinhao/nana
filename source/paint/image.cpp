/**
 *	Paint Image Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2020 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file nana/paint/image.cpp
 *	@contributors nabijaczleweli(pr#106)
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

		// Check file type through file format signature
		std::shared_ptr<image::image_impl_interface> create_image(const char* buf, std::size_t len)
		{
			if (buf && len >= 8)
			{
				if (std::strncmp("\x00\x00\x01\x00", buf, 4) == 0)
				{
					return std::make_shared<detail::image_ico>();
				}
				else if (std::strncmp(buf, "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A", 8) == 0)
				{
#if defined(NANA_ENABLE_PNG)
					return std::make_shared<detail::image_png>();
#endif
				}
				else if (std::strncmp("\xFF\xD8\xFF", buf, 3) == 0)
				{
#if defined(NANA_ENABLE_JPEG)
					return std::make_shared<detail::image_jpeg>();
#endif
				}
				else if (*reinterpret_cast<const short*>("BM") == *reinterpret_cast<const short*>(buf))
					return std::make_shared<detail::image_bmp>();
				else if (*reinterpret_cast<const short*>("MZ") == *reinterpret_cast<const short*>(buf))
					return std::make_shared<detail::image_ico_resource>();
				}

			return nullptr;
			}

		bool image::open(const ::std::string& img)
		{
			fs::path p(img);
			image_ptr_.reset();

			std::ifstream file{ p, std::ios::binary };
			if (file)
			{
				char buf[8];
				if (file.read(buf, 8).gcount() == 8)
					image_ptr_ = create_image(buf, 8);
			}

			return (image_ptr_ ? image_ptr_->open(p) : false);
		}

		bool image::open(const std::wstring& img)
		{
			fs::path p(img);
			image_ptr_.reset();

			std::ifstream file{ p, std::ios::binary };
			if (file)
			{
				char buf[8];
				if (file.read(buf, 8).gcount() == 8)
					image_ptr_ = create_image(buf, 8);
			}

			return (image_ptr_ ? image_ptr_->open(p) : false);
		}

		bool image::open(const void* data, std::size_t bytes)
		{
			image_ptr_ = create_image(static_cast<const char*>(data), bytes);
			return (image_ptr_ ? image_ptr_->open(data, bytes) : false);
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

