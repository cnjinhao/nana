/*
 *	Paint Image Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/paint/image.cpp
 */

#include <nana/detail/platform_spec_selector.hpp>
#include <nana/paint/image.hpp>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <stdexcept>

#include <nana/paint/detail/image_impl_interface.hpp>
#include <nana/paint/pixel_buffer.hpp>

#if defined(NANA_ENABLE_JPEG)
#include "detail/image_jpeg.hpp"
#endif

#if defined(NANA_ENABLE_PNG)
	#include "detail/image_png.hpp"
#endif

#include "detail/image_bmp.hpp"
#include "detail/image_ico.hpp"

namespace nana
{
namespace paint
{
	namespace detail
	{
		//class image_ico
			image_ico::image_ico(bool is_ico): is_ico_(is_ico){}

			bool image_ico::open(const nana::experimental::filesystem::path& file)
			{
				close();
#if defined(NANA_WINDOWS)
				HICON handle = 0;
				if(is_ico_)
				{
					handle = (HICON)::LoadImage(0, file.c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
				}
				else
				{
					SHFILEINFO    sfi;
					::SHGetFileInfo(file.c_str(), 0, &sfi, sizeof(sfi), SHGFI_ICON);
					handle = sfi.hIcon;
				}

				if(handle)
				{
					HICON * p = new HICON(handle);
					ptr_ = std::shared_ptr<HICON>(p, handle_deleter());
					ICONINFO info;
					::GetIconInfo(handle, &info);
					size_.width = (info.xHotspot << 1);
					size_.height = (info.yHotspot << 1);

					::DeleteObject(info.hbmColor);
					::DeleteObject(info.hbmMask);
					return true;
				}
#else
				if(is_ico_){}	//kill the unused compiler warning in Linux.
#endif
				return false;
			}

			bool image_ico::open(const void* data, std::size_t bytes)
			{
				close();
#if defined(NANA_WINDOWS)
				HICON handle = ::CreateIconFromResource((PBYTE)data, static_cast<DWORD>(bytes), TRUE, 0x00030000);
				if(handle)
				{
					ICONINFO info;
					if (::GetIconInfo(handle, &info) != 0)
					{
						HICON * p = new HICON(handle);
						ptr_ = std::shared_ptr<HICON>(p, handle_deleter());
						size_.width = (info.xHotspot << 1);
						size_.height = (info.yHotspot << 1);
						::DeleteObject(info.hbmColor);
						::DeleteObject(info.hbmMask);
						return true;
					}
				}
#else
				if(is_ico_){}	//kill the unused compiler warning in Linux.
#endif
				return false;
			}

			bool image_ico::alpha_channel() const
			{
				return false;
			}

			bool image_ico::empty() const
			{
				return (nullptr == ptr_);
			}

			void image_ico::close()
			{
				ptr_.reset();
			}

			nana::size image_ico::size() const
			{
				return size_;
			}

			void image_ico::paste(const nana::rectangle& src_r, graph_reference graph, const point& p_dst) const
			{
				if(ptr_ && (graph.empty() == false))
				{
#if defined(NANA_WINDOWS)
					::DrawIconEx(graph.handle()->context, p_dst.x, p_dst.y, *ptr_, src_r.width, src_r.height, 0, 0, DI_NORMAL);
#endif
				}
			}

			void image_ico::stretch(const nana::rectangle&, graph_reference graph, const nana::rectangle& r) const
			{
				if(ptr_ && (graph.empty() == false))
				{
#if defined(NANA_WINDOWS)
					::DrawIconEx(graph.handle()->context, r.x, r.y, *ptr_, r.width, r.height, 0, 0, DI_NORMAL);
#endif
				}
			}

			const image_ico::ptr_t& image_ico::ptr() const
			{
				return ptr_;
			}

#if defined(NANA_WINDOWS)
			//struct handle_deleter
				void image_ico::handle_deleter::operator()(HICON* p) const
				{
					if(p && *p)
						::DestroyIcon(*p);
					delete p;
				}
			//end struct handle_deleter
#endif
		//end class image_ico
	}

	image::image_impl_interface::~image_impl_interface()
	{}

	//class image
		image::image()
		{}

		image::image(const image& rhs)
			:	image_ptr_(rhs.image_ptr_)
		{}

		image::image(image&& r)
			:	image_ptr_(std::move(r.image_ptr_))
		{}

		image::image(const ::nana::experimental::filesystem::path& file)
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

		bool image::open(const ::nana::experimental::filesystem::path& file)
		{
			image_ptr_.reset();

			auto extension = file.extension().native();
			if (extension.empty())
				return false;

			image::image_impl_interface * helper = nullptr;

			{
					std::transform(extension.begin(), extension.end(), extension.begin(), [](int ch)
					{
						if('A' <= ch && ch <= 'Z')
							ch  -= ('A' - 'a');
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
						if (ext_ico == extension)
						{
#if defined(NANA_WINDOWS)
							helper = new detail::image_ico(true);
#else
							return false;
#endif
							break;
						}

						if (ext_png == extension)
						{
#if defined(NANA_ENABLE_PNG)
							helper = new detail::image_png;
#else
							return false;
#endif
							break;
						}

						if (ext_jpg == extension || ext_jpeg == extension)
						{
#if defined(NANA_ENABLE_JPEG)
							helper = new detail::image_jpeg;
#else
							return false;
#endif
							break;
						}
					} while (false);

				//Check for BMP
				if (!helper)
				{
					std::ifstream ifs(file.c_str(), std::ios::binary);
					if (ifs)
					{
						unsigned short meta = 0;
						ifs.read(reinterpret_cast<char*>(&meta), 2);
						if (*reinterpret_cast<const short*>("BM") == meta)
							helper = new detail::image_bmp;
						else if (*reinterpret_cast<const short*>("MZ") == meta)
							helper = new detail::image_ico(false);
					}
				}

				if (helper)
				{
					image_ptr_ = std::shared_ptr<image_impl_interface>(helper);
					return helper->open(file.c_str());
				}
			}
			return false;
		}

		bool image::open_icon(const void* data, std::size_t bytes)
		{
			image::image_impl_interface * helper = new detail::image_ico(true);
			image_ptr_ = std::shared_ptr<image_impl_interface>(helper);
			return helper->open(data, bytes);
		}

		bool image::empty() const
		{
			return ((nullptr == image_ptr_) || image_ptr_->empty());
		}

		image::operator unspecified_bool_t() const
		{
			return (image_ptr_ ? &image::empty : nullptr);
		}

		void image::close()
		{
			image_ptr_.reset();
		}

		bool image::alpha() const
		{
			return (image_ptr_ ? image_ptr_->alpha_channel() : false);
		}

		nana::size image::size() const
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

