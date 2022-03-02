/*
 *	Icon Resource
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2017-2021 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/paint/detail/image_ico_resource.hpp
 */

#ifndef NANA_PAINT_DETAIL_IMAGE_ICO_RESOURCE_INCLUDED
#define NANA_PAINT_DETAIL_IMAGE_ICO_RESOURCE_INCLUDED

#include <filesystem>
#include <nana/paint/detail/image_impl_interface.hpp>
#include <nana/paint/graphics.hpp>

#if defined(NANA_WINDOWS)
#	include <windows.h>
#endif

namespace nana{	namespace paint
{
	namespace detail
	{
		class image_ico_resource
			:public image::image_impl_interface
		{
		public:
			bool open(const std::filesystem::path& filename) override
			{
#if defined(NANA_WINDOWS)
				SHFILEINFO sfi;
				::SHGetFileInfo(filename.c_str(), 0, &sfi, sizeof sfi, SHGFI_ICON);

				native_handle_ = sfi.hIcon;
#else
				static_cast<void>(filename);	//eliminate unused parameter compiler warnings
#endif
				return (nullptr != native_handle_);
			}

			bool open(const void* /*data*/, std::size_t /*bytes*/) override
			{
				return false;
			}

			bool alpha_channel() const override
			{
				return false;
			}

			bool empty() const override
			{
				return !native_handle_;
			}

			void close() override
			{
#if defined(NANA_WINDOWS)
				if (native_handle_)
					::DestroyIcon(reinterpret_cast<HICON>(native_handle_));
#endif
			}

			nana::size size() const override
			{
#if defined(NANA_WINDOWS)
				ICONINFO info;
				if ((!native_handle_) || !::GetIconInfo(reinterpret_cast<HICON>(native_handle_), &info))
					return{};
				
				::nana::size sz{
					info.xHotspot << 1,
					info.yHotspot << 1
				};

				::DeleteObject(info.hbmColor);
				::DeleteObject(info.hbmMask);
				return sz;
#else
				return{};
#endif
			}

			virtual void paste(const nana::rectangle& src_r, graph_reference graph, const point& p_dst) const override
			{
				if (!native_handle_)
					return;

#if defined(NANA_WINDOWS)
				::DrawIconEx(graph.handle()->context, p_dst.x, p_dst.y, reinterpret_cast<HICON>(native_handle_), src_r.width, src_r.height, 0, 0, DI_NORMAL);
#else
				static_cast<void>(src_r);	//eliminate unused parameter compiler warning.
				static_cast<void>(graph);
				static_cast<void>(p_dst);
#endif

			}

			virtual void stretch(const nana::rectangle&, graph_reference graph, const nana::rectangle& r) const override
			{
				if (!native_handle_)
					return;

#if defined(NANA_WINDOWS)
				::DrawIconEx(graph.handle()->context, r.x, r.y, reinterpret_cast<HICON>(native_handle_), r.width, r.height, 0, 0, DI_NORMAL);
#else
				static_cast<void>(graph);	//eliminate unused parameter compiler warning.
				static_cast<void>(r);
#endif			
			}

			bool save(const std::filesystem::path& p) const override
			{
				paint::graphics graph{size()};

				paste(rectangle{ size() }, graph, {});

				graph.save_as_file(to_utf8(p.wstring()).c_str());
				return true;
			}

			std::size_t length() const override
			{
				return (native_handle_ ? 1 : 0);
			}

			std::size_t frame() const override
			{
				return 0;
			}

			std::size_t frame_duration() const override
			{
				return 0;
			}

			bool set_frame(std::size_t pos) override
			{
				return native_handle_ && (0 == pos);
			}
			
			void* native_handle()
			{
				return native_handle_;
			}

			paint::pixel_buffer& pxbuf() override
			{
				auto sz = size();
				if (!pxbuf_)
					pxbuf_.open(sz.width, sz.height);

				paint::graphics graph{ size() };

				paste(rectangle{ size() }, graph, {});
				
				pxbuf_.open(graph.handle());
				return pxbuf_;
			}

			const paint::pixel_buffer& pxbuf() const override
			{
				auto sz = size();
				if (!pxbuf_)
					pxbuf_.open(sz.width, sz.height);

				paint::graphics graph{ size() };

				paste(rectangle{ size() }, graph, {});

				pxbuf_.open(graph.handle());
				return pxbuf_;
			}
		private:
			void* native_handle_;
			mutable paint::pixel_buffer pxbuf_;
		};//end class image_ico
	}
}//end namespace paint
}//end namespace nana

#endif
