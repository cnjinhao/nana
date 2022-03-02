/*
*	Basic Image PixelBuffer Implementation
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2021 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file: nana/paint/detail/image_pixbuf.hpp
*/

#ifndef NANA_PAINT_DETAIL_IMAGE_PIXBUF_HPP
#define NANA_PAINT_DETAIL_IMAGE_PIXBUF_HPP

#include <nana/paint/detail/image_impl_interface.hpp>
#include <nana/paint/pixel_buffer.hpp>

namespace nana
{
	namespace paint{	namespace detail{

		class basic_image_pixbuf
			: public image::image_impl_interface
		{
		public:
			bool alpha_channel() const override
			{
				return pixbuf_.alpha_channel();
			}

			bool empty() const override
			{
				return pixbuf_.empty();
			}

			void close() override
			{
				pixbuf_.close();
			}

			::nana::size size() const override
			{
				return pixbuf_.size();
			}

			void paste(const ::nana::rectangle& src_r, graph_reference graph, const point& p_dst) const override
			{
				pixbuf_.paste(src_r, graph.handle(), p_dst);
			}

			void stretch(const ::nana::rectangle& src_r, graph_reference dst, const nana::rectangle& r) const override
			{
				pixbuf_.stretch(src_r, dst.handle(), r);
			}

			bool save(const std::filesystem::path& p) const override
			{
				return pixbuf_.save(p);
			}

			std::size_t length() const override
			{
				return (pixbuf_.empty() ? 0 : 1);
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
				return (0 == pos);
			}

			paint::pixel_buffer& pxbuf() override
			{
				return pixbuf_;
			}

			const paint::pixel_buffer& pxbuf() const override
			{
				return pixbuf_;
			}
		protected:
			pixel_buffer pixbuf_;
		};
	}//end namespace detail
	}//end namespace paint
}//end namespace nana

#endif
